// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/animation/CSSInterpolationTypesMap.h"

#include "core/animation/CSSBasicShapeInterpolationType.h"
#include "core/animation/CSSBorderImageLengthBoxInterpolationType.h"
#include "core/animation/CSSClipInterpolationType.h"
#include "core/animation/CSSColorInterpolationType.h"
#include "core/animation/CSSFilterListInterpolationType.h"
#include "core/animation/CSSFontSizeInterpolationType.h"
#include "core/animation/CSSFontWeightInterpolationType.h"
#include "core/animation/CSSImageInterpolationType.h"
#include "core/animation/CSSImageListInterpolationType.h"
#include "core/animation/CSSImageSliceInterpolationType.h"
#include "core/animation/CSSLengthInterpolationType.h"
#include "core/animation/CSSLengthListInterpolationType.h"
#include "core/animation/CSSLengthPairInterpolationType.h"
#include "core/animation/CSSNumberInterpolationType.h"
#include "core/animation/CSSOffsetRotationInterpolationType.h"
#include "core/animation/CSSPaintInterpolationType.h"
#include "core/animation/CSSPathInterpolationType.h"
#include "core/animation/CSSPositionAxisListInterpolationType.h"
#include "core/animation/CSSPositionInterpolationType.h"
#include "core/animation/CSSRotateInterpolationType.h"
#include "core/animation/CSSScaleInterpolationType.h"
#include "core/animation/CSSShadowListInterpolationType.h"
#include "core/animation/CSSSizeListInterpolationType.h"
#include "core/animation/CSSTextIndentInterpolationType.h"
#include "core/animation/CSSTransformInterpolationType.h"
#include "core/animation/CSSTransformOriginInterpolationType.h"
#include "core/animation/CSSTranslateInterpolationType.h"
#include "core/animation/CSSValueInterpolationType.h"
#include "core/animation/CSSVisibilityInterpolationType.h"
#include "core/css/CSSPropertyMetadata.h"
#include "core/css/PropertyRegistry.h"
#include "wtf/PtrUtil.h"
#include <memory>

