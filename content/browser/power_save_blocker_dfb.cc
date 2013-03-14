// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/power_save_blocker_impl.h"

#include "base/basictypes.h"
#include "base/bind.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "chromeos/power/power_state_override.h"

namespace content {

class PowerSaveBlockerImpl::Delegate
    : public base::RefCountedThreadSafe<PowerSaveBlockerImpl::Delegate> {
 public:
  Delegate(PowerSaveBlockerType type) : type_(type) {}

 private:
  friend class base::RefCountedThreadSafe<Delegate>;
  virtual ~Delegate() {}

  PowerSaveBlockerType type_;

  scoped_refptr<chromeos::PowerStateOverride> override_;

  DISALLOW_COPY_AND_ASSIGN(Delegate);
};

PowerSaveBlockerImpl::PowerSaveBlockerImpl(PowerSaveBlockerType type,
                                           const std::string& reason)
    : delegate_(new Delegate(type)) {
}

PowerSaveBlockerImpl::~PowerSaveBlockerImpl() {
}

}  // namespace content
