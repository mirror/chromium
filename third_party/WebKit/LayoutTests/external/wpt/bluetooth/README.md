# Web Bluetooth Testing

Web Bluetooth testing relies on the [Web Bluetooth Testing API] which must be
provided by browsers under test.

In this test suite `resources/bluetooth-helpers.js` detects and triggers
the API to be loaded as needed.

The Chromium implementation is provided by
`../resources/chromium/web-bluetooth-test.js`.

[Web Bluetooth Testing API]: https://docs.google.com/document/d/1Nhv_oVDCodd1pEH_jj9k8gF4rPGb_84VYaZ9IG8M_WY/

# generator.py

For all .js files in script-tests/, generator.py will attempt to build tests
in bluetooth/.

Note that for each subdirectory in script-test there is a matching directory
under bluetooth/.  The generator will expand CALL functions into this
corresponding directory.

Example:

bluetooth/script-tests/server/get-same-object.js expanded CALL will generate 3
files:

```
bluetooth/server/getPrimaryService/gen-get-same-object.html
bluetooth/server/getPrimaryServices/gen-get-same-object.html
bluetooth/server/getPrimaryServices/gen-get-same-object-with-uuid.html
```

This is because of the following lines CALL:

```
gattServer.CALLS([
        getPrimaryService('heart_rate')|
        getPrimaryServices()|
        getPrimaryServices('heart_rate')[UUID]]),
```

Generate test files from templates in script-tests/*.js by running:

```
$ python //third_party/WebKit/LayoutTests/bluetooth/generate.py
```