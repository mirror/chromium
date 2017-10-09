// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/animation/CSSPathInterpolationType.h"

#include <memory>
#include "core/animation/PathInterpolationFunctions.h"
#include "core/animation/PathPropertyFunctions.h"
#include "core/css/CSSIdentifierValue.h"
#include "core/css/CSSPathValue.h"
#include "core/css/resolver/StyleResolverState.h"
#include "core/style/ComputedStyle.h"
#include "platform/wtf/PtrUtil.h"

namespace blink {

void CSSPathInterpolationType::ApplyStandardPropertyValue(
    const InterpolableValue& interpolable_value,
    const NonInterpolableValue* non_interpolable_value,
    StyleResolverState& state) const {
  std::unique_ptr<SVGPathByteStream> path_byte_stream =
      PathInterpolationFunctions::AppliedValue(interpolable_value,
                                               non_interpolable_value);
  if (path_byte_stream->IsEmpty()) {
    PathPropertyFunctions::SetPath(CssProperty(), *state.Style(), nullptr);
    return;
  }
  PathPropertyFunctions::SetPath(
      CssProperty(), *state.Style(),
      StylePath::Create(std::move(path_byte_stream)));
}

void CSSPathInterpolationType::Composite(
    UnderlyingValueOwner& underlying_value_owner,
    double underlying_fraction,
    const InterpolationValue& value,
    double interpolation_fraction) const {
  PathInterpolationFunctions::Composite(underlying_value_owner,
                                        underlying_fraction, *this, value);
}

InterpolationValue CSSPathInterpolationType::MaybeConvertNeutral(
    const InterpolationValue& underlying,
    ConversionCheckers& conversion_checkers) const {
  return PathInterpolationFunctions::MaybeConvertNeutral(underlying,
                                                         conversion_checkers);
}

InterpolationValue CSSPathInterpolationType::MaybeConvertInitial(
    const StyleResolverState&,
    ConversionCheckers&) const {
  return PathInterpolationFunctions::ConvertValue(nullptr);
}

class InheritedPathChecker : public CSSInterpolationType::CSSConversionChecker {
 public:
  static std::unique_ptr<InheritedPathChecker> Create(
      CSSPropertyID property,
      RefPtr<StylePath> style_path) {
    return WTF::WrapUnique(
        new InheritedPathChecker(property, std::move(style_path)));
  }

 private:
  InheritedPathChecker(CSSPropertyID property, RefPtr<StylePath> style_path)
      : property_(property), style_path_(std::move(style_path)) {}

  bool IsValid(const StyleResolverState& state,
               const InterpolationValue& underlying) const final {
    return PathPropertyFunctions::GetPath(property_, *state.ParentStyle()) ==
           style_path_.get();
  }

  const CSSPropertyID property_;
  const RefPtr<StylePath> style_path_;
};

InterpolationValue CSSPathInterpolationType::MaybeConvertInherit(
    const StyleResolverState& state,
    ConversionCheckers& conversion_checkers) const {
  if (!state.ParentStyle())
    return nullptr;

  conversion_checkers.push_back(InheritedPathChecker::Create(
      CssProperty(),
      PathPropertyFunctions::GetPath(CssProperty(), *state.ParentStyle())));
  return PathInterpolationFunctions::ConvertValue(
      PathPropertyFunctions::GetPath(CssProperty(), *state.ParentStyle()));
}

InterpolationValue CSSPathInterpolationType::MaybeConvertValue(
    const CSSValue& value,
    const StyleResolverState*,
    ConversionCheckers& conversion_checkers) const {
  if (!value.IsPathValue()) {
    DCHECK_EQ(ToCSSIdentifierValue(value).GetValueID(), CSSValueNone);
    return nullptr;
  }
  return PathInterpolationFunctions::ConvertValue(
      cssvalue::ToCSSPathValue(value).ByteStream());
}

InterpolationValue
CSSPathInterpolationType::MaybeConvertStandardPropertyUnderlyingValue(
    const ComputedStyle& style) const {
  return PathInterpolationFunctions::ConvertValue(
      PathPropertyFunctions::GetPath(CssProperty(), style));
}

PairwiseInterpolationValue CSSPathInterpolationType::MaybeMergeSingles(
    InterpolationValue&& start,
    InterpolationValue&& end) const {
  return PathInterpolationFunctions::MaybeMergeSingles(std::move(start),
                                                       std::move(end));
}

}  // namespace blink
