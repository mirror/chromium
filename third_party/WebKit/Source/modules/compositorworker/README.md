# Animation Worklet
This directory contains source code that implements Animation Worklet API and Compositor Worker.

See [Animation Worklet Explainer](https://github.com/WICG/animation-worklet/blob/gh-pages/README.md)
for the set of web-exposed API that implements.

Compositor Worker is considered deprecated as it is superceded by Animation Worklet. It will be
removed once Animation Worklet tests cover all the common machinery that the Compositor Worker
currently tests.

## Implementation

### Worklet and Global Scope

[`AbstractAnimationWorkletThread`](AbstractAnimationWorkletThread.h):Represents the shared backing
thread that is used by all worklets and participates in Blink garbage collection process. At the
moment, instead of creating a dedicated backing thread it uses the existing compositor thread (i.e.,
`Platform::CompositorThread()`). [`AnimationWorkletThread`](AnimationWorkletThread.h) and
[`CompositorWorkerThread`](CompositorWorkerThread.h) are two concrete subclasses of it.

[`AnimationWorklet`](AnimationWorklet.h): Represents the animation worklet and lives on main thread.
All the logic related to loading a new source module is implemented in its parent class `Worklet`.
The sole responsibility of this class it to create the appropriate `WorkletGlobalScopeProxy`
instances that are responsible to proxy a corresponding `AnimationWorkletGlobalScope` that lives on
the worklet thread.

[`AnimationWorkletGlobalScope`](AnimationWorkletGlobalScope.h): Represents the animation worklet
global scope and implements all methods that the global scope exposes to user script (See
[`AnimationWorkletGlobalScope.idl`](AnimationWorkletGlobalScope)). The instances of this class live
on the worker thread but have a corresponding proxy on the main thread which is accessed by the
worklet instance. User scripts can register animator definitions with the global scope (via
`registerAnimator` method). The scope keeps a map of these animator definitions and can look them up
based on their name. The scope also owns a list of active animators. Once global scope receives a
"Mutate" signal, it in turn invokes the `animate` function for each of the animators.

[`AnimationWorkletMessagingProxy`](AnimationWorkletMessagingProxy.h): Acts as a proxy for the
animation worklet globals scopes that live on the worklet thread. All of the logic on how to
actually proxy an off thread global scope is coming from its superclasses
`ThreadedWorkletMessagingProxy` and `WorkletGlobalScopeProxy`. The main contribution of this class
is that it knows how to create an appropriate worklet thread (in this case an
`AnimationWorkletThread`) which is used during initialization process.

### Animator

[`AnimatorDefinition`](AnimatorDefinition.h): Represents a registered animator definition. In
particular it owns two `V8::Function`s that are the "constructor" and "animate" functions of the
resgistered class. It does not do any validation itself and relies on
`AnimationWorkletGlobalScope::registerAnimator` to validate the provided Javascript class before
completing the registration.

[`Animator`](Animator.h): Represents an instance of an animator. It owns the underlying `v8::Object`
for the instance and knows how to invoke the `animate` function on it.


## Testing

Layout tests that cover web-exposed API for Animation Worklet are tested in [`LayoutTests/virtual/th
readed/fast/compositorworker/`](../../../LayoutTests/virtual/threaded/fast/compositorworker/).

There are unit tests covering animation worklet and global scope in [`modules/compositorworker`](.).