# The controller/ directory

controller/ contains the system infrastructure of the renderer process that uses or drives the web platform. controller/ can directly use core/ and modules/ without using Web types (but with some DEPS rules). Examples are RenderProcess, RenderThread, Android View, Extensions, Native Client etc.
