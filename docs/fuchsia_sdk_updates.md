# Updating Chromium to a new Fuchsia SDK

1. Check the [Fuchsia-side
   job](https://luci-scheduler.appspot.com/jobs/fuchsia/sdk-x86_64-linux) for a
   recent green archive. On the "SUCCEEDED" link, copy the SHA-1 from the
   `gsutil.upload` link of the `upload fuchsia-sdk` step.
0. Put that into Chromium's src.git `build/fuchsia/update_sdk.py` as `SDK_HASH`.
0. `gclient sync && ninja ...` and make sure things go OK locally.
0. Upload the roll CL, making sure to include the `fuchsia` trybot. Tag the roll
   with `Bug: 707030`.

If you would like to build an SDK locally, `tools/fuchsia/local-sdk.py` tries to
do this (so you can iterate on ToT Fuchsia against your Chromium build), however
it's simply a copy of the steps run on the bot above, and so may be out of date.

In order to sync a Fuchsia tree to the state matching an SDK hash, you can use:

`jiri update https://storage.googleapis.com/fuchsia/jiri/snapshots/SDK_HASH_HERE`

If you are waiting for a Zircon CL to roll into the SDK, you can check the
status of the [Zircon
roller](https://luci-scheduler.appspot.com/jobs/fuchsia/zircon-roller).
Checking the bot's [list of
CLs](https://fuchsia-review.googlesource.com/q/owner:zircon-roller%40fuchsia-infra.iam.gserviceaccount.com)
might be useful too.
