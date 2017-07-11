// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PASSWORD_REUSE_DEFINES_H_
#define COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PASSWORD_REUSE_DEFINES_H_

#if defined(OS_WIN) || (defined(OS_MACOSX) && !defined(OS_IOS)) || \
    (defined(OS_LINUX) && !defined(OS_CHROMEOS))
#define SYNC_PASSWORD_REUSE_ENABLED
#endif

#endif  // COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PASSWORD_REUSE_DEFINES_H_
