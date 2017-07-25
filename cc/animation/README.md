# cc/animation

[TOC]

## Overview

cc/animation enables animating supported properties of CC layers on the
compositor. Both web (i.e. Blink created) and user interface (i.e. ui/ created)
layers are supported. A given CC layer may have multiple animations active at a
time, which may affect the same or different properties. Animations are 'ticked'
based on a timeline and provide a value based on an animation curve.

**TODO(smcgruer):** It seems like the AnimationTimeline doesn't do anything? The
ticking comes directly from LayerTreeHost{Impl} as far as I can tell, so what is
AnimationTimeline for?

### Code Independence

**TODO(smcgruer):** Bikeshed a better name for this section.

Most of the code in cc/animation is not actually specific to compositor
animations. Animation curves, ticks, and keyframes are not specific to the
compositor. The long term goal is to generalize the logic here and transfer the
majority of the code to a location shared between ui/, cc/, Blink, and others.

The rest of this document focuses on the compositor-specific parts of
cc/animation. Documentation on generalizable concepts such as animation curves
can be found in the appropriate file (e.g.
[animation\_curve.h](animation_curve.h)).

### Supported Properties

As CC layers are just textures which are reused for performance, only certain
properties can be animated without needing to redraw content. For example, a
layer's opacity can be animated as CC layers are aware of the content behind
them. On the other hand we cannot animate layer width as this could modify
layout - which then requires redrawing.

## Threading Model

**TODO(smcgruer):** Write this.

## Interaction between cc and Blink

**TODO(smcgruer):** Write this.

## Interaction between cc/animation and ui/

**TODO(smcgruer):** Write this.
