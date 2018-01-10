/*
 * Copyright (C) 2004 Zack Rusin <zack@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012 Apple Inc.
 * All rights reserved.
 * Copyright (C) 2007 Alexey Proskuryakov <ap@webkit.org>
 * Copyright (C) 2007 Nicholas Shanks <webkit@nickshanks.com>
 * Copyright (C) 2011 Sencha, Inc. All rights reserved.
 * Copyright (C) 2015 Google Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include "core/css/ComputedStyleCSSValueMapping.h"

#include "base/macros.h"
#include "core/StylePropertyShorthand.h"
#include "core/animation/css/CSSAnimationData.h"
#include "core/animation/css/CSSTransitionData.h"
#include "core/css/BasicShapeFunctions.h"
#include "core/css/CSSBasicShapeValues.h"
#include "core/css/CSSColorValue.h"
#include "core/css/CSSCounterValue.h"
#include "core/css/CSSCursorImageValue.h"
#include "core/css/CSSCustomIdentValue.h"
#include "core/css/CSSCustomPropertyDeclaration.h"
#include "core/css/CSSFontFamilyValue.h"
#include "core/css/CSSFontFeatureValue.h"
#include "core/css/CSSFontStyleRangeValue.h"
#include "core/css/CSSFontVariationValue.h"
#include "core/css/CSSFunctionValue.h"
#include "core/css/CSSGridLineNamesValue.h"
#include "core/css/CSSGridTemplateAreasValue.h"
#include "core/css/CSSIdentifierValue.h"
#include "core/css/CSSInitialValue.h"
#include "core/css/CSSPathValue.h"
#include "core/css/CSSPrimitiveValue.h"
#include "core/css/CSSPrimitiveValueMappings.h"
#include "core/css/CSSQuadValue.h"
#include "core/css/CSSShadowValue.h"
#include "core/css/CSSStringValue.h"
#include "core/css/CSSTimingFunctionValue.h"
#include "core/css/CSSURIValue.h"
#include "core/css/CSSValueList.h"
#include "core/css/CSSValuePair.h"
#include "core/css/PropertyRegistry.h"
#include "core/css/ZoomAdjustedPixelValue.h"
#include "core/css/properties/ComputedStyleUtils.h"
#include "core/layout/LayoutBlock.h"
#include "core/layout/LayoutBox.h"
#include "core/layout/LayoutGrid.h"
#include "core/layout/LayoutObject.h"
#include "core/style/ComputedStyle.h"
#include "core/style/ContentData.h"
#include "core/style/CursorData.h"
#include "core/style/QuotesData.h"
#include "core/style/ShadowList.h"
#include "core/style/StyleInheritedVariables.h"
#include "core/style/StyleNonInheritedVariables.h"
#include "platform/LengthFunctions.h"

namespace blink {

using namespace cssvalue;

static CSSValueList* ValuesForGridShorthand(
    const StylePropertyShorthand& shorthand,
    const ComputedStyle& style,
    const LayoutObject* layout_object,
    Node* styled_node,
    bool allow_visited_style) {
  CSSValueList* list = CSSValueList::CreateSlashSeparated();
  for (size_t i = 0; i < shorthand.length(); ++i) {
    const CSSValue* value = ComputedStyleCSSValueMapping::Get(
        *shorthand.properties()[i], style, layout_object, styled_node,
        allow_visited_style);
    DCHECK(value);
    list->Append(*value);
  }
  return list;
}

static CSSValueList* ValuesForShorthandProperty(
    const StylePropertyShorthand& shorthand,
    const ComputedStyle& style,
    const LayoutObject* layout_object,
    Node* styled_node,
    bool allow_visited_style) {
  CSSValueList* list = CSSValueList::CreateSpaceSeparated();
  for (size_t i = 0; i < shorthand.length(); ++i) {
    const CSSValue* value = ComputedStyleCSSValueMapping::Get(
        *shorthand.properties()[i], style, layout_object, styled_node,
        allow_visited_style);
    DCHECK(value);
    list->Append(*value);
  }
  return list;
}

static CSSValue* ExpandNoneLigaturesValue() {
  CSSValueList* list = CSSValueList::CreateSpaceSeparated();
  list->Append(*CSSIdentifierValue::Create(CSSValueNoCommonLigatures));
  list->Append(*CSSIdentifierValue::Create(CSSValueNoDiscretionaryLigatures));
  list->Append(*CSSIdentifierValue::Create(CSSValueNoHistoricalLigatures));
  list->Append(*CSSIdentifierValue::Create(CSSValueNoContextual));
  return list;
}

static CSSValue* ValuesForFontVariantProperty(const ComputedStyle& style,
                                              const LayoutObject* layout_object,
                                              Node* styled_node,
                                              bool allow_visited_style) {
  enum VariantShorthandCases {
    kAllNormal,
    kNoneLigatures,
    kConcatenateNonNormal
  };
  StylePropertyShorthand shorthand = fontVariantShorthand();
  VariantShorthandCases shorthand_case = kAllNormal;
  for (size_t i = 0; i < shorthand.length(); ++i) {
    const CSSValue* value = ComputedStyleCSSValueMapping::Get(
        *shorthand.properties()[i], style, layout_object, styled_node,
        allow_visited_style);

    if (shorthand_case == kAllNormal && value->IsIdentifierValue() &&
        ToCSSIdentifierValue(value)->GetValueID() == CSSValueNone &&
        shorthand.properties()[i]->IDEquals(CSSPropertyFontVariantLigatures)) {
      shorthand_case = kNoneLigatures;
    } else if (!(value->IsIdentifierValue() &&
                 ToCSSIdentifierValue(value)->GetValueID() == CSSValueNormal)) {
      shorthand_case = kConcatenateNonNormal;
      break;
    }
  }

  switch (shorthand_case) {
    case kAllNormal:
      return CSSIdentifierValue::Create(CSSValueNormal);
    case kNoneLigatures:
      return CSSIdentifierValue::Create(CSSValueNone);
    case kConcatenateNonNormal: {
      CSSValueList* list = CSSValueList::CreateSpaceSeparated();
      for (size_t i = 0; i < shorthand.length(); ++i) {
        const CSSValue* value = ComputedStyleCSSValueMapping::Get(
            *shorthand.properties()[i], style, layout_object, styled_node,
            allow_visited_style);
        DCHECK(value);
        if (value->IsIdentifierValue() &&
            ToCSSIdentifierValue(value)->GetValueID() == CSSValueNone) {
          list->Append(*ExpandNoneLigaturesValue());
        } else if (!(value->IsIdentifierValue() &&
                     ToCSSIdentifierValue(value)->GetValueID() ==
                         CSSValueNormal)) {
          list->Append(*value);
        }
      }
      return list;
    }
    default:
      NOTREACHED();
      return nullptr;
  }
}

static CSSValueList* ValuesForSidesShorthand(
    const StylePropertyShorthand& shorthand,
    const ComputedStyle& style,
    const LayoutObject* layout_object,
    Node* styled_node,
    bool allow_visited_style) {
  CSSValueList* list = CSSValueList::CreateSpaceSeparated();
  // Assume the properties are in the usual order top, right, bottom, left.
  const CSSValue* top_value = ComputedStyleCSSValueMapping::Get(
      *shorthand.properties()[0], style, layout_object, styled_node,
      allow_visited_style);
  const CSSValue* right_value = ComputedStyleCSSValueMapping::Get(
      *shorthand.properties()[1], style, layout_object, styled_node,
      allow_visited_style);
  const CSSValue* bottom_value = ComputedStyleCSSValueMapping::Get(
      *shorthand.properties()[2], style, layout_object, styled_node,
      allow_visited_style);
  const CSSValue* left_value = ComputedStyleCSSValueMapping::Get(
      *shorthand.properties()[3], style, layout_object, styled_node,
      allow_visited_style);

  // All 4 properties must be specified.
  if (!top_value || !right_value || !bottom_value || !left_value)
    return nullptr;

  bool show_left = !DataEquivalent(right_value, left_value);
  bool show_bottom = !DataEquivalent(top_value, bottom_value) || show_left;
  bool show_right = !DataEquivalent(top_value, right_value) || show_bottom;

  list->Append(*top_value);
  if (show_right)
    list->Append(*right_value);
  if (show_bottom)
    list->Append(*bottom_value);
  if (show_left)
    list->Append(*left_value);

  return list;
}

static CSSValuePair* ValuesForInlineBlockShorthand(
    const StylePropertyShorthand& shorthand,
    const ComputedStyle& style,
    const LayoutObject* layout_object,
    Node* styled_node,
    bool allow_visited_style) {
  const CSSValue* start_value = ComputedStyleCSSValueMapping::Get(
      *shorthand.properties()[0], style, layout_object, styled_node,
      allow_visited_style);
  const CSSValue* end_value = ComputedStyleCSSValueMapping::Get(
      *shorthand.properties()[1], style, layout_object, styled_node,
      allow_visited_style);
  // Both properties must be specified.
  if (!start_value || !end_value)
    return nullptr;

  CSSValuePair* pair = CSSValuePair::Create(start_value, end_value,
                                            CSSValuePair::kDropIdenticalValues);
  return pair;
}

const CSSValue* ComputedStyleCSSValueMapping::Get(
    const AtomicString custom_property_name,
    const ComputedStyle& style,
    const PropertyRegistry* registry) {
  if (registry) {
    const PropertyRegistration* registration =
        registry->Registration(custom_property_name);
    if (registration) {
      const CSSValue* result = style.GetRegisteredVariable(
          custom_property_name, registration->Inherits());
      if (result)
        return result;
      return registration->Initial();
    }
  }

  bool is_inherited_property = true;
  CSSVariableData* data =
      style.GetVariable(custom_property_name, is_inherited_property);
  if (!data)
    return nullptr;

  return CSSCustomPropertyDeclaration::Create(custom_property_name, data);
}

std::unique_ptr<HashMap<AtomicString, scoped_refptr<CSSVariableData>>>
ComputedStyleCSSValueMapping::GetVariables(const ComputedStyle& style) {
  // TODO(timloh): Also return non-inherited variables
  StyleInheritedVariables* variables = style.InheritedVariables();
  if (variables)
    return variables->GetVariables();
  return nullptr;
}

const CSSValue* ComputedStyleCSSValueMapping::Get(
    const CSSProperty& property,
    const ComputedStyle& style,
    const LayoutObject* layout_object,
    Node* styled_node,
    bool allow_visited_style) {
  const SVGComputedStyle& svg_style = style.SvgStyle();
  const CSSProperty& resolved_property = property.ResolveDirectionAwareProperty(
      style.Direction(), style.GetWritingMode());

  switch (resolved_property.PropertyID()) {
    case CSSPropertyGridColumn:
      return ValuesForGridShorthand(gridColumnShorthand(), style, layout_object,
                                    styled_node, allow_visited_style);
    case CSSPropertyGridRow:
      return ValuesForGridShorthand(gridRowShorthand(), style, layout_object,
                                    styled_node, allow_visited_style);
    case CSSPropertyGridArea:
      return ValuesForGridShorthand(gridAreaShorthand(), style, layout_object,
                                    styled_node, allow_visited_style);
    case CSSPropertyGridTemplate:
      return ValuesForGridShorthand(gridTemplateShorthand(), style,
                                    layout_object, styled_node,
                                    allow_visited_style);
    case CSSPropertyGrid:
      return ValuesForGridShorthand(gridShorthand(), style, layout_object,
                                    styled_node, allow_visited_style);
    case CSSPropertyPlaceContent: {
      // TODO (jfernandez): The spec states that we should return the specified
      // value.
      return ValuesForShorthandProperty(placeContentShorthand(), style,
                                        layout_object, styled_node,
                                        allow_visited_style);
    }
    case CSSPropertyPlaceItems: {
      // TODO (jfernandez): The spec states that we should return the specified
      // value.
      return ValuesForShorthandProperty(placeItemsShorthand(), style,
                                        layout_object, styled_node,
                                        allow_visited_style);
    }
    case CSSPropertyPlaceSelf: {
      // TODO (jfernandez): The spec states that we should return the specified
      // value.
      return ValuesForShorthandProperty(placeSelfShorthand(), style,
                                        layout_object, styled_node,
                                        allow_visited_style);
    }
    case CSSPropertyFlex:
      return ValuesForShorthandProperty(flexShorthand(), style, layout_object,
                                        styled_node, allow_visited_style);
    case CSSPropertyFlexFlow:
      return ValuesForShorthandProperty(flexFlowShorthand(), style,
                                        layout_object, styled_node,
                                        allow_visited_style);
    case CSSPropertyGridGap:
      return ValuesForShorthandProperty(gridGapShorthand(), style,
                                        layout_object, styled_node,
                                        allow_visited_style);
    case CSSPropertyTextDecoration:
      return ValuesForShorthandProperty(textDecorationShorthand(), style,
                                        layout_object, styled_node,
                                        allow_visited_style);
    case CSSPropertyBorderBottom:
      return ValuesForShorthandProperty(borderBottomShorthand(), style,
                                        layout_object, styled_node,
                                        allow_visited_style);
    case CSSPropertyBorderColor:
      return ValuesForSidesShorthand(borderColorShorthand(), style,
                                     layout_object, styled_node,
                                     allow_visited_style);
    case CSSPropertyBorderLeft:
      return ValuesForShorthandProperty(borderLeftShorthand(), style,
                                        layout_object, styled_node,
                                        allow_visited_style);
    case CSSPropertyBorderRight:
      return ValuesForShorthandProperty(borderRightShorthand(), style,
                                        layout_object, styled_node,
                                        allow_visited_style);
    case CSSPropertyBorderStyle:
      return ValuesForSidesShorthand(borderStyleShorthand(), style,
                                     layout_object, styled_node,
                                     allow_visited_style);
    case CSSPropertyBorderTop:
      return ValuesForShorthandProperty(borderTopShorthand(), style,
                                        layout_object, styled_node,
                                        allow_visited_style);
    case CSSPropertyBorderWidth:
      return ValuesForSidesShorthand(borderWidthShorthand(), style,
                                     layout_object, styled_node,
                                     allow_visited_style);
    case CSSPropertyColumnRule:
      return ValuesForShorthandProperty(columnRuleShorthand(), style,
                                        layout_object, styled_node,
                                        allow_visited_style);
    case CSSPropertyColumns:
      return ValuesForShorthandProperty(columnsShorthand(), style,
                                        layout_object, styled_node,
                                        allow_visited_style);
    case CSSPropertyListStyle:
      return ValuesForShorthandProperty(listStyleShorthand(), style,
                                        layout_object, styled_node,
                                        allow_visited_style);
    case CSSPropertyMargin:
      return ValuesForSidesShorthand(marginShorthand(), style, layout_object,
                                     styled_node, allow_visited_style);
    case CSSPropertyOutline:
      return ValuesForShorthandProperty(outlineShorthand(), style,
                                        layout_object, styled_node,
                                        allow_visited_style);
    case CSSPropertyFontVariant:
      return ValuesForFontVariantProperty(style, layout_object, styled_node,
                                          allow_visited_style);
    case CSSPropertyPadding:
      return ValuesForSidesShorthand(paddingShorthand(), style, layout_object,
                                     styled_node, allow_visited_style);
    case CSSPropertyScrollPadding:
      return ValuesForSidesShorthand(scrollPaddingShorthand(), style,
                                     layout_object, styled_node,
                                     allow_visited_style);
    case CSSPropertyScrollPaddingBlock:
      return ValuesForInlineBlockShorthand(scrollPaddingBlockShorthand(), style,
                                           layout_object, styled_node,
                                           allow_visited_style);
    case CSSPropertyScrollPaddingInline:
      return ValuesForInlineBlockShorthand(scrollPaddingInlineShorthand(),
                                           style, layout_object, styled_node,
                                           allow_visited_style);
    case CSSPropertyScrollSnapMargin:
      return ValuesForSidesShorthand(scrollSnapMarginShorthand(), style,
                                     layout_object, styled_node,
                                     allow_visited_style);
    case CSSPropertyScrollSnapMarginBlock:
      return ValuesForInlineBlockShorthand(scrollSnapMarginBlockShorthand(),
                                           style, layout_object, styled_node,
                                           allow_visited_style);
    case CSSPropertyScrollSnapMarginInline:
      return ValuesForInlineBlockShorthand(scrollSnapMarginInlineShorthand(),
                                           style, layout_object, styled_node,
                                           allow_visited_style);

    ////////////////////////////////////////////////////////////////////////
    case CSSPropertyBorderCollapse:
      if (style.BorderCollapse() == EBorderCollapse::kCollapse)
        return CSSIdentifierValue::Create(CSSValueCollapse);
      return CSSIdentifierValue::Create(CSSValueSeparate);
    case CSSPropertyBottom:
      return ComputedStyleUtils::ValueForPositionOffset(
          style, resolved_property, layout_object);
    case CSSPropertyWebkitBoxDecorationBreak:
      if (style.BoxDecorationBreak() == EBoxDecorationBreak::kSlice)
        return CSSIdentifierValue::Create(CSSValueSlice);
      return CSSIdentifierValue::Create(CSSValueClone);
    case CSSPropertyBoxShadow:
      return ComputedStyleUtils::ValueForShadowList(style.BoxShadow(), style,
                                                    true);
    case CSSPropertyWebkitColumnBreakAfter:
      return ComputedStyleUtils::ValueForWebkitColumnBreakBetween(
          style.BreakAfter());
    case CSSPropertyWebkitColumnBreakBefore:
      return ComputedStyleUtils::ValueForWebkitColumnBreakBetween(
          style.BreakBefore());
    case CSSPropertyWebkitColumnBreakInside:
      return ComputedStyleUtils::ValueForWebkitColumnBreakInside(
          style.BreakInside());
    case CSSPropertyAlignItems:
      return ComputedStyleUtils::ValueForItemPositionWithOverflowAlignment(
          style.AlignItems());
    case CSSPropertyAlignSelf:
      return ComputedStyleUtils::ValueForItemPositionWithOverflowAlignment(
          style.AlignSelf());
    case CSSPropertyFloat:
      if (style.Display() != EDisplay::kNone && style.HasOutOfFlowPosition())
        return CSSIdentifierValue::Create(CSSValueNone);
      return CSSIdentifierValue::Create(style.Floating());
    case CSSPropertyFont:
      return ComputedStyleUtils::ValueForFont(style);
    case CSSPropertyFontFamily:
      return ComputedStyleUtils::ValueForFontFamily(style);
    case CSSPropertyFontSize:
      return ComputedStyleUtils::ValueForFontSize(style);
    case CSSPropertyFontSizeAdjust:
      if (style.HasFontSizeAdjust())
        return CSSPrimitiveValue::Create(style.FontSizeAdjust(),
                                         CSSPrimitiveValue::UnitType::kNumber);
      return CSSIdentifierValue::Create(CSSValueNone);
    case CSSPropertyFontStretch:
      return ComputedStyleUtils::ValueForFontStretch(style);
    case CSSPropertyFontStyle:
      return ComputedStyleUtils::ValueForFontStyle(style);
    case CSSPropertyFontWeight:
      return ComputedStyleUtils::ValueForFontWeight(style);
    // Specs mention that getComputedStyle() should return the used value of the
    // property instead of the computed one for grid-template-{rows|columns} but
    // not for the grid-auto-{rows|columns} as things like grid-auto-columns:
    // 2fr; cannot be resolved to a value in pixels as the '2fr' means very
    // different things depending on the size of the explicit grid or the number
    // of implicit tracks added to the grid. See
    // http://lists.w3.org/Archives/Public/www-style/2013Nov/0014.html
    case CSSPropertyGridAutoColumns:
      return ComputedStyleUtils::ValueForGridTrackSizeList(kForColumns, style);
    case CSSPropertyGridAutoRows:
      return ComputedStyleUtils::ValueForGridTrackSizeList(kForRows, style);

    case CSSPropertyGridTemplateColumns:
      return ComputedStyleUtils::ValueForGridTrackList(kForColumns,
                                                       layout_object, style);
    case CSSPropertyGridTemplateRows:
      return ComputedStyleUtils::ValueForGridTrackList(kForRows, layout_object,
                                                       style);

    case CSSPropertyGridColumnStart:
      return ComputedStyleUtils::ValueForGridPosition(style.GridColumnStart());
    case CSSPropertyGridColumnEnd:
      return ComputedStyleUtils::ValueForGridPosition(style.GridColumnEnd());
    case CSSPropertyGridRowStart:
      return ComputedStyleUtils::ValueForGridPosition(style.GridRowStart());
    case CSSPropertyGridRowEnd:
      return ComputedStyleUtils::ValueForGridPosition(style.GridRowEnd());
    case CSSPropertyGridTemplateAreas:
      if (!style.NamedGridAreaRowCount()) {
        DCHECK(!style.NamedGridAreaColumnCount());
        return CSSIdentifierValue::Create(CSSValueNone);
      }

      return CSSGridTemplateAreasValue::Create(
          style.NamedGridArea(), style.NamedGridAreaRowCount(),
          style.NamedGridAreaColumnCount());
    case CSSPropertyHeight:
      if (ComputedStyleUtils::WidthOrHeightShouldReturnUsedValue(
              layout_object)) {
        return ZoomAdjustedPixelValue(
            ComputedStyleUtils::SizingBox(*layout_object).Height(), style);
      }
      return ComputedStyleUtils::ZoomAdjustedPixelValueForLength(style.Height(),
                                                                 style);
    case CSSPropertyJustifyItems:
      return ComputedStyleUtils::ValueForItemPositionWithOverflowAlignment(
          style.JustifyItems().GetPosition() == ItemPosition::kAuto
              ? ComputedStyleInitialValues::InitialDefaultAlignment()
              : style.JustifyItems());
    case CSSPropertyJustifySelf:
      return ComputedStyleUtils::ValueForItemPositionWithOverflowAlignment(
          style.JustifySelf());
    case CSSPropertyLeft:
      return ComputedStyleUtils::ValueForPositionOffset(
          style, resolved_property, layout_object);
    case CSSPropertyLineHeight:
      return ComputedStyleUtils::ValueForLineHeight(style);
    case CSSPropertyMarginTop: {
      const Length& margin_top = style.MarginTop();
      if (margin_top.IsFixed() || !layout_object || !layout_object->IsBox()) {
        return ComputedStyleUtils::ZoomAdjustedPixelValueForLength(margin_top,
                                                                   style);
      }
      return ZoomAdjustedPixelValue(ToLayoutBox(layout_object)->MarginTop(),
                                    style);
    }
    case CSSPropertyMarginRight: {
      const Length& margin_right = style.MarginRight();
      if (margin_right.IsFixed() || !layout_object || !layout_object->IsBox()) {
        return ComputedStyleUtils::ZoomAdjustedPixelValueForLength(margin_right,
                                                                   style);
      }
      float value;
      if (margin_right.IsPercentOrCalc()) {
        // LayoutBox gives a marginRight() that is the distance between the
        // right-edge of the child box and the right-edge of the containing box,
        // when display == EDisplay::kBlock. Let's calculate the absolute value
        // of the specified margin-right % instead of relying on LayoutBox's
        // marginRight() value.
        value = MinimumValueForLength(
                    margin_right, ToLayoutBox(layout_object)
                                      ->ContainingBlockLogicalWidthForContent())
                    .ToFloat();
      } else {
        value = ToLayoutBox(layout_object)->MarginRight().ToFloat();
      }
      return ZoomAdjustedPixelValue(value, style);
    }
    case CSSPropertyMarginBottom: {
      const Length& margin_bottom = style.MarginBottom();
      if (margin_bottom.IsFixed() || !layout_object ||
          !layout_object->IsBox()) {
        return ComputedStyleUtils::ZoomAdjustedPixelValueForLength(
            margin_bottom, style);
      }
      return ZoomAdjustedPixelValue(ToLayoutBox(layout_object)->MarginBottom(),
                                    style);
    }
    case CSSPropertyMarginLeft: {
      const Length& margin_left = style.MarginLeft();
      if (margin_left.IsFixed() || !layout_object || !layout_object->IsBox()) {
        return ComputedStyleUtils::ZoomAdjustedPixelValueForLength(margin_left,
                                                                   style);
      }
      return ZoomAdjustedPixelValue(ToLayoutBox(layout_object)->MarginLeft(),
                                    style);
    }
    case CSSPropertyMaxHeight: {
      const Length& max_height = style.MaxHeight();
      if (max_height.IsMaxSizeNone())
        return CSSIdentifierValue::Create(CSSValueNone);
      return ComputedStyleUtils::ZoomAdjustedPixelValueForLength(max_height,
                                                                 style);
    }
    case CSSPropertyMaxWidth: {
      const Length& max_width = style.MaxWidth();
      if (max_width.IsMaxSizeNone())
        return CSSIdentifierValue::Create(CSSValueNone);
      return ComputedStyleUtils::ZoomAdjustedPixelValueForLength(max_width,
                                                                 style);
    }
    case CSSPropertyObjectPosition:
      return CSSValuePair::Create(
          ComputedStyleUtils::ZoomAdjustedPixelValueForLength(
              style.ObjectPosition().X(), style),
          ComputedStyleUtils::ZoomAdjustedPixelValueForLength(
              style.ObjectPosition().Y(), style),
          CSSValuePair::kKeepIdenticalValues);
    case CSSPropertyOutlineStyle:
      if (style.OutlineStyleIsAuto())
        return CSSIdentifierValue::Create(CSSValueAuto);
      return CSSIdentifierValue::Create(style.OutlineStyle());
    case CSSPropertyPaddingTop: {
      const Length& padding_top = style.PaddingTop();
      if (padding_top.IsFixed() || !layout_object || !layout_object->IsBox()) {
        return ComputedStyleUtils::ZoomAdjustedPixelValueForLength(padding_top,
                                                                   style);
      }
      return ZoomAdjustedPixelValue(
          ToLayoutBox(layout_object)->ComputedCSSPaddingTop(), style);
    }
    case CSSPropertyPaddingRight: {
      const Length& padding_right = style.PaddingRight();
      if (padding_right.IsFixed() || !layout_object ||
          !layout_object->IsBox()) {
        return ComputedStyleUtils::ZoomAdjustedPixelValueForLength(
            padding_right, style);
      }
      return ZoomAdjustedPixelValue(
          ToLayoutBox(layout_object)->ComputedCSSPaddingRight(), style);
    }
    case CSSPropertyPaddingBottom: {
      const Length& padding_bottom = style.PaddingBottom();
      if (padding_bottom.IsFixed() || !layout_object ||
          !layout_object->IsBox()) {
        return ComputedStyleUtils::ZoomAdjustedPixelValueForLength(
            padding_bottom, style);
      }
      return ZoomAdjustedPixelValue(
          ToLayoutBox(layout_object)->ComputedCSSPaddingBottom(), style);
    }
    case CSSPropertyPaddingLeft: {
      const Length& padding_left = style.PaddingLeft();
      if (padding_left.IsFixed() || !layout_object || !layout_object->IsBox()) {
        return ComputedStyleUtils::ZoomAdjustedPixelValueForLength(padding_left,
                                                                   style);
      }
      return ZoomAdjustedPixelValue(
          ToLayoutBox(layout_object)->ComputedCSSPaddingLeft(), style);
    }
    case CSSPropertyPageBreakAfter:
      return ComputedStyleUtils::ValueForPageBreakBetween(style.BreakAfter());
    case CSSPropertyPageBreakBefore:
      return ComputedStyleUtils::ValueForPageBreakBetween(style.BreakBefore());
    case CSSPropertyPageBreakInside:
      return ComputedStyleUtils::ValueForPageBreakInside(style.BreakInside());
    case CSSPropertyRight:
      return ComputedStyleUtils::ValueForPositionOffset(
          style, resolved_property, layout_object);
    case CSSPropertyTextDecorationLine:
      return ComputedStyleUtils::RenderTextDecorationFlagsToCSSValue(
          style.GetTextDecoration());
    case CSSPropertyTextDecorationSkipInk:
      return ComputedStyleUtils::ValueForTextDecorationSkipInk(
          style.TextDecorationSkipInk());
    case CSSPropertyTextDecorationStyle:
      return ComputedStyleUtils::ValueForTextDecorationStyle(
          style.TextDecorationStyle());
    case CSSPropertyWebkitTextDecorationsInEffect:
      return ComputedStyleUtils::RenderTextDecorationFlagsToCSSValue(
          style.TextDecorationsInEffect());
    case CSSPropertyTextIndent: {
      CSSValueList* list = CSSValueList::CreateSpaceSeparated();
      list->Append(*ComputedStyleUtils::ZoomAdjustedPixelValueForLength(
          style.TextIndent(), style));
      if (RuntimeEnabledFeatures::CSS3TextEnabled() &&
          (style.GetTextIndentLine() == TextIndentLine::kEachLine ||
           style.GetTextIndentType() == TextIndentType::kHanging)) {
        if (style.GetTextIndentLine() == TextIndentLine::kEachLine)
          list->Append(*CSSIdentifierValue::Create(CSSValueEachLine));
        if (style.GetTextIndentType() == TextIndentType::kHanging)
          list->Append(*CSSIdentifierValue::Create(CSSValueHanging));
      }
      return list;
    }
    case CSSPropertyTextShadow:
      return ComputedStyleUtils::ValueForShadowList(style.TextShadow(), style,
                                                    false);
    case CSSPropertyTextOverflow:
      if (style.TextOverflow() != ETextOverflow::kClip)
        return CSSIdentifierValue::Create(CSSValueEllipsis);
      return CSSIdentifierValue::Create(CSSValueClip);
    case CSSPropertyTop:
      return ComputedStyleUtils::ValueForPositionOffset(
          style, resolved_property, layout_object);
    case CSSPropertyTouchAction:
      return ComputedStyleUtils::TouchActionFlagsToCSSValue(
          style.GetTouchAction());
    case CSSPropertyVerticalAlign:
      switch (style.VerticalAlign()) {
        case EVerticalAlign::kBaseline:
          return CSSIdentifierValue::Create(CSSValueBaseline);
        case EVerticalAlign::kMiddle:
          return CSSIdentifierValue::Create(CSSValueMiddle);
        case EVerticalAlign::kSub:
          return CSSIdentifierValue::Create(CSSValueSub);
        case EVerticalAlign::kSuper:
          return CSSIdentifierValue::Create(CSSValueSuper);
        case EVerticalAlign::kTextTop:
          return CSSIdentifierValue::Create(CSSValueTextTop);
        case EVerticalAlign::kTextBottom:
          return CSSIdentifierValue::Create(CSSValueTextBottom);
        case EVerticalAlign::kTop:
          return CSSIdentifierValue::Create(CSSValueTop);
        case EVerticalAlign::kBottom:
          return CSSIdentifierValue::Create(CSSValueBottom);
        case EVerticalAlign::kBaselineMiddle:
          return CSSIdentifierValue::Create(CSSValueWebkitBaselineMiddle);
        case EVerticalAlign::kLength:
          return ComputedStyleUtils::ZoomAdjustedPixelValueForLength(
              style.GetVerticalAlignLength(), style);
      }
      NOTREACHED();
      return nullptr;
    case CSSPropertyWidth: {
      if (ComputedStyleUtils::WidthOrHeightShouldReturnUsedValue(
              layout_object)) {
        return ZoomAdjustedPixelValue(
            ComputedStyleUtils::SizingBox(*layout_object).Width(), style);
      }
      return ComputedStyleUtils::ZoomAdjustedPixelValueForLength(style.Width(),
                                                                 style);
    }
    case CSSPropertyWillChange:
      return ComputedStyleUtils::ValueForWillChange(
          style.WillChangeProperties(), style.WillChangeContents(),
          style.WillChangeScrollPosition());
    case CSSPropertyFontVariantLigatures:
      return ComputedStyleUtils::ValueForFontVariantLigatures(style);
    case CSSPropertyFontVariantCaps:
      return ComputedStyleUtils::ValueForFontVariantCaps(style);
    case CSSPropertyFontVariantNumeric:
      return ComputedStyleUtils::ValueForFontVariantNumeric(style);
    case CSSPropertyFontVariantEastAsian:
      return ComputedStyleUtils::ValueForFontVariantEastAsian(style);
    case CSSPropertyZIndex:
      if (style.HasAutoZIndex() || !style.IsStackingContext())
        return CSSIdentifierValue::Create(CSSValueAuto);
      return CSSPrimitiveValue::Create(style.ZIndex(),
                                       CSSPrimitiveValue::UnitType::kInteger);
    case CSSPropertyBoxSizing:
      if (style.BoxSizing() == EBoxSizing::kContentBox)
        return CSSIdentifierValue::Create(CSSValueContentBox);
      return CSSIdentifierValue::Create(CSSValueBorderBox);
    case CSSPropertyAnimationDelay:
      return ComputedStyleUtils::ValueForAnimationDelay(style.Animations());
    case CSSPropertyAnimationDirection: {
      CSSValueList* list = CSSValueList::CreateCommaSeparated();
      const CSSAnimationData* animation_data = style.Animations();
      if (animation_data) {
        for (size_t i = 0; i < animation_data->DirectionList().size(); ++i) {
          list->Append(*ComputedStyleUtils::ValueForAnimationDirection(
              animation_data->DirectionList()[i]));
        }
      } else {
        list->Append(*CSSIdentifierValue::Create(CSSValueNormal));
      }
      return list;
    }
    case CSSPropertyAnimationDuration:
      return ComputedStyleUtils::ValueForAnimationDuration(style.Animations());
    case CSSPropertyAnimationFillMode: {
      CSSValueList* list = CSSValueList::CreateCommaSeparated();
      const CSSAnimationData* animation_data = style.Animations();
      if (animation_data) {
        for (size_t i = 0; i < animation_data->FillModeList().size(); ++i) {
          list->Append(*ComputedStyleUtils::ValueForAnimationFillMode(
              animation_data->FillModeList()[i]));
        }
      } else {
        list->Append(*CSSIdentifierValue::Create(CSSValueNone));
      }
      return list;
    }
    case CSSPropertyAnimationIterationCount: {
      CSSValueList* list = CSSValueList::CreateCommaSeparated();
      const CSSAnimationData* animation_data = style.Animations();
      if (animation_data) {
        for (size_t i = 0; i < animation_data->IterationCountList().size(); ++i)
          list->Append(*ComputedStyleUtils::ValueForAnimationIterationCount(
              animation_data->IterationCountList()[i]));
      } else {
        list->Append(*CSSPrimitiveValue::Create(
            CSSAnimationData::InitialIterationCount(),
            CSSPrimitiveValue::UnitType::kNumber));
      }
      return list;
    }
    case CSSPropertyAnimationPlayState: {
      CSSValueList* list = CSSValueList::CreateCommaSeparated();
      const CSSAnimationData* animation_data = style.Animations();
      if (animation_data) {
        for (size_t i = 0; i < animation_data->PlayStateList().size(); ++i) {
          list->Append(*ComputedStyleUtils::ValueForAnimationPlayState(
              animation_data->PlayStateList()[i]));
        }
      } else {
        list->Append(*CSSIdentifierValue::Create(CSSValueRunning));
      }
      return list;
    }
    case CSSPropertyAnimationTimingFunction:
      return ComputedStyleUtils::ValueForAnimationTimingFunction(
          style.Animations());
    case CSSPropertyAnimation: {
      const CSSAnimationData* animation_data = style.Animations();
      if (animation_data) {
        CSSValueList* animations_list = CSSValueList::CreateCommaSeparated();
        for (size_t i = 0; i < animation_data->NameList().size(); ++i) {
          CSSValueList* list = CSSValueList::CreateSpaceSeparated();
          list->Append(
              *CSSCustomIdentValue::Create(animation_data->NameList()[i]));
          list->Append(*CSSPrimitiveValue::Create(
              CSSTimingData::GetRepeated(animation_data->DurationList(), i),
              CSSPrimitiveValue::UnitType::kSeconds));
          list->Append(*ComputedStyleUtils::CreateTimingFunctionValue(
              CSSTimingData::GetRepeated(animation_data->TimingFunctionList(),
                                         i)
                  .get()));
          list->Append(*CSSPrimitiveValue::Create(
              CSSTimingData::GetRepeated(animation_data->DelayList(), i),
              CSSPrimitiveValue::UnitType::kSeconds));
          list->Append(*ComputedStyleUtils::ValueForAnimationIterationCount(
              CSSTimingData::GetRepeated(animation_data->IterationCountList(),
                                         i)));
          list->Append(*ComputedStyleUtils::ValueForAnimationDirection(
              CSSTimingData::GetRepeated(animation_data->DirectionList(), i)));
          list->Append(*ComputedStyleUtils::ValueForAnimationFillMode(
              CSSTimingData::GetRepeated(animation_data->FillModeList(), i)));
          list->Append(*ComputedStyleUtils::ValueForAnimationPlayState(
              CSSTimingData::GetRepeated(animation_data->PlayStateList(), i)));
          animations_list->Append(*list);
        }
        return animations_list;
      }

      CSSValueList* list = CSSValueList::CreateSpaceSeparated();
      // animation-name default value.
      list->Append(*CSSIdentifierValue::Create(CSSValueNone));
      list->Append(
          *CSSPrimitiveValue::Create(CSSAnimationData::InitialDuration(),
                                     CSSPrimitiveValue::UnitType::kSeconds));
      list->Append(*ComputedStyleUtils::CreateTimingFunctionValue(
          CSSAnimationData::InitialTimingFunction().get()));
      list->Append(
          *CSSPrimitiveValue::Create(CSSAnimationData::InitialDelay(),
                                     CSSPrimitiveValue::UnitType::kSeconds));
      list->Append(
          *CSSPrimitiveValue::Create(CSSAnimationData::InitialIterationCount(),
                                     CSSPrimitiveValue::UnitType::kNumber));
      list->Append(*ComputedStyleUtils::ValueForAnimationDirection(
          CSSAnimationData::InitialDirection()));
      list->Append(*ComputedStyleUtils::ValueForAnimationFillMode(
          CSSAnimationData::InitialFillMode()));
      // Initial animation-play-state.
      list->Append(*CSSIdentifierValue::Create(CSSValueRunning));
      return list;
    }
    case CSSPropertyPerspectiveOrigin: {
      CSSValueList* list = CSSValueList::CreateSpaceSeparated();
      if (layout_object) {
        LayoutRect box;
        if (layout_object->IsBox())
          box = ToLayoutBox(layout_object)->BorderBoxRect();

        list->Append(*ZoomAdjustedPixelValue(
            MinimumValueForLength(style.PerspectiveOriginX(), box.Width()),
            style));
        list->Append(*ZoomAdjustedPixelValue(
            MinimumValueForLength(style.PerspectiveOriginY(), box.Height()),
            style));
      } else {
        list->Append(*ComputedStyleUtils::ZoomAdjustedPixelValueForLength(
            style.PerspectiveOriginX(), style));
        list->Append(*ComputedStyleUtils::ZoomAdjustedPixelValueForLength(
            style.PerspectiveOriginY(), style));
      }
      return list;
    }
    case CSSPropertyBorderBottomLeftRadius:
      return &ComputedStyleUtils::ValueForBorderRadiusCorner(
          style.BorderBottomLeftRadius(), style);
    case CSSPropertyBorderBottomRightRadius:
      return &ComputedStyleUtils::ValueForBorderRadiusCorner(
          style.BorderBottomRightRadius(), style);
    case CSSPropertyBorderTopLeftRadius:
      return &ComputedStyleUtils::ValueForBorderRadiusCorner(
          style.BorderTopLeftRadius(), style);
    case CSSPropertyBorderTopRightRadius:
      return &ComputedStyleUtils::ValueForBorderRadiusCorner(
          style.BorderTopRightRadius(), style);
    case CSSPropertyClip: {
      if (style.HasAutoClip())
        return CSSIdentifierValue::Create(CSSValueAuto);
      CSSValue* top = ComputedStyleUtils::ZoomAdjustedPixelValueOrAuto(
          style.Clip().Top(), style);
      CSSValue* right = ComputedStyleUtils::ZoomAdjustedPixelValueOrAuto(
          style.Clip().Right(), style);
      CSSValue* bottom = ComputedStyleUtils::ZoomAdjustedPixelValueOrAuto(
          style.Clip().Bottom(), style);
      CSSValue* left = ComputedStyleUtils::ZoomAdjustedPixelValueOrAuto(
          style.Clip().Left(), style);
      return CSSQuadValue::Create(top, right, bottom, left,
                                  CSSQuadValue::kSerializeAsRect);
    }
    case CSSPropertyTransform:
      return ComputedStyleUtils::ComputedTransform(layout_object, style);
    case CSSPropertyTransformOrigin: {
      CSSValueList* list = CSSValueList::CreateSpaceSeparated();
      if (layout_object) {
        LayoutRect box;
        if (layout_object->IsBox())
          box = ToLayoutBox(layout_object)->BorderBoxRect();

        list->Append(*ZoomAdjustedPixelValue(
            MinimumValueForLength(style.TransformOriginX(), box.Width()),
            style));
        list->Append(*ZoomAdjustedPixelValue(
            MinimumValueForLength(style.TransformOriginY(), box.Height()),
            style));
        if (style.TransformOriginZ() != 0)
          list->Append(
              *ZoomAdjustedPixelValue(style.TransformOriginZ(), style));
      } else {
        list->Append(*ComputedStyleUtils::ZoomAdjustedPixelValueForLength(
            style.TransformOriginX(), style));
        list->Append(*ComputedStyleUtils::ZoomAdjustedPixelValueForLength(
            style.TransformOriginY(), style));
        if (style.TransformOriginZ() != 0)
          list->Append(
              *ZoomAdjustedPixelValue(style.TransformOriginZ(), style));
      }
      return list;
    }
    case CSSPropertyTransitionDelay:
      return ComputedStyleUtils::ValueForAnimationDelay(style.Transitions());
    case CSSPropertyTransitionDuration:
      return ComputedStyleUtils::ValueForAnimationDuration(style.Transitions());
    case CSSPropertyTransitionProperty:
      return ComputedStyleUtils::ValueForTransitionProperty(
          style.Transitions());
    case CSSPropertyTransitionTimingFunction:
      return ComputedStyleUtils::ValueForAnimationTimingFunction(
          style.Transitions());
    case CSSPropertyTransition: {
      const CSSTransitionData* transition_data = style.Transitions();
      if (transition_data) {
        CSSValueList* transitions_list = CSSValueList::CreateCommaSeparated();
        for (size_t i = 0; i < transition_data->PropertyList().size(); ++i) {
          CSSValueList* list = CSSValueList::CreateSpaceSeparated();
          list->Append(*ComputedStyleUtils::CreateTransitionPropertyValue(
              transition_data->PropertyList()[i]));
          list->Append(*CSSPrimitiveValue::Create(
              CSSTimingData::GetRepeated(transition_data->DurationList(), i),
              CSSPrimitiveValue::UnitType::kSeconds));
          list->Append(*ComputedStyleUtils::CreateTimingFunctionValue(
              CSSTimingData::GetRepeated(transition_data->TimingFunctionList(),
                                         i)
                  .get()));
          list->Append(*CSSPrimitiveValue::Create(
              CSSTimingData::GetRepeated(transition_data->DelayList(), i),
              CSSPrimitiveValue::UnitType::kSeconds));
          transitions_list->Append(*list);
        }
        return transitions_list;
      }

      CSSValueList* list = CSSValueList::CreateSpaceSeparated();
      // transition-property default value.
      list->Append(*CSSIdentifierValue::Create(CSSValueAll));
      list->Append(
          *CSSPrimitiveValue::Create(CSSTransitionData::InitialDuration(),
                                     CSSPrimitiveValue::UnitType::kSeconds));
      list->Append(*ComputedStyleUtils::CreateTimingFunctionValue(
          CSSTransitionData::InitialTimingFunction().get()));
      list->Append(
          *CSSPrimitiveValue::Create(CSSTransitionData::InitialDelay(),
                                     CSSPrimitiveValue::UnitType::kSeconds));
      return list;
    }
    case CSSPropertyWebkitTextCombine:
      if (style.TextCombine() == ETextCombine::kAll)
        return CSSIdentifierValue::Create(CSSValueHorizontal);
      return CSSIdentifierValue::Create(style.TextCombine());
    case CSSPropertyWebkitTextOrientation:
      if (style.GetTextOrientation() == ETextOrientation::kMixed)
        return CSSIdentifierValue::Create(CSSValueVerticalRight);
      return CSSIdentifierValue::Create(style.GetTextOrientation());
    case CSSPropertyContent:
      return ComputedStyleUtils::ValueForContentData(style);
    case CSSPropertyCounterIncrement:
    case CSSPropertyCounterReset:
      return ComputedStyleUtils::ValueForCounterDirectives(style,
                                                           resolved_property);
    case CSSPropertyShapeOutside:
      return ComputedStyleUtils::ValueForShape(style, style.ShapeOutside());
    case CSSPropertyFilter:
      return ComputedStyleUtils::ValueForFilter(style, style.Filter());
    case CSSPropertyBackdropFilter:
      return ComputedStyleUtils::ValueForFilter(style, style.BackdropFilter());
    case CSSPropertyBorderRadius:
      return ComputedStyleUtils::ValueForBorderRadiusShorthand(style);
    // SVG properties.
    case CSSPropertyFill:
      return ComputedStyleUtils::AdjustSVGPaintForCurrentColor(
          svg_style.FillPaintType(), svg_style.FillPaintUri(),
          svg_style.FillPaintColor(), style.GetColor());
    case CSSPropertyStroke:
      return ComputedStyleUtils::AdjustSVGPaintForCurrentColor(
          svg_style.StrokePaintType(), svg_style.StrokePaintUri(),
          svg_style.StrokePaintColor(), style.GetColor());
    case CSSPropertyStrokeDasharray:
      return ComputedStyleUtils::StrokeDashArrayToCSSValueList(
          *svg_style.StrokeDashArray(), style);
    case CSSPropertyBaselineShift: {
      switch (svg_style.BaselineShift()) {
        case BS_SUPER:
          return CSSIdentifierValue::Create(CSSValueSuper);
        case BS_SUB:
          return CSSIdentifierValue::Create(CSSValueSub);
        case BS_LENGTH:
          return ComputedStyleUtils::ZoomAdjustedPixelValueForLength(
              svg_style.BaselineShiftValue(), style);
      }
      NOTREACHED();
      return nullptr;
    }
    case CSSPropertyPaintOrder:
      return ComputedStyleUtils::PaintOrderToCSSValueList(svg_style);
    case CSSPropertyScrollSnapType:
      return ComputedStyleUtils::ValueForScrollSnapType(
          style.GetScrollSnapType(), style);
    case CSSPropertyScrollSnapAlign:
      return ComputedStyleUtils::ValueForScrollSnapAlign(
          style.GetScrollSnapAlign(), style);
    case CSSPropertyTranslate: {
      if (!style.Translate())
        return CSSIdentifierValue::Create(CSSValueNone);

      CSSValueList* list = CSSValueList::CreateSpaceSeparated();
      if (layout_object && layout_object->IsBox()) {
        LayoutRect box = ToLayoutBox(layout_object)->BorderBoxRect();
        list->Append(*ZoomAdjustedPixelValue(
            FloatValueForLength(style.Translate()->X(), box.Width().ToFloat()),
            style));
        if (!style.Translate()->Y().IsZero() || style.Translate()->Z() != 0) {
          list->Append(*ZoomAdjustedPixelValue(
              FloatValueForLength(style.Translate()->Y(),
                                  box.Height().ToFloat()),
              style));
        }
      } else {
        // No box to resolve the percentage values
        list->Append(*ComputedStyleUtils::ZoomAdjustedPixelValueForLength(
            style.Translate()->X(), style));

        if (!style.Translate()->Y().IsZero() || style.Translate()->Z() != 0) {
          list->Append(*ComputedStyleUtils::ZoomAdjustedPixelValueForLength(
              style.Translate()->Y(), style));
        }
      }

      if (style.Translate()->Z() != 0)
        list->Append(*ZoomAdjustedPixelValue(style.Translate()->Z(), style));

      return list;
    }
    default:
      return resolved_property.CSSValueFromComputedStyle(
          style, layout_object, styled_node, allow_visited_style);
  }
}

}  // namespace blink
