# Web Bluetooth Testing

Web Bluetooth testing relies on the [Web Bluetooth Testing API] which must be
provided by browsers under test.

In this test suite `resources/bluetooth-helpers.js` detects and triggers
the API to be loaded as needed.

The Chromium implementation is provided by
`../resources/chromium/web-bluetooth-test.js`.

The Chromium implementation is not included in stable Chrome builds due to
adding too much to the binary size. It is run on the Chromium infrastructure
when web platform tests are run using the 'content\_shell' executable.

It is possible in the future that Chromium src/device/bluetooth is refactored
into a Mojo service. Then web platform tests could inject an implementation for
testing into stable Chrome run on Web Platform Tests infrastructure, similar to
WebUSB.

These bluetooth tests are upstreamed here because other browsers can reuse them
by implementing the [Web Bluetooth Testing API], even if only on their internal
infrastructure.

[Web Bluetooth Testing API]: https://docs.google.com/document/d/1Nhv_oVDCodd1pEH_jj9k8gF4rPGb_84VYaZ9IG8M_WY/
