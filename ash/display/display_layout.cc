// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/display/display_layout.h"

#include <ostream>

#include "ash/ash_switches.h"
#include "ash/display/display_pref_util.h"
#include "ash/shell.h"
#include "base/json/json_value_converter.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_piece.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "ui/gfx/display.h"

namespace ash {
namespace  {

// The maximum value for 'offset' in DisplayLayout in case of outliers.  Need
// to change this value in case to support even larger displays.
const int kMaxValidOffset = 10000;

// Persistent key names
const char kPositionKey[] = "position";
const char kOffsetKey[] = "offset";
const char kMirroredKey[] = "mirrored";
const char kDefaultUnifiedKey[] = "default_unified";
const char kPrimaryIdKey[] = "primary-id";

// TODO(oshima): Support multiple placements.
const char kPlacementDisplayIdKey[] = "placement.display_id";
const char kPlacementParentDisplayIdKey[] = "placement.parent_display_id";

using PositionToStringMap = std::map<DisplayPlacement::Position, std::string>;
using DisplayPlacementMap = std::unordered_map<int64_t, DisplayPlacement>;

const PositionToStringMap* GetPositionToStringMap() {
  static const PositionToStringMap* map = CreateToStringMap(
      DisplayPlacement::TOP, "top", DisplayPlacement::BOTTOM, "bottom",
      DisplayPlacement::RIGHT, "right", DisplayPlacement::LEFT, "left");
  return map;
}

std::string ToPositionString(DisplayPlacement::Position position) {
  const PositionToStringMap* map = GetPositionToStringMap();
  PositionToStringMap::const_iterator iter = map->find(position);
  return iter != map->end() ? iter->second : std::string("unknown");
}

bool GetPositionFromString(const base::StringPiece& position,
                           DisplayPlacement::Position* field) {
  if (ReverseFind(GetPositionToStringMap(), position, field))
    return true;
  LOG(ERROR) << "Invalid position value:" << position;
  return false;
}

bool GetDisplayIdFromString(const base::StringPiece& position, int64_t* field) {
  return base::StringToInt64(position, field);
}

bool GetDisplayIdByKey(const base::DictionaryValue* dict_value,
                       const char* key,
                       int64_t* id_out) {
  std::string out;
  return dict_value->GetString(key, &out) && base::StringToInt64(out, id_out);
}

}  // namespace

////////////////////////////////////////////////////////////////////////////////
// DisplayPlacement
DisplayPlacement::DisplayPlacement(Position pos, int offset)
    : display_id(gfx::Display::kInvalidDisplayID),
      parent_display_id(gfx::Display::kInvalidDisplayID),
      position(pos),
      offset(offset) {
  DCHECK_LE(TOP, position);
  DCHECK_GE(LEFT, position);
  // Set the default value to |position| in case position is invalid.  DCHECKs
  // above doesn't stop in Release builds.
  if (TOP > position || LEFT < position)
    this->position = RIGHT;

  DCHECK_GE(kMaxValidOffset, abs(offset));
}

DisplayPlacement& DisplayPlacement::Swap() {
  switch (position) {
    case TOP:
      position = BOTTOM;
      break;
    case BOTTOM:
      position = TOP;
      break;
    case RIGHT:
      position = LEFT;
      break;
    case LEFT:
      position = RIGHT;
      break;
  }
  offset = -offset;
  std::swap(display_id, parent_display_id);
  return *this;
}

std::string DisplayPlacement::ToString() const {
  return base::StringPrintf("%s, %d", ToPositionString(position).c_str(),
                            offset);
}

////////////////////////////////////////////////////////////////////////////////
// DisplayLayout

DisplayLayout::DisplayLayout()
    : placement(DisplayPlacement::RIGHT, 0),
      mirrored(false),
      default_unified(true),
      primary_id(gfx::Display::kInvalidDisplayID) {}

DisplayLayout::~DisplayLayout() {}

// static
bool DisplayLayout::ConvertFromValue(const base::Value& value,
                                     DisplayLayout* layout) {
  base::JSONValueConverter<DisplayLayout> converter;
  converter.Convert(value, layout);

  const base::DictionaryValue* dict_value = nullptr;
  if (!value.GetAsDictionary(&dict_value) || dict_value == nullptr)
    return false;

  dict_value->GetInteger(kOffsetKey, &layout->placement.offset);
  std::string position;
  if (dict_value->GetString(kPositionKey, &position))
    GetPositionFromString(position, &layout->placement.position);

  GetDisplayIdByKey(dict_value, kPlacementDisplayIdKey,
                    &layout->placement.display_id);
  GetDisplayIdByKey(dict_value, kPlacementParentDisplayIdKey,
                    &layout->placement.parent_display_id);
  return true;
}

// static
bool DisplayLayout::ConvertToValue(const DisplayLayout& layout,
                                   base::Value* value) {
  base::DictionaryValue* dict_value = nullptr;
  if (!value->GetAsDictionary(&dict_value) || dict_value == nullptr)
    return false;

  dict_value->SetString(kPositionKey,
                        ToPositionString(layout.placement.position));
  dict_value->SetInteger(kOffsetKey, layout.placement.offset);
  dict_value->SetBoolean(kMirroredKey, layout.mirrored);
  dict_value->SetBoolean(kDefaultUnifiedKey, layout.default_unified);
  dict_value->SetString(kPrimaryIdKey, base::Int64ToString(layout.primary_id));

  dict_value->SetString(kPlacementDisplayIdKey,
                        base::Int64ToString(layout.placement.display_id));
  dict_value->SetString(
      kPlacementParentDisplayIdKey,
      base::Int64ToString(layout.placement.parent_display_id));
  return true;
}

std::string DisplayLayout::ToString() const {
  return base::StringPrintf("%s%s%s", placement.ToString().c_str(),
                            mirrored ? ", mirrored" : "",
                            default_unified ? ", default_unified" : "");
}

scoped_ptr<DisplayLayout> DisplayLayout::Copy() const {
  scoped_ptr<DisplayLayout> copy(new DisplayLayout);
  copy->placement = placement;
  copy->mirrored = mirrored;
  copy->default_unified = default_unified;
  copy->primary_id = primary_id;
  return copy;
}

// static
void DisplayLayout::RegisterJSONConverter(
    base::JSONValueConverter<DisplayLayout>* converter) {
  converter->RegisterBoolField(kMirroredKey, &DisplayLayout::mirrored);
  converter->RegisterBoolField(kDefaultUnifiedKey,
                               &DisplayLayout::default_unified);
  converter->RegisterCustomField<int64_t>(
      kPrimaryIdKey, &DisplayLayout::primary_id, &GetDisplayIdFromString);
}

}  // namespace ash
