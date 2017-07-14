# WebUSB Blink Module

`Source/modules/webusb` implements the renderer process details and bindings
for the [WebUSB specification]. It uses the [WebUSB Service] via the interface
defined in [USB mojom].

[WebUSB specification]: https://wicg.github.io/webusb/
[WebUSB Service]: /device/usb/usb_service.h
[USB mojom]: /device/usb/public/interfaces


## Testing

WebUSB is primarily tested in [Web Platform Tests].
Chromium implementation details are tested in [Layout Tests].

[Web Platform Tests]: ../../../LayoutTests/external/wpt/webusb/
[Layout Tests]: ../../../LayoutTests/usb/