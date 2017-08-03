# Blink 'common' directory

This directory contains the common Web Platform stuff that needs to be shared
by renderer-side and browser-side code.

Things that live in third_party/WebKit/Source and browser-side code
(e.g. content/browser for the time being) can both depend on this directory,
while anything in this directory should NOT depend on them.

Unlike other directories in WebKit, code in this directory should:

* Use Chromium's common types (e.g. //base ones) rather than Blink's ones
  (e.g. WTF types)

* Follow [Chromium's common coding style guide](https://chromium.googlesource.com/chromium/src/+/master/styleguide/c++/c++.md)

* Use full-path from src/ for includes (e.g. `third_party/WebKit/common/foo.h` rather than `common/foo.h`)
