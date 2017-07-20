// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ReachedUrlSet_h
#define ReachedUrlSet_h

#include "core/CoreExport.h"
#include "core/dom/AncestorList.h"
#include "platform/heap/Handle.h"
#include "platform/weborigin/KURL.h"
#include "platform/weborigin/KURLHash.h"
#include "platform/wtf/HashSet.h"

namespace blink {

class CORE_EXPORT ReachedUrlSet
    : public GarbageCollectedFinalized<ReachedUrlSet> {
 public:
  static ReachedUrlSet* CreateFromTopLevelAncestorList(
      const AncestorList& list) {
    ReachedUrlSet* set = new ReachedUrlSet;
    CHECK_LE(list.size(), 1u);
    set->set_ = list;
    return set;
  }

  void ObserveModuleTreeLink(const KURL& url) {
    auto result = set_.insert(url);
    CHECK(result.is_new_entry);
  }

  bool IsAlreadyBeingFetched(const KURL& url) const {
    return set_.Contains(url);
  }

  DEFINE_INLINE_TRACE() {}

 private:
  HashSet<KURL> set_;
};

}  // namespace blink

#endif
