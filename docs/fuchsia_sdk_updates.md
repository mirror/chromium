# Publishing a new Fuchsia SDK, and updating Chrome to use it

Updating the Fuchsia SDK in Chromium currently involves:
1. Building and testing the latest Fuchsia version.
0. Packaging the SDK from that and uploading to cloud storage.
0. Updating Chromium with DEPS and any other changes to build with the new SDK.

## Build and test Fuchsia

For current documentation on Fuchsia setup, visit the [Fuchsia Getting Started Guide](https://fuchsia.googlesource.com/docs/+/HEAD/getting_started.md).

Perform these steps on your Linux workstation:
1. Check if fuchsia-dashboard.appspot.com looks green.
0. Fetch the latest Fuchsia source.

       $ jiri update

0. Build Magenta, the sysroot, and Fuchsia.

       $ fbuild

Now verify that the build is stable:

1. Either: Boot the build in QEMU - you will probably want to use -k to enable KVM acceleration (see the guide for details of other options to enable networking, graphics, etc).

        $ frun -k
  
0. Or (preferably): Boot the build on hardware (see the guide for details of first-time setup).

        $ fboot

0. Run tests to verify the build. All the tests should pass!

        $ runtests

## Build and upload the SDK
1. Configure and build a release mode build with sdk config:

        $ ./packages/gn/gen.py --release --goma -m runtime
        $ ./packages/gn/build.py --release -j1000

0. Package the sysroot and tools.

        $ ./scripts/makesdk.go .

0. Compute the SHA-1 hash of the tarball, to use as its filename.

        $ sha1sum fuchsia-sdk.tgz
        f79f55be4e69ebd90ea84f79d7322525853256c3

0. Upload the tarball to the fuchsia-build build, under the SDK path. This will require "create" access to the bucket.

        $ gsutil.py cp fuchsia-sdk.tgz gs://fuchsia-build/fuchsia/sdk/linux64/f79f55be4e69ebd90ea84f79d7322525853256c

## Update Chromium

Then, on the chromium side:
1. Update the fuchsia entry in the top-level DEPS to specify the SHA-1 for this revision.
0. Run gclient sync, to have it pull down your new SDK build.
0. Verify that things build, and tests run.
0. Make any necessary changes to get things working, and include them in the DEPS-rolling CL.
