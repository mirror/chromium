// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_COMMON_RESOURCES_REMOTABLE_RESOURCE_FENCE_H_
#define COMPONENTS_VIZ_COMMON_RESOURCES_REMOTABLE_RESOURCE_FENCE_H_

namespace viz {

// An abstract interface used to ensure reading from resources passed between
// client and service does not happen before writing is completed.
class RemotableResourceFence : public base::RefCounted<RemotableResourceFence> {
 public:
  virtual void Set() = 0;
  virtual bool HasPassed() = 0;
  virtual void Wait() = 0;

 protected:
  friend class base::RefCounted<RemotableResourceFence>;
  RemotableResourceFence() = default;
  virtual ~RemotableResourceFence() = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(RemotableResourceFence);
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_COMMON_RESOURCES_REMOTABLE_RESOURCE_FENCE_H_
