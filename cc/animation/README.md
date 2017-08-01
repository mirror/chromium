# cc/animation

[TOC]

## Overview

cc/animation provides animation support - generating output values (usually
visual properties) based on a predefined function and changing input values.
Currently the main clients of cc/animation are Blink and ui/, targeting
composited layers, but the code is intended to be agnostic of the client it is
supporting.  Aspirationally we could eventually merge cc/animation and Blink
animation and have only a single animation system for all of Chromium.

This document covers two main topics. The first section explains how
cc/animation actually works: how animations are ticked, what animation curves
are, what the ownership model is, etc. Later sections document how other parts
of Chromium interact with cc/animation, most prominently Blink and ui/.

## How cc/animation works

The root concept in cc/animation is an
[animation](https://codesearch.chromium.org/chromium/src/cc/animation/animation.h).
Animations contain the state necessary to 'play' (interpolate values from) an
[animation curve](https://codesearch.chromium.org/chromium/src/cc/animation/animation_curve.h),
which is a function that returns a value given an input time. Aside from the
animation curve itself, an animation's state includes the run state (playing,
paused, etc), the start time, the current direction (forwards, reverse), etc.
An animation does not know or care what property is being animated, and holds
only an opaque identifier for the property to allow clients to map output values
to the correct properties. Targeting only a single property means that cc
animations are distinct from the Blink concept of an animation, which wraps the
animation of multiple properties.

Animations are grouped together based on their
[animation target](https://codesearch.chromium.org/chromium/src/cc/animation/animation_target.h)
(the entity whose property is being animated) and each such group is owned by an
[animation player](https://codesearch.chromium.org/chromium/src/cc/animation/animation_player.h).
Note that there may be multiple animation players with the same target (each
with a set of animations for that target); the
[ElementAnimations](https://codesearch.chromium.org/chromium/src/cc/animation/element_animations.h)
class wraps the multiple animation players and has a 1:1 relationship with
target entities.

`TODO(smcgruer): Why are ElementAnimations and AnimationPlayers separate?`

In order to play an animation, input time values must be provided to the
animation curve and output values fed back into the animating entity. This is
called 'ticking' an animation and is the responsibility of the
[animation host](https://codesearch.chromium.org/chromium/src/cc/animation/animation_host.h).
The animation host has a list of currently ticking players (i.e. those that have
any non-deleted animations), which it iterates through whenever it receives a
tick call from the client (along with a corresponding input time).  The
animation players then call into their non-deleted animations, retrieving the
value from the animation curve.  As they are computed, output values are sent to
the target which is responsible for passing them to the client entity that is
being animated.

### Types of Animation Curve

As noted above, an animation curve is simply a function which converts an input
time value into some output value. Animation curves are categorized based on
their output type, and each such category can have multiple implementations that
provide different conversion functions. There are many categories of animation
curve, but some common ones are `FloatAnimationCurve`, `ColorAnimationCurve`,
and `TransformAnimationCurve`.

The most common implementation of the various animation curve categories are the
[keyframed animation curves](https://codesearch.chromium.org/chromium/src/cc/animation/keyframed_animation_curve.h).
These curves each have a set of keyframes which map a specific time to a
specific output value. Producing an output value for a given input time is then
a matter of identifying the two keyframes the time lies between, and
interpolating between the keyframe output values. (Or simply using a keyframe
output value directly, if the input time happens to line up exactly.) Exact
details of how each animation curve category is interpolated can be found in the
implementations.

There is one category of animation curve that stands somewhat apart, the
[scroll offset animation curve](https://codesearch.chromium.org/chromium/src/cc/animation/scroll_offset_animation_curve.h).
This curve converts the input time into a scroll offset, interpolating between
an initial scroll offset and an updateable target scroll offset. It has logic to
handle different types of scrolling such as programmatic, keyboard, and mouse
wheel scrolls.

### Animation Timelines

cc/animation has a concept of an
[animation timeline](https://codesearch.chromium.org/chromium/src/cc/animation/animation_timeline.h).
This should not be confused with the identically named Blink concept. In
cc/animation, animation timelines are an implementation detail - they hold the
animation players and are responsible for syncing them to the impl thread (see
below), but do not participate in the ticking process in anyway.

### Threading Model

`TODO(smcgruer): Write this.`

## cc/animation Clients

`TODO(smcgruer): Write something here.`

### Supported Properties

As layers are just textures which are reused for performance, only certain
properties can be animated without needing to redraw content. For example, a
layer's opacity can be animated as CC layers are aware of the content behind
them. On the other hand we cannot animate layer width as this could modify
layout - which then requires redrawing.

### Interaction between cc/animation and Blink

`TODO(smcgruer): Write this.`

### Interaction between cc/animation and ui/

`TODO(smcgruer): Write this.`
