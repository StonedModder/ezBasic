#include <TypingEngine.hpp>
#include <switch.h>

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>

namespace {
constexpr std::size_t DmaAlignment = 4096;
constexpr std::size_t MaxTransfer = 4096;

Event stateEvent;
UsbDsInterface mockInterface;
UsbDsEndpoint mockEndpoint;
UsbState busState = UsbState_Detached;
usb_control_setup setupPacket{};
u64 nowNs = 0;
u32 nextUrb = 1;
u32 lastControlInId = 0;
u32 lastControlOutId = 0;
u32 lastEndpointId = 0;
std::size_t lastControlInSize = 0;
std::size_t lastControlOutSize = 0;
std::array<u8, MaxTransfer> controlInData{};
std::array<std::array<u8, 8>, 16> endpointReports{};
std::size_t endpointReportCount = 0;
unsigned clearCount = 0;
unsigned languageCount = 0;
unsigned stringCount = 0;
unsigned stalledCount = 0;
unsigned disabledCount = 0;
unsigned exitedCount = 0;
std::array<usb_device_descriptor, 5> deviceDescriptors{};
std::array<std::array<u8, 128>, 3> configurations{};
std::array<std::size_t, 3> configurationSizes{};
std::array<u8, 32> bosData{};
std::size_t bosSize = 0;

std::size_t SpeedIndex(UsbDeviceSpeed speed) {
    return static_cast<std::size_t>(speed) - static_cast<std::size_t>(UsbDeviceSpeed_Full);
}

void AssertAligned(const void* ptr) {
    assert(reinterpret_cast<std::uintptr_t>(ptr) % DmaAlignment == 0);
}
}

u64 armGetSystemTick(void) { return nowNs; }
u64 armTicksToNs(u64 ticks) { return ticks; }
void svcSleepThread(u64 ns) { nowNs += ns; }
void armDCacheFlush(void*, std::size_t) {}
Result eventWait(Event* event, u64) { return event->fired ? 0 : KERNELRESULT(TimedOut); }
Result eventClear(Event* event) { event->fired = false; return 0; }
Result usbDsInitialize(void) { return 0; }
void usbDsExit(void) { ++exitedCount; }
void usbDsDisable(void) { ++disabledCount; }
Event* usbDsGetStateChangeEvent(void) { return &stateEvent; }
Result usbDsGetState(UsbState* out) { *out = busState; return 0; }
Result usbDsClearDeviceData(void) { ++clearCount; return 0; }
Result usbDsAddUsbLanguageStringDescriptor(u8* out, const u16* ids, u16 count) { assert(count == 1 && ids[0] == 0x0409); *out = 0; ++languageCount; return 0; }
Result usbDsAddUsbStringDescriptor(u8* out, const char*) { *out = static_cast<u8>(++stringCount); return 0; }
Result usbDsSetUsbDeviceDescriptor(UsbDeviceSpeed speed, usb_device_descriptor* desc) {
    assert(desc->idVendor == 0x1209 && desc->idProduct == 0x0001);
    deviceDescriptors[static_cast<std::size_t>(speed)] = *desc;
    return 0;
}
Result usbDsSetBinaryObjectStore(const void* data, std::size_t size) {
    assert(size <= bosData.size());
    bosSize = size;
    std::memcpy(bosData.data(), data, size);
    return 0;
}
Result usbDsRegisterInterfaceEx(UsbDsInterface** out, u8 number) { assert(number == 0); *out = &mockInterface; return 0; }
Result usbDsInterface_AppendConfigurationData(UsbDsInterface* iface, UsbDeviceSpeed speed, const void* data, std::size_t size) {
    assert(iface == &mockInterface);
    const std::size_t index = SpeedIndex(speed);
    assert(index < configurations.size() && configurationSizes[index] + size <= configurations[index].size());
    std::memcpy(configurations[index].data() + configurationSizes[index], data, size);
    configurationSizes[index] += size;
    return 0;
}
Result usbDsInterface_RegisterEndpoint(UsbDsInterface* iface, UsbDsEndpoint** out, u8 address) { assert(iface == &mockInterface && address == 0x81); *out = &mockEndpoint; return 0; }
Result usbDsInterface_EnableInterface(UsbDsInterface* iface) { assert(iface == &mockInterface); return 0; }
Result usbDsInterface_DisableInterface(UsbDsInterface* iface) { assert(iface == &mockInterface); return 0; }
Result usbDsEnable(void) { return 0; }
Result usbDsInterface_GetSetupPacket(UsbDsInterface* iface, void* out, std::size_t size) {
    assert(iface == &mockInterface && size == sizeof(setupPacket));
    std::memcpy(out, &setupPacket, size);
    return 0;
}
Result usbDsInterface_StallCtrl(UsbDsInterface* iface) { assert(iface == &mockInterface); ++stalledCount; return 0; }
Result usbDsInterface_CtrlInPostBufferAsync(UsbDsInterface* iface, void* buffer, std::size_t size, u32* id) {
    assert(iface == &mockInterface && size <= controlInData.size());
    AssertAligned(buffer);
    lastControlInSize = size;
    std::memcpy(controlInData.data(), buffer, size);
    lastControlInId = *id = nextUrb++;
    return 0;
}
Result usbDsInterface_CtrlOutPostBufferAsync(UsbDsInterface* iface, void* buffer, std::size_t size, u32* id) {
    assert(iface == &mockInterface && size <= 1);
    AssertAligned(buffer);
    lastControlOutSize = size;
    if(size == 1) static_cast<u8*>(buffer)[0] = 2;
    lastControlOutId = *id = nextUrb++;
    return 0;
}
Result usbDsInterface_GetCtrlInReportData(UsbDsInterface* iface, UsbDsReportData* out) {
    assert(iface == &mockInterface);
    *out = {};
    out->report_count = 1;
    out->report[0] = {lastControlInId, static_cast<u32>(lastControlInSize), static_cast<u32>(lastControlInSize), 3};
    return 0;
}
Result usbDsInterface_GetCtrlOutReportData(UsbDsInterface* iface, UsbDsReportData* out) {
    assert(iface == &mockInterface);
    *out = {};
    out->report_count = 1;
    out->report[0] = {lastControlOutId, static_cast<u32>(lastControlOutSize), static_cast<u32>(lastControlOutSize), 3};
    return 0;
}
Result usbDsEndpoint_Cancel(UsbDsEndpoint* endpoint) { assert(endpoint == &mockEndpoint); return 0; }
Result usbDsEndpoint_PostBufferAsync(UsbDsEndpoint* endpoint, void* buffer, std::size_t size, u32* id) {
    assert(endpoint == &mockEndpoint && size == 8 && endpointReportCount < endpointReports.size());
    AssertAligned(buffer);
    std::memcpy(endpointReports[endpointReportCount++].data(), buffer, 8);
    lastEndpointId = *id = nextUrb++;
    mockEndpoint.CompletionEvent.fired = true;
    return 0;
}
Result usbDsEndpoint_GetReportData(UsbDsEndpoint* endpoint, UsbDsReportData* out) {
    assert(endpoint == &mockEndpoint);
    *out = {};
    out->report_count = 1;
    out->report[0] = {lastEndpointId, 8, 8, 3};
    return 0;
}
Result usbDsParseReportData(UsbDsReportData* reports, u32 id, u32* requested, u32* transferred) {
    if(reports->report_count == 0 || reports->report[0].id != id || reports->report[0].urb_status != 3) return 1;
    if(requested) *requested = reports->report[0].requestedSize;
    if(transferred) *transferred = reports->report[0].transferredSize;
    return 0;
}

