# Directory layout:

core/
-----
Contains only C++11 code and doesn't have any dependency on chromium, not even
on //base. The only dependencies allowed are via platform_{light,full}.h,
which the embedder will implement and eventually route to their own "base".
This folder is exported via git subtree and used by other third-party projects
like v8.
The only notable exception is unit-tests which are allowed to depend on
gtest. If you care about building unittests outside of the chrome tree need
to provide a gtest implementation as well.

core/impl/  [build target: libtracing_portable_impl]
----------------------------------------------------
This target has the actual implementation of the tracing machinery (trace log,
enabled state of categories, etc). There should be one and only one copy of
libtracing_core linked in the final embedder executable.
Can depend on core/public.

core/public/ [build target: libtracing_core_public]
---------------------------------------------------
The public API surface of tracing, which is mostly represented by the
TRACE_EVENT* macros. All code from all codebases, in and out of the chromium
tree, should be able to depend on this. This target is dependency clean and
like core/impl depends only on platform_light.h, provided by the embedder, for
things like atomics.

chromium/
---------
the chromium-specific part of tracing. This can depend on //base and know
about things like TaskRunner.
