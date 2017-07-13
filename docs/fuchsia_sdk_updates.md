# Publishing a new Fuchsia SDK, and updating Chrome to use it

Updating the Fuchsia SDK in Chromium currently involves:
* Building and testing the latest Fuchsia version.
* Packaging the SDK from that and uploading to cloud storage.
* Updating Chromium with DEPS and any other changes to build with the new SDK.

## Build and test Fuchsia

* check if fuchsia-dashboard.appspot.com looks green
* sync
* fbuild
* fboot on Intel NUC
* runtests on device and make sure everything passes

## Build and upload the SDK
* configure and build a release mode build with sdk config:

  $ ./packages/gn/gen.py -r --goma -m runtime
  $ ./packages/gn/build.py -r -j1000

* assemble sysroot and package tools into tgz

  $ ./scripts/makesdk.go .

* compute sha1

  $ sha1sum fuchsia-sdk.tgz
  f79f55be4e69ebd90ea84f79d7322525853256c3

* upload to gs bucket

$ gsutil.py cp fuchsia-sdk.tgz gs://fuchsia-build/fuchsia/sdk/linux64/f79f55be4e69ebd90ea84f79d7322525853256c

## Update Chromium

Then, on the chromium side, update DEPS to point at this sdk rev, build
and confirm things are OK, and make any changes required.
