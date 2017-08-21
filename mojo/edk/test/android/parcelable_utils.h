// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <vector>

#include "base/android/scoped_java_ref.h"

namespace mojo {
namespace edk {
namespace test {

// Returns an android.graphics.Point instance. The Point class is parcelable and
// useful for testing sending Parcelable objects over Mojo channels .

base::android::ScopedJavaLocalRef<jobject> CreateJavaPoint(jint x, jint y);

bool AreJavaObjectsEqual(jobject object1, jobject object2);

}  // namespace test
}  // namespace edk
}  // namespace mojo