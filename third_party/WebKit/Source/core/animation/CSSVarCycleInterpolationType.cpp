// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/animation/CSSVarCycleInterpolationType.h"

#include "core/animation/CSSInterpolationEnvironment.h"
#include "core/animation/StringKeyframe.h"
#include "core/css/CSSCustomPropertyDeclaration.h"
#include "core/css/PropertyRegistration.h"
#include "core/css/resolver/StyleBuilder.h"

namespace blink {

CSSVarCycleInterpolationType::CSSVarCycleInterpolationType(
    const PropertyHandle& property,
    const PropertyRegistration& registration)
    : InterpolationType(property), registration_(registration) {
  DCHECK(property.IsCSSCustomProperty());
}

static InterpolationValue CreateValue() {
  return InterpolationValue(InterpolableList::Create(0));
}

InterpolationValue CSSVarCycleInterpolationType::MaybeConvertSingle(
    const PropertySpecificKeyframe& keyframe,
    const InterpolationEnvironment&,
    const InterpolationValue&,
    ConversionCheckers&) const {
  const CSSCustomPropertyDeclaration& declaration =
      *ToCSSCustomPropertyDeclaration(
          ToCSSPropertySpecificKeyframe(keyframe).Value());
  DCHECK_EQ(GetProperty().CustomPropertyName(), declaration.GetName());
  if (!declaration.Value() || !declaration.Value()->NeedsVariableResolution()) {
    return nullptr;
  }
  return CreateValue();
}

PairwiseInterpolationValue CSSVarCycleInterpolationType::MaybeConvertPairwise(
    const PropertySpecificKeyframe& start_keyframe,
    const PropertySpecificKeyframe& end_keyframe,
    const InterpolationEnvironment& environment,
    const InterpolationValue& underlying,
    ConversionCheckers& conversionCheckers) const {
  InterpolationValue start = MaybeConvertSingle(start_keyframe, environment,
                                                underlying, conversionCheckers);
  InterpolationValue end = MaybeConvertSingle(start_keyframe, environment,
                                              underlying, conversionCheckers);
  // If either keyframe has a cyclic dependency then the entire interpolation
  // unsets the custom property.
  if (!start && !end) {
    return nullptr;
  }
  return PairwiseInterpolationValue(
      std::move((start ? std::move(start) : CreateValue()).interpolable_value),
      std::move((end ? std::move(end) : CreateValue()).interpolable_value));
}

void CSSVarCycleInterpolationType::Apply(
    const InterpolableValue&,
    const NonInterpolableValue*,
    InterpolationEnvironment& environment) const {
  StyleResolverState& state =
      ToCSSInterpolationEnvironment(environment).GetState();
  state.Style()->RemoveVariable(GetProperty().CustomPropertyName(),
                                registration_->Inherits());
}

}  // namespace blink
