# `core/animation/`

This directory contains the main thread animation engine. This implements the
Web Animations timing model that drives CSS Animations, Transitions and exposes
the Web Animations API (`element.animate()`) to Javascript.


## Contacts

As of 2018 Blink animations is maintained by the
[cc/ team](https://cs.chromium.org/chromium/src/cc/OWNERS).  


## Specifications Implemented

*   [CSS Animations Level 1](https://www.w3.org/TR/css-animations-1/)
*   [CSS Transitions](https://www.w3.org/TR/css-transitions-1/)
*   [Web Animations](https://www.w3.org/TR/web-animations-1/)
*   [CSS Properties and Values API Level 1 - Animation Behavior of Custom Properties](https://www.w3.org/TR/css-properties-values-api-1/#animation-behavior-of-custom-properties)
*   Individual CSS property animation behaviour [e.g. transform interolation](https://www.w3.org/TR/css-transforms-1/#interpolation-of-transforms).


## Integration with Chromium

The Blink animation engine has three main integration points:

*   ### [Blink's Style engine](../css)

    The primary functionality of the animation engine is to change CSS values over time.  
    This means animations have a place in the [CSS cascade][] and influence the the [ComputedStyle][]s returned by [styleForElement()][].
    The implementation for this lives under [ApplyAnimatedStandardProperties()][] for standard properties and [ApplyAnimatedCustomProperties()][] for custom properties. Custom properties have more complex application logic due to dynamic dependencies introduced by [variable references].

    Animations can be controlled by CSS via the [`animation`](https://www.w3.org/TR/css-animations-1/#animation) and [`transition`](https://www.w3.org/TR/css-transitions-1/#transition-shorthand-property) properties.  
    In code this happens when [styleForElement()][] uses [CalculateAnimationUpdate()][] and [CalculateTransitionUpdate()][] to build a [set of mutations][] to make to the animation engine which gets [applied later][].

*   ### [Chromium's Compositor](../../../../../cc/README.md)


[CSS cascade]: https://www.w3.org/TR/css-cascade-3/#cascade-origin
[ComputedStyle]: https://cs.chromium.org/search/?q=class:blink::ComputedStyle$
[styleForElement()]: https://cs.chromium.org/search/?q=function:StyleResolver::styleForElement
[variable references]: https://www.w3.org/TR/css-variables-1/#using-variables
[ApplyAnimatedStandardProperties()]: https://cs.chromium.org/?type=cs&q=function:StyleResolver::ApplyAnimatedStandardProperties
[ApplyAnimatedCustomProperties()]: https://cs.chromium.org/?type=cs&q=function:ApplyAnimatedCustomProperties
[CalculateAnimationUpdate()]: https://cs.chromium.org/search/?q=function:CSSAnimations::CalculateAnimationUpdate
[CalculateTransitionUpdate()]: https://cs.chromium.org/search/?q=function:CSSAnimations::CalculateTransitionUpdate
[MaybeApplyPendingUpdate()]: https://cs.chromium.org/search/?q=function:CSSAnimations::MaybeApplyPendingUpdate
[set of mutations]: https://cs.chromium.org/search/?q=class:CSSAnimationUpdate
[applied later]: https://cs.chromium.org/search/?q=function:Element::StyleForLayoutObject+MaybeApplyPendingUpdate