namespace blink {

const InterpolationTypes& CSSInterpolationTypesMap::get(
    const PropertyHandle& property) const {
  using ApplicableTypesMap =
      HashMap<PropertyHandle, std::unique_ptr<const InterpolationTypes>>;
  DEFINE_STATIC_LOCAL(ApplicableTypesMap, applicableTypesMap, ());
  auto entry = applicableTypesMap.find(property);
  if (entry != applicableTypesMap.end())
    return *entry->value.get();

  std::unique_ptr<InterpolationTypes> applicableTypes =
      WTF::makeUnique<InterpolationTypes>();

  CSSPropertyID cssProperty = property.isCSSProperty()
                                  ? property.cssProperty()
                                  : property.presentationAttribute();
  // We treat presentation attributes identically to their CSS property
  // equivalents when interpolating.
  PropertyHandle usedProperty =
      property.isCSSProperty() ? property : PropertyHandle(cssProperty);
  switch (cssProperty) {
    case CSSPropertyBaselineShift:
    case CSSPropertyBorderBottomWidth:
    case CSSPropertyBorderLeftWidth:
    case CSSPropertyBorderRightWidth:
    case CSSPropertyBorderTopWidth:
    case CSSPropertyBottom:
    case CSSPropertyCx:
    case CSSPropertyCy:
    case CSSPropertyFlexBasis:
    case CSSPropertyHeight:
    case CSSPropertyLeft:
    case CSSPropertyLetterSpacing:
    case CSSPropertyMarginBottom:
    case CSSPropertyMarginLeft:
    case CSSPropertyMarginRight:
    case CSSPropertyMarginTop:
    case CSSPropertyMaxHeight:
    case CSSPropertyMaxWidth:
    case CSSPropertyMinHeight:
    case CSSPropertyMinWidth:
    case CSSPropertyOffsetDistance:
    case CSSPropertyOutlineOffset:
    case CSSPropertyOutlineWidth:
    case CSSPropertyPaddingBottom:
    case CSSPropertyPaddingLeft:
    case CSSPropertyPaddingRight:
    case CSSPropertyPaddingTop:
    case CSSPropertyPerspective:
    case CSSPropertyR:
    case CSSPropertyRight:
    case CSSPropertyRx:
    case CSSPropertyRy:
    case CSSPropertyShapeMargin:
    case CSSPropertyStrokeDashoffset:
    case CSSPropertyStrokeWidth:
    case CSSPropertyTop:
    case CSSPropertyVerticalAlign:
    case CSSPropertyWebkitBorderHorizontalSpacing:
    case CSSPropertyWebkitBorderVerticalSpacing:
    case CSSPropertyColumnGap:
    case CSSPropertyColumnRuleWidth:
    case CSSPropertyColumnWidth:
    case CSSPropertyWebkitPerspectiveOriginX:
    case CSSPropertyWebkitPerspectiveOriginY:
    case CSSPropertyWebkitTransformOriginX:
    case CSSPropertyWebkitTransformOriginY:
    case CSSPropertyWebkitTransformOriginZ:
    case CSSPropertyWidth:
    case CSSPropertyWordSpacing:
    case CSSPropertyX:
    case CSSPropertyY:
      applicableTypes->append(
          WTF::makeUnique<CSSLengthInterpolationType>(usedProperty));
      break;
    case CSSPropertyFlexGrow:
    case CSSPropertyFlexShrink:
    case CSSPropertyFillOpacity:
    case CSSPropertyFloodOpacity:
    case CSSPropertyFontSizeAdjust:
    case CSSPropertyOpacity:
    case CSSPropertyOrphans:
    case CSSPropertyShapeImageThreshold:
    case CSSPropertyStopOpacity:
    case CSSPropertyStrokeMiterlimit:
    case CSSPropertyStrokeOpacity:
    case CSSPropertyColumnCount:
    case CSSPropertyWidows:
    case CSSPropertyZIndex:
      applicableTypes->append(
          WTF::makeUnique<CSSNumberInterpolationType>(usedProperty));
      break;
    case CSSPropertyLineHeight:
      applicableTypes->append(
          WTF::makeUnique<CSSLengthInterpolationType>(usedProperty));
      applicableTypes->append(
          WTF::makeUnique<CSSNumberInterpolationType>(usedProperty));
      break;
    case CSSPropertyBackgroundColor:
    case CSSPropertyBorderBottomColor:
    case CSSPropertyBorderLeftColor:
    case CSSPropertyBorderRightColor:
    case CSSPropertyBorderTopColor:
    case CSSPropertyColor:
    case CSSPropertyFloodColor:
    case CSSPropertyLightingColor:
    case CSSPropertyOutlineColor:
    case CSSPropertyStopColor:
    case CSSPropertyTextDecorationColor:
    case CSSPropertyColumnRuleColor:
    case CSSPropertyWebkitTextStrokeColor:
      applicableTypes->append(
          WTF::makeUnique<CSSColorInterpolationType>(usedProperty));
      break;
    case CSSPropertyFill:
    case CSSPropertyStroke:
      applicableTypes->append(
          WTF::makeUnique<CSSPaintInterpolationType>(usedProperty));
      break;
    case CSSPropertyD:
      applicableTypes->append(
          WTF::makeUnique<CSSPathInterpolationType>(usedProperty));
      break;
    case CSSPropertyBoxShadow:
    case CSSPropertyTextShadow:
      applicableTypes->append(
          WTF::makeUnique<CSSShadowListInterpolationType>(usedProperty));
      break;
    case CSSPropertyBorderImageSource:
    case CSSPropertyListStyleImage:
    case CSSPropertyWebkitMaskBoxImageSource:
      applicableTypes->append(
          WTF::makeUnique<CSSImageInterpolationType>(usedProperty));
      break;
    case CSSPropertyBackgroundImage:
    case CSSPropertyWebkitMaskImage:
      applicableTypes->append(
          WTF::makeUnique<CSSImageListInterpolationType>(usedProperty));
      break;
    case CSSPropertyStrokeDasharray:
      applicableTypes->append(
          WTF::makeUnique<CSSLengthListInterpolationType>(usedProperty));
      break;
    case CSSPropertyFontWeight:
      applicableTypes->append(
          WTF::makeUnique<CSSFontWeightInterpolationType>(usedProperty));
      break;
    case CSSPropertyVisibility:
      applicableTypes->append(
          WTF::makeUnique<CSSVisibilityInterpolationType>(usedProperty));
      break;
    case CSSPropertyClip:
      applicableTypes->append(
          WTF::makeUnique<CSSClipInterpolationType>(usedProperty));
      break;
    case CSSPropertyOffsetRotation:
    case CSSPropertyOffsetRotate:
      applicableTypes->append(
          WTF::makeUnique<CSSOffsetRotationInterpolationType>(usedProperty));
      break;
    case CSSPropertyBackgroundPositionX:
    case CSSPropertyBackgroundPositionY:
    case CSSPropertyWebkitMaskPositionX:
    case CSSPropertyWebkitMaskPositionY:
      applicableTypes->append(
          WTF::makeUnique<CSSPositionAxisListInterpolationType>(usedProperty));
      break;
    case CSSPropertyObjectPosition:
    case CSSPropertyOffsetAnchor:
    case CSSPropertyOffsetPosition:
    case CSSPropertyPerspectiveOrigin:
      applicableTypes->append(
          WTF::makeUnique<CSSPositionInterpolationType>(usedProperty));
      break;
    case CSSPropertyBorderBottomLeftRadius:
    case CSSPropertyBorderBottomRightRadius:
    case CSSPropertyBorderTopLeftRadius:
    case CSSPropertyBorderTopRightRadius:
      applicableTypes->append(
          WTF::makeUnique<CSSLengthPairInterpolationType>(usedProperty));
      break;
    case CSSPropertyTranslate:
      applicableTypes->append(
          WTF::makeUnique<CSSTranslateInterpolationType>(usedProperty));
      break;
    case CSSPropertyTransformOrigin:
      applicableTypes->append(
          WTF::makeUnique<CSSTransformOriginInterpolationType>(usedProperty));
      break;
    case CSSPropertyBackgroundSize:
    case CSSPropertyWebkitMaskSize:
      applicableTypes->append(
          WTF::makeUnique<CSSSizeListInterpolationType>(usedProperty));
      break;
    case CSSPropertyBorderImageOutset:
    case CSSPropertyBorderImageWidth:
    case CSSPropertyWebkitMaskBoxImageOutset:
    case CSSPropertyWebkitMaskBoxImageWidth:
      applicableTypes->append(
          WTF::makeUnique<CSSBorderImageLengthBoxInterpolationType>(
              usedProperty));
      break;
    case CSSPropertyScale:
      applicableTypes->append(
          WTF::makeUnique<CSSScaleInterpolationType>(usedProperty));
      break;
    case CSSPropertyFontSize:
      applicableTypes->append(
          WTF::makeUnique<CSSFontSizeInterpolationType>(usedProperty));
      break;
    case CSSPropertyTextIndent:
      applicableTypes->append(
          WTF::makeUnique<CSSTextIndentInterpolationType>(usedProperty));
      break;
    case CSSPropertyBorderImageSlice:
    case CSSPropertyWebkitMaskBoxImageSlice:
      applicableTypes->append(
          WTF::makeUnique<CSSImageSliceInterpolationType>(usedProperty));
      break;
    case CSSPropertyClipPath:
    case CSSPropertyShapeOutside:
      applicableTypes->append(
          WTF::makeUnique<CSSBasicShapeInterpolationType>(usedProperty));
      break;
    case CSSPropertyRotate:
      applicableTypes->append(
          WTF::makeUnique<CSSRotateInterpolationType>(usedProperty));
      break;
    case CSSPropertyBackdropFilter:
    case CSSPropertyFilter:
      applicableTypes->append(
          WTF::makeUnique<CSSFilterListInterpolationType>(usedProperty));
      break;
    case CSSPropertyTransform:
      applicableTypes->append(
          WTF::makeUnique<CSSTransformInterpolationType>(usedProperty));
      break;
    default:
      DCHECK(!CSSPropertyMetadata::isInterpolableProperty(cssProperty));
      // TODO(crbug.com/671904): Look up m_registry for custom property
      // InterpolationTypes.
      break;
  }

  applicableTypes->append(
      WTF::makeUnique<CSSValueInterpolationType>(usedProperty));

  auto addResult = applicableTypesMap.add(property, std::move(applicableTypes));
  return *addResult.storedValue->value.get();
}

size_t CSSInterpolationTypesMap::version() const {
  // Property registrations are never removed so the number of registered
  // custom properties is equivalent to how many changes there have been to the
  // property registry.
  return m_registry ? m_registry->registrationCount() : 0;
}

}  // namespace blink
