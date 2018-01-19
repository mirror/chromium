// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CRAZY_LINKER_MACHINE_TRANSLATION_H
#define CRAZY_LINKER_MACHINE_TRANSLATION_H

namespace crazy {

// Try to detect whether the current program is running under machine
// translation, e.g. when ARM binaries are run on an x86 device through a
// machine code translation layer like Intel's Houdini.
bool MaybeRunningUnderMachineTranslation();

}  // namespace crazy

#endif  // CRAZY_LINKER_MACHINE_TRANSLATION_H
