// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/properties/CSSPropertyAPICustomHelper.h"

#include "core/StylePropertyShorthand.h"
#include "core/css/CSSFunctionValue.h"
#include "core/css/CSSGridAutoRepeatValue.h"
#include "core/css/CSSGridLineNamesValue.h"
#include "core/css/CSSGridTemplateAreasValue.h"
#include "core/css/CSSIdentifierValue.h"
#include "core/css/CSSInheritedValue.h"
#include "core/css/CSSInitialValue.h"
#include "core/css/CSSPrimitiveValueMappings.h"
#include "core/css/CSSValue.h"
#include "core/css/CSSValuePair.h"
#include "core/css/parser/CSSParserContext.h"
#include "core/css/parser/CSSParserIdioms.h"
#include "core/css/parser/CSSParserMode.h"
#include "core/css/parser/CSSPropertyParserHelpers.h"
#include "core/css/properties/CSSPropertyAlignmentUtils.h"
#include "core/css/properties/CSSPropertyBackgroundUtils.h"
#include "core/css/properties/CSSPropertyGridUtils.h"
#include "core/css/properties/CSSPropertyPositionUtils.h"
#include "core/style/GridArea.h"
#include "platform/wtf/text/StringBuilder.h"

namespace blink {

namespace CSSPropertyAPICustomHelper {

using namespace CSSPropertyParserHelpers;

namespace {

CSSValue* ConsumeBackgroundComponent(CSSPropertyID unresolved_property,
                                     CSSParserTokenRange& range,
                                     const CSSParserContext* context) {
  switch (unresolved_property) {
    case CSSPropertyBackgroundClip:
      return CSSPropertyBackgroundUtils::ConsumeBackgroundBox(range);
    case CSSPropertyBackgroundBlendMode:
      return CSSPropertyBackgroundUtils::ConsumeBackgroundBlendMode(range);
    case CSSPropertyBackgroundAttachment:
      return CSSPropertyBackgroundUtils::ConsumeBackgroundAttachment(range);
    case CSSPropertyBackgroundOrigin:
      return CSSPropertyBackgroundUtils::ConsumeBackgroundBox(range);
    case CSSPropertyWebkitMaskComposite:
      return CSSPropertyBackgroundUtils::ConsumeBackgroundComposite(range);
    case CSSPropertyMaskSourceType:
      return CSSPropertyBackgroundUtils::ConsumeMaskSourceType(range);
    case CSSPropertyWebkitBackgroundClip:
    case CSSPropertyWebkitMaskClip:
      return ConsumePrefixedBackgroundBox(range, context,
                                          true /* allow_text_value */);
    case CSSPropertyWebkitBackgroundOrigin:
    case CSSPropertyWebkitMaskOrigin:
      return ConsumePrefixedBackgroundBox(range, context,
                                          false /* allow_text_value */);
    case CSSPropertyBackgroundImage:
    case CSSPropertyWebkitMaskImage:
      return ConsumeImageOrNone(range, context);
    case CSSPropertyBackgroundPositionX:
    case CSSPropertyWebkitMaskPositionX:
      return CSSPropertyPositionUtils::ConsumePositionLonghand<CSSValueLeft,
                                                               CSSValueRight>(
          range, context->Mode());
    case CSSPropertyBackgroundPositionY:
    case CSSPropertyWebkitMaskPositionY:
      return CSSPropertyPositionUtils::ConsumePositionLonghand<CSSValueTop,
                                                               CSSValueBottom>(
          range, context->Mode());
    case CSSPropertyBackgroundSize:
    case CSSPropertyWebkitMaskSize:
      return ConsumeBackgroundSize(range, context->Mode(),
                                   false /* use_legacy_parsing */);
    case CSSPropertyAliasWebkitBackgroundSize:
      return ConsumeBackgroundSize(range, context->Mode(),
                                   true /* use_legacy_parsing */);
    case CSSPropertyBackgroundColor:
      return ConsumeColor(range, context->Mode());
    default:
      break;
  };
  return nullptr;
}

// This is copied from CSSPropertyParser for now. This should go away once we
// finish ribbonizing all the shorthands.
void AddExpandedPropertyForValue(CSSPropertyID property,
                                 const CSSValue& value,
                                 bool important,
                                 HeapVector<CSSProperty, 256>& properties) {
  const StylePropertyShorthand& shorthand = shorthandForProperty(property);
  unsigned shorthand_length = shorthand.length();
  DCHECK(shorthand_length);
  const CSSPropertyID* longhands = shorthand.properties();
  for (unsigned i = 0; i < shorthand_length; ++i) {
    AddProperty(longhands[i], shorthand.id(), value, important,
                IsImplicitProperty::kNotImplicit, properties);
  }
}

}  // namespace

CSSValue* ConsumePrefixedBackgroundBox(CSSParserTokenRange& range,
                                       const CSSParserContext* context,
                                       bool allow_text_value) {
  // The values 'border', 'padding' and 'content' are deprecated and do not
  // apply to the version of the property that has the -webkit- prefix removed.
  if (CSSValue* value =
          ConsumeIdentRange(range, CSSValueBorder, CSSValuePaddingBox))
    return value;
  if (allow_text_value && range.Peek().Id() == CSSValueText)
    return ConsumeIdent(range);
  return nullptr;
}

CSSValue* ConsumeBackgroundSize(CSSParserTokenRange& range,
                                CSSParserMode css_parser_mode,
                                bool use_legacy_parsing) {
  if (IdentMatches<CSSValueContain, CSSValueCover>(range.Peek().Id()))
    return ConsumeIdent(range);

  CSSValue* horizontal = ConsumeIdent<CSSValueAuto>(range);
  if (!horizontal) {
    horizontal = ConsumeLengthOrPercent(range, css_parser_mode, kValueRangeAll,
                                        UnitlessQuirk::kForbid);
  }

  CSSValue* vertical = nullptr;
  if (!range.AtEnd()) {
    if (range.Peek().Id() == CSSValueAuto) {  // `auto' is the default
      range.ConsumeIncludingWhitespace();
    } else {
      vertical = ConsumeLengthOrPercent(range, css_parser_mode, kValueRangeAll,
                                        UnitlessQuirk::kForbid);
    }
  } else if (use_legacy_parsing) {
    // Legacy syntax: "-webkit-background-size: 10px" is equivalent to
    // "background-size: 10px 10px".
    vertical = horizontal;
  }
  if (!vertical)
    return horizontal;
  return CSSValuePair::Create(horizontal, vertical,
                              CSSValuePair::kKeepIdenticalValues);
}

bool ConsumeRepeatStyleComponent(CSSParserTokenRange& range,
                                 CSSValue*& value1,
                                 CSSValue*& value2,
                                 bool& implicit) {
  if (ConsumeIdent<CSSValueRepeatX>(range)) {
    value1 = CSSIdentifierValue::Create(CSSValueRepeat);
    value2 = CSSIdentifierValue::Create(CSSValueNoRepeat);
    implicit = true;
    return true;
  }
  if (ConsumeIdent<CSSValueRepeatY>(range)) {
    value1 = CSSIdentifierValue::Create(CSSValueNoRepeat);
    value2 = CSSIdentifierValue::Create(CSSValueRepeat);
    implicit = true;
    return true;
  }
  value1 = ConsumeIdent<CSSValueRepeat, CSSValueNoRepeat, CSSValueRound,
                        CSSValueSpace>(range);
  if (!value1)
    return false;

  value2 = ConsumeIdent<CSSValueRepeat, CSSValueNoRepeat, CSSValueRound,
                        CSSValueSpace>(range);
  if (!value2) {
    value2 = value1;
    implicit = true;
  }
  return true;
}

// Note: consumeBackgroundShorthand assumes y properties (for example
// background-position-y) follow the x properties in the shorthand array.
bool ConsumeBackgroundShorthand(const StylePropertyShorthand& shorthand,
                                bool important,
                                CSSParserTokenRange& range,
                                const CSSParserContext& context,
                                HeapVector<CSSProperty, 256>& properties) {
  const unsigned longhand_count = shorthand.length();
  CSSValue* longhands[10] = {0};
  DCHECK_LE(longhand_count, 10u);

  bool implicit = false;
  do {
    bool parsed_longhand[10] = {false};
    CSSValue* origin_value = nullptr;
    do {
      bool found_property = false;
      for (size_t i = 0; i < longhand_count; ++i) {
        if (parsed_longhand[i])
          continue;

        CSSValue* value = nullptr;
        CSSValue* value_y = nullptr;
        CSSPropertyID property = shorthand.properties()[i];
        if (property == CSSPropertyBackgroundRepeatX ||
            property == CSSPropertyWebkitMaskRepeatX) {
          ConsumeRepeatStyleComponent(range, value, value_y, implicit);
        } else if (property == CSSPropertyBackgroundPositionX ||
                   property == CSSPropertyWebkitMaskPositionX) {
          if (!ConsumePosition(range, context, UnitlessQuirk::kForbid,
                               WebFeature::kThreeValuedPositionBackground,
                               value, value_y))
            continue;
        } else if (property == CSSPropertyBackgroundSize ||
                   property == CSSPropertyWebkitMaskSize) {
          if (!ConsumeSlashIncludingWhitespace(range))
            continue;
          value = ConsumeBackgroundSize(range, context.Mode(),
                                        false /* use_legacy_parsing */);
          if (!value || !parsed_longhand[i - 1]) {
            // Position must have been parsed in the current layer.
            return false;
          }
        } else if (property == CSSPropertyBackgroundPositionY ||
                   property == CSSPropertyBackgroundRepeatY ||
                   property == CSSPropertyWebkitMaskPositionY ||
                   property == CSSPropertyWebkitMaskRepeatY) {
          continue;
        } else {
          value = ConsumeBackgroundComponent(property, range, &context);
        }
        if (value) {
          if (property == CSSPropertyBackgroundOrigin ||
              property == CSSPropertyWebkitMaskOrigin)
            origin_value = value;
          parsed_longhand[i] = true;
          found_property = true;
          CSSPropertyBackgroundUtils::AddBackgroundValue(longhands[i], value);
          if (value_y) {
            parsed_longhand[i + 1] = true;
            CSSPropertyBackgroundUtils::AddBackgroundValue(longhands[i + 1],
                                                           value_y);
          }
        }
      }
      if (!found_property)
        return false;
    } while (!range.AtEnd() && range.Peek().GetType() != kCommaToken);

    // TODO(timloh): This will make invalid longhands, see crbug.com/386459
    for (size_t i = 0; i < longhand_count; ++i) {
      CSSPropertyID property = shorthand.properties()[i];
      if (property == CSSPropertyBackgroundColor && !range.AtEnd()) {
        if (parsed_longhand[i])
          return false;  // Colors are only allowed in the last layer.
        continue;
      }
      if ((property == CSSPropertyBackgroundClip ||
           property == CSSPropertyWebkitMaskClip) &&
          !parsed_longhand[i] && origin_value) {
        CSSPropertyBackgroundUtils::AddBackgroundValue(longhands[i],
                                                       origin_value);
        continue;
      }
      if (!parsed_longhand[i]) {
        CSSPropertyBackgroundUtils::AddBackgroundValue(
            longhands[i], CSSInitialValue::Create());
      }
    }
  } while (ConsumeCommaIncludingWhitespace(range));
  if (!range.AtEnd())
    return false;

  for (size_t i = 0; i < longhand_count; ++i) {
    CSSPropertyID property = shorthand.properties()[i];
    if (property == CSSPropertyBackgroundSize && longhands[i] &&
        context.UseLegacyBackgroundSizeShorthandBehavior())
      continue;
    AddProperty(property, shorthand.id(), *longhands[i], important,
                implicit ? IsImplicitProperty::kImplicit
                         : IsImplicitProperty::kNotImplicit,
                properties);
  }
  return true;
}

CSSValueList* ConsumeImplicitAutoFlow(CSSParserTokenRange& range,
                                      const CSSValue& flow_direction) {
  // [ auto-flow && dense? ]
  CSSValue* dense_algorithm = nullptr;
  if ((ConsumeIdent<CSSValueAutoFlow>(range))) {
    dense_algorithm = ConsumeIdent<CSSValueDense>(range);
  } else {
    dense_algorithm = ConsumeIdent<CSSValueDense>(range);
    if (!dense_algorithm)
      return nullptr;
    if (!ConsumeIdent<CSSValueAutoFlow>(range))
      return nullptr;
  }
  CSSValueList* list = CSSValueList::CreateSpaceSeparated();
  list->Append(flow_direction);
  if (dense_algorithm)
    list->Append(*dense_algorithm);
  return list;
}

static bool IsGridBreadthFixedSized(const CSSValue& value) {
  if (value.IsIdentifierValue()) {
    CSSValueID value_id = ToCSSIdentifierValue(value).GetValueID();
    return !(value_id == CSSValueMinContent || value_id == CSSValueMaxContent ||
             value_id == CSSValueAuto);
  }

  if (value.IsPrimitiveValue()) {
    return !ToCSSPrimitiveValue(value).IsFlex();
  }

  NOTREACHED();
  return true;
}

static bool IsGridTrackFixedSized(const CSSValue& value) {
  if (value.IsPrimitiveValue() || value.IsIdentifierValue())
    return IsGridBreadthFixedSized(value);

  DCHECK(value.IsFunctionValue());
  auto& function = ToCSSFunctionValue(value);
  if (function.FunctionType() == CSSValueFitContent)
    return false;

  const CSSValue& min_value = function.Item(0);
  const CSSValue& max_value = function.Item(1);
  return IsGridBreadthFixedSized(min_value) ||
         IsGridBreadthFixedSized(max_value);
}

static Vector<String> ParseGridTemplateAreasColumnNames(
    const String& grid_row_names) {
  DCHECK(!grid_row_names.IsEmpty());
  Vector<String> column_names;
  // Using StringImpl to avoid checks and indirection in every call to
  // String::operator[].
  StringImpl& text = *grid_row_names.Impl();

  StringBuilder area_name;
  for (unsigned i = 0; i < text.length(); ++i) {
    if (IsCSSSpace(text[i])) {
      if (!area_name.IsEmpty()) {
        column_names.push_back(area_name.ToString());
        area_name.Clear();
      }
      continue;
    }
    if (text[i] == '.') {
      if (area_name == ".")
        continue;
      if (!area_name.IsEmpty()) {
        column_names.push_back(area_name.ToString());
        area_name.Clear();
      }
    } else {
      if (!IsNameCodePoint(text[i]))
        return Vector<String>();
      if (area_name == ".") {
        column_names.push_back(area_name.ToString());
        area_name.Clear();
      }
    }

    area_name.Append(text[i]);
  }

  if (!area_name.IsEmpty())
    column_names.push_back(area_name.ToString());

  return column_names;
}

static bool ParseGridTemplateAreasRow(const String& grid_row_names,
                                      NamedGridAreaMap& grid_area_map,
                                      const size_t row_count,
                                      size_t& column_count) {
  if (grid_row_names.IsEmpty() || grid_row_names.ContainsOnlyWhitespace())
    return false;

  Vector<String> column_names =
      ParseGridTemplateAreasColumnNames(grid_row_names);
  if (row_count == 0) {
    column_count = column_names.size();
    if (column_count == 0)
      return false;
  } else if (column_count != column_names.size()) {
    // The declaration is invalid if all the rows don't have the number of
    // columns.
    return false;
  }

  for (size_t current_column = 0; current_column < column_count;
       ++current_column) {
    const String& grid_area_name = column_names[current_column];

    // Unamed areas are always valid (we consider them to be 1x1).
    if (grid_area_name == ".")
      continue;

    size_t look_ahead_column = current_column + 1;
    while (look_ahead_column < column_count &&
           column_names[look_ahead_column] == grid_area_name)
      look_ahead_column++;

    NamedGridAreaMap::iterator grid_area_it =
        grid_area_map.find(grid_area_name);
    if (grid_area_it == grid_area_map.end()) {
      grid_area_map.insert(grid_area_name,
                           GridArea(GridSpan::TranslatedDefiniteGridSpan(
                                        row_count, row_count + 1),
                                    GridSpan::TranslatedDefiniteGridSpan(
                                        current_column, look_ahead_column)));
    } else {
      GridArea& grid_area = grid_area_it->value;

      // The following checks test that the grid area is a single filled-in
      // rectangle.
      // 1. The new row is adjacent to the previously parsed row.
      if (row_count != grid_area.rows.EndLine())
        return false;

      // 2. The new area starts at the same position as the previously parsed
      // area.
      if (current_column != grid_area.columns.StartLine())
        return false;

      // 3. The new area ends at the same position as the previously parsed
      // area.
      if (look_ahead_column != grid_area.columns.EndLine())
        return false;

      grid_area.rows = GridSpan::TranslatedDefiniteGridSpan(
          grid_area.rows.StartLine(), grid_area.rows.EndLine() + 1);
    }
    current_column = look_ahead_column - 1;
  }

  return true;
}

static CSSValue* ConsumeGridBreadth(CSSParserTokenRange& range,
                                    CSSParserMode css_parser_mode) {
  const CSSParserToken& token = range.Peek();
  if (IdentMatches<CSSValueMinContent, CSSValueMaxContent, CSSValueAuto>(
          token.Id()))
    return ConsumeIdent(range);
  if (token.GetType() == kDimensionToken &&
      token.GetUnitType() == CSSPrimitiveValue::UnitType::kFraction) {
    if (range.Peek().NumericValue() < 0)
      return nullptr;
    return CSSPrimitiveValue::Create(
        range.ConsumeIncludingWhitespace().NumericValue(),
        CSSPrimitiveValue::UnitType::kFraction);
  }
  return ConsumeLengthOrPercent(range, css_parser_mode, kValueRangeNonNegative,
                                UnitlessQuirk::kAllow);
}

static CSSValue* ConsumeFitContent(CSSParserTokenRange& range,
                                   CSSParserMode css_parser_mode) {
  CSSParserTokenRange rangecopy = range;
  CSSParserTokenRange args = ConsumeFunction(rangecopy);
  CSSPrimitiveValue* length = ConsumeLengthOrPercent(
      args, css_parser_mode, kValueRangeNonNegative, UnitlessQuirk::kAllow);
  if (!length || !args.AtEnd())
    return nullptr;
  range = rangecopy;
  CSSFunctionValue* result = CSSFunctionValue::Create(CSSValueFitContent);
  result->Append(*length);
  return result;
}

static CSSValue* ConsumeGridTrackSize(CSSParserTokenRange& range,
                                      CSSParserMode css_parser_mode) {
  const CSSParserToken& token = range.Peek();
  if (IdentMatches<CSSValueAuto>(token.Id()))
    return ConsumeIdent(range);

  if (token.FunctionId() == CSSValueMinmax) {
    CSSParserTokenRange rangecopy = range;
    CSSParserTokenRange args = ConsumeFunction(rangecopy);
    CSSValue* min_track_breadth = ConsumeGridBreadth(args, css_parser_mode);
    if (!min_track_breadth ||
        (min_track_breadth->IsPrimitiveValue() &&
         ToCSSPrimitiveValue(min_track_breadth)->IsFlex()) ||
        !ConsumeCommaIncludingWhitespace(args))
      return nullptr;
    CSSValue* max_track_breadth = ConsumeGridBreadth(args, css_parser_mode);
    if (!max_track_breadth || !args.AtEnd())
      return nullptr;
    range = rangecopy;
    CSSFunctionValue* result = CSSFunctionValue::Create(CSSValueMinmax);
    result->Append(*min_track_breadth);
    result->Append(*max_track_breadth);
    return result;
  }

  if (token.FunctionId() == CSSValueFitContent)
    return ConsumeFitContent(range, css_parser_mode);

  return ConsumeGridBreadth(range, css_parser_mode);
}

// Appends to the passed in CSSGridLineNamesValue if any, otherwise creates a
// new one.
static CSSGridLineNamesValue* ConsumeGridLineNames(
    CSSParserTokenRange& range,
    CSSGridLineNamesValue* line_names = nullptr) {
  CSSParserTokenRange rangecopy = range;
  if (rangecopy.ConsumeIncludingWhitespace().GetType() != kLeftBracketToken)
    return nullptr;
  if (!line_names)
    line_names = CSSGridLineNamesValue::Create();
  while (CSSCustomIdentValue* line_name =
             CSSPropertyGridUtils::ConsumeCustomIdentForGridLine(rangecopy))
    line_names->Append(*line_name);
  if (rangecopy.ConsumeIncludingWhitespace().GetType() != kRightBracketToken)
    return nullptr;
  range = rangecopy;
  return line_names;
}

static bool ConsumeGridTrackRepeatFunction(CSSParserTokenRange& range,
                                           CSSParserMode css_parser_mode,
                                           CSSValueList& list,
                                           bool& is_auto_repeat,
                                           bool& all_tracks_are_fixed_sized) {
  CSSParserTokenRange args = ConsumeFunction(range);
  // The number of repetitions for <auto-repeat> is not important at parsing
  // level because it will be computed later, let's set it to 1.
  size_t repetitions = 1;
  is_auto_repeat =
      IdentMatches<CSSValueAutoFill, CSSValueAutoFit>(args.Peek().Id());
  CSSValueList* repeated_values;
  if (is_auto_repeat) {
    repeated_values =
        CSSGridAutoRepeatValue::Create(args.ConsumeIncludingWhitespace().Id());
  } else {
    // TODO(rob.buis): a consumeIntegerRaw would be more efficient here.
    CSSPrimitiveValue* repetition = ConsumePositiveInteger(args);
    if (!repetition)
      return false;
    repetitions =
        clampTo<size_t>(repetition->GetDoubleValue(), 0, kGridMaxTracks);
    repeated_values = CSSValueList::CreateSpaceSeparated();
  }
  if (!ConsumeCommaIncludingWhitespace(args))
    return false;
  CSSGridLineNamesValue* line_names = ConsumeGridLineNames(args);
  if (line_names)
    repeated_values->Append(*line_names);

  size_t number_of_tracks = 0;
  while (!args.AtEnd()) {
    CSSValue* track_size = ConsumeGridTrackSize(args, css_parser_mode);
    if (!track_size)
      return false;
    if (all_tracks_are_fixed_sized)
      all_tracks_are_fixed_sized = IsGridTrackFixedSized(*track_size);
    repeated_values->Append(*track_size);
    ++number_of_tracks;
    line_names = ConsumeGridLineNames(args);
    if (line_names)
      repeated_values->Append(*line_names);
  }
  // We should have found at least one <track-size> or else it is not a valid
  // <track-list>.
  if (!number_of_tracks)
    return false;

  if (is_auto_repeat) {
    list.Append(*repeated_values);
  } else {
    // We clamp the repetitions to a multiple of the repeat() track list's size,
    // while staying below the max grid size.
    repetitions = std::min(repetitions, kGridMaxTracks / number_of_tracks);
    for (size_t i = 0; i < repetitions; ++i) {
      for (size_t j = 0; j < repeated_values->length(); ++j)
        list.Append(repeated_values->Item(j));
    }
  }
  return true;
}

CSSValue* ConsumeGridTrackList(CSSParserTokenRange& range,
                               CSSParserMode css_parser_mode,
                               TrackListType track_list_type) {
  bool allow_grid_line_names = track_list_type != kGridAuto;
  CSSValueList* values = CSSValueList::CreateSpaceSeparated();
  CSSGridLineNamesValue* line_names = ConsumeGridLineNames(range);
  if (line_names) {
    if (!allow_grid_line_names)
      return nullptr;
    values->Append(*line_names);
  }

  bool allow_repeat = track_list_type == kGridTemplate;
  bool seen_auto_repeat = false;
  bool all_tracks_are_fixed_sized = true;
  do {
    bool is_auto_repeat;
    if (range.Peek().FunctionId() == CSSValueRepeat) {
      if (!allow_repeat)
        return nullptr;
      if (!ConsumeGridTrackRepeatFunction(range, css_parser_mode, *values,
                                          is_auto_repeat,
                                          all_tracks_are_fixed_sized))
        return nullptr;
      if (is_auto_repeat && seen_auto_repeat)
        return nullptr;
      seen_auto_repeat = seen_auto_repeat || is_auto_repeat;
    } else if (CSSValue* value = ConsumeGridTrackSize(range, css_parser_mode)) {
      if (all_tracks_are_fixed_sized)
        all_tracks_are_fixed_sized = IsGridTrackFixedSized(*value);
      values->Append(*value);
    } else {
      return nullptr;
    }
    if (seen_auto_repeat && !all_tracks_are_fixed_sized)
      return nullptr;
    line_names = ConsumeGridLineNames(range);
    if (line_names) {
      if (!allow_grid_line_names)
        return nullptr;
      values->Append(*line_names);
    }
  } while (!range.AtEnd() && range.Peek().GetType() != kDelimiterToken);
  return values;
}

CSSValue* ConsumeGridTemplatesRowsOrColumns(CSSParserTokenRange& range,
                                            CSSParserMode css_parser_mode) {
  if (range.Peek().Id() == CSSValueNone)
    return ConsumeIdent(range);
  return ConsumeGridTrackList(range, css_parser_mode, kGridTemplate);
}

bool ConsumeGridTemplateRowsAndAreasAndColumns(
    CSSPropertyID shorthand_id,
    bool important,
    CSSParserTokenRange& range,
    const CSSParserContext& context,
    HeapVector<CSSProperty, 256>& properties) {
  NamedGridAreaMap grid_area_map;
  size_t row_count = 0;
  size_t column_count = 0;
  CSSValueList* template_rows = CSSValueList::CreateSpaceSeparated();

  // Persists between loop iterations so we can use the same value for
  // consecutive <line-names> values
  CSSGridLineNamesValue* line_names = nullptr;

  do {
    // Handle leading <custom-ident>*.
    bool has_previous_line_names = line_names;
    line_names = ConsumeGridLineNames(range, line_names);
    if (line_names && !has_previous_line_names)
      template_rows->Append(*line_names);

    // Handle a template-area's row.
    if (range.Peek().GetType() != kStringToken ||
        !ParseGridTemplateAreasRow(
            range.ConsumeIncludingWhitespace().Value().ToString(),
            grid_area_map, row_count, column_count))
      return false;
    ++row_count;

    // Handle template-rows's track-size.
    CSSValue* value = ConsumeGridTrackSize(range, context.Mode());
    if (!value)
      value = CSSIdentifierValue::Create(CSSValueAuto);
    template_rows->Append(*value);

    // This will handle the trailing/leading <custom-ident>* in the grammar.
    line_names = ConsumeGridLineNames(range);
    if (line_names)
      template_rows->Append(*line_names);
  } while (!range.AtEnd() && !(range.Peek().GetType() == kDelimiterToken &&
                               range.Peek().Delimiter() == '/'));

  CSSValue* columns_value = nullptr;
  if (!range.AtEnd()) {
    if (!ConsumeSlashIncludingWhitespace(range))
      return false;
    columns_value =
        ConsumeGridTrackList(range, context.Mode(), kGridTemplateNoRepeat);
    if (!columns_value || !range.AtEnd())
      return false;
  } else {
    columns_value = CSSIdentifierValue::Create(CSSValueNone);
  }
  AddProperty(CSSPropertyGridTemplateRows, shorthand_id, *template_rows,
              important, IsImplicitProperty::kNotImplicit, properties);
  AddProperty(CSSPropertyGridTemplateColumns, shorthand_id, *columns_value,
              important, IsImplicitProperty::kNotImplicit, properties);
  AddProperty(CSSPropertyGridTemplateAreas, shorthand_id,
              *CSSGridTemplateAreasValue::Create(grid_area_map, row_count,
                                                 column_count),
              important, IsImplicitProperty::kNotImplicit, properties);
  return true;
}

bool ConsumeGridTemplateShorthand(CSSPropertyID shorthand_id,
                                  bool important,
                                  CSSParserTokenRange& range,
                                  const CSSParserContext& context,
                                  HeapVector<CSSProperty, 256>& properties) {
  DCHECK(RuntimeEnabledFeatures::CSSGridLayoutEnabled());
  DCHECK_EQ(gridTemplateShorthand().length(), 3u);

  CSSParserTokenRange rangecopy = range;
  CSSValue* rows_value = ConsumeIdent<CSSValueNone>(range);

  // 1- 'none' case.
  if (rows_value && range.AtEnd()) {
    AddProperty(CSSPropertyGridTemplateRows, shorthand_id,
                *CSSIdentifierValue::Create(CSSValueNone), important,
                IsImplicitProperty::kNotImplicit, properties);
    AddProperty(CSSPropertyGridTemplateColumns, shorthand_id,
                *CSSIdentifierValue::Create(CSSValueNone), important,
                IsImplicitProperty::kNotImplicit, properties);
    AddProperty(CSSPropertyGridTemplateAreas, shorthand_id,
                *CSSIdentifierValue::Create(CSSValueNone), important,
                IsImplicitProperty::kNotImplicit, properties);
    return true;
  }

  // 2- <grid-template-rows> / <grid-template-columns>
  if (!rows_value)
    rows_value = ConsumeGridTrackList(range, context.Mode(), kGridTemplate);

  if (rows_value) {
    if (!ConsumeSlashIncludingWhitespace(range))
      return false;
    CSSValue* columns_value =
        ConsumeGridTemplatesRowsOrColumns(range, context.Mode());
    if (!columns_value || !range.AtEnd())
      return false;

    AddProperty(CSSPropertyGridTemplateRows, shorthand_id, *rows_value,
                important, IsImplicitProperty::kNotImplicit, properties);
    AddProperty(CSSPropertyGridTemplateColumns, shorthand_id, *columns_value,
                important, IsImplicitProperty::kNotImplicit, properties);
    AddProperty(CSSPropertyGridTemplateAreas, shorthand_id,
                *CSSIdentifierValue::Create(CSSValueNone), important,
                IsImplicitProperty::kNotImplicit, properties);
    return true;
  }

  // 3- [ <line-names>? <string> <track-size>? <line-names>? ]+
  // [ / <track-list> ]?
  range = rangecopy;
  return ConsumeGridTemplateRowsAndAreasAndColumns(shorthand_id, important,
                                                   range, context, properties);
}

bool ConsumeGridShorthand(bool important,
                          CSSParserTokenRange& range,
                          const CSSParserContext& context,
                          HeapVector<CSSProperty, 256>& properties) {
  DCHECK(RuntimeEnabledFeatures::CSSGridLayoutEnabled());
  DCHECK_EQ(shorthandForProperty(CSSPropertyGrid).length(), 8u);

  CSSParserTokenRange rangecopy = range;

  // 1- <grid-template>
  if (ConsumeGridTemplateShorthand(CSSPropertyGrid, important, range, context,
                                   properties)) {
    // It can only be specified the explicit or the implicit grid properties in
    // a single grid declaration. The sub-properties not specified are set to
    // their initial value, as normal for shorthands.
    AddProperty(CSSPropertyGridAutoFlow, CSSPropertyGrid,
                *CSSInitialValue::Create(), important,
                IsImplicitProperty::kNotImplicit, properties);
    AddProperty(CSSPropertyGridAutoColumns, CSSPropertyGrid,
                *CSSInitialValue::Create(), important,
                IsImplicitProperty::kNotImplicit, properties);
    AddProperty(CSSPropertyGridAutoRows, CSSPropertyGrid,
                *CSSInitialValue::Create(), important,
                IsImplicitProperty::kNotImplicit, properties);
    AddProperty(CSSPropertyGridColumnGap, CSSPropertyGrid,
                *CSSInitialValue::Create(), important,
                IsImplicitProperty::kNotImplicit, properties);
    AddProperty(CSSPropertyGridRowGap, CSSPropertyGrid,
                *CSSInitialValue::Create(), important,
                IsImplicitProperty::kNotImplicit, properties);
    return true;
  }

  range = rangecopy;

  CSSValue* auto_columns_value = nullptr;
  CSSValue* auto_rows_value = nullptr;
  CSSValue* template_rows = nullptr;
  CSSValue* template_columns = nullptr;
  CSSValueList* grid_auto_flow = nullptr;
  if (IdentMatches<CSSValueDense, CSSValueAutoFlow>(range.Peek().Id())) {
    // 2- [ auto-flow && dense? ] <grid-auto-rows>? / <grid-template-columns>
    grid_auto_flow = ConsumeImplicitAutoFlow(
        range, *CSSIdentifierValue::Create(CSSValueRow));
    if (!grid_auto_flow)
      return false;
    if (ConsumeSlashIncludingWhitespace(range)) {
      auto_rows_value = CSSInitialValue::Create();
    } else {
      auto_rows_value = ConsumeGridTrackList(range, context.Mode(), kGridAuto);
      if (!auto_rows_value)
        return false;
      if (!ConsumeSlashIncludingWhitespace(range))
        return false;
    }
    if (!(template_columns =
              ConsumeGridTemplatesRowsOrColumns(range, context.Mode())))
      return false;
    template_rows = CSSInitialValue::Create();
    auto_columns_value = CSSInitialValue::Create();
  } else {
    // 3- <grid-template-rows> / [ auto-flow && dense? ] <grid-auto-columns>?
    template_rows = ConsumeGridTemplatesRowsOrColumns(range, context.Mode());
    if (!template_rows)
      return false;
    if (!ConsumeSlashIncludingWhitespace(range))
      return false;
    grid_auto_flow = ConsumeImplicitAutoFlow(
        range, *CSSIdentifierValue::Create(CSSValueColumn));
    if (!grid_auto_flow)
      return false;
    if (range.AtEnd()) {
      auto_columns_value = CSSInitialValue::Create();
    } else {
      auto_columns_value =
          ConsumeGridTrackList(range, context.Mode(), kGridAuto);
      if (!auto_columns_value)
        return false;
    }
    template_columns = CSSInitialValue::Create();
    auto_rows_value = CSSInitialValue::Create();
  }

  if (!range.AtEnd())
    return false;

  // It can only be specified the explicit or the implicit grid properties in a
  // single grid declaration. The sub-properties not specified are set to their
  // initial value, as normal for shorthands.
  AddProperty(CSSPropertyGridTemplateColumns, CSSPropertyGrid,
              *template_columns, important, IsImplicitProperty::kNotImplicit,
              properties);
  AddProperty(CSSPropertyGridTemplateRows, CSSPropertyGrid, *template_rows,
              important, IsImplicitProperty::kNotImplicit, properties);
  AddProperty(CSSPropertyGridTemplateAreas, CSSPropertyGrid,
              *CSSInitialValue::Create(), important,
              IsImplicitProperty::kNotImplicit, properties);
  AddProperty(CSSPropertyGridAutoFlow, CSSPropertyGrid, *grid_auto_flow,
              important, IsImplicitProperty::kNotImplicit, properties);
  AddProperty(CSSPropertyGridAutoColumns, CSSPropertyGrid, *auto_columns_value,
              important, IsImplicitProperty::kNotImplicit, properties);
  AddProperty(CSSPropertyGridAutoRows, CSSPropertyGrid, *auto_rows_value,
              important, IsImplicitProperty::kNotImplicit, properties);
  AddProperty(CSSPropertyGridColumnGap, CSSPropertyGrid,
              *CSSInitialValue::Create(), important,
              IsImplicitProperty::kNotImplicit, properties);
  AddProperty(CSSPropertyGridRowGap, CSSPropertyGrid,
              *CSSInitialValue::Create(), important,
              IsImplicitProperty::kNotImplicit, properties);
  return true;
}

bool ConsumeBorder(bool important,
                   CSSParserTokenRange& range,
                   const CSSParserContext& context,
                   HeapVector<CSSProperty, 256>& properties) {
  CSSValue* width = nullptr;
  const CSSValue* style = nullptr;
  CSSValue* color = nullptr;

  while (!width || !style || !color) {
    if (!width) {
      width = ConsumeLineWidth(range, context.Mode(), UnitlessQuirk::kForbid);
      if (width)
        continue;
    }
    if (!style) {
      style = CSSPropertyParserHelpers::ParseLonghandViaAPI(
          CSSPropertyBorderLeftStyle, CSSPropertyBorder, context, range);
      if (style)
        continue;
    }
    if (!color) {
      color = ConsumeColor(range, context.Mode());
      if (color)
        continue;
    }
    break;
  }

  if (!width && !style && !color)
    return false;

  if (!width)
    width = CSSInitialValue::Create();
  if (!style)
    style = CSSInitialValue::Create();
  if (!color)
    color = CSSInitialValue::Create();

  AddExpandedPropertyForValue(CSSPropertyBorderWidth, *width, important,
                              properties);
  AddExpandedPropertyForValue(CSSPropertyBorderStyle, *style, important,
                              properties);
  AddExpandedPropertyForValue(CSSPropertyBorderColor, *color, important,
                              properties);
  AddExpandedPropertyForValue(CSSPropertyBorderImage,
                              *CSSInitialValue::Create(), important,
                              properties);

  return range.AtEnd();
}

}  // namespace CSSPropertyAPICustomHelper

}  // namespace blink
