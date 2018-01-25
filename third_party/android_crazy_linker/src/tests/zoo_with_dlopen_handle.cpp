// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <dlfcn.h>
#include <stdio.h>

using dlclose_func_t = int(void*);

extern "C" dlclose_func_t* Zoo(void* bar_lib) {
  printf("%s: Entering\n", __FUNCTION__);
  auto* bar_func = reinterpret_cast<void (*)()>(::dlsym(bar_lib, "Bar"));
  if (!bar_func) {
    fprintf(stderr, "Could not find 'Bar' symbol in library\n");
    return nullptr;
  }
  printf("%s: Found 'Bar' symbol at @%p\n", __FUNCTION__, bar_func);

  // Call it.
  printf("%s: Calling Bar()\n", __FUNCTION__);
  (*bar_func)();

  printf("%s: Closing libbar\n", __FUNCTION__);
  int ret = dlclose(bar_lib);
  if (ret != 0) {
    printf("%s: Failed to close libbar: %s\n", __FUNCTION__, dlerror());
    return nullptr;
  }

  printf("%s: Exiting\n", __FUNCTION__);

  // If the library is loaded with the crazy linker, this should return the
  // address of the wrapper function, not of the system dlclose one. This
  // will be checked by the caller.
  return &::dlclose;
}
