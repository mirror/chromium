# Blink C++ Style Guide

This document is a list of differences from the overall [Chromium style guide],
which is in turn a set of differences from the [Google C++ style guide]. The
long-term goal is to make both Chromium and Blink style more similar to Google
style over time, so this page will solely make certain things more precise about
the Google guide.

[TOC]

## Use references for all non-null pointer arguments
Pointer arguments that can never be null should be passed as a reference, even
if this results in a mutable reference argument.

**Good:**
```c++
// Passed by mutable reference since |frame| is assumed to be non-null.
FrameLoader::FrameLoader(LocalFrame& frame)
    : frame_(&frame),
      progress_tracker_(ProgressTracker::Create(frame)) {
  TRACE_EVENT_OBJECT_CREATED_WITH_ID("loading", "FrameLoader", this);
  TakeObjectSnapshot();
}

// Optional arguments should still be passed by pointer.
void LocalFrame::SetDOMWindow(LocalDOMWindow* dom_window) {
  if (dom_window)
    GetScriptController().ClearWindowProxy();

  if (this->DomWindow())
    this->DomWindow()->Reset();
  dom_window_ = dom_window;
}
```

**Bad:**
```c++
// Since the constructor assumes that |frame| is never null, it should be
// passed as a mutable reference.
FrameLoader::FrameLoader(LocalFrame& frame)
    : frame_(&frame),
      progress_tracker_(ProgressTracker::Create(frame)) {
  DCHECK(frame_);
  TRACE_EVENT_OBJECT_CREATED_WITH_ID("loading", "FrameLoader", this);
  TakeObjectSnapshot();
}
```

## Naming

### Use `CamelCase` for all function names

All function names should use `CamelCase()`-style names, beginning with an
uppercase letter.

As an exception, method names for web-exposed bindings begin with a lowercase
letter to match JavaScript.

**Good:**
```c++
class Document {
 public:
  // web-exposed function names begin with a lowercase letter
  LocalDOMWindow* defaultView();

  // all other functions begin with an uppercase letter
  virtual void Shutdown();

  // ...
};
```

**Bad:**
```c++
class Document {
 public:
  // Though this is a getter, all Blink function names must use camel case.
  LocalFrame* frame() const { return frame_; }

  // ...
};
```

### Precede boolean values with words like “is” and “did”
```c++
bool is_valid;
bool did_send_data;
```

**Bad:**
```c++
bool valid;
bool sent_data;
```

### Precede setters with the word “Set”; use bare words for getters
Precede setters with the word “set”. Prefer bare words for getters. Setter and
getter names should match the names of the variable being accessed/mutated.

If a getter’s name collides with a type name, prefix it with “Get”.

**Good:**
```c++
class FrameTree {
 public:
  // Prefixed with “Get” since the type name and the function name conflict.
  Frame* GetFrame() const { return frame_; }

  // Otherwise prefer the bare word.
  Frame* FirstChild() const { return first_child_; }
  Frame* LastChild() const { return last_child_; }

  // ...
};
```

**Bad:**
```c++
class FrameTree {
 public:
  // Should not be prefixed with “Get”
  Frame* GetFirstChild() const { return first_child_; }
  Frame* GetLastChild() const { return last_child_; }

  // ...
};
```

### Precede getters that return values via out-arguments with the word “Get”.
**Good:**
```c++
class RootInlineBox {
 public:
  Node* GetLogicalStartBoxWithNode(InlineBox*&) const;
  // ...
}
```

**Bad:**
```c++
class RootInlineBox {
 public:
  Node* LogicalStartBoxWithNode(InlineBox*&) const;
  // ...
}
```

### Use descriptive verbs in function names.
**Good:**
```c++
bool ConvertToASCII(short*, size_t);
```

**Bad:**
```c++
bool ToASCII(short*, size_t);
```

### Leave obvious parameter names out of function declarations
Leave obvious parameter names out of function declarations. A good rule of
thumb is if the parameter type name contains the parameter name (without
trailing numbers or pluralization), then the parameter name isn’t needed.

`bool`, string, and numerical arguments should usually be named unless the
meaning is apparent from the function name.

**Good:**
```c++
class HTMLCanvasElement {
 public:
  void ContextDestroyed(ExecutionContext*);

  // The function name makes the meaning of the parameters clear.
  void setHeight(unsigned);
  void setWidth(unsigned);

  // ...
};
```

**Bad:**
```c++
class HTMLCanvasElement {
 public:
  // The parameter name is redundant with the parameter type.
  void Destroyed(ExecutionContext* context);

  // The parameter name is redundant with the function name.
  void setHeight(unsigned height);
  void setWidth(unsigned width);

  // ...
};
```

## Prefer enums to bools on function parameters
Prefer enums to bools on function parameters if callers are likely to be
passing constants, since named constants are easier to read at the call site.
An exception to this rule is a setter function, where the name of the function
already makes clear what the boolean is.

**Good:**
```c++
// An named enum value makes it clear what the parameter is for.
if (frame->Loader().ShouldClose(CloseType::kNotForReload)) {
  // No need to use enums for boolean setters, since the meaning is clear.
  frame_->SetIsClosing(true);

  // ...
```

**Bad:**
```c++
// Not obvious what false means here.
if (frame->Loader().ShouldClose(false)) {
  frame_->SetIsClosing(ClosingState::kTrue);

  // ...
```

## Comments
Comments should say “why”, not “what”. Comments that merely repeat the name of
a class, function, or variable are worse than nothing. If the name of an entity
is enough to describe it to someone unfamiliar with the surrounding code,
there’s no need to comment. Also see the webkit-dev thread on [coding style and
comments].

[Chromium style guide]: c++.md
[Google C++ style guide]: https://google.github.io/styleguide/cppguide.html
[coding style and comments]: https://lists.webkit.org/pipermail/webkit-dev/2011-January/015769.html
