# Directory layout:

core/
-----
Contains only C++11 code and doesn't have any dependency on chromium, not even
on //base. The only dependencies allowed are via platform.h,
which the embedder will implement and eventually route to their own "base".
This folder is exported via git subtree and used by other third-party projects
like v8.

The only notable exception is unit-tests which are allowed to depend on
gtest. If you care about building unittests outside of the chrome tree need
to provide a gtest implementation as well.

core/impl/  [build target: libtracing_core_impl]
----------------------------------------------------
This target has the actual implementation of the tracing machinery (trace log,
enabled-state of tracing categories, etc). There should be one and only one copy
of libtracing_core_impl linked in the final embedder executable.
This can depend on core/public.

core/public/ [build target: libtracing_core_public]
---------------------------------------------------
The public API surface of tracing, mostly the TRACE_EVENT* macros.
Code in and out of the chromium tree, should be able to depend on this.

chromium/
---------
the chromium-specific part of tracing. This can depend on //base.

chromium/public/
----------------
Chromium-specific blocks of tracing (e.g., ChromiumTraceConfig). Chromium
code can depend on this.

chromium/impl/
--------------
TBD, see bit.ly/tracing-unbundling
