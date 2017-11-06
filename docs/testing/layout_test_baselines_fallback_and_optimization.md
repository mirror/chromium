# Layout Test Baselines -- Fallback and Optimization


*** promo
Read [Layout Test Expectations and Baselines](layout_test_expectations.md) first
if you have not.
***

Baselines can vary by platforms, in which case we need to check in multiple
versions of a baseline, and we would like to store as few versions as possible.
This document first introduces how platform-specific baselines are structured
and how we search for a baseline (the fallback mechanism). Lastly, we describe
the algorithm we use to optimize baselines (remove duplicates).

[TOC]

## Basics

* **Root directory**:
    [`//src/third_party/WebKit/LayoutTests`](../../third_party/WebKit/LayoutTests)
    is the root directory (of all the layout tests and baselines). All relative
    paths in this document start from this directory.
* **Test name**: the name of a test is its relative path from the root
    directory (e.g. `html/dom/foo/bar.html`).
* **Baseline name**: replacing the extension of a test name with
    `-expected.{txt,png,wav}` gives the corresponding baseline name.
* **Virtual tests**: tests can have virtual variants. For example,
    `virtual/gpu/html/dom/foo/bar.html` is the virtual variant of
    `html/dom/foo/bar.html` in the `gpu` virtual suite. Only the latter file
    exists on disk. See
    [Layout Tests#Testing Runtime Flags](layout_tests.md#testing-runtime-flags)
    for more details on virtual tests.
* **Platform directory**: each directory under
    [`platform/`](../../third_party/WebKit/LayoutTests/platform) is a platform
    directory that contains baselines (no tests) for that platform. Directory
    names are in the form of `PLATFORM-VERSION` (e.g. `mac-mac10.12`), except
    for the latest version of a platform in which case it is simply `PLATFORM`
    (e.g. `mac`).

## Baseline Fallback

In order to avoid having to store baselines for every platform and version, we
define a fallback for each platform when a baseline cannot be found in this
platform directory. A general rule is to have older versions of an OS falling
back to newer versions. Besides, Android falls back to Linux, which then falls
back to Windows. Eventually, all platforms fall back to the root directory (i.e.
the generic baselines that live alongside tests). The rules are configured by
`FALLBACK_PATHS` in each Port class in
[`//src/third_party/WebKit/Tools/Scripts/webkitpy/layout_tests/port`](../../third_party/WebKit/Tools/Scripts/webkitpy/layout_tests/port).

All platforms can be organized into a tree by their fallback relations (we are
not considering virtual test suites yet). See the lower half (the non-virtual
subtree) of this
[graph](https://docs.google.com/drawings/d/13l3IUlSE99RoKjDwEWuY1O77simAhhF6Wi0fZdkSaMA/).
Walking from a platform to the root gives the **search path** of that platform.
We check each directory on the search path in order and see if "directory +
baseline name" exists (note that baseline names are relative paths), and stop at
the first one found.

### Virtual test suites

Now we add virtual test suites to the picture, using a test named
`virtual/gpu/html/dom/foo/bar.html` as an example to demonstrate the process.
Recall that the actual test file on disk is `html/dom/foo/bar.html`. The
baseline search process for a virtual test can consist of two runs:

1. Treat the virtual test name as a regular test name and search for the
   corresponding baseline name using the same search path, which means we are in
   fact searching directories like `platform/*/virtual/gpu/...`, and eventually
   `virtual/gpu/...` (a.k.a. the virtual root).
2. If no baseline can be found so far, we retry with the non-virtual test name
   `html/dom/foo/bar.html` and walk the search path again.

The [graph](https://docs.google.com/drawings/d/13l3IUlSE99RoKjDwEWuY1O77simAhhF6Wi0fZdkSaMA/)
visualizes the full picture. Note that the two runs are in fact the same with
different test names, so the virtual subtree is a mirror of the non-virtual
subtree. The two trees are connected by the virtual root, which has different
ancestors (fallbacks) depending on which platform we start.

*** promo
__Note:__ there are in fact two more places to be searched before everything
else, additional directories given via command line arguments and flag-specific
baseline directories. They are maintained manually and are not discussed in this
document.
***

## Optimization

## Rebaseline
