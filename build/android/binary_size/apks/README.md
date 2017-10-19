## Milestone Reference APKs

This folder contains APKs for official (upstream) builds for each milestone.
The primary use for these APKs is per-milestone binary size analysis.
  * //build/android/resource_sizes.py uses them for calculating patch size
  * They can be used with tools/binary_size/diagnose_bloat.py for examing
    what grew in an APK milestone-to-milestone

## Downloading Reference APKs

```bash
# Downloads ARM 32 MonochromePublic.apk for the latest milestone that we've
# uploaded apks for.
build/android/binary_size/apk_downloader.py

# Print usage and see all options.
build/android/binary_size/apk_downloader.py -h
```

## Updating Reference APKs
```bash
# Downloads build products from perf builders and uploads the following APKs
# for M62 and M63:
#   ARM 32 - ChromePublic.apk, ChromeModernPublic.apk, MonochromePublic.apk
#   ARM 64 - ChromePublic.apk ChromeModernPublic.apk
build/android/binary_size/apk_downloader.py --update 63 508578 --update 62 499187
```

  * **Remember to commit the generated .sha1 files and update the
    CURRENT_MILESTONE variable in apk_downloader.py**
