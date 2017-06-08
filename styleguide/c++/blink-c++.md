<style>
.badcode {
  background-color: #ffe6d8 !important
}
.code {
  background-color: transparent !important;
  margin: 1em 0;
}
</style>

<script>
document.addEventListener('DOMContentLoaded', function() {
  var badcodes = document.querySelectorAll('.code');
  for (let badcode of badcodes) {
    if (badcode.textContent.indexOf('// Bad') >= 0)
      badcode.classList.add('badcode');
  }
});
</script>

# Blink C++ Style Guide

This document is a list of differences from the overall [Chromium style guide],
which is in turn a set of differences from the [Google C++ style guide]. The
long-term goal is to make both Chromium and Blink style more similar to Google
style over time, so this page will solely make certain things more precise about
the Google guide.

[TOC]

## Use references for non-null arguments, including mutable ones
Until further notice, function arguments that aren't being copied or moved and
can't be null should be passed by reference. In rare cases where the argument
is optional and can be null, it should be passed by pointer instead.

```c++
// Good
void MyClass::GetSomeValue(OutArgumentType& out_argument) const {
  out_argument = value_;
}

void MyClass::DoSomething(OutArgumentType* out_argument) const {
  DoSomething();
  if (out_argument)
    *out_argument = value_;
}
```

```c++
// Bad
void MyClass::GetSomeValue(OutArgumentType* out_argument) const {
  *out_argument = value_;
}
```

## Naming

### CamelCase function names

Do not name functions in `lower_case_with_underscores()`, even if they are
getters or setters. Always use `CamelCase()`.
 
As an exception, method names for web-exposed bindings are namedLikeThis(), as
they are in Javascript.


### Precede boolean values with words like "is" and "did"
```c++
// Good
bool is_valid;
bool did_send_data;
```

```c++
// Bad
bool valid;
bool sent_data;
```

### Precede setters with the word "Set"; use bare words for getters
Precede setters with the word "set". Prefer to use bare words for getters.
Setter and getter names should match the names of the variables being
set/gotten.
 
Where a getter's name collides with a type name, prefix it with "Get".

```c++
// A getter that returns |count_|.
size_t Count();
// A getter that returns a conflicting type name.
BigFloat GetBigFloat();
```

```c++
// Bad: a getter that returns |count_|.
size_t GetCount();
```

### Precede getters that return values via out-arguments with the word "Get".
```c++
void GetInlineBoxAndOffset(InlineBox*&, int& caret_offset) const;
```

```c++
void InlineBoxAndOffset(InlineBox*&, int& caret_offset) const;  // Bad
```

### Use descriptive verbs in function names.
```c++
bool ConvertToASCII(short*, size_t);  // Good
```

```c++
bool ToASCII(short*, size_t);  // Bad
```

### Leave meaningless variable names out of function declarations
Leave meaningless variable names out of function declarations. A good rule of thumb is if the parameter type name contains the parameter name (without trailing numbers or pluralization), then the parameter name isn't needed. Usually, there should be a parameter name for bools, strings, and numerical types.

```c++
void SetCount(size_t);
void DoSomething(ScriptExecutionContext*);
```

```c++
// Bad
void SetCount(size_t count);
void DoSomething(ScriptExecutionContext* context);
```

### Prefer enums to bools on function parameters
Prefer enums to bools on function parameters if callers are likely to be passing constants, since named constants are easier to read at the call site. An exception to this rule is a setter function, where the name of the function already makes clear what the boolean is.

```c++
DoSomething(something, AllowFooBar);
PaintTextWithShadows(context, ..., text_stroke_width > 0, IsHorizontal());
SetResizable(false);
```

```c++
// Bad
DoSomething(something, false);
SetResizable(NotResizable);
```

### Comments
Comments should say “why”, not “what”. Comments that merely repeat the name of
a class, function, or variable are worse than nothing. If the name of an entity
is really enough to describe it to someone unfamiliar with the surrounding
code, there's no need to comment.

[Chromium style guide]: c++.md
[Google C++ style guide]: https://google.github.io/styleguide/cppguide.html
