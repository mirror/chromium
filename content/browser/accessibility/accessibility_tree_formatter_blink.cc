// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/accessibility/accessibility_tree_formatter_blink.h"

#include <math.h>
#include <stddef.h>

#include <utility>

#include "base/memory/ptr_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "content/browser/accessibility/browser_accessibility_manager.h"
#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/gfx/geometry/rect_conversions.h"
#include "ui/gfx/transform.h"

namespace content {

AccessibilityTreeFormatterBlink::AccessibilityTreeFormatterBlink()
    : AccessibilityTreeFormatterBrowser() {}

AccessibilityTreeFormatterBlink::~AccessibilityTreeFormatterBlink() {
}

const char* const TREE_DATA_ATTRIBUTES[] = {"TreeData.textSelStartOffset",
                                            "TreeData.textSelEndOffset"};

const char* STATE_FOCUSED = "focused";
const char* STATE_OFFSCREEN = "offscreen";

uint32_t AccessibilityTreeFormatterBlink::ChildCount(
    const BrowserAccessibility& node) const {
  if (node.HasIntAttribute(ax::mojom::IntAttribute::CHILD_TREE_ID))
    return node.PlatformChildCount();
  else
    return node.InternalChildCount();
}

BrowserAccessibility* AccessibilityTreeFormatterBlink::GetChild(
    const BrowserAccessibility& node,
    uint32_t i) const {
  if (node.HasIntAttribute(ax::mojom::IntAttribute::CHILD_TREE_ID))
    return node.PlatformGetChild(i);
  else
    return node.InternalGetChild(i);
}

// TODO(aleventhal) Convert ax enums to friendly strings, e.g. ax::mojom::CheckedState.
std::string AccessibilityTreeFormatterBlink::IntAttrToString(
    const BrowserAccessibility& node,
    ax::mojom::IntAttribute attr,
    int value) const {
  if (ui::IsNodeIdIntAttribute(attr)) {
    // Relation
    BrowserAccessibility* target = node.manager()->GetFromID(value);
    return target ? ui::ToString(target->GetData().role) : std::string("null");
  }

  switch (attr) {
    case ax::mojom::IntAttribute::ARIA_CURRENT_STATE:
      return ui::ToString(static_cast<ax::mojom::AriaCurrentState>(value));
    case ax::mojom::IntAttribute::CHECKED_STATE:
      return ui::ToString(static_cast<ax::mojom::CheckedState>(value));
    case ax::mojom::IntAttribute::DEFAULT_ACTION_VERB:
      return ui::ToString(static_cast<ax::mojom::DefaultActionVerb>(value));
    case ax::mojom::IntAttribute::DESCRIPTION_FROM:
      return ui::ToString(static_cast<ax::mojom::DescriptionFrom>(value));
    case ax::mojom::IntAttribute::INVALID_STATE:
      return ui::ToString(static_cast<ax::mojom::InvalidState>(value));
    case ax::mojom::IntAttribute::NAME_FROM:
      return ui::ToString(static_cast<ax::mojom::NameFrom>(value));
    case ax::mojom::IntAttribute::RESTRICTION:
      return ui::ToString(static_cast<ax::mojom::Restriction>(value));
    case ax::mojom::IntAttribute::SORT_DIRECTION:
      return ui::ToString(static_cast<ax::mojom::SortDirection>(value));
    case ax::mojom::IntAttribute::TEXT_DIRECTION:
      return ui::ToString(static_cast<ax::mojom::TextDirection>(value));
    // No pretty printing necessary for these:
    case ax::mojom::IntAttribute::ACTIVEDESCENDANT_ID:
    case ax::mojom::IntAttribute::ARIA_CELL_COLUMN_INDEX:
    case ax::mojom::IntAttribute::ARIA_CELL_ROW_INDEX:
    case ax::mojom::IntAttribute::ARIA_COLUMN_COUNT:
    case ax::mojom::IntAttribute::ARIA_ROW_COUNT:
    case ax::mojom::IntAttribute::BACKGROUND_COLOR:
    case ax::mojom::IntAttribute::CHILD_TREE_ID:
    case ax::mojom::IntAttribute::COLOR:
    case ax::mojom::IntAttribute::COLOR_VALUE:
    case ax::mojom::IntAttribute::DETAILS_ID:
    case ax::mojom::IntAttribute::ERRORMESSAGE_ID:
    case ax::mojom::IntAttribute::HIERARCHICAL_LEVEL:
    case ax::mojom::IntAttribute::IN_PAGE_LINK_TARGET_ID:
    case ax::mojom::IntAttribute::MEMBER_OF_ID:
    case ax::mojom::IntAttribute::NEXT_FOCUS_ID:
    case ax::mojom::IntAttribute::NEXT_ON_LINE_ID:
    case ax::mojom::IntAttribute::POS_IN_SET:
    case ax::mojom::IntAttribute::PREVIOUS_FOCUS_ID:
    case ax::mojom::IntAttribute::PREVIOUS_ON_LINE_ID:
    case ax::mojom::IntAttribute::SCROLL_X:
    case ax::mojom::IntAttribute::SCROLL_X_MAX:
    case ax::mojom::IntAttribute::SCROLL_X_MIN:
    case ax::mojom::IntAttribute::SCROLL_Y:
    case ax::mojom::IntAttribute::SCROLL_Y_MAX:
    case ax::mojom::IntAttribute::SCROLL_Y_MIN:
    case ax::mojom::IntAttribute::SET_SIZE:
    case ax::mojom::IntAttribute::TABLE_CELL_COLUMN_INDEX:
    case ax::mojom::IntAttribute::TABLE_CELL_COLUMN_SPAN:
    case ax::mojom::IntAttribute::TABLE_CELL_ROW_INDEX:
    case ax::mojom::IntAttribute::TABLE_CELL_ROW_SPAN:
    case ax::mojom::IntAttribute::TABLE_COLUMN_COUNT:
    case ax::mojom::IntAttribute::TABLE_COLUMN_HEADER_ID:
    case ax::mojom::IntAttribute::TABLE_COLUMN_INDEX:
    case ax::mojom::IntAttribute::TABLE_HEADER_ID:
    case ax::mojom::IntAttribute::TABLE_ROW_COUNT:
    case ax::mojom::IntAttribute::TABLE_ROW_HEADER_ID:
    case ax::mojom::IntAttribute::TABLE_ROW_INDEX:
    case ax::mojom::IntAttribute::TEXT_SEL_END:
    case ax::mojom::IntAttribute::TEXT_SEL_START:
    case ax::mojom::IntAttribute::TEXT_STYLE:
    case ax::mojom::IntAttribute::NONE:
      break;
  }

  // Just return the number
  return std::to_string(value);
}

void AccessibilityTreeFormatterBlink::AddProperties(
    const BrowserAccessibility& node,
    base::DictionaryValue* dict) {
  int id = node.GetId();
  dict->SetInteger("id", id);

  dict->SetString("internalRole", ui::ToString(node.GetData().role));

  gfx::Rect bounds = gfx::ToEnclosingRect(node.GetData().location);
  dict->SetInteger("boundsX", bounds.x());
  dict->SetInteger("boundsY", bounds.y());
  dict->SetInteger("boundsWidth", bounds.width());
  dict->SetInteger("boundsHeight", bounds.height());

  bool offscreen = false;
  gfx::Rect page_bounds = node.GetPageBoundsRect(&offscreen);
  dict->SetInteger("pageBoundsX", page_bounds.x());
  dict->SetInteger("pageBoundsY", page_bounds.y());
  dict->SetInteger("pageBoundsWidth", page_bounds.width());
  dict->SetInteger("pageBoundsHeight", page_bounds.height());

  dict->SetBoolean("transform",
                   node.GetData().transform &&
                   !node.GetData().transform->IsIdentity());

  gfx::Rect unclipped_bounds = node.GetPageBoundsRect(&offscreen, false);
  dict->SetInteger("unclippedBoundsX", unclipped_bounds.x());
  dict->SetInteger("unclippedBoundsY", unclipped_bounds.y());
  dict->SetInteger("unclippedBoundsWidth", unclipped_bounds.width());
  dict->SetInteger("unclippedBoundsHeight", unclipped_bounds.height());

  for (int32_t state_index = static_cast<int32_t>(ax::mojom::State::NONE);
       state_index <= static_cast<int32_t>(ax::mojom::State::LAST); ++state_index) {
    auto state = static_cast<ax::mojom::State>(state_index);
    if (node.HasState(state))
      dict->SetBoolean(ui::ToString(state), true);
  }

  if (offscreen)
    dict->SetBoolean(STATE_OFFSCREEN, true);

  for (int32_t attr_index = static_cast<int32_t>(ax::mojom::StringAttribute::NONE);
       attr_index <= static_cast<int32_t>(ax::mojom::StringAttribute::LAST);
       ++attr_index) {
    auto attr = static_cast<ax::mojom::StringAttribute>(attr_index);
    if (node.HasStringAttribute(attr))
      dict->SetString(ui::ToString(attr), node.GetStringAttribute(attr));
  }

  for (int32_t attr_index = static_cast<int32_t>(ax::mojom::IntAttribute::NONE);
       attr_index <= static_cast<int32_t>(ax::mojom::IntAttribute::LAST);
       ++attr_index) {
    auto attr = static_cast<ax::mojom::IntAttribute>(attr_index);
    if (node.HasIntAttribute(attr)) {
      int value = node.GetIntAttribute(attr);
      dict->SetString(ui::ToString(attr), IntAttrToString(node, attr, value));
    }
  }

  for (int32_t attr_index = static_cast<int32_t>(ax::mojom::FloatAttribute::NONE);
       attr_index <= static_cast<int32_t>(ax::mojom::FloatAttribute::LAST);
       ++attr_index) {
    auto attr = static_cast<ax::mojom::FloatAttribute>(attr_index);
    if (node.HasFloatAttribute(attr) && isfinite(node.GetFloatAttribute(attr)))
      dict->SetDouble(ui::ToString(attr), node.GetFloatAttribute(attr));
  }

  for (int32_t attr_index = static_cast<int32_t>(ax::mojom::BoolAttribute::NONE);
       attr_index <= static_cast<int32_t>(ax::mojom::BoolAttribute::LAST);
       ++attr_index) {
    auto attr = static_cast<ax::mojom::BoolAttribute>(attr_index);
    if (node.HasBoolAttribute(attr))
      dict->SetBoolean(ui::ToString(attr), node.GetBoolAttribute(attr));
  }

  for (int32_t attr_index =
           static_cast<int32_t>(ax::mojom::IntListAttribute::NONE);
       attr_index <= static_cast<int32_t>(ax::mojom::IntListAttribute::LAST);
       ++attr_index) {
    auto attr = static_cast<ax::mojom::IntListAttribute>(attr_index);
    if (node.HasIntListAttribute(attr)) {
      std::vector<int32_t> values;
      node.GetIntListAttribute(attr, &values);
      auto value_list = std::make_unique<base::ListValue>();
      for (size_t i = 0; i < values.size(); ++i) {
        if (ui::IsNodeIdIntListAttribute(attr)) {
          BrowserAccessibility* target = node.manager()->GetFromID(values[i]);
          if (target)
            value_list->AppendString(ui::ToString(target->GetData().role));
          else
            value_list->AppendString("null");
        } else {
          value_list->AppendInteger(values[i]);
        }
      }
      dict->Set(ui::ToString(attr), std::move(value_list));
    }
  }

  //  Check for relevant rich text selection info in AXTreeData
  int anchor_id = node.manager()->GetTreeData().sel_anchor_object_id;
  if (id == anchor_id) {
    int anchor_offset = node.manager()->GetTreeData().sel_anchor_offset;
    dict->SetInteger("TreeData.textSelStartOffset", anchor_offset);
  }
  int focus_id = node.manager()->GetTreeData().sel_focus_object_id;
  if (id == focus_id) {
    int focus_offset = node.manager()->GetTreeData().sel_focus_offset;
    dict->SetInteger("TreeData.textSelEndOffset", focus_offset);
  }

  std::vector<std::string> actions_strings;
  for (int32_t action_index = static_cast<int32_t>(ax::mojom::Action::NONE) + 1;
       action_index <= static_cast<int32_t>(ax::mojom::Action::LAST);
       ++action_index) {
    auto action = static_cast<ax::mojom::Action>(action_index);
    if (node.HasAction(action))
      actions_strings.push_back(ui::ToString(action));
  }
  if (!actions_strings.empty())
    dict->SetString("actions", base::JoinString(actions_strings, ","));
}

base::string16 AccessibilityTreeFormatterBlink::ProcessTreeForOutput(
    const base::DictionaryValue& dict,
    base::DictionaryValue* filtered_dict_result) {
  base::string16 error_value;
  if (dict.GetString("error", &error_value))
    return error_value;

  base::string16 line;

  if (show_ids()) {
    int id_value;
    dict.GetInteger("id", &id_value);
    WriteAttribute(true, base::IntToString16(id_value), &line);
  }

  base::string16 role_value;
  dict.GetString("internalRole", &role_value);
  WriteAttribute(true, base::UTF16ToUTF8(role_value), &line);

  for (int state_index = static_cast<int32_t>(ax::mojom::State::NONE);
       state_index <= static_cast<int32_t>(ax::mojom::State::LAST); ++state_index) {
    auto state = static_cast<ax::mojom::State>(state_index);
    const base::Value* value;
    if (!dict.Get(ui::ToString(state), &value))
      continue;

    WriteAttribute(false, ui::ToString(state), &line);
  }

  // Offscreen and Focused states are not in the state list.
  bool offscreen = false;
  dict.GetBoolean(STATE_OFFSCREEN, &offscreen);
  if (offscreen)
    WriteAttribute(false, STATE_OFFSCREEN, &line);
  bool focused = false;
  dict.GetBoolean(STATE_FOCUSED, &focused);
  if (focused)
    WriteAttribute(false, STATE_FOCUSED, &line);

  WriteAttribute(false,
                 FormatCoordinates("location", "boundsX", "boundsY", dict),
                 &line);
  WriteAttribute(false,
                 FormatCoordinates("size", "boundsWidth", "boundsHeight", dict),
                 &line);

  WriteAttribute(false,
                 FormatCoordinates("pageLocation",
                                   "pageBoundsX", "pageBoundsY", dict),
                 &line);
  WriteAttribute(false,
                 FormatCoordinates("pageSize",
                                   "pageBoundsWidth", "pageBoundsHeight", dict),
                 &line);
  WriteAttribute(false,
                 FormatCoordinates("unclippedLocation", "unclippedBoundsX",
                                   "unclippedBoundsY", dict),
                 &line);
  WriteAttribute(false,
                 FormatCoordinates("unclippedSize", "unclippedBoundsWidth",
                                   "unclippedBoundsHeight", dict),
                 &line);

  bool transform;
  if (dict.GetBoolean("transform", &transform) && transform)
    WriteAttribute(false, "transform", &line);

  for (int attr_index = static_cast<int32_t>(ax::mojom::StringAttribute::NONE);
       attr_index <= static_cast<int32_t>(ax::mojom::StringAttribute::LAST);
       ++attr_index) {
    auto attr = static_cast<ax::mojom::StringAttribute>(attr_index);
    std::string string_value;
    if (!dict.GetString(ui::ToString(attr), &string_value))
      continue;
    WriteAttribute(
        false,
        base::StringPrintf("%s='%s'", ui::ToString(attr), string_value.c_str()),
        &line);
  }

  for (int attr_index = static_cast<int32_t>(ax::mojom::IntAttribute::NONE);
       attr_index <= static_cast<int32_t>(ax::mojom::IntAttribute::LAST);
       ++attr_index) {
    auto attr = static_cast<ax::mojom::IntAttribute>(attr_index);
    std::string string_value;
    if (!dict.GetString(ui::ToString(attr), &string_value))
      continue;
    WriteAttribute(
        false,
        base::StringPrintf("%s=%s", ui::ToString(attr), string_value.c_str()),
        &line);
  }

  for (int attr_index = static_cast<int32_t>(ax::mojom::BoolAttribute::NONE);
       attr_index <= static_cast<int32_t>(ax::mojom::BoolAttribute::LAST);
       ++attr_index) {
    auto attr = static_cast<ax::mojom::BoolAttribute>(attr_index);
    bool bool_value;
    if (!dict.GetBoolean(ui::ToString(attr), &bool_value))
      continue;
    WriteAttribute(false,
                   base::StringPrintf("%s=%s", ui::ToString(attr),
                                      bool_value ? "true" : "false"),
                   &line);
  }

  for (int attr_index = static_cast<int32_t>(ax::mojom::FloatAttribute::NONE);
       attr_index <= static_cast<int32_t>(ax::mojom::FloatAttribute::LAST);
       ++attr_index) {
    auto attr = static_cast<ax::mojom::FloatAttribute>(attr_index);
    double float_value;
    if (!dict.GetDouble(ui::ToString(attr), &float_value))
      continue;
    WriteAttribute(
        false, base::StringPrintf("%s=%.2f", ui::ToString(attr), float_value),
        &line);
  }

  for (int attr_index = static_cast<int32_t>(ax::mojom::IntListAttribute::NONE);
       attr_index <= static_cast<int32_t>(ax::mojom::IntListAttribute::LAST);
       ++attr_index) {
    auto attr = static_cast<ax::mojom::IntListAttribute>(attr_index);
    const base::ListValue* value;
    if (!dict.GetList(ui::ToString(attr), &value))
      continue;
    std::string attr_string(ui::ToString(attr));
    attr_string.push_back('=');
    for (size_t i = 0; i < value->GetSize(); ++i) {
      if (i > 0)
        attr_string += ",";
      if (ui::IsNodeIdIntListAttribute(attr)) {
        std::string string_value;
        value->GetString(i, &string_value);
        attr_string += string_value;
      } else {
        int int_value;
        value->GetInteger(i, &int_value);
        attr_string += base::IntToString(int_value);
      }
    }
    WriteAttribute(false, attr_string, &line);
  }

  std::string actions_value;
  if (dict.GetString("actions", &actions_value)) {
    WriteAttribute(
        false, base::StringPrintf("%s=%s", "actions", actions_value.c_str()),
        &line);
  }

  for (const char* attribute_name : TREE_DATA_ATTRIBUTES) {
    const base::Value* value;
    if (!dict.Get(attribute_name, &value))
      continue;

    switch (value->type()) {
      case base::Value::Type::STRING: {
        std::string string_value;
        value->GetAsString(&string_value);
        WriteAttribute(
            false,
            base::StringPrintf("%s=%s", attribute_name, string_value.c_str()),
            &line);
        break;
      }
      case base::Value::Type::INTEGER: {
        int int_value = 0;
        value->GetAsInteger(&int_value);
        WriteAttribute(false,
                       base::StringPrintf("%s=%d", attribute_name, int_value),
                       &line);
        break;
      }
      case base::Value::Type::DOUBLE: {
        double double_value = 0.0;
        value->GetAsDouble(&double_value);
        WriteAttribute(
            false, base::StringPrintf("%s=%.2f", attribute_name, double_value),
            &line);
        break;
      }
      default:
        NOTREACHED();
        break;
    }
  }

  return line;
}

const base::FilePath::StringType
AccessibilityTreeFormatterBlink::GetExpectedFileSuffix() {
  return FILE_PATH_LITERAL("-expected-blink.txt");
}

const std::string AccessibilityTreeFormatterBlink::GetAllowEmptyString() {
  return "@BLINK-ALLOW-EMPTY:";
}

const std::string AccessibilityTreeFormatterBlink::GetAllowString() {
  return "@BLINK-ALLOW:";
}

const std::string AccessibilityTreeFormatterBlink::GetDenyString() {
  return "@BLINK-DENY:";
}

}  // namespace content
