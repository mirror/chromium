// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/idle_query_linux.h"

#include <X11/extensions/scrnsaver.h>
#include "ui/base/x/x11_util.h"

namespace browser {

class IdleData {
 public:
  IdleData() {
#if defined(USE_X11)
    int event_base;
    int error_base;
    if (XScreenSaverQueryExtension(ui::GetXDisplay(), &event_base,
                                   &error_base)) {
      mit_info = XScreenSaverAllocInfo();
    } else {
      mit_info = NULL;
    }
#endif
  }

  ~IdleData() {
#if defined(USE_X11)
    if (mit_info)
      XFree(mit_info);
#endif
  }

#if defined(USE_X11)
  XScreenSaverInfo *mit_info;
#endif
};

IdleQueryLinux::IdleQueryLinux() : idle_data_(new IdleData()) {}

IdleQueryLinux::~IdleQueryLinux() {}

int IdleQueryLinux::IdleTime() {
#if defined(USE_X11)
  if (!idle_data_->mit_info)
    return 0;

  if (XScreenSaverQueryInfo(ui::GetXDisplay(),
                            RootWindow(ui::GetXDisplay(), 0),
                            idle_data_->mit_info)) {
    return (idle_data_->mit_info->idle) / 1000;
  } else {
    return 0;
  }
#else
  return 0;
#endif
}

}  // namespace browser
