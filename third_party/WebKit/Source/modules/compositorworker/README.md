# Animation Worklet
This directory contains source code that implements Animation Worklet API and Compositor Worker.

See [Animation Worklet Explainer](https://github.com/WICG/animation-worklet/blob/gh-pages/README.md)
for the set of web-exposed API that this implements.

Compositor Worker is now considered deprecated and superceded by Animation Worklet. It will be
removed once Animation Worklet tests cover all machinery that the Compositor Worker currently tests.

# Key Classes

[`AbstractAnimationWorkletThread`](AbstractAnimationWorkletThread.h): Represents the shared backing
thread that is used by all worklets and participates in Blink garbage collection. This inherits from
the `WorkerThread`. At the moment, instead of creating a dedicated backing thread it uses the
underlying Compositor Thread (see `Platform::CompositorThread()`).
[`AnimationWorkletThread`](AnimationWorkletThread.h) and
[`CompositorWorkerThread`](CompositorWorkerThread.h) are two concrete implementations of it.

[`AnimationWorklet`](AnimationWorklet.h): This is a sub-class of `Worklet`. For each animation
worklet there is a instance of this class that lives on main thread. All the logic related to
loading a new source module is implemented in its parent. The sole responsibility of this class it
to create the appropriate `WorkletGlobalScopeProxy` instanes that are responsible to proxy a
corresponding AnimationWorkletGlobalScope that lives on the worklet thread.

[`AnimationWorkletGlobalScope`](AnimationWorkletGlobalScope.h): Implements the animation worklet
global scope and all globals methods that the scope exposes to user script (See
`AnimationWorkletGlobalScope.idl`). The instances of this class live on the worker thread but have a
corresponding proxy on the main thread which is accessed by the worklet instance. User scripts can
register animator definitions with the global scope. The scope keeps a map of these animator
definitions that can be used to look them up based on their name. The scope also has a list of
active animators. Once global scope receives "Mutate" signal, it in turns invoke the `animate`
function for each of the animators.


[`AnimationWorkletMessagingProxy`](AnimationWorkletMessagingProxy.h): Act as a proxy for the
animation worklet globals scopes that live on the worklet thread. All of the logic on how to
actually proxy an off thread global scope is coming from its superclasses
`ThreadedWorkletMessagingProxy` and `WorkletGlobalScopeProxy`. The main contribution of this class
is that it knows how to create an appropriate worklet thread (in this case an
AnimationWorkletThread) which is used during initialization process.

[`AnimatorDefinition`](AnimatorDefinition.h): Represents a registerd animator definition. In
particular it owns two `V8::Function`s that are the "constructor" and "animate" functions of the
resgistered class. It does not do any validation itself and relies on
`AnimationWorkletGlobalScope::registerAnimator` to validate the provided Javascript class before
completing the registration.


[`AnimatorDefinition`](AnimatorDefinition.h): Represents an instance of animatior. It owns the
`v8::Object` that is the instance and knows how to invoke the `animate` function on that instance.