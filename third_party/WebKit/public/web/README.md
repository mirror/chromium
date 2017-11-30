# Blink public/web/

Web specs, services and APIs exposed by Blink.

It's implementation is split between 2 main directories:

* **`third_party/WebKit/Source/core/exported/`** - Most basic parts of web spec
that doesn't depend on others.
* **`third_party/WebKit/Source/modules/exported/`** - Additional parts of web
spec that may use/depend on **`third_party/WebKit/Source/core/`**.

There isn't a simple rule for finding which directory implements an interface, we
recommend [Code Search](https://cs.chromium.org) to find it.

