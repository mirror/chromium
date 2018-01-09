// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/properties/ComputedStyleUtils.h"

#include "core/css/CSSBorderImage.h"
#include "core/css/CSSBorderImageSliceValue.h"
#include "core/css/CSSColorValue.h"
#include "core/css/CSSPrimitiveValueMappings.h"
#include "core/css/CSSQuadValue.h"
#include "core/css/CSSReflectValue.h"
#include "core/css/CSSValue.h"
#include "core/css/CSSValuePair.h"
#include "core/css/StyleColor.h"
#include "core/layout/LayoutBlock.h"
#include "core/layout/LayoutBox.h"

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

}  // namespace blink
