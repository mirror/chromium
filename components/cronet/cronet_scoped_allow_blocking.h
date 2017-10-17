// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "base/threading/thread_restrictions.h"

#ifndef COMPONENTS_CRONET_CRONET_SCOPED_ALLOW_BLOCKING_H_
#define COMPONENTS_CRONET_CRONET_SCOPED_ALLOW_BLOCKING_H_

namespace cronet {
class CronetScopedAllowBlocking {
 public:
  CronetScopedAllowBlocking() {}
  ~CronetScopedAllowBlocking() {}

 private:
#if DCHECK_IS_ON()
  base::ScopedAllowBlocking scoped_allow_blocking_;
#endif

  DISALLOW_COPY_AND_ASSIGN(CronetScopedAllowBlocking);
};

}  // namespace cronet

#endif  // COMPONENTS_CRONET_CRONET_SCOPED_ALLOW_BLOCKING_H_