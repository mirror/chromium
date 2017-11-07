// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This header file replaces includes of X11 system includes while
// preventing them from redefining and making a mess of commonly used
// keywords like "None" and "Status". Instead those are placed inside
// an X11 namespace where they will not clash with other code.

#ifndef UI_GFX_X_X11
#define UI_GFX_X_X11
extern "C" {
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#define Bool int  // Ironic, but other headers undef Bool
#include <X11/Xatom.h>
#include <X11/Xcursor/Xcursor.h>
#include <X11/Xlib-xcb.h>
#include <X11/Xregion.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/extensions/XInput.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/XTest.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/Xrandr.h>
#include <X11/extensions/Xrender.h>
#include <X11/extensions/scrnsaver.h>
#include <X11/extensions/shape.h>
#include <X11/keysym.h>

// These commonly used names are undefined and if necessary recreated
// in the X11 namespace below. This is the main purpose of this header
// file.

#undef None    // Defined by X11/X.h to 0L which collides with other headers
#undef Status  // Defined by X11/Xlib.h to int which collides with other headers
#undef True    // Defined by X11/Xlib.h to 1 which collides with other headers
#undef False   // Defined by X11/Xlib.h to 0 which collides with other headers
#undef CurrentTime  // Defined by X11/X.h to 0L which collides with other
                    // headers
#undef Success      // Defined by X11/X.h to 0 which collides with other headers
#undef CREATE      // Defined by XInput.h to 1 which collides with other headers
#undef DestroyAll  // Defined by X11/X.h to 0 which collides with other headers
#undef COUNT       // Defined by X11/extensions/XI.h to 0 which collides

#undef DeviceAdded    // Defined to 0 (or 1?)
#undef DeviceRemoved  // Defined to 1 (or 0?)
#undef RootWindow     // It clashes with our RootWindow.

#undef Bool
}

namespace X11 {
static const long None = 0L;
static const long CurrentTime = 0L;
static const int False = 0;
static const int True = 1;
static const int Success = 0;
typedef int Status;
// auto RootWindow(int dpy, int scr) { return ScreenOfDisplay(dpy,scr)->root); }
}  // namespace X11

#endif  // UI_GFX_X_X11
