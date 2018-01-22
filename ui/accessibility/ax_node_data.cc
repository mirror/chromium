// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/accessibility/ax_node_data.h"

#include <stddef.h>

#include <algorithm>
#include <set>

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/accessibility/ax_enum_util.h"
#include "ui/accessibility/ax_text_utils.h"
#include "ui/gfx/transform.h"

namespace ui {

namespace {

bool IsFlagSet(uint32_t bitfield, uint32_t flag) {
  return 0 != (bitfield & (1 << flag));
}

uint32_t ModifyFlag(uint32_t bitfield, uint32_t flag, bool set) {
  return set ? (bitfield |= (1 << flag)) : (bitfield &= ~(1 << flag));
}

std::string StateBitfieldToString(uint32_t state) {
  std::string str;
  for (uint32_t i = static_cast<uint32_t>(ax::mojom::State::NONE) + 1;
       i <= static_cast<uint32_t>(ax::mojom::State::LAST); ++i) {
    if (IsFlagSet(state, i))
      str += " " + base::ToUpperASCII(ui::ToString(static_cast<ax::mojom::State>(i)));
  }
  return str;
}

std::string ActionsBitfieldToString(uint32_t actions) {
  std::string str;
  for (uint32_t i = static_cast<uint32_t>(ax::mojom::Action::NONE) + 1;
       i <= static_cast<uint32_t>(ax::mojom::Action::LAST); ++i) {
    if (IsFlagSet(actions, i)) {
      str += ui::ToString(static_cast<ax::mojom::Action>(i));
      actions = ModifyFlag(actions, i, false);
      str += actions ? "," : "";
    }
  }
  return str;
}

std::string IntVectorToString(const std::vector<int>& items) {
  std::string str;
  for (size_t i = 0; i < items.size(); ++i) {
    if (i > 0)
      str += ",";
    str += base::NumberToString(items[i]);
  }
  return str;
}

// Predicate that returns true if the first value of a pair is |first|.
template<typename FirstType, typename SecondType>
struct FirstIs {
  FirstIs(FirstType first)
      : first_(first) {}
  bool operator()(std::pair<FirstType, SecondType> const& p) {
    return p.first == first_;
  }
  FirstType first_;
};

// Helper function that finds a key in a vector of pairs by matching on the
// first value, and returns an iterator.
template<typename FirstType, typename SecondType>
typename std::vector<std::pair<FirstType, SecondType>>::const_iterator
    FindInVectorOfPairs(
        FirstType first,
        const std::vector<std::pair<FirstType, SecondType>>& vector) {
  return std::find_if(vector.begin(),
                      vector.end(),
                      FirstIs<FirstType, SecondType>(first));
}

}  // namespace

// Return true if |attr| is a node ID that would need to be mapped when
// renumbering the ids in a combined tree.
bool IsNodeIdIntAttribute(ax::mojom::IntAttribute attr) {
  switch (attr) {
    case ax::mojom::IntAttribute::ACTIVEDESCENDANT_ID:
    case ax::mojom::IntAttribute::DETAILS_ID:
    case ax::mojom::IntAttribute::ERRORMESSAGE_ID:
    case ax::mojom::IntAttribute::IN_PAGE_LINK_TARGET_ID:
    case ax::mojom::IntAttribute::MEMBER_OF_ID:
    case ax::mojom::IntAttribute::NEXT_ON_LINE_ID:
    case ax::mojom::IntAttribute::PREVIOUS_ON_LINE_ID:
    case ax::mojom::IntAttribute::TABLE_HEADER_ID:
    case ax::mojom::IntAttribute::TABLE_COLUMN_HEADER_ID:
    case ax::mojom::IntAttribute::TABLE_ROW_HEADER_ID:
    case ax::mojom::IntAttribute::NEXT_FOCUS_ID:
    case ax::mojom::IntAttribute::PREVIOUS_FOCUS_ID:
      return true;

    // Note: all of the attributes are included here explicitly,
    // rather than using "default:", so that it's a compiler error to
    // add a new attribute without explicitly considering whether it's
    // a node id attribute or not.
    case ax::mojom::IntAttribute::NONE:
    case ax::mojom::IntAttribute::DEFAULT_ACTION_VERB:
    case ax::mojom::IntAttribute::SCROLL_X:
    case ax::mojom::IntAttribute::SCROLL_X_MIN:
    case ax::mojom::IntAttribute::SCROLL_X_MAX:
    case ax::mojom::IntAttribute::SCROLL_Y:
    case ax::mojom::IntAttribute::SCROLL_Y_MIN:
    case ax::mojom::IntAttribute::SCROLL_Y_MAX:
    case ax::mojom::IntAttribute::TEXT_SEL_START:
    case ax::mojom::IntAttribute::TEXT_SEL_END:
    case ax::mojom::IntAttribute::TABLE_ROW_COUNT:
    case ax::mojom::IntAttribute::TABLE_COLUMN_COUNT:
    case ax::mojom::IntAttribute::TABLE_ROW_INDEX:
    case ax::mojom::IntAttribute::TABLE_COLUMN_INDEX:
    case ax::mojom::IntAttribute::TABLE_CELL_COLUMN_INDEX:
    case ax::mojom::IntAttribute::TABLE_CELL_COLUMN_SPAN:
    case ax::mojom::IntAttribute::TABLE_CELL_ROW_INDEX:
    case ax::mojom::IntAttribute::TABLE_CELL_ROW_SPAN:
    case ax::mojom::IntAttribute::SORT_DIRECTION:
    case ax::mojom::IntAttribute::HIERARCHICAL_LEVEL:
    case ax::mojom::IntAttribute::NAME_FROM:
    case ax::mojom::IntAttribute::DESCRIPTION_FROM:
    case ax::mojom::IntAttribute::CHILD_TREE_ID:
    case ax::mojom::IntAttribute::SET_SIZE:
    case ax::mojom::IntAttribute::POS_IN_SET:
    case ax::mojom::IntAttribute::COLOR_VALUE:
    case ax::mojom::IntAttribute::ARIA_CURRENT_STATE:
    case ax::mojom::IntAttribute::BACKGROUND_COLOR:
    case ax::mojom::IntAttribute::COLOR:
    case ax::mojom::IntAttribute::INVALID_STATE:
    case ax::mojom::IntAttribute::CHECKED_STATE:
    case ax::mojom::IntAttribute::RESTRICTION:
    case ax::mojom::IntAttribute::TEXT_DIRECTION:
    case ax::mojom::IntAttribute::TEXT_STYLE:
    case ax::mojom::IntAttribute::ARIA_COLUMN_COUNT:
    case ax::mojom::IntAttribute::ARIA_CELL_COLUMN_INDEX:
    case ax::mojom::IntAttribute::ARIA_ROW_COUNT:
    case ax::mojom::IntAttribute::ARIA_CELL_ROW_INDEX:
      return false;
  }

  NOTREACHED();
  return false;
}

// Return true if |attr| contains a vector of node ids that would need
// to be mapped when renumbering the ids in a combined tree.
bool IsNodeIdIntListAttribute(ax::mojom::IntListAttribute attr) {
  switch (attr) {
    case ax::mojom::IntListAttribute::CELL_IDS:
    case ax::mojom::IntListAttribute::CONTROLS_IDS:
    case ax::mojom::IntListAttribute::DESCRIBEDBY_IDS:
    case ax::mojom::IntListAttribute::FLOWTO_IDS:
    case ax::mojom::IntListAttribute::INDIRECT_CHILD_IDS:
    case ax::mojom::IntListAttribute::LABELLEDBY_IDS:
    case ax::mojom::IntListAttribute::RADIO_GROUP_IDS:
    case ax::mojom::IntListAttribute::UNIQUE_CELL_IDS:
      return true;

    // Note: all of the attributes are included here explicitly,
    // rather than using "default:", so that it's a compiler error to
    // add a new attribute without explicitly considering whether it's
    // a node id attribute or not.
    case ax::mojom::IntListAttribute::NONE:
    case ax::mojom::IntListAttribute::LINE_BREAKS:
    case ax::mojom::IntListAttribute::MARKER_TYPES:
    case ax::mojom::IntListAttribute::MARKER_STARTS:
    case ax::mojom::IntListAttribute::MARKER_ENDS:
    case ax::mojom::IntListAttribute::CHARACTER_OFFSETS:
    case ax::mojom::IntListAttribute::CACHED_LINE_STARTS:
    case ax::mojom::IntListAttribute::WORD_STARTS:
    case ax::mojom::IntListAttribute::WORD_ENDS:
    case ax::mojom::IntListAttribute::CUSTOM_ACTION_IDS:
      return false;
  }

  NOTREACHED();
  return false;
}

AXNodeData::AXNodeData() = default;
AXNodeData::~AXNodeData() = default;

AXNodeData::AXNodeData(const AXNodeData& other) {
  id = other.id;
  role = other.role;
  state = other.state;
  actions = other.actions;
  string_attributes = other.string_attributes;
  int_attributes = other.int_attributes;
  float_attributes = other.float_attributes;
  bool_attributes = other.bool_attributes;
  intlist_attributes = other.intlist_attributes;
  stringlist_attributes = other.stringlist_attributes;
  html_attributes = other.html_attributes;
  child_ids = other.child_ids;
  location = other.location;
  offset_container_id = other.offset_container_id;
  if (other.transform)
    transform.reset(new gfx::Transform(*other.transform));
}

AXNodeData& AXNodeData::operator=(AXNodeData other) {
  id = other.id;
  role = other.role;
  state = other.state;
  actions = other.actions;
  string_attributes = other.string_attributes;
  int_attributes = other.int_attributes;
  float_attributes = other.float_attributes;
  bool_attributes = other.bool_attributes;
  intlist_attributes = other.intlist_attributes;
  stringlist_attributes = other.stringlist_attributes;
  html_attributes = other.html_attributes;
  child_ids = other.child_ids;
  location = other.location;
  offset_container_id = other.offset_container_id;
  if (other.transform)
    transform.reset(new gfx::Transform(*other.transform));
  else
    transform.reset(nullptr);
  return *this;
}

bool AXNodeData::HasBoolAttribute(ax::mojom::BoolAttribute attribute) const {
  auto iter = FindInVectorOfPairs(attribute, bool_attributes);
  return iter != bool_attributes.end();
}

bool AXNodeData::GetBoolAttribute(ax::mojom::BoolAttribute attribute) const {
  bool result;
  if (GetBoolAttribute(attribute, &result))
    return result;
  return false;
}

bool AXNodeData::GetBoolAttribute(
    ax::mojom::BoolAttribute attribute, bool* value) const {
  auto iter = FindInVectorOfPairs(attribute, bool_attributes);
  if (iter != bool_attributes.end()) {
    *value = iter->second;
    return true;
  }

  return false;
}

bool AXNodeData::HasFloatAttribute(ax::mojom::FloatAttribute attribute) const {
  auto iter = FindInVectorOfPairs(attribute, float_attributes);
  return iter != float_attributes.end();
}

float AXNodeData::GetFloatAttribute(ax::mojom::FloatAttribute attribute) const {
  float result;
  if (GetFloatAttribute(attribute, &result))
    return result;
  return 0.0;
}

bool AXNodeData::GetFloatAttribute(
    ax::mojom::FloatAttribute attribute, float* value) const {
  auto iter = FindInVectorOfPairs(attribute, float_attributes);
  if (iter != float_attributes.end()) {
    *value = iter->second;
    return true;
  }

  return false;
}

bool AXNodeData::HasIntAttribute(ax::mojom::IntAttribute attribute) const {
  auto iter = FindInVectorOfPairs(attribute, int_attributes);
  return iter != int_attributes.end();
}

int AXNodeData::GetIntAttribute(ax::mojom::IntAttribute attribute) const {
  int result;
  if (GetIntAttribute(attribute, &result))
    return result;
  return 0;
}

bool AXNodeData::GetIntAttribute(
    ax::mojom::IntAttribute attribute, int* value) const {
  auto iter = FindInVectorOfPairs(attribute, int_attributes);
  if (iter != int_attributes.end()) {
    *value = iter->second;
    return true;
  }

  return false;
}

bool AXNodeData::HasStringAttribute(ax::mojom::StringAttribute attribute) const {
  auto iter = FindInVectorOfPairs(attribute, string_attributes);
  return iter != string_attributes.end();
}

const std::string& AXNodeData::GetStringAttribute(
    ax::mojom::StringAttribute attribute) const {
  CR_DEFINE_STATIC_LOCAL(std::string, empty_string, ());
  auto iter = FindInVectorOfPairs(attribute, string_attributes);
  return iter != string_attributes.end() ? iter->second : empty_string;
}

bool AXNodeData::GetStringAttribute(
    ax::mojom::StringAttribute attribute, std::string* value) const {
  auto iter = FindInVectorOfPairs(attribute, string_attributes);
  if (iter != string_attributes.end()) {
    *value = iter->second;
    return true;
  }

  return false;
}

base::string16 AXNodeData::GetString16Attribute(
    ax::mojom::StringAttribute attribute) const {
  std::string value_utf8;
  if (!GetStringAttribute(attribute, &value_utf8))
    return base::string16();
  return base::UTF8ToUTF16(value_utf8);
}

bool AXNodeData::GetString16Attribute(
    ax::mojom::StringAttribute attribute,
    base::string16* value) const {
  std::string value_utf8;
  if (!GetStringAttribute(attribute, &value_utf8))
    return false;
  *value = base::UTF8ToUTF16(value_utf8);
  return true;
}

bool AXNodeData::HasIntListAttribute(ax::mojom::IntListAttribute attribute) const {
  auto iter = FindInVectorOfPairs(attribute, intlist_attributes);
  return iter != intlist_attributes.end();
}

const std::vector<int32_t>& AXNodeData::GetIntListAttribute(
    ax::mojom::IntListAttribute attribute) const {
  CR_DEFINE_STATIC_LOCAL(std::vector<int32_t>, empty_vector, ());
  auto iter = FindInVectorOfPairs(attribute, intlist_attributes);
  if (iter != intlist_attributes.end())
    return iter->second;
  return empty_vector;
}

bool AXNodeData::GetIntListAttribute(ax::mojom::IntListAttribute attribute,
                                     std::vector<int32_t>* value) const {
  auto iter = FindInVectorOfPairs(attribute, intlist_attributes);
  if (iter != intlist_attributes.end()) {
    *value = iter->second;
    return true;
  }

  return false;
}

bool AXNodeData::HasStringListAttribute(ax::mojom::StringListAttribute attribute) const {
  auto iter = FindInVectorOfPairs(attribute, stringlist_attributes);
  return iter != stringlist_attributes.end();
}

const std::vector<std::string>& AXNodeData::GetStringListAttribute(
    ax::mojom::StringListAttribute attribute) const {
  CR_DEFINE_STATIC_LOCAL(std::vector<std::string>, empty_vector, ());
  auto iter = FindInVectorOfPairs(attribute, stringlist_attributes);
  if (iter != stringlist_attributes.end())
    return iter->second;
  return empty_vector;
}

bool AXNodeData::GetStringListAttribute(ax::mojom::StringListAttribute attribute,
                                        std::vector<std::string>* value) const {
  auto iter = FindInVectorOfPairs(attribute, stringlist_attributes);
  if (iter != stringlist_attributes.end()) {
    *value = iter->second;
    return true;
  }

  return false;
}

bool AXNodeData::GetHtmlAttribute(
    const char* html_attr, std::string* value) const {
  for (size_t i = 0; i < html_attributes.size(); ++i) {
    const std::string& attr = html_attributes[i].first;
    if (base::LowerCaseEqualsASCII(attr, html_attr)) {
      *value = html_attributes[i].second;
      return true;
    }
  }

  return false;
}

bool AXNodeData::GetHtmlAttribute(
    const char* html_attr, base::string16* value) const {
  std::string value_utf8;
  if (!GetHtmlAttribute(html_attr, &value_utf8))
    return false;
  *value = base::UTF8ToUTF16(value_utf8);
  return true;
}

void AXNodeData::AddStringAttribute(
    ax::mojom::StringAttribute attribute, const std::string& value) {
  string_attributes.push_back(std::make_pair(attribute, value));
}

void AXNodeData::AddIntAttribute(
    ax::mojom::IntAttribute attribute, int value) {
  int_attributes.push_back(std::make_pair(attribute, value));
}

void AXNodeData::AddFloatAttribute(
    ax::mojom::FloatAttribute attribute, float value) {
  float_attributes.push_back(std::make_pair(attribute, value));
}

void AXNodeData::AddBoolAttribute(
    ax::mojom::BoolAttribute attribute, bool value) {
  bool_attributes.push_back(std::make_pair(attribute, value));
}

void AXNodeData::AddIntListAttribute(ax::mojom::IntListAttribute attribute,
                                     const std::vector<int32_t>& value) {
  intlist_attributes.push_back(std::make_pair(attribute, value));
}

void AXNodeData::AddStringListAttribute(ax::mojom::StringListAttribute attribute,
                                        const std::vector<std::string>& value) {
  stringlist_attributes.push_back(std::make_pair(attribute, value));
}

void AXNodeData::SetName(const std::string& name) {
  for (size_t i = 0; i < string_attributes.size(); ++i) {
    if (string_attributes[i].first == ax::mojom::StringAttribute::NAME) {
      string_attributes[i].second = name;
      return;
    }
  }

  string_attributes.push_back(std::make_pair(ax::mojom::StringAttribute::NAME, name));
}

void AXNodeData::SetName(const base::string16& name) {
  SetName(base::UTF16ToUTF8(name));
}

void AXNodeData::SetValue(const std::string& value) {
  for (size_t i = 0; i < string_attributes.size(); ++i) {
    if (string_attributes[i].first == ax::mojom::StringAttribute::VALUE) {
      string_attributes[i].second = value;
      return;
    }
  }

  string_attributes.push_back(std::make_pair(ax::mojom::StringAttribute::VALUE, value));
}

void AXNodeData::SetValue(const base::string16& value) {
  SetValue(base::UTF16ToUTF8(value));
}

bool AXNodeData::HasState(ax::mojom::State state_enum) const {
  return IsFlagSet(state, static_cast<uint32_t>(state_enum));
}

bool AXNodeData::HasAction(ax::mojom::Action action_enum) const {
  return IsFlagSet(actions, static_cast<uint32_t>(action_enum));
}

void AXNodeData::AddState(ax::mojom::State state_enum) {
  DCHECK_NE(state_enum, ax::mojom::State::NONE);
  state = ModifyFlag(state, static_cast<uint32_t>(state_enum), true);
}

void AXNodeData::AddAction(ax::mojom::Action action_enum) {
  switch (action_enum) {
    case ax::mojom::Action::NONE:
      NOTREACHED();
      break;

    // Note: all of the attributes are included here explicitly, rather than
    // using "default:", so that it's a compiler error to add a new action
    // without explicitly considering whether there are mutually exclusive
    // actions that can be performed on a UI control at the same time.
    case ax::mojom::Action::BLUR:
    case ax::mojom::Action::FOCUS: {
      ax::mojom::Action excluded_action =
          (action_enum == ax::mojom::Action::BLUR) ? ax::mojom::Action::FOCUS : ax::mojom::Action::BLUR;
      DCHECK(HasAction(excluded_action));
    } break;
    case ax::mojom::Action::CUSTOM_ACTION:
    case ax::mojom::Action::DECREMENT:
    case ax::mojom::Action::DO_DEFAULT:
    case ax::mojom::Action::GET_IMAGE_DATA:
    case ax::mojom::Action::HIT_TEST:
    case ax::mojom::Action::INCREMENT:
    case ax::mojom::Action::LOAD_INLINE_TEXT_BOXES:
    case ax::mojom::Action::REPLACE_SELECTED_TEXT:
    case ax::mojom::Action::SCROLL_TO_MAKE_VISIBLE:
    case ax::mojom::Action::SCROLL_TO_POINT:
    case ax::mojom::Action::SET_SCROLL_OFFSET:
    case ax::mojom::Action::SET_SELECTION:
    case ax::mojom::Action::SET_SEQUENTIAL_FOCUS_NAVIGATION_STARTING_POINT:
    case ax::mojom::Action::SET_VALUE:
    case ax::mojom::Action::SHOW_CONTEXT_MENU:
    case ax::mojom::Action::SCROLL_BACKWARD:
    case ax::mojom::Action::SCROLL_FORWARD:
    case ax::mojom::Action::SCROLL_UP:
    case ax::mojom::Action::SCROLL_DOWN:
    case ax::mojom::Action::SCROLL_LEFT:
    case ax::mojom::Action::SCROLL_RIGHT:
      break;
  }

  actions = ModifyFlag(actions, static_cast<uint32_t>(action_enum), true);
}

std::string AXNodeData::ToString() const {
  std::string result;

  result += "id=" + base::NumberToString(id);
  result += " ";
  result += ui::ToString(role);

  result += StateBitfieldToString(state);

  result += " (" + base::NumberToString(location.x()) + ", " +
            base::NumberToString(location.y()) + ")-(" +
            base::NumberToString(location.width()) + ", " +
            base::NumberToString(location.height()) + ")";

  if (offset_container_id != -1)
    result +=
        " offset_container_id=" + base::NumberToString(offset_container_id);

  if (transform && !transform->IsIdentity())
    result += " transform=" + transform->ToString();

  for (size_t i = 0; i < int_attributes.size(); ++i) {
    std::string value = base::NumberToString(int_attributes[i].second);
    switch (int_attributes[i].first) {
      case ax::mojom::IntAttribute::DEFAULT_ACTION_VERB:
        result +=
            " action=" +
            base::UTF16ToUTF8(ActionVerbToUnlocalizedString(
                static_cast<ax::mojom::DefaultActionVerb>(int_attributes[i].second)));
        break;
      case ax::mojom::IntAttribute::SCROLL_X:
        result += " scroll_x=" + value;
        break;
      case ax::mojom::IntAttribute::SCROLL_X_MIN:
        result += " scroll_x_min=" + value;
        break;
      case ax::mojom::IntAttribute::SCROLL_X_MAX:
        result += " scroll_x_max=" + value;
        break;
      case ax::mojom::IntAttribute::SCROLL_Y:
        result += " scroll_y=" + value;
        break;
      case ax::mojom::IntAttribute::SCROLL_Y_MIN:
        result += " scroll_y_min=" + value;
        break;
      case ax::mojom::IntAttribute::SCROLL_Y_MAX:
        result += " scroll_y_max=" + value;
        break;
      case ax::mojom::IntAttribute::HIERARCHICAL_LEVEL:
        result += " level=" + value;
        break;
      case ax::mojom::IntAttribute::TEXT_SEL_START:
        result += " sel_start=" + value;
        break;
      case ax::mojom::IntAttribute::TEXT_SEL_END:
        result += " sel_end=" + value;
        break;
      case ax::mojom::IntAttribute::ARIA_COLUMN_COUNT:
        result += " aria_column_count=" + value;
        break;
      case ax::mojom::IntAttribute::ARIA_CELL_COLUMN_INDEX:
        result += " aria_cell_column_index=" + value;
        break;
      case ax::mojom::IntAttribute::ARIA_ROW_COUNT:
        result += " aria_row_count=" + value;
        break;
      case ax::mojom::IntAttribute::ARIA_CELL_ROW_INDEX:
        result += " aria_cell_row_index=" + value;
        break;
      case ax::mojom::IntAttribute::TABLE_ROW_COUNT:
        result += " rows=" + value;
        break;
      case ax::mojom::IntAttribute::TABLE_COLUMN_COUNT:
        result += " cols=" + value;
        break;
      case ax::mojom::IntAttribute::TABLE_CELL_COLUMN_INDEX:
        result += " col=" + value;
        break;
      case ax::mojom::IntAttribute::TABLE_CELL_ROW_INDEX:
        result += " row=" + value;
        break;
      case ax::mojom::IntAttribute::TABLE_CELL_COLUMN_SPAN:
        result += " colspan=" + value;
        break;
      case ax::mojom::IntAttribute::TABLE_CELL_ROW_SPAN:
        result += " rowspan=" + value;
        break;
      case ax::mojom::IntAttribute::TABLE_COLUMN_HEADER_ID:
        result += " column_header_id=" + value;
        break;
      case ax::mojom::IntAttribute::TABLE_COLUMN_INDEX:
        result += " column_index=" + value;
        break;
      case ax::mojom::IntAttribute::TABLE_HEADER_ID:
        result += " header_id=" + value;
        break;
      case ax::mojom::IntAttribute::TABLE_ROW_HEADER_ID:
        result += " row_header_id=" + value;
        break;
      case ax::mojom::IntAttribute::TABLE_ROW_INDEX:
        result += " row_index=" + value;
        break;
      case ax::mojom::IntAttribute::SORT_DIRECTION:
        switch (static_cast<ax::mojom::SortDirection>(int_attributes[i].second)) {
          case ax::mojom::SortDirection::UNSORTED:
            result += " sort_direction=none";
            break;
          case ax::mojom::SortDirection::ASCENDING:
            result += " sort_direction=ascending";
            break;
          case ax::mojom::SortDirection::DESCENDING:
            result += " sort_direction=descending";
            break;
          case ax::mojom::SortDirection::OTHER:
            result += " sort_direction=other";
            break;
          default:
            break;
        }
        break;
      case ax::mojom::IntAttribute::NAME_FROM:
        result += " name_from=";
        result +=
            ui::ToString(static_cast<ax::mojom::NameFrom>(int_attributes[i].second));
        break;
      case ax::mojom::IntAttribute::DESCRIPTION_FROM:
        result += " description_from=";
        result += ui::ToString(
            static_cast<ax::mojom::DescriptionFrom>(int_attributes[i].second));
        break;
      case ax::mojom::IntAttribute::ACTIVEDESCENDANT_ID:
        result += " activedescendant=" + value;
        break;
      case ax::mojom::IntAttribute::DETAILS_ID:
        result += " details=" + value;
        break;
      case ax::mojom::IntAttribute::ERRORMESSAGE_ID:
        result += " errormessage=" + value;
        break;
      case ax::mojom::IntAttribute::IN_PAGE_LINK_TARGET_ID:
        result += " in_page_link_target_id=" + value;
        break;
      case ax::mojom::IntAttribute::MEMBER_OF_ID:
        result += " member_of_id=" + value;
        break;
      case ax::mojom::IntAttribute::NEXT_ON_LINE_ID:
        result += " next_on_line_id=" + value;
        break;
      case ax::mojom::IntAttribute::PREVIOUS_ON_LINE_ID:
        result += " previous_on_line_id=" + value;
        break;
      case ax::mojom::IntAttribute::CHILD_TREE_ID:
        result += " child_tree_id=" + value;
        break;
      case ax::mojom::IntAttribute::COLOR_VALUE:
        result += base::StringPrintf(" color_value=&%X",
                                     int_attributes[i].second);
        break;
      case ax::mojom::IntAttribute::ARIA_CURRENT_STATE:
        switch (static_cast<ax::mojom::AriaCurrentState>(int_attributes[i].second)) {
          case ax::mojom::AriaCurrentState::FALSE_VALUE:
            result += " aria_current_state=false";
            break;
          case ax::mojom::AriaCurrentState::TRUE_VALUE:
            result += " aria_current_state=true";
            break;
          case ax::mojom::AriaCurrentState::PAGE:
            result += " aria_current_state=page";
            break;
          case ax::mojom::AriaCurrentState::STEP:
            result += " aria_current_state=step";
            break;
          case ax::mojom::AriaCurrentState::LOCATION:
            result += " aria_current_state=location";
            break;
          case ax::mojom::AriaCurrentState::DATE:
            result += " aria_current_state=date";
            break;
          case ax::mojom::AriaCurrentState::TIME:
            result += " aria_current_state=time";
            break;
          default:
            break;
        }
        break;
      case ax::mojom::IntAttribute::BACKGROUND_COLOR:
        result += base::StringPrintf(" background_color=&%X",
                                     int_attributes[i].second);
        break;
      case ax::mojom::IntAttribute::COLOR:
        result += base::StringPrintf(" color=&%X", int_attributes[i].second);
        break;
      case ax::mojom::IntAttribute::TEXT_DIRECTION:
        switch (static_cast<ax::mojom::TextDirection>(int_attributes[i].second)) {
          case ax::mojom::TextDirection::LTR:
            result += " text_direction=ltr";
            break;
          case ax::mojom::TextDirection::RTL:
            result += " text_direction=rtl";
            break;
          case ax::mojom::TextDirection::TTB:
            result += " text_direction=ttb";
            break;
          case ax::mojom::TextDirection::BTT:
            result += " text_direction=btt";
            break;
          default:
            break;
        }
        break;
      case ax::mojom::IntAttribute::TEXT_STYLE: {
        int32_t text_style = int_attributes[i].second;
        if (text_style == static_cast<int32_t>(ax::mojom::TextStyle::NONE))
          break;
        std::string text_style_value(" text_style=");
        if (text_style & static_cast<int32_t>(ax::mojom::TextStyle::TEXT_STYLE_BOLD))
          text_style_value += "bold,";
        if (text_style & static_cast<int32_t>(ax::mojom::TextStyle::TEXT_STYLE_ITALIC))
          text_style_value += "italic,";
        if (text_style & static_cast<int32_t>(ax::mojom::TextStyle::TEXT_STYLE_UNDERLINE))
          text_style_value += "underline,";
        if (text_style & static_cast<int32_t>(ax::mojom::TextStyle::TEXT_STYLE_LINE_THROUGH))
          text_style_value += "line-through,";
        result += text_style_value.substr(0, text_style_value.size() - 1);
        break;
      }
      case ax::mojom::IntAttribute::SET_SIZE:
        result += " setsize=" + value;
        break;
      case ax::mojom::IntAttribute::POS_IN_SET:
        result += " posinset=" + value;
        break;
      case ax::mojom::IntAttribute::INVALID_STATE:
        switch (static_cast<ax::mojom::InvalidState>(int_attributes[i].second)) {
          case ax::mojom::InvalidState::FALSE_VALUE:
            result += " invalid_state=false";
            break;
          case ax::mojom::InvalidState::TRUE_VALUE:
            result += " invalid_state=true";
            break;
          case ax::mojom::InvalidState::SPELLING:
            result += " invalid_state=spelling";
            break;
          case ax::mojom::InvalidState::GRAMMAR:
            result += " invalid_state=grammar";
            break;
          case ax::mojom::InvalidState::OTHER:
            result += " invalid_state=other";
            break;
          default:
            break;
        }
        break;
      case ax::mojom::IntAttribute::CHECKED_STATE:
        switch (static_cast<ax::mojom::CheckedState>(int_attributes[i].second)) {
          case ax::mojom::CheckedState::FALSE_VALUE:
            result += " checked_state=false";
            break;
          case ax::mojom::CheckedState::TRUE_VALUE:
            result += " checked_state=true";
            break;
          case ax::mojom::CheckedState::MIXED:
            result += " checked_state=mixed";
            break;
          default:
            break;
        }
        break;
      case ax::mojom::IntAttribute::RESTRICTION:
        switch (static_cast<ax::mojom::Restriction>(int_attributes[i].second)) {
          case ax::mojom::Restriction::READ_ONLY:
            result += " restriction=readonly";
            break;
          case ax::mojom::Restriction::DISABLED:
            result += " restriction=disabled";
            break;
          default:
            break;
        }
        break;
      case ax::mojom::IntAttribute::NEXT_FOCUS_ID:
        result += " next_focus_id=" + value;
        break;
      case ax::mojom::IntAttribute::PREVIOUS_FOCUS_ID:
        result += " previous_focus_id=" + value;
        break;
      case ax::mojom::IntAttribute::NONE:
        break;
    }
  }

  for (size_t i = 0; i < string_attributes.size(); ++i) {
    std::string value = string_attributes[i].second;
    switch (string_attributes[i].first) {
      case ax::mojom::StringAttribute::ACCESS_KEY:
        result += " access_key=" + value;
        break;
      case ax::mojom::StringAttribute::ARIA_INVALID_VALUE:
        result += " aria_invalid_value=" + value;
        break;
      case ax::mojom::StringAttribute::AUTO_COMPLETE:
        result += " autocomplete=" + value;
        break;
      case ax::mojom::StringAttribute::CHROME_CHANNEL:
        result += " chrome_channel=" + value;
        break;
      case ax::mojom::StringAttribute::CLASS_NAME:
        result += " class_name=" + value;
        break;
      case ax::mojom::StringAttribute::DESCRIPTION:
        result += " description=" + value;
        break;
      case ax::mojom::StringAttribute::DISPLAY:
        result += " display=" + value;
        break;
      case ax::mojom::StringAttribute::FONT_FAMILY:
        result += " font-family=" + value;
        break;
      case ax::mojom::StringAttribute::HTML_TAG:
        result += " html_tag=" + value;
        break;
      case ax::mojom::StringAttribute::IMAGE_DATA_URL:
        result += " image_data_url=(" +
                  base::NumberToString(static_cast<int>(value.size())) +
                  " bytes)";
        break;
      case ax::mojom::StringAttribute::INNER_HTML:
        result += " inner_html=" + value;
        break;
      case ax::mojom::StringAttribute::KEY_SHORTCUTS:
        result += " key_shortcuts=" + value;
        break;
      case ax::mojom::StringAttribute::LANGUAGE:
        result += " language=" + value;
        break;
      case ax::mojom::StringAttribute::LIVE_RELEVANT:
        result += " relevant=" + value;
        break;
      case ax::mojom::StringAttribute::LIVE_STATUS:
        result += " live=" + value;
        break;
      case ax::mojom::StringAttribute::CONTAINER_LIVE_RELEVANT:
        result += " container_relevant=" + value;
        break;
      case ax::mojom::StringAttribute::CONTAINER_LIVE_STATUS:
        result += " container_live=" + value;
        break;
      case ax::mojom::StringAttribute::PLACEHOLDER:
        result += " placeholder=" + value;
        break;
      case ax::mojom::StringAttribute::ROLE:
        result += " role=" + value;
        break;
      case ax::mojom::StringAttribute::ROLE_DESCRIPTION:
        result += " role_description=" + value;
        break;
      case ax::mojom::StringAttribute::URL:
        result += " url=" + value;
        break;
      case ax::mojom::StringAttribute::NAME:
        result += " name=" + value;
        break;
      case ax::mojom::StringAttribute::VALUE:
        result += " value=" + value;
        break;
      case ax::mojom::StringAttribute::NONE:
        break;
    }
  }

  for (size_t i = 0; i < float_attributes.size(); ++i) {
    std::string value = base::NumberToString(float_attributes[i].second);
    switch (float_attributes[i].first) {
      case ax::mojom::FloatAttribute::VALUE_FOR_RANGE:
        result += " value_for_range=" + value;
        break;
      case ax::mojom::FloatAttribute::MAX_VALUE_FOR_RANGE:
        result += " max_value=" + value;
        break;
      case ax::mojom::FloatAttribute::MIN_VALUE_FOR_RANGE:
        result += " min_value=" + value;
        break;
      case ax::mojom::FloatAttribute::STEP_VALUE_FOR_RANGE:
        result += " step_value=" + value;
        break;
      case ax::mojom::FloatAttribute::FONT_SIZE:
        result += " font_size=" + value;
        break;
      case ax::mojom::FloatAttribute::NONE:
        break;
    }
  }

  for (size_t i = 0; i < bool_attributes.size(); ++i) {
    std::string value = bool_attributes[i].second ? "true" : "false";
    switch (bool_attributes[i].first) {
      case ax::mojom::BoolAttribute::EDITABLE_ROOT:
        result += " editable_root=" + value;
        break;
      case ax::mojom::BoolAttribute::LIVE_ATOMIC:
        result += " atomic=" + value;
        break;
      case ax::mojom::BoolAttribute::BUSY:
        result += " busy=" + value;
        break;
      case ax::mojom::BoolAttribute::CONTAINER_LIVE_ATOMIC:
        result += " container_atomic=" + value;
        break;
      case ax::mojom::BoolAttribute::CONTAINER_LIVE_BUSY:
        result += " container_busy=" + value;
        break;
      case ax::mojom::BoolAttribute::UPDATE_LOCATION_ONLY:
        result += " update_location_only=" + value;
        break;
      case ax::mojom::BoolAttribute::CANVAS_HAS_FALLBACK:
        result += " has_fallback=" + value;
        break;
      case ax::mojom::BoolAttribute::MODAL:
        result += " modal=" + value;
        break;
      case ax::mojom::BoolAttribute::SCROLLABLE:
        result += " scrollable=" + value;
        break;
      case ax::mojom::BoolAttribute::CLICKABLE:
        result += " clickable=" + value;
        break;
      case ax::mojom::BoolAttribute::CLIPS_CHILDREN:
        result += " clips_children=" + value;
        break;
      case ax::mojom::BoolAttribute::NONE:
        break;
    }
  }

  for (size_t i = 0; i < intlist_attributes.size(); ++i) {
    const std::vector<int32_t>& values = intlist_attributes[i].second;
    switch (intlist_attributes[i].first) {
      case ax::mojom::IntListAttribute::INDIRECT_CHILD_IDS:
        result += " indirect_child_ids=" + IntVectorToString(values);
        break;
      case ax::mojom::IntListAttribute::CONTROLS_IDS:
        result += " controls_ids=" + IntVectorToString(values);
        break;
      case ax::mojom::IntListAttribute::DESCRIBEDBY_IDS:
        result += " describedby_ids=" + IntVectorToString(values);
        break;
      case ax::mojom::IntListAttribute::FLOWTO_IDS:
        result += " flowto_ids=" + IntVectorToString(values);
        break;
      case ax::mojom::IntListAttribute::LABELLEDBY_IDS:
        result += " labelledby_ids=" + IntVectorToString(values);
        break;
      case ax::mojom::IntListAttribute::RADIO_GROUP_IDS:
        result += " radio_group_ids=" + IntVectorToString(values);
        break;
      case ax::mojom::IntListAttribute::LINE_BREAKS:
        result += " line_breaks=" + IntVectorToString(values);
        break;
      case ax::mojom::IntListAttribute::MARKER_TYPES: {
        std::string types_str;
        for (size_t i = 0; i < values.size(); ++i) {
          int32_t type = values[i];
          if (type == static_cast<int32_t>(ax::mojom::MarkerType::NONE))
            continue;

          if (i > 0)
            types_str += ',';

          if (type & static_cast<int32_t>(ax::mojom::MarkerType::SPELLING))
            types_str += "spelling&";
          if (type & static_cast<int32_t>(ax::mojom::MarkerType::GRAMMAR))
            types_str += "grammar&";
          if (type & static_cast<int32_t>(ax::mojom::MarkerType::TEXT_MATCH))
            types_str += "text_match&";
          if (type & static_cast<int32_t>(ax::mojom::MarkerType::ACTIVE_SUGGESTION))
            types_str += "active_suggestion&";
          if (type & static_cast<int32_t>(ax::mojom::MarkerType::SUGGESTION))
            types_str += "suggestion&";

          if (!types_str.empty())
            types_str = types_str.substr(0, types_str.size() - 1);
        }

        if (!types_str.empty())
          result += " marker_types=" + types_str;
        break;
      }
      case ax::mojom::IntListAttribute::MARKER_STARTS:
        result += " marker_starts=" + IntVectorToString(values);
        break;
      case ax::mojom::IntListAttribute::MARKER_ENDS:
        result += " marker_ends=" + IntVectorToString(values);
        break;
      case ax::mojom::IntListAttribute::CELL_IDS:
        result += " cell_ids=" + IntVectorToString(values);
        break;
      case ax::mojom::IntListAttribute::UNIQUE_CELL_IDS:
        result += " unique_cell_ids=" + IntVectorToString(values);
        break;
      case ax::mojom::IntListAttribute::CHARACTER_OFFSETS:
        result += " character_offsets=" + IntVectorToString(values);
        break;
      case ax::mojom::IntListAttribute::CACHED_LINE_STARTS:
        result += " cached_line_start_offsets=" + IntVectorToString(values);
        break;
      case ax::mojom::IntListAttribute::WORD_STARTS:
        result += " word_starts=" + IntVectorToString(values);
        break;
      case ax::mojom::IntListAttribute::WORD_ENDS:
        result += " word_ends=" + IntVectorToString(values);
        break;
      case ax::mojom::IntListAttribute::CUSTOM_ACTION_IDS:
        result += " custom_action_ids=" + IntVectorToString(values);
        break;
      case ax::mojom::IntListAttribute::NONE:
        break;
    }
  }

  for (size_t i = 0; i < stringlist_attributes.size(); ++i) {
    const std::vector<std::string>& values = stringlist_attributes[i].second;
    switch (stringlist_attributes[i].first) {
      case ax::mojom::StringListAttribute::CUSTOM_ACTION_DESCRIPTIONS:
        result +=
            " custom_action_descriptions: " + base::JoinString(values, ",");
        break;
      case ax::mojom::StringListAttribute::NONE:
        break;
    }
  }

  result += " actions=" + ActionsBitfieldToString(actions);

  if (!child_ids.empty())
    result += " child_ids=" + IntVectorToString(child_ids);

  return result;
}

}  // namespace ui
