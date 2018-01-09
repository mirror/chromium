// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/properties/ComputedStyleUtils.h"

#include "core/css/CSSBorderImage.h"
#include "core/css/CSSBorderImageSliceValue.h"
#include "core/css/CSSColorValue.h"
#include "core/css/CSSCustomIdentValue.h"
#include "core/css/CSSFontFamilyValue.h"
#include "core/css/CSSFunctionValue.h"
#include "core/css/CSSGridLineNamesValue.h"
#include "core/css/CSSInitialValue.h"
#include "core/css/CSSPrimitiveValueMappings.h"
#include "core/css/CSSQuadValue.h"
#include "core/css/CSSReflectValue.h"
#include "core/css/CSSTimingFunctionValue.h"
#include "core/css/CSSValue.h"
#include "core/css/CSSValuePair.h"
#include "core/css/StyleColor.h"
#include "core/layout/LayoutBlock.h"
#include "core/layout/LayoutBox.h"
#include "core/layout/LayoutGrid.h"

namespace blink {

CSSValue* ComputedStyleUtils::CurrentColorOrValidColor(
    const ComputedStyle& style,
    const StyleColor& color) {
  // This function does NOT look at visited information, so that computed style
  // doesn't expose that.
  return cssvalue::CSSColorValue::Create(color.Resolve(style.GetColor()).Rgb());
}

const blink::Color ComputedStyleUtils::BorderSideColor(
    const ComputedStyle& style,
    const StyleColor& color,
    EBorderStyle border_style,
    bool visited_link) {
  if (!color.IsCurrentColor())
    return color.GetColor();
  // FIXME: Treating styled borders with initial color differently causes
  // problems, see crbug.com/316559, crbug.com/276231
  if (!visited_link && (border_style == EBorderStyle::kInset ||
                        border_style == EBorderStyle::kOutset ||
                        border_style == EBorderStyle::kRidge ||
                        border_style == EBorderStyle::kGroove))
    return blink::Color(238, 238, 238);
  return visited_link ? style.VisitedLinkColor() : style.GetColor();
}

// TODO(rjwright): make this const
CSSValue* ComputedStyleUtils::ZoomAdjustedPixelValueForLength(
    const Length& length,
    const ComputedStyle& style) {
  if (length.IsFixed())
    return ZoomAdjustedPixelValue(length.Value(), style);
  return CSSValue::Create(length, style.EffectiveZoom());
}

const CSSValue* ComputedStyleUtils::BackgroundImageOrWebkitMaskImage(
    const FillLayer& fill_layer) {
  CSSValueList* list = CSSValueList::CreateCommaSeparated();
  const FillLayer* curr_layer = &fill_layer;
  for (; curr_layer; curr_layer = curr_layer->Next()) {
    if (curr_layer->GetImage())
      list->Append(*curr_layer->GetImage()->ComputedCSSValue());
    else
      list->Append(*CSSIdentifierValue::Create(CSSValueNone));
  }
  return list;
}

const CSSValue* ComputedStyleUtils::ValueForFillSize(
    const FillSize& fill_size,
    const ComputedStyle& style) {
  if (fill_size.type == EFillSizeType::kContain)
    return CSSIdentifierValue::Create(CSSValueContain);

  if (fill_size.type == EFillSizeType::kCover)
    return CSSIdentifierValue::Create(CSSValueCover);

  if (fill_size.size.Height().IsAuto()) {
    return ZoomAdjustedPixelValueForLength(fill_size.size.Width(), style);
  }

  CSSValueList* list = CSSValueList::CreateSpaceSeparated();
  list->Append(*ZoomAdjustedPixelValueForLength(fill_size.size.Width(), style));
  list->Append(
      *ZoomAdjustedPixelValueForLength(fill_size.size.Height(), style));
  return list;
}

const CSSValue* ComputedStyleUtils::BackgroundImageOrWebkitMaskSize(
    const ComputedStyle& style,
    const FillLayer& fill_layer) {
  CSSValueList* list = CSSValueList::CreateCommaSeparated();
  const FillLayer* curr_layer = &fill_layer;
  for (; curr_layer; curr_layer = curr_layer->Next())
    list->Append(*ValueForFillSize(curr_layer->Size(), style));
  return list;
}

const CSSValueList* ComputedStyleUtils::CreatePositionListForLayer(
    const CSSProperty& property,
    const FillLayer& layer,
    const ComputedStyle& style) {
  CSSValueList* position_list = CSSValueList::CreateSpaceSeparated();
  if (layer.IsBackgroundXOriginSet()) {
    DCHECK(property.IDEquals(CSSPropertyBackgroundPosition) ||
           property.IDEquals(CSSPropertyWebkitMaskPosition));
    position_list->Append(
        *CSSIdentifierValue::Create(layer.BackgroundXOrigin()));
  }
  position_list->Append(
      *ZoomAdjustedPixelValueForLength(layer.PositionX(), style));
  if (layer.IsBackgroundYOriginSet()) {
    DCHECK(property.IDEquals(CSSPropertyBackgroundPosition) ||
           property.IDEquals(CSSPropertyWebkitMaskPosition));
    position_list->Append(
        *CSSIdentifierValue::Create(layer.BackgroundYOrigin()));
  }
  position_list->Append(
      *ZoomAdjustedPixelValueForLength(layer.PositionY(), style));
  return position_list;
}

const CSSValue* ComputedStyleUtils::ValueForFillRepeat(EFillRepeat x_repeat,
                                                       EFillRepeat y_repeat) {
  // For backwards compatibility, if both values are equal, just return one of
  // them. And if the two values are equivalent to repeat-x or repeat-y, just
  // return the shorthand.
  if (x_repeat == y_repeat)
    return CSSIdentifierValue::Create(x_repeat);
  if (x_repeat == EFillRepeat::kRepeatFill &&
      y_repeat == EFillRepeat::kNoRepeatFill)
    return CSSIdentifierValue::Create(CSSValueRepeatX);
  if (x_repeat == EFillRepeat::kNoRepeatFill &&
      y_repeat == EFillRepeat::kRepeatFill)
    return CSSIdentifierValue::Create(CSSValueRepeatY);

  CSSValueList* list = CSSValueList::CreateSpaceSeparated();
  list->Append(*CSSIdentifierValue::Create(x_repeat));
  list->Append(*CSSIdentifierValue::Create(y_repeat));
  return list;
}

const CSSValueList* ComputedStyleUtils::ValuesForBackgroundShorthand(
    const ComputedStyle& style,
    const LayoutObject* layout_object,
    Node* styled_node,
    bool allow_visited_style) {
  CSSValueList* result = CSSValueList::CreateCommaSeparated();
  const FillLayer* curr_layer = &style.BackgroundLayers();
  for (; curr_layer; curr_layer = curr_layer->Next()) {
    CSSValueList* list = CSSValueList::CreateSlashSeparated();
    CSSValueList* before_slash = CSSValueList::CreateSpaceSeparated();
    if (!curr_layer->Next()) {  // color only for final layer
      const CSSValue* value =
          GetCSSPropertyBackgroundColor().CSSValueFromComputedStyle(
              style, layout_object, styled_node, allow_visited_style);
      DCHECK(value);
      before_slash->Append(*value);
    }
    before_slash->Append(curr_layer->GetImage()
                             ? *curr_layer->GetImage()->ComputedCSSValue()
                             : *CSSIdentifierValue::Create(CSSValueNone));
    before_slash->Append(
        *ValueForFillRepeat(curr_layer->RepeatX(), curr_layer->RepeatY()));
    before_slash->Append(*CSSIdentifierValue::Create(curr_layer->Attachment()));
    before_slash->Append(*CreatePositionListForLayer(
        GetCSSPropertyBackgroundPosition(), *curr_layer, style));
    list->Append(*before_slash);
    CSSValueList* after_slash = CSSValueList::CreateSpaceSeparated();
    after_slash->Append(*ValueForFillSize(curr_layer->Size(), style));
    after_slash->Append(*CSSIdentifierValue::Create(curr_layer->Origin()));
    after_slash->Append(*CSSIdentifierValue::Create(curr_layer->Clip()));
    list->Append(*after_slash);
    result->Append(*list);
  }
  return result;
}

const CSSValue* ComputedStyleUtils::BackgroundRepeatOrWebkitMaskRepeat(
    const FillLayer* curr_layer) {
  CSSValueList* list = CSSValueList::CreateCommaSeparated();
  for (; curr_layer; curr_layer = curr_layer->Next()) {
    list->Append(
        *ValueForFillRepeat(curr_layer->RepeatX(), curr_layer->RepeatY()));
  }
  return list;
}

const CSSValue* ComputedStyleUtils::BackgroundPositionOrWebkitMaskPosition(
    const CSSProperty& resolved_property,
    const ComputedStyle& style,
    const FillLayer* curr_layer) {
  CSSValueList* list = CSSValueList::CreateCommaSeparated();
  for (; curr_layer; curr_layer = curr_layer->Next()) {
    list->Append(
        *CreatePositionListForLayer(resolved_property, *curr_layer, style));
  }
  return list;
}

const CSSValue* ComputedStyleUtils::BackgroundPositionXOrWebkitMaskPositionX(
    const ComputedStyle& style,
    const FillLayer* curr_layer) {
  CSSValueList* list = CSSValueList::CreateCommaSeparated();
  for (; curr_layer; curr_layer = curr_layer->Next()) {
    list->Append(
        *ZoomAdjustedPixelValueForLength(curr_layer->PositionX(), style));
  }
  return list;
}

const CSSValue* ComputedStyleUtils::BackgroundPositionYOrWebkitMaskPositionY(
    const ComputedStyle& style,
    const FillLayer* curr_layer) {
  CSSValueList* list = CSSValueList::CreateCommaSeparated();
  for (; curr_layer; curr_layer = curr_layer->Next()) {
    list->Append(
        *ZoomAdjustedPixelValueForLength(curr_layer->PositionY(), style));
  }
  return list;
}

cssvalue::CSSBorderImageSliceValue*
ComputedStyleUtils::ValueForNinePieceImageSlice(const NinePieceImage& image) {
  // Create the slices.
  CSSPrimitiveValue* top = nullptr;
  CSSPrimitiveValue* right = nullptr;
  CSSPrimitiveValue* bottom = nullptr;
  CSSPrimitiveValue* left = nullptr;

  // TODO(alancutter): Make this code aware of calc lengths.
  if (image.ImageSlices().Top().IsPercentOrCalc()) {
    top = CSSPrimitiveValue::Create(image.ImageSlices().Top().Value(),
                                    CSSPrimitiveValue::UnitType::kPercentage);
  } else {
    top = CSSPrimitiveValue::Create(image.ImageSlices().Top().Value(),
                                    CSSPrimitiveValue::UnitType::kNumber);
  }

  if (image.ImageSlices().Right() == image.ImageSlices().Top() &&
      image.ImageSlices().Bottom() == image.ImageSlices().Top() &&
      image.ImageSlices().Left() == image.ImageSlices().Top()) {
    right = top;
    bottom = top;
    left = top;
  } else {
    if (image.ImageSlices().Right().IsPercentOrCalc()) {
      right =
          CSSPrimitiveValue::Create(image.ImageSlices().Right().Value(),
                                    CSSPrimitiveValue::UnitType::kPercentage);
    } else {
      right = CSSPrimitiveValue::Create(image.ImageSlices().Right().Value(),
                                        CSSPrimitiveValue::UnitType::kNumber);
    }

    if (image.ImageSlices().Bottom() == image.ImageSlices().Top() &&
        image.ImageSlices().Right() == image.ImageSlices().Left()) {
      bottom = top;
      left = right;
    } else {
      if (image.ImageSlices().Bottom().IsPercentOrCalc()) {
        bottom =
            CSSPrimitiveValue::Create(image.ImageSlices().Bottom().Value(),
                                      CSSPrimitiveValue::UnitType::kPercentage);
      } else {
        bottom =
            CSSPrimitiveValue::Create(image.ImageSlices().Bottom().Value(),
                                      CSSPrimitiveValue::UnitType::kNumber);
      }

      if (image.ImageSlices().Left() == image.ImageSlices().Right()) {
        left = right;
      } else {
        if (image.ImageSlices().Left().IsPercentOrCalc()) {
          left = CSSPrimitiveValue::Create(
              image.ImageSlices().Left().Value(),
              CSSPrimitiveValue::UnitType::kPercentage);
        } else {
          left =
              CSSPrimitiveValue::Create(image.ImageSlices().Left().Value(),
                                        CSSPrimitiveValue::UnitType::kNumber);
        }
      }
    }
  }

  return CSSBorderImageSliceValue::Create(
      CSSQuadValue::Create(top, right, bottom, left,
                           CSSQuadValue::kSerializeAsQuad),
      image.Fill());
}

CSSValue* ValueForBorderImageLength(
    const BorderImageLength& border_image_length,
    const ComputedStyle& style) {
  if (border_image_length.IsNumber()) {
    return CSSPrimitiveValue::Create(border_image_length.Number(),
                                     CSSPrimitiveValue::UnitType::kNumber);
  }
  return CSSValue::Create(border_image_length.length(), style.EffectiveZoom());
}

CSSQuadValue* ComputedStyleUtils::ValueForNinePieceImageQuad(
    const BorderImageLengthBox& box,
    const ComputedStyle& style) {
  // Create the slices.
  CSSValue* top = nullptr;
  CSSValue* right = nullptr;
  CSSValue* bottom = nullptr;
  CSSValue* left = nullptr;

  top = ValueForBorderImageLength(box.Top(), style);

  if (box.Right() == box.Top() && box.Bottom() == box.Top() &&
      box.Left() == box.Top()) {
    right = top;
    bottom = top;
    left = top;
  } else {
    right = ValueForBorderImageLength(box.Right(), style);

    if (box.Bottom() == box.Top() && box.Right() == box.Left()) {
      bottom = top;
      left = right;
    } else {
      bottom = ValueForBorderImageLength(box.Bottom(), style);

      if (box.Left() == box.Right())
        left = right;
      else
        left = ValueForBorderImageLength(box.Left(), style);
    }
  }
  return CSSQuadValue::Create(top, right, bottom, left,
                              CSSQuadValue::kSerializeAsQuad);
}

CSSValueID ValueForRepeatRule(int rule) {
  switch (rule) {
    case kRepeatImageRule:
      return CSSValueRepeat;
    case kRoundImageRule:
      return CSSValueRound;
    case kSpaceImageRule:
      return CSSValueSpace;
    default:
      return CSSValueStretch;
  }
}

CSSValue* ComputedStyleUtils::ValueForNinePieceImageRepeat(
    const NinePieceImage& image) {
  CSSIdentifierValue* horizontal_repeat = nullptr;
  CSSIdentifierValue* vertical_repeat = nullptr;

  horizontal_repeat =
      CSSIdentifierValue::Create(ValueForRepeatRule(image.HorizontalRule()));
  if (image.HorizontalRule() == image.VerticalRule()) {
    vertical_repeat = horizontal_repeat;
  } else {
    vertical_repeat =
        CSSIdentifierValue::Create(ValueForRepeatRule(image.VerticalRule()));
  }
  return CSSValuePair::Create(horizontal_repeat, vertical_repeat,
                              CSSValuePair::kDropIdenticalValues);
}

CSSValue* ComputedStyleUtils::ValueForNinePieceImage(
    const NinePieceImage& image,
    const ComputedStyle& style) {
  if (!image.HasImage())
    return CSSIdentifierValue::Create(CSSValueNone);

  // Image first.
  CSSValue* image_value = nullptr;
  if (image.GetImage())
    image_value = image.GetImage()->ComputedCSSValue();

  // Create the image slice.
  cssvalue::CSSBorderImageSliceValue* image_slices =
      ValueForNinePieceImageSlice(image);

  // Create the border area slices.
  CSSValue* border_slices =
      ValueForNinePieceImageQuad(image.BorderSlices(), style);

  // Create the border outset.
  CSSValue* outset = ValueForNinePieceImageQuad(image.Outset(), style);

  // Create the repeat rules.
  CSSValue* repeat = ValueForNinePieceImageRepeat(image);

  return CreateBorderImageValue(image_value, image_slices, border_slices,
                                outset, repeat);
}

CSSValue* ComputedStyleUtils::ValueForReflection(
    const StyleReflection* reflection,
    const ComputedStyle& style) {
  if (!reflection)
    return CSSIdentifierValue::Create(CSSValueNone);

  CSSPrimitiveValue* offset = nullptr;
  // TODO(alancutter): Make this work correctly for calc lengths.
  if (reflection->Offset().IsPercentOrCalc()) {
    offset =
        CSSPrimitiveValue::Create(reflection->Offset().Percent(),
                                  CSSPrimitiveValue::UnitType::kPercentage);
  } else {
    offset = ZoomAdjustedPixelValue(reflection->Offset().Value(), style);
  }

  CSSIdentifierValue* direction = nullptr;
  switch (reflection->Direction()) {
    case kReflectionBelow:
      direction = CSSIdentifierValue::Create(CSSValueBelow);
      break;
    case kReflectionAbove:
      direction = CSSIdentifierValue::Create(CSSValueAbove);
      break;
    case kReflectionLeft:
      direction = CSSIdentifierValue::Create(CSSValueLeft);
      break;
    case kReflectionRight:
      direction = CSSIdentifierValue::Create(CSSValueRight);
      break;
  }

  return CSSReflectValue::Create(
      direction, offset, ValueForNinePieceImage(reflection->Mask(), style));
}

CSSValue* ComputedStyleUtils::MinWidthOrMinHeightAuto(
    Node* styled_node,
    const ComputedStyle& style) {
  Node* parent = styled_node->parentNode();
  const ComputedStyle* ensured_style =
      parent ? parent->EnsureComputedStyle() : nullptr;
  if (ensured_style && ensured_style->IsDisplayFlexibleOrGridBox())
    return CSSIdentifierValue::Create(CSSValueAuto);
  return ZoomAdjustedPixelValue(0, style);
}

CSSValue* ComputedStyleUtils::ValueForPositionOffset(
    const ComputedStyle& style,
    const CSSProperty& property,
    const LayoutObject* layout_object) {
  std::pair<const Length*, const Length*> positions;
  switch (property.PropertyID()) {
    case CSSPropertyLeft:
      positions = std::make_pair(&style.Left(), &style.Right());
      break;
    case CSSPropertyRight:
      positions = std::make_pair(&style.Right(), &style.Left());
      break;
    case CSSPropertyTop:
      positions = std::make_pair(&style.Top(), &style.Bottom());
      break;
    case CSSPropertyBottom:
      positions = std::make_pair(&style.Bottom(), &style.Top());
      break;
    default:
      NOTREACHED();
      return nullptr;
  }
  DCHECK(positions.first && positions.second);

  const Length& offset = *positions.first;
  const Length& opposite = *positions.second;

  if (offset.IsPercentOrCalc() && layout_object && layout_object->IsBox() &&
      layout_object->IsPositioned()) {
    LayoutUnit containing_block_size =
        (property.IDEquals(CSSPropertyLeft) ||
         property.IDEquals(CSSPropertyRight))
            ? ToLayoutBox(layout_object)
                  ->ContainingBlockLogicalWidthForContent()
            : ToLayoutBox(layout_object)
                  ->ContainingBlockLogicalHeightForGetComputedStyle();
    return ZoomAdjustedPixelValue(ValueForLength(offset, containing_block_size),
                                  style);
  }

  if (offset.IsAuto() && layout_object) {
    // If the property applies to a positioned element and the resolved value of
    // the display property is not none, the resolved value is the used value.
    // Position offsets have special meaning for position sticky so we return
    // auto when offset.isAuto() on a sticky position object:
    // https://crbug.com/703816.
    if (layout_object->IsRelPositioned()) {
      // If e.g. left is auto and right is not auto, then left's computed value
      // is negative right. So we get the opposite length unit and see if it is
      // auto.
      if (opposite.IsAuto()) {
        return CSSPrimitiveValue::Create(0,
                                         CSSPrimitiveValue::UnitType::kPixels);
      }

      if (opposite.IsPercentOrCalc()) {
        if (layout_object->IsBox()) {
          LayoutUnit containing_block_size =
              (property.IDEquals(CSSPropertyLeft) ||
               property.IDEquals(CSSPropertyRight))
                  ? ToLayoutBox(layout_object)
                        ->ContainingBlockLogicalWidthForContent()
                  : ToLayoutBox(layout_object)
                        ->ContainingBlockLogicalHeightForGetComputedStyle();
          return ZoomAdjustedPixelValue(
              -FloatValueForLength(opposite, containing_block_size), style);
        }
        // FIXME:  fall back to auto for position:relative, display:inline
        return CSSIdentifierValue::Create(CSSValueAuto);
      }

      // Length doesn't provide operator -, so multiply by -1.
      Length negated_opposite = opposite;
      negated_opposite *= -1.f;
      return ComputedStyleUtils::ZoomAdjustedPixelValueForLength(
          negated_opposite, style);
    }

    if (layout_object->IsOutOfFlowPositioned() && layout_object->IsBox()) {
      // For fixed and absolute positioned elements, the top, left, bottom, and
      // right are defined relative to the corresponding sides of the containing
      // block.
      LayoutBlock* container = layout_object->ContainingBlock();
      const LayoutBox* layout_box = ToLayoutBox(layout_object);

      // clientOffset is the distance from this object's border edge to the
      // container's padding edge. Thus it includes margins which we subtract
      // below.
      const LayoutSize client_offset =
          layout_box->LocationOffset() -
          LayoutSize(container->ClientLeft(), container->ClientTop());
      LayoutUnit position;

      switch (property.PropertyID()) {
        case CSSPropertyLeft:
          position = client_offset.Width() - layout_box->MarginLeft();
          break;
        case CSSPropertyTop:
          position = client_offset.Height() - layout_box->MarginTop();
          break;
        case CSSPropertyRight:
          position = container->ClientWidth() - layout_box->MarginRight() -
                     (layout_box->OffsetWidth() + client_offset.Width());
          break;
        case CSSPropertyBottom:
          position = container->ClientHeight() - layout_box->MarginBottom() -
                     (layout_box->OffsetHeight() + client_offset.Height());
          break;
        default:
          NOTREACHED();
      }
      return ZoomAdjustedPixelValue(position, style);
    }
  }

  if (offset.IsAuto())
    return CSSIdentifierValue::Create(CSSValueAuto);

  return ComputedStyleUtils::ZoomAdjustedPixelValueForLength(offset, style);
}

CSSValueList* ComputedStyleUtils::ValueForItemPositionWithOverflowAlignment(
    const StyleSelfAlignmentData& data) {
  CSSValueList* result = CSSValueList::CreateSpaceSeparated();
  if (data.PositionType() == ItemPositionType::kLegacy)
    result->Append(*CSSIdentifierValue::Create(CSSValueLegacy));
  if (data.GetPosition() == ItemPosition::kBaseline) {
    result->Append(
        *CSSValuePair::Create(CSSIdentifierValue::Create(CSSValueBaseline),
                              CSSIdentifierValue::Create(CSSValueBaseline),
                              CSSValuePair::kDropIdenticalValues));
  } else if (data.GetPosition() == ItemPosition::kLastBaseline) {
    result->Append(
        *CSSValuePair::Create(CSSIdentifierValue::Create(CSSValueLast),
                              CSSIdentifierValue::Create(CSSValueBaseline),
                              CSSValuePair::kDropIdenticalValues));
  } else {
    result->Append(*CSSIdentifierValue::Create(data.GetPosition()));
  }
  if (data.GetPosition() >= ItemPosition::kCenter &&
      data.Overflow() != OverflowAlignment::kDefault)
    result->Append(*CSSIdentifierValue::Create(data.Overflow()));
  DCHECK_LE(result->length(), 2u);
  return result;
}

CSSValueList*
ComputedStyleUtils::ValueForContentPositionAndDistributionWithOverflowAlignment(
    const StyleContentAlignmentData& data) {
  CSSValueList* result = CSSValueList::CreateSpaceSeparated();
  // Handle content-distribution values
  if (data.Distribution() != ContentDistributionType::kDefault)
    result->Append(*CSSIdentifierValue::Create(data.Distribution()));

  // Handle content-position values (either as fallback or actual value)
  switch (data.GetPosition()) {
    case ContentPosition::kNormal:
      // Handle 'normal' value, not valid as content-distribution fallback.
      if (data.Distribution() == ContentDistributionType::kDefault) {
        result->Append(*CSSIdentifierValue::Create(CSSValueNormal));
      }
      break;
    case ContentPosition::kLastBaseline:
      result->Append(
          *CSSValuePair::Create(CSSIdentifierValue::Create(CSSValueLast),
                                CSSIdentifierValue::Create(CSSValueBaseline),
                                CSSValuePair::kDropIdenticalValues));
      break;
    default:
      result->Append(*CSSIdentifierValue::Create(data.GetPosition()));
  }

  // Handle overflow-alignment (only allowed for content-position values)
  if ((data.GetPosition() >= ContentPosition::kCenter ||
       data.Distribution() != ContentDistributionType::kDefault) &&
      data.Overflow() != OverflowAlignment::kDefault)
    result->Append(*CSSIdentifierValue::Create(data.Overflow()));
  DCHECK_GT(result->length(), 0u);
  DCHECK_LE(result->length(), 3u);
  return result;
}

CSSValue* ComputedStyleUtils::ValueForLineHeight(const ComputedStyle& style) {
  const Length& length = style.LineHeight();
  if (length.IsNegative())
    return CSSIdentifierValue::Create(CSSValueNormal);

  return ZoomAdjustedPixelValue(
      FloatValueForLength(length, style.GetFontDescription().ComputedSize()),
      style);
}

CSSValue* ComputedStyleUtils::ValueForPosition(const LengthPoint& position,
                                               const ComputedStyle& style) {
  DCHECK_EQ(position.X().IsAuto(), position.Y().IsAuto());
  if (position.X().IsAuto())
    return CSSIdentifierValue::Create(CSSValueAuto);

  CSSValueList* list = CSSValueList::CreateSpaceSeparated();
  list->Append(*ComputedStyleUtils::ZoomAdjustedPixelValueForLength(
      position.X(), style));
  list->Append(*ComputedStyleUtils::ZoomAdjustedPixelValueForLength(
      position.Y(), style));
  return list;
}

CSSValueID IdentifierForFamily(const AtomicString& family) {
  if (family == FontFamilyNames::webkit_cursive)
    return CSSValueCursive;
  if (family == FontFamilyNames::webkit_fantasy)
    return CSSValueFantasy;
  if (family == FontFamilyNames::webkit_monospace)
    return CSSValueMonospace;
  if (family == FontFamilyNames::webkit_pictograph)
    return CSSValueWebkitPictograph;
  if (family == FontFamilyNames::webkit_sans_serif)
    return CSSValueSansSerif;
  if (family == FontFamilyNames::webkit_serif)
    return CSSValueSerif;
  return CSSValueInvalid;
}

CSSValue* ValueForFamily(const AtomicString& family) {
  if (CSSValueID family_identifier = IdentifierForFamily(family))
    return CSSIdentifierValue::Create(family_identifier);
  return CSSFontFamilyValue::Create(family.GetString());
}

CSSValueList* ComputedStyleUtils::ValueForFontFamily(
    const ComputedStyle& style) {
  const FontFamily& first_family = style.GetFontDescription().Family();
  CSSValueList* list = CSSValueList::CreateCommaSeparated();
  for (const FontFamily* family = &first_family; family;
       family = family->Next())
    list->Append(*ValueForFamily(family->Family()));
  return list;
}

CSSPrimitiveValue* ComputedStyleUtils::ValueForFontSize(
    const ComputedStyle& style) {
  return ZoomAdjustedPixelValue(style.GetFontDescription().ComputedSize(),
                                style);
}

CSSPrimitiveValue* ComputedStyleUtils::ValueForFontStretch(
    const ComputedStyle& style) {
  return CSSPrimitiveValue::Create(style.GetFontDescription().Stretch(),
                                   CSSPrimitiveValue::UnitType::kPercentage);
}

CSSIdentifierValue* ComputedStyleUtils::ValueForFontStyle(
    const ComputedStyle& style) {
  FontSelectionValue angle = style.GetFontDescription().Style();
  if (angle == NormalSlopeValue()) {
    return CSSIdentifierValue::Create(CSSValueNormal);
  }

  if (angle == ItalicSlopeValue()) {
    return CSSIdentifierValue::Create(CSSValueItalic);
  }

  NOTREACHED();
  return CSSIdentifierValue::Create(CSSValueNormal);
}

CSSPrimitiveValue* ComputedStyleUtils::ValueForFontWeight(
    const ComputedStyle& style) {
  return CSSPrimitiveValue::Create(style.GetFontDescription().Weight(),
                                   CSSPrimitiveValue::UnitType::kNumber);
}

CSSIdentifierValue* ComputedStyleUtils::ValueForFontVariantCaps(
    const ComputedStyle& style) {
  FontDescription::FontVariantCaps variant_caps =
      style.GetFontDescription().VariantCaps();
  switch (variant_caps) {
    case FontDescription::kCapsNormal:
      return CSSIdentifierValue::Create(CSSValueNormal);
    case FontDescription::kSmallCaps:
      return CSSIdentifierValue::Create(CSSValueSmallCaps);
    case FontDescription::kAllSmallCaps:
      return CSSIdentifierValue::Create(CSSValueAllSmallCaps);
    case FontDescription::kPetiteCaps:
      return CSSIdentifierValue::Create(CSSValuePetiteCaps);
    case FontDescription::kAllPetiteCaps:
      return CSSIdentifierValue::Create(CSSValueAllPetiteCaps);
    case FontDescription::kUnicase:
      return CSSIdentifierValue::Create(CSSValueUnicase);
    case FontDescription::kTitlingCaps:
      return CSSIdentifierValue::Create(CSSValueTitlingCaps);
    default:
      NOTREACHED();
      return nullptr;
  }
}

CSSValue* ComputedStyleUtils::ValueForFontVariantLigatures(
    const ComputedStyle& style) {
  FontDescription::LigaturesState common_ligatures_state =
      style.GetFontDescription().CommonLigaturesState();
  FontDescription::LigaturesState discretionary_ligatures_state =
      style.GetFontDescription().DiscretionaryLigaturesState();
  FontDescription::LigaturesState historical_ligatures_state =
      style.GetFontDescription().HistoricalLigaturesState();
  FontDescription::LigaturesState contextual_ligatures_state =
      style.GetFontDescription().ContextualLigaturesState();
  if (common_ligatures_state == FontDescription::kNormalLigaturesState &&
      discretionary_ligatures_state == FontDescription::kNormalLigaturesState &&
      historical_ligatures_state == FontDescription::kNormalLigaturesState &&
      contextual_ligatures_state == FontDescription::kNormalLigaturesState)
    return CSSIdentifierValue::Create(CSSValueNormal);

  if (common_ligatures_state == FontDescription::kDisabledLigaturesState &&
      discretionary_ligatures_state ==
          FontDescription::kDisabledLigaturesState &&
      historical_ligatures_state == FontDescription::kDisabledLigaturesState &&
      contextual_ligatures_state == FontDescription::kDisabledLigaturesState)
    return CSSIdentifierValue::Create(CSSValueNone);

  CSSValueList* value_list = CSSValueList::CreateSpaceSeparated();
  if (common_ligatures_state != FontDescription::kNormalLigaturesState) {
    value_list->Append(*CSSIdentifierValue::Create(
        common_ligatures_state == FontDescription::kDisabledLigaturesState
            ? CSSValueNoCommonLigatures
            : CSSValueCommonLigatures));
  }
  if (discretionary_ligatures_state != FontDescription::kNormalLigaturesState) {
    value_list->Append(*CSSIdentifierValue::Create(
        discretionary_ligatures_state ==
                FontDescription::kDisabledLigaturesState
            ? CSSValueNoDiscretionaryLigatures
            : CSSValueDiscretionaryLigatures));
  }
  if (historical_ligatures_state != FontDescription::kNormalLigaturesState) {
    value_list->Append(*CSSIdentifierValue::Create(
        historical_ligatures_state == FontDescription::kDisabledLigaturesState
            ? CSSValueNoHistoricalLigatures
            : CSSValueHistoricalLigatures));
  }
  if (contextual_ligatures_state != FontDescription::kNormalLigaturesState) {
    value_list->Append(*CSSIdentifierValue::Create(
        contextual_ligatures_state == FontDescription::kDisabledLigaturesState
            ? CSSValueNoContextual
            : CSSValueContextual));
  }
  return value_list;
}

CSSValue* ComputedStyleUtils::ValueForFontVariantNumeric(
    const ComputedStyle& style) {
  FontVariantNumeric variant_numeric =
      style.GetFontDescription().VariantNumeric();
  if (variant_numeric.IsAllNormal())
    return CSSIdentifierValue::Create(CSSValueNormal);

  CSSValueList* value_list = CSSValueList::CreateSpaceSeparated();
  if (variant_numeric.NumericFigureValue() !=
      FontVariantNumeric::kNormalFigure) {
    value_list->Append(*CSSIdentifierValue::Create(
        variant_numeric.NumericFigureValue() == FontVariantNumeric::kLiningNums
            ? CSSValueLiningNums
            : CSSValueOldstyleNums));
  }
  if (variant_numeric.NumericSpacingValue() !=
      FontVariantNumeric::kNormalSpacing) {
    value_list->Append(*CSSIdentifierValue::Create(
        variant_numeric.NumericSpacingValue() ==
                FontVariantNumeric::kProportionalNums
            ? CSSValueProportionalNums
            : CSSValueTabularNums));
  }
  if (variant_numeric.NumericFractionValue() !=
      FontVariantNumeric::kNormalFraction) {
    value_list->Append(*CSSIdentifierValue::Create(
        variant_numeric.NumericFractionValue() ==
                FontVariantNumeric::kDiagonalFractions
            ? CSSValueDiagonalFractions
            : CSSValueStackedFractions));
  }
  if (variant_numeric.OrdinalValue() == FontVariantNumeric::kOrdinalOn)
    value_list->Append(*CSSIdentifierValue::Create(CSSValueOrdinal));
  if (variant_numeric.SlashedZeroValue() == FontVariantNumeric::kSlashedZeroOn)
    value_list->Append(*CSSIdentifierValue::Create(CSSValueSlashedZero));

  return value_list;
}

CSSIdentifierValue* ValueForFontStretchAsKeyword(const ComputedStyle& style) {
  FontSelectionValue stretch_value = style.GetFontDescription().Stretch();
  CSSValueID value_id = CSSValueInvalid;
  if (stretch_value == UltraCondensedWidthValue())
    value_id = CSSValueUltraCondensed;
  if (stretch_value == UltraCondensedWidthValue())
    value_id = CSSValueUltraCondensed;
  if (stretch_value == ExtraCondensedWidthValue())
    value_id = CSSValueExtraCondensed;
  if (stretch_value == CondensedWidthValue())
    value_id = CSSValueCondensed;
  if (stretch_value == SemiCondensedWidthValue())
    value_id = CSSValueSemiCondensed;
  if (stretch_value == NormalWidthValue())
    value_id = CSSValueNormal;
  if (stretch_value == SemiExpandedWidthValue())
    value_id = CSSValueSemiExpanded;
  if (stretch_value == ExpandedWidthValue())
    value_id = CSSValueExpanded;
  if (stretch_value == ExtraExpandedWidthValue())
    value_id = CSSValueExtraExpanded;
  if (stretch_value == UltraExpandedWidthValue())
    value_id = CSSValueUltraExpanded;

  if (value_id != CSSValueInvalid)
    return CSSIdentifierValue::Create(value_id);
  return nullptr;
}

CSSValue* ComputedStyleUtils::ValueForFontVariantEastAsian(
    const ComputedStyle& style) {
  FontVariantEastAsian east_asian =
      style.GetFontDescription().VariantEastAsian();
  if (east_asian.IsAllNormal())
    return CSSIdentifierValue::Create(CSSValueNormal);

  CSSValueList* value_list = CSSValueList::CreateSpaceSeparated();
  switch (east_asian.Form()) {
    case FontVariantEastAsian::kNormalForm:
      break;
    case FontVariantEastAsian::kJis78:
      value_list->Append(*CSSIdentifierValue::Create(CSSValueJis78));
      break;
    case FontVariantEastAsian::kJis83:
      value_list->Append(*CSSIdentifierValue::Create(CSSValueJis83));
      break;
    case FontVariantEastAsian::kJis90:
      value_list->Append(*CSSIdentifierValue::Create(CSSValueJis90));
      break;
    case FontVariantEastAsian::kJis04:
      value_list->Append(*CSSIdentifierValue::Create(CSSValueJis04));
      break;
    case FontVariantEastAsian::kSimplified:
      value_list->Append(*CSSIdentifierValue::Create(CSSValueSimplified));
      break;
    case FontVariantEastAsian::kTraditional:
      value_list->Append(*CSSIdentifierValue::Create(CSSValueTraditional));
      break;
    default:
      NOTREACHED();
  }
  switch (east_asian.Width()) {
    case FontVariantEastAsian::kNormalWidth:
      break;
    case FontVariantEastAsian::kFullWidth:
      value_list->Append(*CSSIdentifierValue::Create(CSSValueFullWidth));
      break;
    case FontVariantEastAsian::kProportionalWidth:
      value_list->Append(
          *CSSIdentifierValue::Create(CSSValueProportionalWidth));
      break;
    default:
      NOTREACHED();
  }
  if (east_asian.Ruby())
    value_list->Append(*CSSIdentifierValue::Create(CSSValueRuby));
  return value_list;
}

CSSValue* ComputedStyleUtils::ValueForFont(const ComputedStyle& style) {
  // Add a slash between size and line-height.
  CSSValueList* size_and_line_height = CSSValueList::CreateSlashSeparated();
  size_and_line_height->Append(*ComputedStyleUtils::ValueForFontSize(style));
  size_and_line_height->Append(*ComputedStyleUtils::ValueForLineHeight(style));

  CSSValueList* list = CSSValueList::CreateSpaceSeparated();
  list->Append(*ComputedStyleUtils::ValueForFontStyle(style));

  // Check that non-initial font-variant subproperties are not conflicting with
  // this serialization.
  CSSValue* ligatures_value =
      ComputedStyleUtils::ValueForFontVariantLigatures(style);
  CSSValue* numeric_value =
      ComputedStyleUtils::ValueForFontVariantNumeric(style);
  CSSValue* east_asian_value = ValueForFontVariantEastAsian(style);
  // FIXME: Use DataEquivalent<CSSValue>(...) once http://crbug.com/729447 is
  // resolved.
  if (!DataEquivalent(
          ligatures_value,
          static_cast<CSSValue*>(CSSIdentifierValue::Create(CSSValueNormal))) ||
      !DataEquivalent(
          numeric_value,
          static_cast<CSSValue*>(CSSIdentifierValue::Create(CSSValueNormal))) ||
      !DataEquivalent(
          east_asian_value,
          static_cast<CSSValue*>(CSSIdentifierValue::Create(CSSValueNormal))))
    return nullptr;

  if (!ValueForFontStretchAsKeyword(style))
    return nullptr;

  CSSIdentifierValue* caps_value =
      ComputedStyleUtils::ValueForFontVariantCaps(style);
  if (caps_value->GetValueID() != CSSValueNormal &&
      caps_value->GetValueID() != CSSValueSmallCaps)
    return nullptr;
  list->Append(*caps_value);

  list->Append(*ComputedStyleUtils::ValueForFontWeight(style));
  list->Append(*ValueForFontStretchAsKeyword(style));
  list->Append(*size_and_line_height);
  list->Append(*ComputedStyleUtils::ValueForFontFamily(style));

  return list;
}

CSSValue* SpecifiedValueForGridTrackBreadth(const GridLength& track_breadth,
                                            const ComputedStyle& style) {
  if (!track_breadth.IsLength()) {
    return CSSPrimitiveValue::Create(track_breadth.Flex(),
                                     CSSPrimitiveValue::UnitType::kFraction);
  }

  const Length& track_breadth_length = track_breadth.length();
  if (track_breadth_length.IsAuto())
    return CSSIdentifierValue::Create(CSSValueAuto);
  return ComputedStyleUtils::ZoomAdjustedPixelValueForLength(
      track_breadth_length, style);
}

CSSValue* ComputedStyleUtils::SpecifiedValueForGridTrackSize(
    const GridTrackSize& track_size,
    const ComputedStyle& style) {
  switch (track_size.GetType()) {
    case kLengthTrackSizing:
      return SpecifiedValueForGridTrackBreadth(track_size.MinTrackBreadth(),
                                               style);
    case kMinMaxTrackSizing: {
      if (track_size.MinTrackBreadth().IsAuto() &&
          track_size.MaxTrackBreadth().IsFlex()) {
        return CSSPrimitiveValue::Create(
            track_size.MaxTrackBreadth().Flex(),
            CSSPrimitiveValue::UnitType::kFraction);
      }

      auto* min_max_track_breadths = CSSFunctionValue::Create(CSSValueMinmax);
      min_max_track_breadths->Append(*SpecifiedValueForGridTrackBreadth(
          track_size.MinTrackBreadth(), style));
      min_max_track_breadths->Append(*SpecifiedValueForGridTrackBreadth(
          track_size.MaxTrackBreadth(), style));
      return min_max_track_breadths;
    }
    case kFitContentTrackSizing: {
      auto* fit_content_track_breadth =
          CSSFunctionValue::Create(CSSValueFitContent);
      fit_content_track_breadth->Append(*SpecifiedValueForGridTrackBreadth(
          track_size.FitContentTrackBreadth(), style));
      return fit_content_track_breadth;
    }
  }
  NOTREACHED();
  return nullptr;
}

class OrderedNamedLinesCollector {
  STACK_ALLOCATED();

