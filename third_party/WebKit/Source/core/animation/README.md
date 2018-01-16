# `Source/core/animation/`

This directory contains the main thread animation engine. This implements the
Web Animations timing model that drives CSS Animations, Transitions and exposes
to Javascript the Web Animations API (`element.animate()`).

## Contacts

As of 2018 Blink animations is maintained by the [cc/ team](https://cs.chromium.org/chromium/src/cc/OWNERS).
Past maintainers include alancutter@chromium.org, ericwilligers@chromium.org, dstockwell@chromium.org and shans@chromium.org.

## Specifications Implemented

* [CSS Animations Level 1](https://www.w3.org/TR/css-animations-1/)
* [CSS Transitions](https://www.w3.org/TR/css-transitions-1/)
* [Web Animations](https://www.w3.org/TR/web-animations-1/)
* [CSS Properties and Values API Level 1 - Animation Behavior of Custom Properties](https://www.w3.org/TR/css-properties-values-api-1/#animation-behavior-of-custom-properties)
* Individual CSS property animation behaviour ([example](https://www.w3.org/TR/css-transforms-1/#interpolation-of-transforms)).
