# Blink public/web/

Web specs, services and APIs exposed by Blink.

Implementation for these APIS are located in:

* **`third_party/WebKit/Source/core/exported/`** - Essential Web specification
parts that the rest of the Web platform builds on.
* **`third_party/WebKit/Source/modules/exported/`** - Higher-level parts of the
Web platform. May depend on **`core/`**, however **`core/`** must not depend
on **`modules/`**.

There isn't a simple rule for finding which directory implements an interface, we
recommend [Code Search](https://cs.chromium.org) to find it.
