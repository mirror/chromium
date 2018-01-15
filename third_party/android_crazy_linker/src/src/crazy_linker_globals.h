// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CRAZY_LINKER_GLOBALS_H
#define CRAZY_LINKER_GLOBALS_H

#include <pthread.h>

#include "crazy_linker_library_list.h"
#include "crazy_linker_rdebug.h"
#include "crazy_linker_search_path_list.h"
#include "crazy_linker_util.h"

// All crazy linker globals are declared in this header.

namespace crazy {

class Globals {
 public:
  Globals();
  ~Globals();

  void Lock() { pthread_mutex_lock(&lock_); }

  void Unlock() { pthread_mutex_unlock(&lock_); }

  static Globals* Get();

  static SearchPathList* GetSearchPaths() { return &Get()->search_paths_; }

  static RDebug* GetRDebug() { return Get()->rdebug(); }

  static int* GetSDKBuildVersion() { return &sdk_build_version_; }

  LibraryList* libraries() { return &libraries_; }
  RDebug* rdebug() { return &rdebug_; }

 private:
  pthread_mutex_t lock_;
  SearchPathList search_paths_;
  RDebug rdebug_;
  LibraryList libraries_;
  static int sdk_build_version_;
};

// Helper class to access the globals with scoped locking.
class ScopedLockedGlobals {
 public:
  ScopedLockedGlobals() : globals_(Globals::Get()) { globals_->Lock(); }

  ~ScopedLockedGlobals() { globals_->Unlock(); }

  Globals* operator->() { return globals_; }

 private:
  Globals* globals_;
};

// Helper class to perform the opposite: unlock the global lock on construction
// then re-acquire it on destructor.
class ScopedGlobalsUnlocker {
 public:
  ScopedGlobalsUnlocker() : globals_(Globals::Get()) { globals_->Unlock(); }

  ~ScopedGlobalsUnlocker() { globals_->Lock(); }

 private:
  Globals* globals_;
};

}  // namespace crazy

#endif  // CRAZY_LINKER_GLOBALS_H
