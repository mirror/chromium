// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_TOOLBAR_TOOLBAR_FIELD_TRIAL_H_
#define COMPONENTS_TOOLBAR_TOOLBAR_FIELD_TRIAL_H_

namespace base {
struct Feature;
}

namespace toolbar {

extern const base::Feature kEVToSecure;
extern const base::Feature kEVToLock;
extern const base::Feature kSecureToLock;

}  // namespace toolbar

#endif  // COMPONENTS_TOOLBAR_TOOLBAR_FIELD_TRIAL_H_
