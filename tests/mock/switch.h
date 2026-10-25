#pragma once
#include <cstddef>
#include <cstdint>

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using Result = std::uint32_t;

struct Event { bool fired = false; };
enum UsbState : int { UsbState_Detached, UsbState_Attached, UsbState_Powered, UsbState_Default, UsbState_Address, UsbState_Configured, UsbState_Suspended };
enum UsbDeviceSpeed : int { UsbDeviceSpeed_None, UsbDeviceSpeed_Low, UsbDeviceSpeed_Full, UsbDeviceSpeed_High, UsbDeviceSpeed_Super };
struct UsbDsInterface { Event SetupEvent, CtrlInCompletionEvent, CtrlOutCompletionEvent; };
struct UsbDsEndpoint { Event CompletionEvent; };
struct UsbDsReportEntry { u32 id, requestedSize, transferredSize, urb_status; };
struct UsbDsReportData { UsbDsReportEntry report[8]; u32 report_count; };

#pragma pack(push, 1)
struct usb_control_setup { u8 bmRequestType, bRequest; u16 wValue, wIndex, wLength; };
struct usb_hid_descriptor { u8 bLength, bDescriptorType; u16 bcdHID; u8 bCountryCode, bNumDescriptors, bClassDescriptorType; u16 wClassDescriptorLength; };
#pragma pack(pop)
struct usb_device_descriptor {
    u8 bLength, bDescriptorType; u16 bcdUSB; u8 bDeviceClass, bDeviceSubClass, bDeviceProtocol, bMaxPacketSize0;
    u16 idVendor, idProduct, bcdDevice; u8 iManufacturer, iProduct, iSerialNumber, bNumConfigurations;
};
struct usb_interface_descriptor { u8 bLength, bDescriptorType, bInterfaceNumber, bAlternateSetting, bNumEndpoints, bInterfaceClass, bInterfaceSubClass, bInterfaceProtocol, iInterface; };
struct __attribute__((packed)) usb_endpoint_descriptor { u8 bLength, bDescriptorType, bEndpointAddress, bmAttributes; u16 wMaxPacketSize; u8 bInterval; };

#define USB_DT_DEVICE_SIZE 0x12
#define USB_DT_INTERFACE_SIZE 0x09
#define USB_DT_ENDPOINT_SIZE 0x07
#define USB_DT_HID_SIZE 0x09
#define USB_DT_DEVICE 0x01
#define USB_DT_INTERFACE 0x04
#define USB_DT_ENDPOINT 0x05
#define USB_DT_BOS 0x0f
#define USB_DT_DEVICE_CAPABILITY 0x10
#define USB_DT_HID 0x21
#define USB_DT_REPORT 0x22
#define USB_DT_SS_ENDPOINT_COMPANION 0x30
#define USB_CLASS_HID 0x03
#define USB_TRANSFER_TYPE_INTERRUPT 0x03
#define USB_REQUEST_GET_STATUS 0x00
#define USB_REQUEST_GET_DESCRIPTOR 0x06
#define USB_REQUEST_GET_INTERFACE 0x0a
#define USB_REQUEST_SET_INTERFACE 0x0b
#define R_FAILED(rc) ((rc) != 0)
#define R_SUCCEEDED(rc) ((rc) == 0)
enum { Kernel_TimedOut = 1, Kernel_InvalidState = 2 };
#define KERNELRESULT(error) ((Result)(0x10000U + Kernel_##error))

u64 armGetSystemTick(void);
u64 armTicksToNs(u64 ticks);
void svcSleepThread(u64 ns);
void armDCacheFlush(void* ptr, std::size_t size);
Result eventWait(Event* event, u64 timeout);
Result eventClear(Event* event);
Result usbDsInitialize(void);
void usbDsExit(void);
void usbDsDisable(void);
Event* usbDsGetStateChangeEvent(void);
Result usbDsGetState(UsbState* out);
Result usbDsClearDeviceData(void);
Result usbDsAddUsbLanguageStringDescriptor(u8* out, const u16* ids, u16 count);
Result usbDsAddUsbStringDescriptor(u8* out, const char* text);
Result usbDsSetUsbDeviceDescriptor(UsbDeviceSpeed speed, usb_device_descriptor* desc);
Result usbDsSetBinaryObjectStore(const void* data, std::size_t size);
Result usbDsRegisterInterfaceEx(UsbDsInterface** out, u8 number);
Result usbDsInterface_AppendConfigurationData(UsbDsInterface* iface, UsbDeviceSpeed speed, const void* data, std::size_t size);
Result usbDsInterface_RegisterEndpoint(UsbDsInterface* iface, UsbDsEndpoint** out, u8 address);
Result usbDsInterface_EnableInterface(UsbDsInterface* iface);
Result usbDsInterface_DisableInterface(UsbDsInterface* iface);
Result usbDsEnable(void);
Result usbDsInterface_GetSetupPacket(UsbDsInterface* iface, void* out, std::size_t size);
Result usbDsInterface_StallCtrl(UsbDsInterface* iface);
Result usbDsInterface_CtrlInPostBufferAsync(UsbDsInterface* iface, void* buffer, std::size_t size, u32* id);
Result usbDsInterface_CtrlOutPostBufferAsync(UsbDsInterface* iface, void* buffer, std::size_t size, u32* id);
Result usbDsInterface_GetCtrlInReportData(UsbDsInterface* iface, UsbDsReportData* out);
Result usbDsInterface_GetCtrlOutReportData(UsbDsInterface* iface, UsbDsReportData* out);
Result usbDsEndpoint_Cancel(UsbDsEndpoint* endpoint);
Result usbDsEndpoint_PostBufferAsync(UsbDsEndpoint* endpoint, void* buffer, std::size_t size, u32* id);
Result usbDsEndpoint_GetReportData(UsbDsEndpoint* endpoint, UsbDsReportData* out);
Result usbDsParseReportData(UsbDsReportData* reports, u32 id, u32* requested, u32* transferred);