int main() {
    ezbasic::TypingEngine engine;
    assert(engine.Init());
    assert(engine.IsInitialized() && !engine.IsReady());
    assert(clearCount == 1 && languageCount == 1 && stringCount == 3);
    assert(bosSize == 22 && bosData[18] == 1 && bosData[19] == 10);
    assert(deviceDescriptors[UsbDeviceSpeed_Full].bMaxPacketSize0 == 64);
    assert(deviceDescriptors[UsbDeviceSpeed_Super].bMaxPacketSize0 == 9);
    assert(configurationSizes[0] == 25 && configurationSizes[1] == 25 && configurationSizes[2] == 31);
    assert(configurations[0][24] == 1 && configurations[1][24] == 4 && configurations[2][24] == 4);
    assert(configurations[2][25] == 6 && configurations[2][26] == USB_DT_SS_ENDPOINT_COMPANION);

    busState = UsbState_Configured;
    assert(!engine.Poll());
    setupPacket = {0x81, USB_REQUEST_GET_DESCRIPTOR, 0x2200, 0, 255};
    mockInterface.SetupEvent.fired = true;
    assert(!engine.Poll());
    assert(lastControlInSize == 63 && controlInData[0] == 0x05 && controlInData[1] == 0x01);
    assert(!engine.IsReady());

    mockInterface.CtrlInCompletionEvent.fired = true;
    assert(!engine.Poll());
    assert(lastControlOutSize == 0);
    mockInterface.CtrlOutCompletionEvent.fired = true;
    assert(!engine.Poll());
    assert(!engine.IsReady());
    svcSleepThread(300000000ULL);
    assert(engine.Poll() && engine.IsReady());

    std::atomic<bool> cancel{false};
    std::size_t progressed = 0;
    assert(engine.TypeText("A\n", 2, [&](std::size_t lines, std::size_t total) {
        assert(total == 1);
        progressed = lines;
    }, cancel));
    assert(progressed == 1 && endpointReportCount == 4);
    assert(endpointReports[0][2] == 4 && endpointReports[2][2] == 40);

    busState = UsbState_Default;
    assert(!engine.Poll() && !engine.IsReady());
    busState = UsbState_Configured;
    assert(!engine.Poll());
    setupPacket = {0x81, USB_REQUEST_GET_DESCRIPTOR, 0x2200, 0, 255};
    mockInterface.SetupEvent.fired = true;
    assert(!engine.Poll());
    mockInterface.CtrlInCompletionEvent.fired = true;
    assert(!engine.Poll());
    mockInterface.CtrlOutCompletionEvent.fired = true;
    assert(!engine.Poll());
    svcSleepThread(300000000ULL);
    assert(engine.Poll() && engine.IsReady());

    engine.Exit();
    assert(exitedCount == 1 && disabledCount == 1);
    std::cout << "PASS: Plume-style async enumeration, descriptor/status acknowledgement, reset recovery, DMA alignment, SS descriptors and HID report completion\n";
    return 0;
}
