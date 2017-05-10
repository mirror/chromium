# How to Deal with Android Size Alerts

*Not all alerts should not have a bug created for them. Please read on...*

### If the alert is for "other lib size" or "Unknown files size":
 * File a bug against agrieve@ to fix
   [resource_sizes.py](https://cs.chromium.org/chromium/src/build/android/resource_sizes.py).
 * ...or fix it yourself. This script will output the list of unknown
   filenames.

### If the alert is a downstream size alert (aka, for Monochrome.apk):
 * The regression most likely already occurred in the upstream
   MonochromePublic.apk target. Look at the
   [upstream graphs](https://chromeperf.appspot.com/report?sid=cfc29eed1238fd38fb5e6cf83bdba6c619be621b606e03e5dfc2e99db14c418b&num_points=1500)
   to find the culprit & de-dupe with upstream alerts.
 * If no upstream regression was found, look through the downstream commits
   within the given date range to find the culprit.
    * Via `git log --format=fuller` (be sure to look at `CommitDate` and not
      `AuthorDate`)

### If the alert is for a roll, or has multiple commits listed:
 * Bisects [will not help you](https://bugs.chromium.org/p/chromium/issues/detail?id=678338).
 * If you do not have the bandwidth to run the command locally, just file a bug
   and leave it untriaged.
 * If you can afford to run a fire-and-forget command locally, use a
   [local Android checkout](https://chromium.googlesource.com/chromium/src/+/master/docs/android_build_instructions.md)
   along with [`//tools/binary_size/diagnose_bloat.py`](https://chromium.googlesource.com/chromium/src/+/master/tools/binary_size/README.md)
   to build all commits locally and find the culprit.

Example:

     tools/binary_size/diagnose_bloat.py AFTER_REV --reference-rev BEFORE_REV --subrepo v8 --all

### What to do once the commit is identified:
If the code seems to justify the size increase:
 * Annotate the code review with the following (replacing **bold** parts):
    > FYI - this added **20kb** to Chrome on Android. No action is required
    > (unless you can think of an obvious way to reduce the overhead).
    >
    > Link to size graph:
[https://chromeperf.appspot.com/report?sid=a097e74b1aa288511afb4cb616efe0f95ba4d347ad61d5e835072f23450938ba&rev=**440074**](https://chromeperf.appspot.com/report?sid=cfc29eed1238fd38fb5e6cf83bdba6c619be621b606e03e5dfc2e99db14c418b&rev=440074)


If the code might not justify the size increase:
 * File a bug (TODO(agrieve):
[https://github.com/catapult-project/catapult/issues/3150](Make this template automatic)).
    * Change the bug's title from X% to XXkb
    * Remove label: `Restrict-View-Google`
    * Assign to commit author.
    * Set description to (replacing **bold** parts):
      > Alert caused by "**First line of commit message**"
      > Commit: abc123abc123abc123abc123abc123abc123abcd
      > Link to size graph:
[https://chromeperf.appspot.com/report?sid=a097e74b1aa288511afb4cb616efe0f95ba4d347ad61d5e835072f23450938ba&rev=**440074**](https://chromeperf.appspot.com/report?sid=cfc29eed1238fd38fb5e6cf83bdba6c619be621b606e03e5dfc2e99db14c418b&rev=440074)
      >
      > How to debug this size regressions is documented at:
https://chromium.googlesource.com/chromium/src/+/master/tools/perf/docs/apk_size_regressions.md#Debugging-Apk-Size-Increase

# Debugging Apk Size Increase

## Step 1: Identify What Grew

Figure out which file within the .apk increased (native library, dex file,
pak files, etc) by looking at the breakdown in the size graphs linked to in the
bug (if it was not linked in the bug, see above).

## Step 2: Analyze

If the growth is from strings within `.grd` files:

 * There is likely nothing that can be done. Translations are expensive.
 * Close as `Won't Fix`.

If the growth is from images, ensure they are optimized:

  * Would it be [smaller as a VectorDrawable](https://codereview.chromium.org/2857893003/)?
  * If it's lossy, consider [using webp](https://codereview.chromium.org/2615243002/).
  * Ensure you've optimized with
    [tools/resources/optimize-png-files.sh](https://cs.chromium.org/chromium/src/tools/resources/optimize-png-files.sh).
  * There is some [Googler-specific guidance](https://goto.google.com/clank/engineering/best-practices/adding-image-assets) as well.

If the growth is from native code:

 * Use [//tools/binary_size/diagnose_bloat.py](https://chromium.googlesource.com/chromium/src/+/master/tools/binary_size/README.md)
to show a diff of ELF symbols.
 * Paste the diff into the bug.
 * If the diff looks reasonable, close as `Won't Fix`.
 * Otherwise, try to refactor a bit (e.g.
 [move code out of templates](https://bugs.chromium.org/p/chromium/issues/detail?id=716393))

If the growth is from java code:

 * Use [tools/android/dexdiffer/dexdiffer.py](https://cs.chromium.org/chromium/src/tools/android/dexdiffer/dexdiffer.py).
    * This currently just shows a list of symbols added / removed rather than
      taking into account method body sizes.
    * Enhancements to this tool tracked at
      [crbug/678044](https://bugs.chromium.org/p/chromium/issues/detail?id=678044).

If you would like assistance with a regression:
 * Feel free to email [binary-size@chromium.org](https://groups.google.com/a/chromium.org/forum/#!forum/binary-size).
