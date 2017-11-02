// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/icc_profile.h"

namespace gfx {

scoped_refptr<ICCProfile> ICCProfileForTestingAdobeRGB();
scoped_refptr<ICCProfile> ICCProfileForTestingColorSpin();
scoped_refptr<ICCProfile> ICCProfileForTestingGenericRGB();
scoped_refptr<ICCProfile> ICCProfileForTestingSRGB();

// A profile that does not have an analytic transfer function.
scoped_refptr<ICCProfile> ICCProfileForTestingNoAnalyticTrFn();

// A profile that is A2B only.
scoped_refptr<ICCProfile> ICCProfileForTestingA2BOnly();

// A profile that with an approxmation that shoots above 1.
scoped_refptr<ICCProfile> ICCProfileForTestingOvershoot();

}  // namespace gfx
