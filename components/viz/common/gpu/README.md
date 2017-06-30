# gpu/

This directory contains viz APIs for access to the Gpu services.

## ContextProvider

The primary interface to control access to the Gpu and lifetime of client-side
Gpu control structures (such as the GLES2Implementation that gives access to
the command buffer).