 public:
  OrderedNamedLinesCollector(const ComputedStyle& style,
                             bool is_row_axis,
                             size_t auto_repeat_tracks_count)
      : ordered_named_grid_lines_(is_row_axis
                                      ? style.OrderedNamedGridColumnLines()
                                      : style.OrderedNamedGridRowLines()),
        ordered_named_auto_repeat_grid_lines_(
            is_row_axis ? style.AutoRepeatOrderedNamedGridColumnLines()
                        : style.AutoRepeatOrderedNamedGridRowLines()),
        insertion_point_(is_row_axis
                             ? style.GridAutoRepeatColumnsInsertionPoint()
                             : style.GridAutoRepeatRowsInsertionPoint()),
        auto_repeat_total_tracks_(auto_repeat_tracks_count),
        auto_repeat_track_list_length_(
            is_row_axis ? style.GridAutoRepeatColumns().size()
                        : style.GridAutoRepeatRows().size()) {}

  bool IsEmpty() const {
    return ordered_named_grid_lines_.IsEmpty() &&
           ordered_named_auto_repeat_grid_lines_.IsEmpty();
  }
  void CollectLineNamesForIndex(CSSGridLineNamesValue&, size_t index) const;

 private:
  enum NamedLinesType { kNamedLines, kAutoRepeatNamedLines };
  void AppendLines(CSSGridLineNamesValue&, size_t index, NamedLinesType) const;

