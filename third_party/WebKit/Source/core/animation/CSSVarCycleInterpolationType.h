// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CSSVarCycleInterpolationType_h
#define CSSVarCycleInterpolationType_h

#include "core/animation/InterpolationType.h"

namespace blink {

class PropertyRegistration;

class CSSVarCycleInterpolationType : public InterpolationType {
 public:
  CSSVarCycleInterpolationType(const PropertyHandle&,
                               const PropertyRegistration&);

 private:
  InterpolationValue MaybeConvertSingle(const PropertySpecificKeyframe&,
                                        const InterpolationEnvironment&,
                                        const InterpolationValue& underlying,
                                        ConversionCheckers&) const final;

  PairwiseInterpolationValue MaybeConvertPairwise(
      const PropertySpecificKeyframe& start_keyframe,
      const PropertySpecificKeyframe& end_keyframe,
      const InterpolationEnvironment&,
      const InterpolationValue& underlying,
      ConversionCheckers&) const final;

  InterpolationValue MaybeConvertUnderlyingValue(
      const InterpolationEnvironment&) const final {
    return nullptr;
  }

  void Composite(UnderlyingValueOwner& underlying_value_owner,
                 double underlying_fraction,
                 const InterpolationValue& value,
                 double interpolation_fraction) const final {
    underlying_value_owner.Set(*this, value);
  }

  void Apply(const InterpolableValue&,
             const NonInterpolableValue*,
             InterpolationEnvironment&) const final;

  WeakPersistent<const PropertyRegistration> registration_;
};

}  // namespace blink

#endif  // CSSVarCycleInterpolationType_h
