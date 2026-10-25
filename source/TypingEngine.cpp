#include "TypingEngine.hpp"
#include "HidEncode.hpp"

#include <switch.h>
#include <cstring>
#include <functional>
#include <atomic>

namespace ezbasic {

// ------------------------------------------------------------------
// Standard HID boot-keyboard report descriptor
// ------------------------------------------------------------------
static const u8 s_reportDescriptor[] = {
    0x05,0x01,0x09,0x06,0xa1,0x01,0x05,0x07,0x19,0xe0,0x29,0xe7,
    0x15,0x00,0x25,0x01,0x75,0x01,0x95,0x08,0x81,0x02,0x95,0x01,
    0x75,0x08,0x81,0x03,0x95,0x05,0x75,0x01,0x05,0x08,0x19,0x01,
    0x29,0x05,0x91,0x02,0x95,0x01,0x75,0x03,0x91,0x03,0x95,0x06,
    0x75,0x08,0x15,0x00,0x25,0x65,0x05,0x07,0x19,0x00,0x29,0x65,
    0x81,0x00,0xc0
};

// ------------------------------------------------------------------
// Binary Object Store (required for SuperSpeed enumeration)
// ------------------------------------------------------------------
static const u8 s_bos[] = {
    0x05, USB_DT_BOS, 0x16, 0x00, 0x02,
    0x07, USB_DT_DEVICE_CAPABILITY, 0x02, 0x02, 0x00, 0x00, 0x00,
    0x0A, USB_DT_DEVICE_CAPABILITY, 0x03, 0x00, 0x0E, 0x00, 0x01, 0x0A, 0xFF, 0x07
};

// ------------------------------------------------------------------
// Internal state
// ------------------------------------------------------------------
struct alignas(0x1000) TypingEngine::State {
    UsbDsInterface* interface = nullptr;
    UsbDsEndpoint* endpoint = nullptr;
    Event* stateEvent = nullptr;
    bool initialized = false;
    bool serviceOpen = false;
    bool deviceEnabled = false;
    bool ready = false;
    bool controlRead = false;
    bool statusOutAfterIn = false;
    bool controlInPending = false;
    bool controlOutPending = false;
    bool ledDataPending = false;
    bool descriptorRequest = false;
    bool descriptorInDone = false;
    bool descriptorOutDone = false;
    bool descriptorValid = false;
    bool transferPending = false;
    bool transferSucceeded = false;
    UsbState busState = UsbState_Detached;
    Result lastError = 0;
    u8 protocol = 1;
    u8 idle = 0;
    u8 leds = 0;
    u8 lastReport[8] = {};
    u32 controlInId = 0;
    u32 controlOutId = 0;
    u32 transferId = 0;
    size_t controlInExpected = 0;
    size_t controlOutExpected = 0;
    u64 configuredAtMs = 0;
    u64 controlStartedMs = 0;
    u64 transferStartedMs = 0;
    alignas(0x1000) u8 controlInBuffer[0x1000] = {};
    alignas(0x1000) u8 controlOutBuffer[0x1000] = {};
    alignas(0x1000) u8 transferBuffer[0x1000] = {};
};

static u64 GetNowMs() {
    return armTicksToNs(armGetSystemTick()) / 1000000ULL;
}

TypingEngine::TypingEngine() : m_state(new State()), m_delayMs(50) {}

TypingEngine::~TypingEngine() {
    Exit();
    delete m_state;
}

// ------------------------------------------------------------------
// Send a single 8-byte HID boot report, then a release report.
// ------------------------------------------------------------------
bool TypingEngine::SendHidReport(uint8_t mods, uint8_t key) {
    if(!m_state->ready || !m_state->endpoint) return false;

    auto sendOne = [&](u8 modifier, u8 usage) {
        if(!m_state->ready || !m_state->endpoint || m_state->transferPending) return false;

        std::memset(m_state->transferBuffer, 0, 8);
        m_state->transferBuffer[0] = modifier;
        m_state->transferBuffer[2] = usage;
        armDCacheFlush(m_state->transferBuffer, 8);

        Result rc = usbDsEndpoint_PostBufferAsync(m_state->endpoint,
                                                  m_state->transferBuffer, 8,
                                                  &m_state->transferId);
        if(R_FAILED(rc)) {
            m_state->lastError = rc;
            return false;
        }

        m_state->transferPending = true;
        m_state->transferSucceeded = false;
        m_state->transferStartedMs = GetNowMs();
        while(m_state->transferPending) {
            this->Poll();
            if(!m_state->transferPending) break;
            if(GetNowMs() - m_state->transferStartedMs > 2500) {
                m_state->lastError = KERNELRESULT(TimedOut);
                this->CancelPendingTransfer();
                return false;
            }
            svcSleepThread(1000000ULL);
        }
        return m_state->transferSucceeded;
    };

    if(!sendOne(mods, key) || !sendOne(0, 0)) return false;

    const u64 delayEnd = GetNowMs() + static_cast<u64>(m_delayMs);
    while(GetNowMs() < delayEnd) {
        this->Poll();
        if(!m_state->ready) return false;
        svcSleepThread(1000000ULL);
    }
    return true;
}

void TypingEngine::StallControl() {
    if(!m_state->interface) return;
    Result rc = usbDsInterface_StallCtrl(m_state->interface);
    if(R_FAILED(rc)) m_state->lastError = rc;
    m_state->controlInPending = false;
    m_state->controlOutPending = false;
    m_state->ledDataPending = false;
    m_state->statusOutAfterIn = false;
    m_state->descriptorRequest = false;
}

bool TypingEngine::QueueControlIn(const void* data, size_t size, uint16_t requested, uint64_t nowMs) {
    if(!m_state->interface || size > sizeof(m_state->controlInBuffer)) return false;
    if(size > requested) size = requested;
    if(size > 0) {
        std::memcpy(m_state->controlInBuffer, data, size);
        armDCacheFlush(m_state->controlInBuffer, size);
    }

    Result rc = usbDsInterface_CtrlInPostBufferAsync(m_state->interface,
                                                      m_state->controlInBuffer, size,
                                                      &m_state->controlInId);
    if(R_FAILED(rc)) {
        m_state->lastError = rc;
        this->StallControl();
        return false;
    }

    m_state->controlInPending = true;
    m_state->controlInExpected = size;
    m_state->controlStartedMs = nowMs;
    m_state->statusOutAfterIn = m_state->controlRead;
    return true;
}

bool TypingEngine::QueueControlOutStatus(uint64_t nowMs) {
    if(!m_state->interface) return false;
    Result rc = usbDsInterface_CtrlOutPostBufferAsync(m_state->interface,
                                                       m_state->controlOutBuffer, 0,
                                                       &m_state->controlOutId);
    if(R_FAILED(rc)) {
        m_state->lastError = rc;
        this->StallControl();
        return false;
    }

    m_state->controlOutPending = true;
    m_state->controlOutExpected = 0;
    m_state->controlStartedMs = nowMs;
    return true;
}

void TypingEngine::HandleSetupPacket(uint64_t nowMs) {
    if(!m_state->interface) return;

    struct usb_control_setup setup = {};
    Result rc = usbDsInterface_GetSetupPacket(m_state->interface, &setup, sizeof(setup));
    if(R_FAILED(rc)) {
        m_state->lastError = rc;
        return;
    }

    m_state->controlInPending = false;
    m_state->controlOutPending = false;
    m_state->ledDataPending = false;
    m_state->statusOutAfterIn = false;
    m_state->descriptorRequest = false;
    m_state->controlRead = (setup.bmRequestType & 0x80) != 0;

    if(setup.wIndex != 0) {
        this->StallControl();
        return;
    }

    if(setup.bmRequestType == 0x81 && setup.bRequest == USB_REQUEST_GET_STATUS) {
        const u16 flags = 0;
        this->QueueControlIn(&flags, sizeof(flags), setup.wLength, nowMs);
    } else if(setup.bmRequestType == 0x81 && setup.bRequest == USB_REQUEST_GET_INTERFACE
              && setup.wValue == 0) {
        const u8 alternate = 0;
        this->QueueControlIn(&alternate, sizeof(alternate), setup.wLength, nowMs);
    } else if(setup.bmRequestType == 0x01 && setup.bRequest == USB_REQUEST_SET_INTERFACE
              && setup.wValue == 0 && setup.wLength == 0) {
        this->QueueControlIn(nullptr, 0, 0, nowMs);
    } else if(setup.bmRequestType == 0x81 && setup.bRequest == USB_REQUEST_GET_DESCRIPTOR
              && (setup.wValue >> 8) == USB_DT_REPORT && (setup.wValue & 0xFF) == 0) {
        m_state->descriptorRequest = setup.wLength >= sizeof(s_reportDescriptor);
        m_state->descriptorInDone = false;
        m_state->descriptorOutDone = false;
        this->QueueControlIn(s_reportDescriptor, sizeof(s_reportDescriptor), setup.wLength, nowMs);
    } else if(setup.bmRequestType == 0x81 && setup.bRequest == USB_REQUEST_GET_DESCRIPTOR
              && (setup.wValue >> 8) == USB_DT_HID && (setup.wValue & 0xFF) == 0) {
        struct usb_hid_descriptor hidDescriptor = {
            USB_DT_HID_SIZE, USB_DT_HID, 0x0111, 0, 1,
            USB_DT_REPORT, static_cast<u16>(sizeof(s_reportDescriptor))
        };
        this->QueueControlIn(&hidDescriptor, sizeof(hidDescriptor), setup.wLength, nowMs);
    } else if(setup.bmRequestType == 0xA1 && setup.bRequest == 1 && (setup.wValue & 0xFF) == 0) {
        const u8 reportType = setup.wValue >> 8;
        if(reportType == 1) this->QueueControlIn(m_state->lastReport, 8, setup.wLength, nowMs);
        else if(reportType == 2) this->QueueControlIn(&m_state->leds, 1, setup.wLength, nowMs);
        else this->StallControl();
    } else if(setup.bmRequestType == 0xA1 && setup.bRequest == 2 && setup.wLength == 1) {
        this->QueueControlIn(&m_state->idle, 1, setup.wLength, nowMs);
    } else if(setup.bmRequestType == 0xA1 && setup.bRequest == 3 && setup.wLength == 1) {
        this->QueueControlIn(&m_state->protocol, 1, setup.wLength, nowMs);
    } else if(setup.bmRequestType == 0x21 && setup.bRequest == 10
              && setup.wLength == 0 && (setup.wValue & 0xFF) == 0) {
        m_state->idle = setup.wValue >> 8;
        this->QueueControlIn(nullptr, 0, 0, nowMs);
    } else if(setup.bmRequestType == 0x21 && setup.bRequest == 11
              && setup.wLength == 0 && setup.wValue <= 1) {
        m_state->protocol = setup.wValue;
        this->QueueControlIn(nullptr, 0, 0, nowMs);
    } else if(setup.bmRequestType == 0x21 && setup.bRequest == 9
              && setup.wValue == 0x0200 && setup.wLength == 1) {
        std::memset(m_state->controlOutBuffer, 0, sizeof(m_state->controlOutBuffer));
        rc = usbDsInterface_CtrlOutPostBufferAsync(m_state->interface,
                                                    m_state->controlOutBuffer, 1,
                                                    &m_state->controlOutId);
        if(R_FAILED(rc)) {
            m_state->lastError = rc;
            this->StallControl();
        } else {
            m_state->controlOutPending = true;
            m_state->controlOutExpected = 1;
            m_state->ledDataPending = true;
            m_state->controlStartedMs = nowMs;
        }
    } else {
        this->StallControl();
    }
}

void TypingEngine::CancelPendingTransfer() {
    if(!m_state->endpoint || !m_state->transferPending) return;
    usbDsEndpoint_Cancel(m_state->endpoint);
    eventClear(&m_state->endpoint->CompletionEvent);
    UsbDsReportData reports = {};
    usbDsEndpoint_GetReportData(m_state->endpoint, &reports);
    m_state->transferPending = false;
    m_state->transferSucceeded = false;
}

bool TypingEngine::Poll() {
    if(!m_state->initialized || !m_state->interface) return false;
    const u64 nowMs = GetNowMs();

    if(m_state->stateEvent && R_SUCCEEDED(eventWait(m_state->stateEvent, 0))) {
        eventClear(m_state->stateEvent);
    }

    UsbState busState = UsbState_Detached;
    Result rc = usbDsGetState(&busState);
    if(R_FAILED(rc)) {
        m_state->lastError = rc;
        m_state->ready = false;
        return false;
    }

    if(busState != m_state->busState) {
        m_state->busState = busState;
        if(busState == UsbState_Configured) {
            m_state->configuredAtMs = nowMs;
        } else {
            m_state->ready = false;
            m_state->controlInPending = false;
            m_state->controlOutPending = false;
            m_state->ledDataPending = false;
            m_state->statusOutAfterIn = false;
            m_state->descriptorRequest = false;
            m_state->descriptorInDone = false;
            m_state->descriptorOutDone = false;
            if(busState != UsbState_Suspended) {
                m_state->descriptorValid = false;
                std::memset(m_state->lastReport, 0, sizeof(m_state->lastReport));
                m_state->leds = 0;
                m_state->protocol = 1;
                m_state->idle = 0;
            }
            this->CancelPendingTransfer();
        }
    }

    if(R_SUCCEEDED(eventWait(&m_state->interface->CtrlInCompletionEvent, 0))) {
        eventClear(&m_state->interface->CtrlInCompletionEvent);
        UsbDsReportData reports = {};
        rc = usbDsInterface_GetCtrlInReportData(m_state->interface, &reports);
        if(R_FAILED(rc)) {
            m_state->lastError = rc;
        } else if(m_state->controlInPending) {
            u32 transferred = 0;
            rc = usbDsParseReportData(&reports, m_state->controlInId, nullptr, &transferred);
            m_state->controlInPending = false;
            if(R_FAILED(rc) || transferred != m_state->controlInExpected) {
                m_state->lastError = R_FAILED(rc) ? rc : KERNELRESULT(InvalidState);
                this->StallControl();
            } else {
                if(m_state->descriptorRequest) m_state->descriptorInDone = true;
                if(m_state->statusOutAfterIn) {
                    m_state->statusOutAfterIn = false;
                    this->QueueControlOutStatus(nowMs);
                }
            }
        }
    }

    if(R_SUCCEEDED(eventWait(&m_state->interface->CtrlOutCompletionEvent, 0))) {
        eventClear(&m_state->interface->CtrlOutCompletionEvent);
        UsbDsReportData reports = {};
        rc = usbDsInterface_GetCtrlOutReportData(m_state->interface, &reports);
        if(R_FAILED(rc)) {
            m_state->lastError = rc;
        } else if(m_state->controlOutPending) {
            u32 transferred = 0;
            rc = usbDsParseReportData(&reports, m_state->controlOutId, nullptr, &transferred);
            m_state->controlOutPending = false;
            if(R_FAILED(rc) || transferred != m_state->controlOutExpected) {
                m_state->lastError = R_FAILED(rc) ? rc : KERNELRESULT(InvalidState);
                this->StallControl();
            } else if(m_state->ledDataPending) {
                armDCacheFlush(m_state->controlOutBuffer, 1);
                m_state->leds = m_state->controlOutBuffer[0];
                m_state->ledDataPending = false;
                this->QueueControlIn(nullptr, 0, 0, nowMs);
            } else if(m_state->descriptorRequest) {
                m_state->descriptorOutDone = true;
            }
        }
    }

    if(m_state->descriptorRequest && m_state->descriptorInDone && m_state->descriptorOutDone) {
        m_state->descriptorValid = true;
    }

    if(R_SUCCEEDED(eventWait(&m_state->interface->SetupEvent, 0))) {
        eventClear(&m_state->interface->SetupEvent);
        this->HandleSetupPacket(nowMs);
    }

    if((m_state->controlInPending || m_state->controlOutPending)
       && nowMs - m_state->controlStartedMs > 1500) {
        m_state->lastError = KERNELRESULT(TimedOut);
        this->StallControl();
    }

    if(m_state->endpoint && R_SUCCEEDED(eventWait(&m_state->endpoint->CompletionEvent, 0))) {
        eventClear(&m_state->endpoint->CompletionEvent);
        UsbDsReportData reports = {};
        rc = usbDsEndpoint_GetReportData(m_state->endpoint, &reports);
        if(R_SUCCEEDED(rc) && m_state->transferPending) {
            u32 transferred = 0;
            rc = usbDsParseReportData(&reports, m_state->transferId, nullptr, &transferred);
            m_state->transferPending = false;
            if(R_SUCCEEDED(rc) && transferred == 8) {
                m_state->transferSucceeded = true;
                std::memcpy(m_state->lastReport, m_state->transferBuffer, 8);
            } else {
                m_state->lastError = R_FAILED(rc) ? rc : KERNELRESULT(InvalidState);
                m_state->transferSucceeded = false;
            }
        } else if(R_FAILED(rc)) {
            m_state->lastError = rc;
            m_state->transferPending = false;
            m_state->transferSucceeded = false;
        }
    }

    if(m_state->transferPending && nowMs - m_state->transferStartedMs > 2500) {
        m_state->lastError = KERNELRESULT(TimedOut);
        this->CancelPendingTransfer();
    }

    m_state->ready = busState == UsbState_Configured
                     && m_state->descriptorValid
                     && nowMs - m_state->configuredAtMs >= 300;
    return m_state->ready;
}

// ------------------------------------------------------------------
// Initialize the device and register the boot-keyboard interface.
// Host enumeration is serviced incrementally by Poll().
// ------------------------------------------------------------------
bool TypingEngine::Init() {
    if(m_state->initialized) return true;

    Result rc = usbDsInitialize();
    if(R_FAILED(rc)) {
        m_state->lastError = rc;
        return false;
    }
    m_state->serviceOpen = true;

    auto fail = [this](Result error) {
        m_state->lastError = error;
        this->Exit();
        m_state->lastError = error;
        return false;
    };

    rc = usbDsClearDeviceData();
    if(R_FAILED(rc)) return fail(rc);

    const u16 languages[] = {0x0409};
    u8 languageIndex = 0, manufacturerIndex = 0, productIndex = 0, serialIndex = 0;
    rc = usbDsAddUsbLanguageStringDescriptor(&languageIndex, languages, 1);
    if(R_FAILED(rc)) return fail(rc);
    rc = usbDsAddUsbStringDescriptor(&manufacturerIndex, "ezBasic");
    if(R_FAILED(rc)) return fail(rc);
    rc = usbDsAddUsbStringDescriptor(&productIndex, "ezBasic HID Keyboard");
    if(R_FAILED(rc)) return fail(rc);
    rc = usbDsAddUsbStringDescriptor(&serialIndex, "EZBASIC-001");
    if(R_FAILED(rc)) return fail(rc);

    struct usb_device_descriptor deviceDesc = {
        .bLength = USB_DT_DEVICE_SIZE,
        .bDescriptorType = USB_DT_DEVICE,
        .bcdUSB = 0x0200,
        .bDeviceClass = 0,
        .bDeviceSubClass = 0,
        .bDeviceProtocol = 0,
        .bMaxPacketSize0 = 0x40,
        .idVendor = 0x1209,
        .idProduct = 0x0001,
        .bcdDevice = 0x0107,
        .iManufacturer = manufacturerIndex,
        .iProduct = productIndex,
        .iSerialNumber = serialIndex,
        .bNumConfigurations = 1
    };
    rc = usbDsSetUsbDeviceDescriptor(UsbDeviceSpeed_Full, &deviceDesc);
    if(R_FAILED(rc)) return fail(rc);
    rc = usbDsSetUsbDeviceDescriptor(UsbDeviceSpeed_High, &deviceDesc);
    if(R_FAILED(rc)) return fail(rc);
    deviceDesc.bcdUSB = 0x0300;
    deviceDesc.bMaxPacketSize0 = 9;
    rc = usbDsSetUsbDeviceDescriptor(UsbDeviceSpeed_Super, &deviceDesc);
    if(R_FAILED(rc)) return fail(rc);

    rc = usbDsSetBinaryObjectStore(s_bos, sizeof(s_bos));
    if(R_FAILED(rc)) return fail(rc);

    rc = usbDsRegisterInterfaceEx(&m_state->interface, 0);
    if(R_FAILED(rc)) return fail(rc);

    struct usb_interface_descriptor interfaceDesc = {
        USB_DT_INTERFACE_SIZE, USB_DT_INTERFACE, 0, 0, 1,
        USB_CLASS_HID, 1, 1, 0
    };
    struct usb_hid_descriptor hidDesc = {
        USB_DT_HID_SIZE, USB_DT_HID, 0x0111, 0, 1,
        USB_DT_REPORT, static_cast<u16>(sizeof(s_reportDescriptor))
    };
    struct usb_endpoint_descriptor endpointDesc = {
        USB_DT_ENDPOINT_SIZE, USB_DT_ENDPOINT, 0x81,
        USB_TRANSFER_TYPE_INTERRUPT, 8, 1
    };
    const UsbDeviceSpeed speeds[] = {
        UsbDeviceSpeed_Full, UsbDeviceSpeed_High, UsbDeviceSpeed_Super
    };
    for(const auto speed : speeds) {
        endpointDesc.bInterval = (speed == UsbDeviceSpeed_Full) ? 1 : 4;
        rc = usbDsInterface_AppendConfigurationData(m_state->interface, speed,
                                                     &interfaceDesc, sizeof(interfaceDesc));
        if(R_FAILED(rc)) return fail(rc);
        rc = usbDsInterface_AppendConfigurationData(m_state->interface, speed,
                                                     &hidDesc, sizeof(hidDesc));
        if(R_FAILED(rc)) return fail(rc);
        rc = usbDsInterface_AppendConfigurationData(m_state->interface, speed,
                                                     &endpointDesc, sizeof(endpointDesc));
        if(R_FAILED(rc)) return fail(rc);
        if(speed == UsbDeviceSpeed_Super) {
            const u8 companion[] = {6, USB_DT_SS_ENDPOINT_COMPANION, 0, 0, 8, 0};
            rc = usbDsInterface_AppendConfigurationData(m_state->interface, speed,
                                                         companion, sizeof(companion));
            if(R_FAILED(rc)) return fail(rc);
        }
    }

    rc = usbDsInterface_RegisterEndpoint(m_state->interface, &m_state->endpoint, 0x81);
    if(R_FAILED(rc)) return fail(rc);
    rc = usbDsInterface_EnableInterface(m_state->interface);
    if(R_FAILED(rc)) return fail(rc);
    rc = usbDsEnable();
    if(R_FAILED(rc)) return fail(rc);
    m_state->deviceEnabled = true;

    m_state->stateEvent = usbDsGetStateChangeEvent();
    rc = usbDsGetState(&m_state->busState);
    if(R_FAILED(rc)) return fail(rc);
    if(m_state->busState == UsbState_Configured) m_state->configuredAtMs = GetNowMs();
    m_state->initialized = true;
    m_state->ready = false;
    m_state->lastError = 0;
    return true;
}

bool TypingEngine::IsInitialized() const {
    return m_state->initialized;
}

bool TypingEngine::IsReady() const {
    return m_state->ready;
}

// ------------------------------------------------------------------
// Clean up usbDs.
// ------------------------------------------------------------------
void TypingEngine::Exit() {
    if(m_state->transferPending) this->CancelPendingTransfer();
    if(m_state->interface) usbDsInterface_DisableInterface(m_state->interface);
    if(m_state->deviceEnabled) usbDsDisable();
    if(m_state->serviceOpen) usbDsExit();

    m_state->endpoint = nullptr;
    m_state->interface = nullptr;
    m_state->stateEvent = nullptr;
    m_state->initialized = false;
    m_state->serviceOpen = false;
    m_state->deviceEnabled = false;
    m_state->ready = false;
    m_state->controlInPending = false;
    m_state->controlOutPending = false;
    m_state->transferPending = false;
    m_state->descriptorValid = false;
}

// ------------------------------------------------------------------
// Pre-flight verification: every byte must encode successfully.
// CRLF pairs are skipped for counting.
// ------------------------------------------------------------------
bool TypingEngine::VerifyText(const char* text, size_t len,
                              size_t* outStrokes,
                              size_t* outLines,
                              size_t* failOffset) {
    size_t strokes = 0;
    size_t lines   = 0;

    for (size_t i = 0; i < len; ++i) {
        if (text[i] == '\r' && i + 1 < len && text[i + 1] == '\n') {
            continue;
        }
        HidStroke key;
        if (!HidEncode(text[i], key)) {
            if (failOffset) *failOffset = i;
            return false;
        }
        strokes++;
        if (text[i] == '\n') lines++;
    }

    if (outStrokes) *outStrokes = strokes;
    if (outLines)   *outLines   = lines;
    return true;
}

// ------------------------------------------------------------------
// Type text with indent management, CRLF coalescing, and progress.
// ------------------------------------------------------------------
bool TypingEngine::TypeText(const char* text, size_t len,
                            std::function<void(size_t lines, size_t totalLines)> progressCb,
                            std::atomic<bool>& cancelFlag) {
    if (!m_state->ready) return false;

    // Verification + line count
    size_t totalLines = 0;
    if (!VerifyText(text, len, nullptr, &totalLines, nullptr)) {
        return false;
    }

    size_t inheritedIndent = 0;
    bool lineStart = true;
    size_t sentLines = 0;

    for (size_t i = 0; i < len; ++i) {
        if (cancelFlag.load()) return false;

        // Skip CRLF pair (treat as single newline)
        if (text[i] == '\r' && i + 1 < len && text[i + 1] == '\n') {
            continue;
        }

        // Indent management at line start
        if (lineStart) {
            size_t desired = 0;
            while (i + desired < len && text[i + desired] == ' ') desired++;

            while (inheritedIndent > desired && !cancelFlag.load()) {
                if (!SendHidReport(0, 0x2A)) return false; // Backspace
                inheritedIndent--;
            }
            while (inheritedIndent < desired && !cancelFlag.load()) {
                if (!SendHidReport(0, 0x2C)) return false; // Space
                inheritedIndent++;
            }

            i += desired;
            lineStart = false;
            if (i >= len || cancelFlag.load()) break;
        }

        unsigned char c = static_cast<unsigned char>(text[i]);

        // Another inline CRLF guard (paranoid, matches upstream)
        if (c == '\r' && i + 1 < len && text[i + 1] == '\n') {
            continue;
        }

        HidStroke key;
        if (!HidEncode(static_cast<char>(c), key)) return false;
        if (!SendHidReport(key.mods, key.key)) return false;

        if (c == '\n') {
            sentLines++;
            lineStart = true;
            if (progressCb) {
                progressCb(sentLines, totalLines);
            }
        }
    }

    return !cancelFlag.load();
}

} // namespace ezbasic
