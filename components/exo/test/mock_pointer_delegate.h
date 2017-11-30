// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_TEST_MOCK_POINTER_DELEGATE_H_
#define COMPONENTS_EXO_TEST_MOCK_POINTER_DELEGATE_H_

#include "components/exo/pointer_delegate.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "ui/gfx/geometry/point_f.h"

namespace exo {
namespace test {

class MockPointerDelegate : public PointerDelegate {
 public:
  MockPointerDelegate();
  ~MockPointerDelegate() override;

  // Overridden from PointerDelegate:
  MOCK_METHOD1(OnPointerDestroying, void(Pointer*));
  MOCK_CONST_METHOD1(CanAcceptPointerEventsForSurface, bool(Surface*));
  MOCK_METHOD3(OnPointerEnter, void(Surface*, const gfx::PointF&, int));
  MOCK_METHOD1(OnPointerLeave, void(Surface*));
  MOCK_METHOD2(OnPointerMotion, void(base::TimeTicks, const gfx::PointF&));
  MOCK_METHOD3(OnPointerButton, void(base::TimeTicks, int, bool));
  MOCK_METHOD3(OnPointerScroll,
               void(base::TimeTicks, const gfx::Vector2dF&, bool));
  MOCK_METHOD1(OnPointerScrollStop, void(base::TimeTicks));
  MOCK_METHOD0(OnPointerFrame, void());
};

}  // namespace test
}  // namespace exo

#endif  // COMPONENTS_EXO_TEST_MOCK_POINTER_DELEGATE_H_