  const OrderedNamedGridLines& ordered_named_grid_lines_;
  const OrderedNamedGridLines& ordered_named_auto_repeat_grid_lines_;
  size_t insertion_point_;
  size_t auto_repeat_total_tracks_;
  size_t auto_repeat_track_list_length_;
  DISALLOW_COPY_AND_ASSIGN(OrderedNamedLinesCollector);
};

// RJW
void OrderedNamedLinesCollector::AppendLines(
    CSSGridLineNamesValue& line_names_value,
    size_t index,
    NamedLinesType type) const {
  auto iter = type == kNamedLines
                  ? ordered_named_grid_lines_.find(index)
                  : ordered_named_auto_repeat_grid_lines_.find(index);
  auto end_iter = type == kNamedLines
                      ? ordered_named_grid_lines_.end()
                      : ordered_named_auto_repeat_grid_lines_.end();
  if (iter == end_iter)
    return;

  for (auto line_name : iter->value) {
    line_names_value.Append(
        *CSSCustomIdentValue::Create(AtomicString(line_name)));
  }
}

// RJW
void OrderedNamedLinesCollector::CollectLineNamesForIndex(
    CSSGridLineNamesValue& line_names_value,
    size_t i) const {
  DCHECK(!IsEmpty());
  if (ordered_named_auto_repeat_grid_lines_.IsEmpty() || i < insertion_point_) {
    AppendLines(line_names_value, i, kNamedLines);
    return;
  }

  DCHECK(auto_repeat_total_tracks_);

  if (i > insertion_point_ + auto_repeat_total_tracks_) {
    AppendLines(line_names_value, i - (auto_repeat_total_tracks_ - 1),
                kNamedLines);
    return;
  }

  if (i == insertion_point_) {
    AppendLines(line_names_value, i, kNamedLines);
    AppendLines(line_names_value, 0, kAutoRepeatNamedLines);
    return;
  }

  if (i == insertion_point_ + auto_repeat_total_tracks_) {
    AppendLines(line_names_value, auto_repeat_track_list_length_,
                kAutoRepeatNamedLines);
    AppendLines(line_names_value, insertion_point_ + 1, kNamedLines);
    return;
  }

  size_t auto_repeat_index_in_first_repetition =
      (i - insertion_point_) % auto_repeat_track_list_length_;
  if (!auto_repeat_index_in_first_repetition && i > insertion_point_) {
    AppendLines(line_names_value, auto_repeat_track_list_length_,
                kAutoRepeatNamedLines);
  }
  AppendLines(line_names_value, auto_repeat_index_in_first_repetition,
              kAutoRepeatNamedLines);
}

void AddValuesForNamedGridLinesAtIndex(OrderedNamedLinesCollector& collector,
                                       size_t i,
                                       CSSValueList& list) {
  if (collector.IsEmpty())
    return;

  CSSGridLineNamesValue* line_names = CSSGridLineNamesValue::Create();
  collector.CollectLineNamesForIndex(*line_names, i);
  if (line_names->length())
    list.Append(*line_names);
}

CSSValue* ComputedStyleUtils::ValueForGridTrackSizeList(
    GridTrackSizingDirection direction,
    const ComputedStyle& style) {
  const Vector<GridTrackSize>& auto_track_sizes =
      direction == kForColumns ? style.GridAutoColumns() : style.GridAutoRows();

  CSSValueList* list = CSSValueList::CreateSpaceSeparated();
  for (auto& track_size : auto_track_sizes) {
    list->Append(
        *ComputedStyleUtils::SpecifiedValueForGridTrackSize(track_size, style));
  }
  return list;
}

CSSValue* ComputedStyleUtils::ValueForGridTrackList(
    GridTrackSizingDirection direction,
    const LayoutObject* layout_object,
    const ComputedStyle& style) {
  bool is_row_axis = direction == kForColumns;
  const Vector<GridTrackSize>& track_sizes =
      is_row_axis ? style.GridTemplateColumns() : style.GridTemplateRows();
  const Vector<GridTrackSize>& auto_repeat_track_sizes =
      is_row_axis ? style.GridAutoRepeatColumns() : style.GridAutoRepeatRows();
  bool is_layout_grid = layout_object && layout_object->IsLayoutGrid();

  // Handle the 'none' case.
  bool track_list_is_empty =
      track_sizes.IsEmpty() && auto_repeat_track_sizes.IsEmpty();
  if (is_layout_grid && track_list_is_empty) {
    // For grids we should consider every listed track, whether implicitly or
    // explicitly created. Empty grids have a sole grid line per axis.
    auto& positions = is_row_axis
                          ? ToLayoutGrid(layout_object)->ColumnPositions()
                          : ToLayoutGrid(layout_object)->RowPositions();
    track_list_is_empty = positions.size() == 1;
  }

  if (track_list_is_empty)
    return CSSIdentifierValue::Create(CSSValueNone);

  size_t auto_repeat_total_tracks =
      is_layout_grid
          ? ToLayoutGrid(layout_object)->AutoRepeatCountForDirection(direction)
          : 0;
  OrderedNamedLinesCollector collector(style, is_row_axis,
                                       auto_repeat_total_tracks);
  CSSValueList* list = CSSValueList::CreateSpaceSeparated();
  size_t insertion_index;
  if (is_layout_grid) {
    const auto* grid = ToLayoutGrid(layout_object);
    Vector<LayoutUnit> computed_track_sizes =
        grid->TrackSizesForComputedStyle(direction);
    size_t num_tracks = computed_track_sizes.size();

    for (size_t i = 0; i < num_tracks; ++i) {
      AddValuesForNamedGridLinesAtIndex(collector, i, *list);
      list->Append(*ZoomAdjustedPixelValue(computed_track_sizes[i], style));
    }
    AddValuesForNamedGridLinesAtIndex(collector, num_tracks + 1, *list);

    insertion_index = num_tracks;
  } else {
    for (size_t i = 0; i < track_sizes.size(); ++i) {
      AddValuesForNamedGridLinesAtIndex(collector, i, *list);
      list->Append(*ComputedStyleUtils::SpecifiedValueForGridTrackSize(
          track_sizes[i], style));
    }
    insertion_index = track_sizes.size();
  }
  // Those are the trailing <string>* allowed in the syntax.
  AddValuesForNamedGridLinesAtIndex(collector, insertion_index, *list);
  return list;
}

CSSValue* ComputedStyleUtils::ValueForGridPosition(
    const GridPosition& position) {
  if (position.IsAuto())
    return CSSIdentifierValue::Create(CSSValueAuto);

  if (position.IsNamedGridArea())
    return CSSCustomIdentValue::Create(position.NamedGridLine());

  CSSValueList* list = CSSValueList::CreateSpaceSeparated();
  if (position.IsSpan()) {
    list->Append(*CSSIdentifierValue::Create(CSSValueSpan));
    list->Append(*CSSPrimitiveValue::Create(
        position.SpanPosition(), CSSPrimitiveValue::UnitType::kNumber));
  } else {
    list->Append(*CSSPrimitiveValue::Create(
        position.IntegerPosition(), CSSPrimitiveValue::UnitType::kNumber));
  }

  if (!position.NamedGridLine().IsNull())
    list->Append(*CSSCustomIdentValue::Create(position.NamedGridLine()));
  return list;
}

LayoutRect ComputedStyleUtils::SizingBox(const LayoutObject& layout_object) {
  if (!layout_object.IsBox())
    return LayoutRect();

  const LayoutBox& box = ToLayoutBox(layout_object);
  return box.StyleRef().BoxSizing() == EBoxSizing::kBorderBox
             ? box.BorderBoxRect()
             : box.ComputedCSSContentBoxRect();
}

CSSValue* ComputedStyleUtils::RenderTextDecorationFlagsToCSSValue(
    TextDecoration text_decoration) {
  // Blink value is ignored.
  CSSValueList* list = CSSValueList::CreateSpaceSeparated();
  if (EnumHasFlags(text_decoration, TextDecoration::kUnderline))
    list->Append(*CSSIdentifierValue::Create(CSSValueUnderline));
  if (EnumHasFlags(text_decoration, TextDecoration::kOverline))
    list->Append(*CSSIdentifierValue::Create(CSSValueOverline));
  if (EnumHasFlags(text_decoration, TextDecoration::kLineThrough))
    list->Append(*CSSIdentifierValue::Create(CSSValueLineThrough));

  if (!list->length())
    return CSSIdentifierValue::Create(CSSValueNone);
  return list;
}

CSSValue* ComputedStyleUtils::ValueForTextDecorationStyle(
    ETextDecorationStyle text_decoration_style) {
  switch (text_decoration_style) {
    case ETextDecorationStyle::kSolid:
      return CSSIdentifierValue::Create(CSSValueSolid);
    case ETextDecorationStyle::kDouble:
      return CSSIdentifierValue::Create(CSSValueDouble);
    case ETextDecorationStyle::kDotted:
      return CSSIdentifierValue::Create(CSSValueDotted);
    case ETextDecorationStyle::kDashed:
      return CSSIdentifierValue::Create(CSSValueDashed);
    case ETextDecorationStyle::kWavy:
      return CSSIdentifierValue::Create(CSSValueWavy);
  }

  NOTREACHED();
  return CSSInitialValue::Create();
}

CSSValue* ComputedStyleUtils::ValueForTextDecorationSkipInk(
    ETextDecorationSkipInk text_decoration_skip_ink) {
  if (text_decoration_skip_ink == ETextDecorationSkipInk::kNone)
    return CSSIdentifierValue::Create(CSSValueNone);
  return CSSIdentifierValue::Create(CSSValueAuto);
}

CSSValue* ComputedStyleUtils::TouchActionFlagsToCSSValue(
    TouchAction touch_action) {
  CSSValueList* list = CSSValueList::CreateSpaceSeparated();
  if (touch_action == TouchAction::kTouchActionAuto) {
    list->Append(*CSSIdentifierValue::Create(CSSValueAuto));
  } else if (touch_action == TouchAction::kTouchActionNone) {
    list->Append(*CSSIdentifierValue::Create(CSSValueNone));
  } else if (touch_action == TouchAction::kTouchActionManipulation) {
    list->Append(*CSSIdentifierValue::Create(CSSValueManipulation));
  } else {
    if ((touch_action & TouchAction::kTouchActionPanX) ==
        TouchAction::kTouchActionPanX)
      list->Append(*CSSIdentifierValue::Create(CSSValuePanX));
    else if (touch_action & TouchAction::kTouchActionPanLeft)
      list->Append(*CSSIdentifierValue::Create(CSSValuePanLeft));
    else if (touch_action & TouchAction::kTouchActionPanRight)
      list->Append(*CSSIdentifierValue::Create(CSSValuePanRight));
    if ((touch_action & TouchAction::kTouchActionPanY) ==
        TouchAction::kTouchActionPanY)
      list->Append(*CSSIdentifierValue::Create(CSSValuePanY));
    else if (touch_action & TouchAction::kTouchActionPanUp)
      list->Append(*CSSIdentifierValue::Create(CSSValuePanUp));
    else if (touch_action & TouchAction::kTouchActionPanDown)
      list->Append(*CSSIdentifierValue::Create(CSSValuePanDown));

    if ((touch_action & TouchAction::kTouchActionPinchZoom) ==
        TouchAction::kTouchActionPinchZoom)
      list->Append(*CSSIdentifierValue::Create(CSSValuePinchZoom));
  }

  DCHECK(list->length());
  return list;
}

CSSValue* ComputedStyleUtils::ValueForWillChange(
    const Vector<CSSPropertyID>& will_change_properties,
    bool will_change_contents,
    bool will_change_scroll_position) {
  CSSValueList* list = CSSValueList::CreateCommaSeparated();
  if (will_change_contents)
    list->Append(*CSSIdentifierValue::Create(CSSValueContents));
  if (will_change_scroll_position)
    list->Append(*CSSIdentifierValue::Create(CSSValueScrollPosition));
  for (size_t i = 0; i < will_change_properties.size(); ++i)
    list->Append(*CSSCustomIdentValue::Create(will_change_properties[i]));
  if (!list->length())
    list->Append(*CSSIdentifierValue::Create(CSSValueAuto));
  return list;
}

CSSValue* ComputedStyleUtils::ValueForAnimationDelay(
    const CSSTimingData* timing_data) {
  CSSValueList* list = CSSValueList::CreateCommaSeparated();
  if (timing_data) {
    for (size_t i = 0; i < timing_data->DelayList().size(); ++i) {
      list->Append(*CSSPrimitiveValue::Create(
          timing_data->DelayList()[i], CSSPrimitiveValue::UnitType::kSeconds));
    }
  } else {
    list->Append(*CSSPrimitiveValue::Create(
        CSSTimingData::InitialDelay(), CSSPrimitiveValue::UnitType::kSeconds));
  }
  return list;
}

CSSValue* ComputedStyleUtils::ValueForAnimationDirection(
    Timing::PlaybackDirection direction) {
  switch (direction) {
    case Timing::PlaybackDirection::NORMAL:
      return CSSIdentifierValue::Create(CSSValueNormal);
    case Timing::PlaybackDirection::ALTERNATE_NORMAL:
      return CSSIdentifierValue::Create(CSSValueAlternate);
    case Timing::PlaybackDirection::REVERSE:
      return CSSIdentifierValue::Create(CSSValueReverse);
    case Timing::PlaybackDirection::ALTERNATE_REVERSE:
      return CSSIdentifierValue::Create(CSSValueAlternateReverse);
    default:
      NOTREACHED();
      return nullptr;
  }
}

CSSValue* ComputedStyleUtils::ValueForAnimationDuration(
    const CSSTimingData* timing_data) {
  CSSValueList* list = CSSValueList::CreateCommaSeparated();
  if (timing_data) {
    for (size_t i = 0; i < timing_data->DurationList().size(); ++i) {
      list->Append(
          *CSSPrimitiveValue::Create(timing_data->DurationList()[i],
                                     CSSPrimitiveValue::UnitType::kSeconds));
    }
  } else {
    list->Append(
        *CSSPrimitiveValue::Create(CSSTimingData::InitialDuration(),
                                   CSSPrimitiveValue::UnitType::kSeconds));
  }
  return list;
}

CSSValue* ComputedStyleUtils::ValueForAnimationFillMode(
    Timing::FillMode fill_mode) {
  switch (fill_mode) {
    case Timing::FillMode::NONE:
      return CSSIdentifierValue::Create(CSSValueNone);
    case Timing::FillMode::FORWARDS:
      return CSSIdentifierValue::Create(CSSValueForwards);
    case Timing::FillMode::BACKWARDS:
      return CSSIdentifierValue::Create(CSSValueBackwards);
    case Timing::FillMode::BOTH:
      return CSSIdentifierValue::Create(CSSValueBoth);
    default:
      NOTREACHED();
      return nullptr;
  }
}

CSSValue* ComputedStyleUtils::ValueForAnimationIterationCount(
    double iteration_count) {
  if (iteration_count == std::numeric_limits<double>::infinity())
    return CSSIdentifierValue::Create(CSSValueInfinite);
  return CSSPrimitiveValue::Create(iteration_count,
                                   CSSPrimitiveValue::UnitType::kNumber);
}

CSSValue* ComputedStyleUtils::ValueForAnimationPlayState(
    EAnimPlayState play_state) {
  if (play_state == EAnimPlayState::kPlaying)
    return CSSIdentifierValue::Create(CSSValueRunning);
  DCHECK_EQ(play_state, EAnimPlayState::kPaused);
  return CSSIdentifierValue::Create(CSSValuePaused);
}

CSSValue* ComputedStyleUtils::CreateTimingFunctionValue(
    const TimingFunction* timing_function) {
  switch (timing_function->GetType()) {
    case TimingFunction::Type::CUBIC_BEZIER: {
      const CubicBezierTimingFunction* bezier_timing_function =
          ToCubicBezierTimingFunction(timing_function);
      if (bezier_timing_function->GetEaseType() !=
          CubicBezierTimingFunction::EaseType::CUSTOM) {
        CSSValueID value_id = CSSValueInvalid;
        switch (bezier_timing_function->GetEaseType()) {
          case CubicBezierTimingFunction::EaseType::EASE:
            value_id = CSSValueEase;
            break;
          case CubicBezierTimingFunction::EaseType::EASE_IN:
            value_id = CSSValueEaseIn;
            break;
          case CubicBezierTimingFunction::EaseType::EASE_OUT:
            value_id = CSSValueEaseOut;
            break;
          case CubicBezierTimingFunction::EaseType::EASE_IN_OUT:
            value_id = CSSValueEaseInOut;
            break;
          default:
            NOTREACHED();
            return nullptr;
        }
        return CSSIdentifierValue::Create(value_id);
      }
      return CSSCubicBezierTimingFunctionValue::Create(
          bezier_timing_function->X1(), bezier_timing_function->Y1(),
          bezier_timing_function->X2(), bezier_timing_function->Y2());
    }

    case TimingFunction::Type::STEPS: {
      const StepsTimingFunction* steps_timing_function =
          ToStepsTimingFunction(timing_function);
      StepsTimingFunction::StepPosition position =
          steps_timing_function->GetStepPosition();
      int steps = steps_timing_function->NumberOfSteps();
      DCHECK(position == StepsTimingFunction::StepPosition::START ||
             position == StepsTimingFunction::StepPosition::END);

      if (steps > 1)
        return CSSStepsTimingFunctionValue::Create(steps, position);
      CSSValueID value_id = position == StepsTimingFunction::StepPosition::START
                                ? CSSValueStepStart
                                : CSSValueStepEnd;
      return CSSIdentifierValue::Create(value_id);
    }

    case TimingFunction::Type::FRAMES: {
      const FramesTimingFunction* frames_timing_function =
          ToFramesTimingFunction(timing_function);
      int frames = frames_timing_function->NumberOfFrames();
      return CSSFramesTimingFunctionValue::Create(frames);
    }

    default:
      return CSSIdentifierValue::Create(CSSValueLinear);
  }
}

CSSValue* ComputedStyleUtils::ValueForAnimationTimingFunction(
    const CSSTimingData* timing_data) {
  CSSValueList* list = CSSValueList::CreateCommaSeparated();
  if (timing_data) {
    for (size_t i = 0; i < timing_data->TimingFunctionList().size(); ++i) {
      list->Append(*ComputedStyleUtils::CreateTimingFunctionValue(
          timing_data->TimingFunctionList()[i].get()));
    }
  } else {
    list->Append(*ComputedStyleUtils::CreateTimingFunctionValue(
        CSSTimingData::InitialTimingFunction().get()));
  }
  return list;
}

CSSValueList* ComputedStyleUtils::ValuesForBorderRadiusCorner(
    const LengthSize& radius,
    const ComputedStyle& style) {
  CSSValueList* list = CSSValueList::CreateSpaceSeparated();
  if (radius.Width().GetType() == kPercent) {
    list->Append(*CSSPrimitiveValue::Create(
        radius.Width().Percent(), CSSPrimitiveValue::UnitType::kPercentage));
  } else {
    list->Append(*ComputedStyleUtils::ZoomAdjustedPixelValueForLength(
        radius.Width(), style));
  }
  if (radius.Height().GetType() == kPercent) {
    list->Append(*CSSPrimitiveValue::Create(
        radius.Height().Percent(), CSSPrimitiveValue::UnitType::kPercentage));
  } else {
    list->Append(*ComputedStyleUtils::ZoomAdjustedPixelValueForLength(
        radius.Height(), style));
  }
  return list;
}

const CSSValue& ComputedStyleUtils::ValueForBorderRadiusCorner(
    const LengthSize& radius,
    const ComputedStyle& style) {
  CSSValueList& list =
      *ComputedStyleUtils::ValuesForBorderRadiusCorner(radius, style);
  if (list.Item(0) == list.Item(1))
    return list.Item(0);
  return list;
}

CSSFunctionValue* ValueForMatrixTransform(
    const TransformationMatrix& transform_param,
    const ComputedStyle& style) {
  // Take TransformationMatrix by reference and then copy it because VC++
  // doesn't guarantee alignment of function parameters.
  TransformationMatrix transform = transform_param;
  CSSFunctionValue* transform_value = nullptr;
  transform.Zoom(1 / style.EffectiveZoom());
  if (transform.IsAffine()) {
    transform_value = CSSFunctionValue::Create(CSSValueMatrix);

    transform_value->Append(*CSSPrimitiveValue::Create(
        transform.A(), CSSPrimitiveValue::UnitType::kNumber));
    transform_value->Append(*CSSPrimitiveValue::Create(
        transform.B(), CSSPrimitiveValue::UnitType::kNumber));
    transform_value->Append(*CSSPrimitiveValue::Create(
        transform.C(), CSSPrimitiveValue::UnitType::kNumber));
    transform_value->Append(*CSSPrimitiveValue::Create(
        transform.D(), CSSPrimitiveValue::UnitType::kNumber));
    transform_value->Append(*CSSPrimitiveValue::Create(
        transform.E(), CSSPrimitiveValue::UnitType::kNumber));
    transform_value->Append(*CSSPrimitiveValue::Create(
        transform.F(), CSSPrimitiveValue::UnitType::kNumber));
  } else {
    transform_value = CSSFunctionValue::Create(CSSValueMatrix3d);

    transform_value->Append(*CSSPrimitiveValue::Create(
        transform.M11(), CSSPrimitiveValue::UnitType::kNumber));
    transform_value->Append(*CSSPrimitiveValue::Create(
        transform.M12(), CSSPrimitiveValue::UnitType::kNumber));
    transform_value->Append(*CSSPrimitiveValue::Create(
        transform.M13(), CSSPrimitiveValue::UnitType::kNumber));
    transform_value->Append(*CSSPrimitiveValue::Create(
        transform.M14(), CSSPrimitiveValue::UnitType::kNumber));

    transform_value->Append(*CSSPrimitiveValue::Create(
        transform.M21(), CSSPrimitiveValue::UnitType::kNumber));
    transform_value->Append(*CSSPrimitiveValue::Create(
        transform.M22(), CSSPrimitiveValue::UnitType::kNumber));
    transform_value->Append(*CSSPrimitiveValue::Create(
        transform.M23(), CSSPrimitiveValue::UnitType::kNumber));
    transform_value->Append(*CSSPrimitiveValue::Create(
        transform.M24(), CSSPrimitiveValue::UnitType::kNumber));

    transform_value->Append(*CSSPrimitiveValue::Create(
        transform.M31(), CSSPrimitiveValue::UnitType::kNumber));
    transform_value->Append(*CSSPrimitiveValue::Create(
        transform.M32(), CSSPrimitiveValue::UnitType::kNumber));
    transform_value->Append(*CSSPrimitiveValue::Create(
        transform.M33(), CSSPrimitiveValue::UnitType::kNumber));
    transform_value->Append(*CSSPrimitiveValue::Create(
        transform.M34(), CSSPrimitiveValue::UnitType::kNumber));

    transform_value->Append(*CSSPrimitiveValue::Create(
        transform.M41(), CSSPrimitiveValue::UnitType::kNumber));
    transform_value->Append(*CSSPrimitiveValue::Create(
        transform.M42(), CSSPrimitiveValue::UnitType::kNumber));
    transform_value->Append(*CSSPrimitiveValue::Create(
        transform.M43(), CSSPrimitiveValue::UnitType::kNumber));
    transform_value->Append(*CSSPrimitiveValue::Create(
        transform.M44(), CSSPrimitiveValue::UnitType::kNumber));
  }

  return transform_value;
}

CSSValue* ComputedStyleUtils::ComputedTransform(
    const LayoutObject* layout_object,
    const ComputedStyle& style) {
  if (!layout_object || !style.HasTransform())
    return CSSIdentifierValue::Create(CSSValueNone);

  IntRect box;
  if (layout_object->IsBox())
    box = PixelSnappedIntRect(ToLayoutBox(layout_object)->BorderBoxRect());

  TransformationMatrix transform;
  style.ApplyTransform(transform, LayoutSize(box.Size()),
                       ComputedStyle::kExcludeTransformOrigin,
                       ComputedStyle::kExcludeMotionPath,
                       ComputedStyle::kExcludeIndependentTransformProperties);

  // FIXME: Need to print out individual functions
  // (https://bugs.webkit.org/show_bug.cgi?id=23924)
  CSSValueList* list = CSSValueList::CreateSpaceSeparated();
  list->Append(*ValueForMatrixTransform(transform, style));

  return list;
}

CSSValue* ComputedStyleUtils::CreateTransitionPropertyValue(
    const CSSTransitionData::TransitionProperty& property) {
  if (property.property_type == CSSTransitionData::kTransitionNone)
    return CSSIdentifierValue::Create(CSSValueNone);
  if (property.property_type == CSSTransitionData::kTransitionUnknownProperty)
    return CSSCustomIdentValue::Create(property.property_string);
  DCHECK_EQ(property.property_type,
            CSSTransitionData::kTransitionKnownProperty);
  return CSSCustomIdentValue::Create(
      CSSUnresolvedProperty::Get(property.unresolved_property)
          .GetPropertyNameAtomicString());
}

}  // namespace blink
