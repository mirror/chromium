// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/animation/CSSPathInterpolationType.h"

#include "core/animation/PathInterpolationFunctions.h"
#include "core/css/CSSPathValue.h"
#include "core/css/resolver/StyleResolverState.h"

namespace blink {

void CSSPathInterpolationType::apply(const InterpolableValue& interpolableValue, const NonInterpolableValue* nonInterpolableValue, InterpolationEnvironment& environment) const
{
    ASSERT(cssProperty() == CSSPropertyD);
    environment.state().style()->setD(StylePath::create(PathInterpolationFunctions::appliedValue(interpolableValue, nonInterpolableValue)));
}

void CSSPathInterpolationType::composite(UnderlyingValue& underlyingValue, double underlyingFraction, const InterpolationValue& value) const
{
    PathInterpolationFunctions::composite(underlyingValue, underlyingFraction, value);
}

PassOwnPtr<InterpolationValue> CSSPathInterpolationType::maybeConvertNeutral(const UnderlyingValue& underlyingValue, ConversionCheckers& conversionCheckers) const
{
    return PathInterpolationFunctions::maybeConvertNeutral(*this, underlyingValue, conversionCheckers);
}

PassOwnPtr<InterpolationValue> CSSPathInterpolationType::maybeConvertInitial() const
{
    return PathInterpolationFunctions::convertValue(*this, CSSPathValue::emptyPathValue()->byteStream());
}

class ParentPathChecker : public InterpolationType::ConversionChecker {
public:
    static PassOwnPtr<ParentPathChecker> create(const InterpolationType& type, PassRefPtr<StylePath> stylePath)
    {
        return adoptPtr(new ParentPathChecker(type, stylePath));
    }

private:
    ParentPathChecker(const InterpolationType& type, PassRefPtr<StylePath> stylePath)
        : ConversionChecker(type)
        , m_stylePath(stylePath)
    { }

    bool isValid(const InterpolationEnvironment& environment, const UnderlyingValue&) const final
    {
        return environment.state().parentStyle()->svgStyle().d() == m_stylePath.get();
    }

    const RefPtr<StylePath> m_stylePath;
};

PassOwnPtr<InterpolationValue> CSSPathInterpolationType::maybeConvertInherit(const StyleResolverState& state, ConversionCheckers& conversionCheckers) const
{
    ASSERT(cssProperty() == CSSPropertyD);
    if (!state.parentStyle())
        return nullptr;

    conversionCheckers.append(ParentPathChecker::create(*this, state.parentStyle()->svgStyle().d()));
    return PathInterpolationFunctions::convertValue(*this, state.parentStyle()->svgStyle().d()->byteStream());
}

PassOwnPtr<InterpolationValue> CSSPathInterpolationType::maybeConvertValue(const CSSValue& value, const StyleResolverState& state, ConversionCheckers& conversionCheckers) const
{
    return PathInterpolationFunctions::convertValue(*this, toCSSPathValue(value).byteStream());
}

PassOwnPtr<InterpolationValue> CSSPathInterpolationType::maybeConvertUnderlyingValue(const InterpolationEnvironment& environment) const
{
    ASSERT(cssProperty() == CSSPropertyD);
    return PathInterpolationFunctions::convertValue(*this, environment.state().style()->svgStyle().d()->byteStream());
}

PassOwnPtr<PairwisePrimitiveInterpolation> CSSPathInterpolationType::mergeSingleConversions(InterpolationValue& startValue, InterpolationValue& endValue) const
{
    return PathInterpolationFunctions::mergeSingleConversions(*this, startValue, endValue);
}

} // namespace blink
