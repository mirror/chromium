// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/extensions/mojom/accessibility_traits.h"

namespace mojo {

// static
extensions::mojom::AXEvent
EnumTraits<extensions::mojom::AXEvent, ui::AXEvent>::ToMojom(
    ui::AXEvent ax_role) {
  using extensions::mojom::AXEvent;
  switch (ax_role) {
    case ui::AX_EVENT_NONE:
      return extensions::mojom::AXEvent::NONE;
    case ui::AX_EVENT_ACTIVEDESCENDANTCHANGED:
      return extensions::mojom::AXEvent::ACTIVEDESCENDANTCHANGED;
    case ui::AX_EVENT_ALERT:
      return extensions::mojom::AXEvent::ALERT;
    case ui::AX_EVENT_ARIA_ATTRIBUTE_CHANGED:
      return extensions::mojom::AXEvent::ARIA_ATTRIBUTE_CHANGED;
    case ui::AX_EVENT_AUTOCORRECTION_OCCURED:
      return extensions::mojom::AXEvent::AUTOCORRECTION_OCCURED;
    case ui::AX_EVENT_BLUR:
      return extensions::mojom::AXEvent::BLUR;
    case ui::AX_EVENT_CHECKED_STATE_CHANGED:
      return extensions::mojom::AXEvent::CHECKED_STATE_CHANGED;
    case ui::AX_EVENT_CHILDREN_CHANGED:
      return extensions::mojom::AXEvent::CHILDREN_CHANGED;
    case ui::AX_EVENT_CLICKED:
      return extensions::mojom::AXEvent::CLICKED;
    case ui::AX_EVENT_DOCUMENT_SELECTION_CHANGED:
      return extensions::mojom::AXEvent::DOCUMENT_SELECTION_CHANGED;
    case ui::AX_EVENT_EXPANDED_CHANGED:
      return extensions::mojom::AXEvent::EXPANDED_CHANGED;
    case ui::AX_EVENT_FOCUS:
      return extensions::mojom::AXEvent::FOCUS;
    case ui::AX_EVENT_HIDE:
      return extensions::mojom::AXEvent::HIDE;
    case ui::AX_EVENT_HOVER:
      return extensions::mojom::AXEvent::HOVER;
    case ui::AX_EVENT_IMAGE_FRAME_UPDATED:
      return extensions::mojom::AXEvent::IMAGE_FRAME_UPDATED;
    case ui::AX_EVENT_INVALID_STATUS_CHANGED:
      return extensions::mojom::AXEvent::INVALID_STATUS_CHANGED;
    case ui::AX_EVENT_LAYOUT_COMPLETE:
      return extensions::mojom::AXEvent::LAYOUT_COMPLETE;
    case ui::AX_EVENT_LIVE_REGION_CREATED:
      return extensions::mojom::AXEvent::LIVE_REGION_CREATED;
    case ui::AX_EVENT_LIVE_REGION_CHANGED:
      return extensions::mojom::AXEvent::LIVE_REGION_CHANGED;
    case ui::AX_EVENT_LOAD_COMPLETE:
      return extensions::mojom::AXEvent::LOAD_COMPLETE;
    case ui::AX_EVENT_LOCATION_CHANGED:
      return extensions::mojom::AXEvent::LOCATION_CHANGED;
    case ui::AX_EVENT_MEDIA_STARTED_PLAYING:
      return extensions::mojom::AXEvent::MEDIA_STARTED_PLAYING;
    case ui::AX_EVENT_MEDIA_STOPPED_PLAYING:
      return extensions::mojom::AXEvent::MEDIA_STOPPED_PLAYING;
    case ui::AX_EVENT_MENU_END:
      return extensions::mojom::AXEvent::MENU_END;
    case ui::AX_EVENT_MENU_LIST_ITEM_SELECTED:
      return extensions::mojom::AXEvent::MENU_LIST_ITEM_SELECTED;
    case ui::AX_EVENT_MENU_LIST_VALUE_CHANGED:
      return extensions::mojom::AXEvent::MENU_LIST_VALUE_CHANGED;
    case ui::AX_EVENT_MENU_POPUP_END:
      return extensions::mojom::AXEvent::MENU_POPUP_END;
    case ui::AX_EVENT_MENU_POPUP_START:
      return extensions::mojom::AXEvent::MENU_POPUP_START;
    case ui::AX_EVENT_MENU_START:
      return extensions::mojom::AXEvent::MENU_START;
    case ui::AX_EVENT_MOUSE_CANCELED:
      return extensions::mojom::AXEvent::MOUSE_CANCELED;
    case ui::AX_EVENT_MOUSE_DRAGGED:
      return extensions::mojom::AXEvent::MOUSE_DRAGGED;
    case ui::AX_EVENT_MOUSE_MOVED:
      return extensions::mojom::AXEvent::MOUSE_MOVED;
    case ui::AX_EVENT_MOUSE_PRESSED:
      return extensions::mojom::AXEvent::MOUSE_PRESSED;
    case ui::AX_EVENT_MOUSE_RELEASED:
      return extensions::mojom::AXEvent::MOUSE_RELEASED;
    case ui::AX_EVENT_ROW_COLLAPSED:
      return extensions::mojom::AXEvent::ROW_COLLAPSED;
    case ui::AX_EVENT_ROW_COUNT_CHANGED:
      return extensions::mojom::AXEvent::ROW_COUNT_CHANGED;
    case ui::AX_EVENT_ROW_EXPANDED:
      return extensions::mojom::AXEvent::ROW_EXPANDED;
    case ui::AX_EVENT_SCROLL_POSITION_CHANGED:
      return extensions::mojom::AXEvent::SCROLL_POSITION_CHANGED;
    case ui::AX_EVENT_SCROLLED_TO_ANCHOR:
      return extensions::mojom::AXEvent::SCROLLED_TO_ANCHOR;
    case ui::AX_EVENT_SELECTED_CHILDREN_CHANGED:
      return extensions::mojom::AXEvent::SELECTED_CHILDREN_CHANGED;
    case ui::AX_EVENT_SELECTION:
      return extensions::mojom::AXEvent::SELECTION;
    case ui::AX_EVENT_SELECTION_ADD:
      return extensions::mojom::AXEvent::SELECTION_ADD;
    case ui::AX_EVENT_SELECTION_REMOVE:
      return extensions::mojom::AXEvent::SELECTION_REMOVE;
    case ui::AX_EVENT_SHOW:
      return extensions::mojom::AXEvent::SHOW;
    case ui::AX_EVENT_TEXT_CHANGED:
      return extensions::mojom::AXEvent::TEXT_CHANGED;
    case ui::AX_EVENT_TEXT_SELECTION_CHANGED:
      return extensions::mojom::AXEvent::TEXT_SELECTION_CHANGED;
    case ui::AX_EVENT_TREE_CHANGED:
      return extensions::mojom::AXEvent::TREE_CHANGED;
    case ui::AX_EVENT_VALUE_CHANGED:
      return extensions::mojom::AXEvent::VALUE_CHANGED;
  }
  NOTREACHED();
  return extensions::mojom::AXEvent::LAST;
}

// static
bool EnumTraits<extensions::mojom::AXEvent, ui::AXEvent>::FromMojom(
    extensions::mojom::AXEvent ax_role,
    ui::AXEvent* out) {
  using extensions::mojom::AXEvent;
  switch (ax_role) {
    case extensions::mojom::AXEvent::NONE:
      *out = ui::AX_EVENT_NONE;
      return true;
    case extensions::mojom::AXEvent::ACTIVEDESCENDANTCHANGED:
      *out = ui::AX_EVENT_ACTIVEDESCENDANTCHANGED;
      return true;
    case extensions::mojom::AXEvent::ALERT:
      *out = ui::AX_EVENT_ALERT;
      return true;
    case extensions::mojom::AXEvent::ARIA_ATTRIBUTE_CHANGED:
      *out = ui::AX_EVENT_ARIA_ATTRIBUTE_CHANGED;
      return true;
    case extensions::mojom::AXEvent::AUTOCORRECTION_OCCURED:
      *out = ui::AX_EVENT_AUTOCORRECTION_OCCURED;
      return true;
    case extensions::mojom::AXEvent::BLUR:
      *out = ui::AX_EVENT_BLUR;
      return true;
    case extensions::mojom::AXEvent::CHECKED_STATE_CHANGED:
      *out = ui::AX_EVENT_CHECKED_STATE_CHANGED;
      return true;
    case extensions::mojom::AXEvent::CHILDREN_CHANGED:
      *out = ui::AX_EVENT_CHILDREN_CHANGED;
      return true;
    case extensions::mojom::AXEvent::CLICKED:
      *out = ui::AX_EVENT_CLICKED;
      return true;
    case extensions::mojom::AXEvent::DOCUMENT_SELECTION_CHANGED:
      *out = ui::AX_EVENT_DOCUMENT_SELECTION_CHANGED;
      return true;
    case extensions::mojom::AXEvent::EXPANDED_CHANGED:
      *out = ui::AX_EVENT_EXPANDED_CHANGED;
      return true;
    case extensions::mojom::AXEvent::FOCUS:
      *out = ui::AX_EVENT_FOCUS;
      return true;
    case extensions::mojom::AXEvent::HIDE:
      *out = ui::AX_EVENT_HIDE;
      return true;
    case extensions::mojom::AXEvent::HOVER:
      *out = ui::AX_EVENT_HOVER;
      return true;
    case extensions::mojom::AXEvent::IMAGE_FRAME_UPDATED:
      *out = ui::AX_EVENT_IMAGE_FRAME_UPDATED;
      return true;
    case extensions::mojom::AXEvent::INVALID_STATUS_CHANGED:
      *out = ui::AX_EVENT_INVALID_STATUS_CHANGED;
      return true;
    case extensions::mojom::AXEvent::LAYOUT_COMPLETE:
      *out = ui::AX_EVENT_LAYOUT_COMPLETE;
      return true;
    case extensions::mojom::AXEvent::LIVE_REGION_CREATED:
      *out = ui::AX_EVENT_LIVE_REGION_CREATED;
      return true;
    case extensions::mojom::AXEvent::LIVE_REGION_CHANGED:
      *out = ui::AX_EVENT_LIVE_REGION_CHANGED;
      return true;
    case extensions::mojom::AXEvent::LOAD_COMPLETE:
      *out = ui::AX_EVENT_LOAD_COMPLETE;
      return true;
    case extensions::mojom::AXEvent::LOCATION_CHANGED:
      *out = ui::AX_EVENT_LOCATION_CHANGED;
      return true;
    case extensions::mojom::AXEvent::MEDIA_STARTED_PLAYING:
      *out = ui::AX_EVENT_MEDIA_STARTED_PLAYING;
      return true;
    case extensions::mojom::AXEvent::MEDIA_STOPPED_PLAYING:
      *out = ui::AX_EVENT_MEDIA_STOPPED_PLAYING;
      return true;
    case extensions::mojom::AXEvent::MENU_END:
      *out = ui::AX_EVENT_MENU_END;
      return true;
    case extensions::mojom::AXEvent::MENU_LIST_ITEM_SELECTED:
      *out = ui::AX_EVENT_MENU_LIST_ITEM_SELECTED;
      return true;
    case extensions::mojom::AXEvent::MENU_LIST_VALUE_CHANGED:
      *out = ui::AX_EVENT_MENU_LIST_VALUE_CHANGED;
      return true;
    case extensions::mojom::AXEvent::MENU_POPUP_END:
      *out = ui::AX_EVENT_MENU_POPUP_END;
      return true;
    case extensions::mojom::AXEvent::MENU_POPUP_START:
      *out = ui::AX_EVENT_MENU_POPUP_START;
      return true;
    case extensions::mojom::AXEvent::MENU_START:
      *out = ui::AX_EVENT_MENU_START;
      return true;
    case extensions::mojom::AXEvent::MOUSE_CANCELED:
      *out = ui::AX_EVENT_MOUSE_CANCELED;
      return true;
    case extensions::mojom::AXEvent::MOUSE_DRAGGED:
      *out = ui::AX_EVENT_MOUSE_DRAGGED;
      return true;
    case extensions::mojom::AXEvent::MOUSE_MOVED:
      *out = ui::AX_EVENT_MOUSE_MOVED;
      return true;
    case extensions::mojom::AXEvent::MOUSE_PRESSED:
      *out = ui::AX_EVENT_MOUSE_PRESSED;
      return true;
    case extensions::mojom::AXEvent::MOUSE_RELEASED:
      *out = ui::AX_EVENT_MOUSE_RELEASED;
      return true;
    case extensions::mojom::AXEvent::ROW_COLLAPSED:
      *out = ui::AX_EVENT_ROW_COLLAPSED;
      return true;
    case extensions::mojom::AXEvent::ROW_COUNT_CHANGED:
      *out = ui::AX_EVENT_ROW_COUNT_CHANGED;
      return true;
    case extensions::mojom::AXEvent::ROW_EXPANDED:
      *out = ui::AX_EVENT_ROW_EXPANDED;
      return true;
    case extensions::mojom::AXEvent::SCROLL_POSITION_CHANGED:
      *out = ui::AX_EVENT_SCROLL_POSITION_CHANGED;
      return true;
    case extensions::mojom::AXEvent::SCROLLED_TO_ANCHOR:
      *out = ui::AX_EVENT_SCROLLED_TO_ANCHOR;
      return true;
    case extensions::mojom::AXEvent::SELECTED_CHILDREN_CHANGED:
      *out = ui::AX_EVENT_SELECTED_CHILDREN_CHANGED;
      return true;
    case extensions::mojom::AXEvent::SELECTION:
      *out = ui::AX_EVENT_SELECTION;
      return true;
    case extensions::mojom::AXEvent::SELECTION_ADD:
      *out = ui::AX_EVENT_SELECTION_ADD;
      return true;
    case extensions::mojom::AXEvent::SELECTION_REMOVE:
      *out = ui::AX_EVENT_SELECTION_REMOVE;
      return true;
    case extensions::mojom::AXEvent::SHOW:
      *out = ui::AX_EVENT_SHOW;
      return true;
    case extensions::mojom::AXEvent::TEXT_CHANGED:
      *out = ui::AX_EVENT_TEXT_CHANGED;
      return true;
    case extensions::mojom::AXEvent::TEXT_SELECTION_CHANGED:
      *out = ui::AX_EVENT_TEXT_SELECTION_CHANGED;
      return true;
    case extensions::mojom::AXEvent::TREE_CHANGED:
      *out = ui::AX_EVENT_TREE_CHANGED;
      return true;
    case extensions::mojom::AXEvent::VALUE_CHANGED:
      *out = ui::AX_EVENT_VALUE_CHANGED;
      return true;
    case extensions::mojom::AXEvent::LAST:
      *out = ui::AX_EVENT_LAST;
      return true;
  }
  return false;
}

// static
extensions::mojom::AXStringAttribute EnumTraits<
    extensions::mojom::AXStringAttribute,
    ui::AXStringAttribute>::ToMojom(ui::AXStringAttribute ax_string_attribute) {
  using extensions::mojom::AXStringAttribute;
  switch (ax_string_attribute) {
    case ui::AX_STRING_ATTRIBUTE_NONE:
      return extensions::mojom::AXStringAttribute::NONE;
    case ui::AX_ATTR_ACCESS_KEY:
      return extensions::mojom::AXStringAttribute::ACCESS_KEY;
    case ui::AX_ATTR_ARIA_INVALID_VALUE:
      return extensions::mojom::AXStringAttribute::ARIA_INVALID_VALUE;
    case ui::AX_ATTR_AUTO_COMPLETE:
      return extensions::mojom::AXStringAttribute::AUTO_COMPLETE;
    case ui::AX_ATTR_CHROME_CHANNEL:
      return extensions::mojom::AXStringAttribute::CHROME_CHANNEL;
    case ui::AX_ATTR_CONTAINER_LIVE_RELEVANT:
      return extensions::mojom::AXStringAttribute::CONTAINER_LIVE_RELEVANT;
    case ui::AX_ATTR_CONTAINER_LIVE_STATUS:
      return extensions::mojom::AXStringAttribute::CONTAINER_LIVE_STATUS;
    case ui::AX_ATTR_DESCRIPTION:
      return extensions::mojom::AXStringAttribute::DESCRIPTION;
    case ui::AX_ATTR_DISPLAY:
      return extensions::mojom::AXStringAttribute::DISPLAY;
    case ui::AX_ATTR_FONT_FAMILY:
      return extensions::mojom::AXStringAttribute::FONT_FAMILY;
    case ui::AX_ATTR_HTML_TAG:
      return extensions::mojom::AXStringAttribute::HTML_TAG;
    case ui::AX_ATTR_IMAGE_DATA_URL:
      return extensions::mojom::AXStringAttribute::IMAGE_DATA_URL;
    case ui::AX_ATTR_INNER_HTML:
      return extensions::mojom::AXStringAttribute::INNER_HTML;
    case ui::AX_ATTR_KEY_SHORTCUTS:
      return extensions::mojom::AXStringAttribute::KEY_SHORTCUTS;
    case ui::AX_ATTR_LANGUAGE:
      return extensions::mojom::AXStringAttribute::LANGUAGE;
    case ui::AX_ATTR_NAME:
      return extensions::mojom::AXStringAttribute::NAME;
    case ui::AX_ATTR_LIVE_RELEVANT:
      return extensions::mojom::AXStringAttribute::LIVE_RELEVANT;
    case ui::AX_ATTR_LIVE_STATUS:
      return extensions::mojom::AXStringAttribute::LIVE_STATUS;
    case ui::AX_ATTR_PLACEHOLDER:
      return extensions::mojom::AXStringAttribute::PLACEHOLDER;
    case ui::AX_ATTR_ROLE:
      return extensions::mojom::AXStringAttribute::ROLE;
    case ui::AX_ATTR_ROLE_DESCRIPTION:
      return extensions::mojom::AXStringAttribute::ROLE_DESCRIPTION;
    case ui::AX_ATTR_SHORTCUT:
      return extensions::mojom::AXStringAttribute::SHORTCUT;
    case ui::AX_ATTR_URL:
      return extensions::mojom::AXStringAttribute::URL;
    case ui::AX_ATTR_VALUE:
      return extensions::mojom::AXStringAttribute::VALUE;
  }
  NOTREACHED();
  return extensions::mojom::AXStringAttribute::STRING_ATTRIBUTE_LAST;
}

// static
bool EnumTraits<extensions::mojom::AXStringAttribute, ui::AXStringAttribute>::
    FromMojom(extensions::mojom::AXStringAttribute ax_string_attribute,
              ui::AXStringAttribute* out) {
  using extensions::mojom::AXStringAttribute;
  switch (ax_string_attribute) {
    case extensions::mojom::AXStringAttribute::NONE:
      *out = ui::AX_STRING_ATTRIBUTE_NONE;
      return true;
    case extensions::mojom::AXStringAttribute::ACCESS_KEY:
      *out = ui::AX_ATTR_ACCESS_KEY;
      return true;
    case extensions::mojom::AXStringAttribute::ARIA_INVALID_VALUE:
      *out = ui::AX_ATTR_ARIA_INVALID_VALUE;
      return true;
    case extensions::mojom::AXStringAttribute::AUTO_COMPLETE:
      *out = ui::AX_ATTR_AUTO_COMPLETE;
      return true;
    case extensions::mojom::AXStringAttribute::CHROME_CHANNEL:
      *out = ui::AX_ATTR_CHROME_CHANNEL;
      return true;
    case extensions::mojom::AXStringAttribute::CONTAINER_LIVE_RELEVANT:
      *out = ui::AX_ATTR_CONTAINER_LIVE_RELEVANT;
      return true;
    case extensions::mojom::AXStringAttribute::CONTAINER_LIVE_STATUS:
      *out = ui::AX_ATTR_CONTAINER_LIVE_STATUS;
      return true;
    case extensions::mojom::AXStringAttribute::DESCRIPTION:
      *out = ui::AX_ATTR_DESCRIPTION;
      return true;
    case extensions::mojom::AXStringAttribute::DISPLAY:
      *out = ui::AX_ATTR_DISPLAY;
      return true;
    case extensions::mojom::AXStringAttribute::FONT_FAMILY:
      *out = ui::AX_ATTR_FONT_FAMILY;
      return true;
    case extensions::mojom::AXStringAttribute::HTML_TAG:
      *out = ui::AX_ATTR_HTML_TAG;
      return true;
    case extensions::mojom::AXStringAttribute::IMAGE_DATA_URL:
      *out = ui::AX_ATTR_IMAGE_DATA_URL;
      return true;
    case extensions::mojom::AXStringAttribute::INNER_HTML:
      *out = ui::AX_ATTR_INNER_HTML;
      return true;
    case extensions::mojom::AXStringAttribute::KEY_SHORTCUTS:
      *out = ui::AX_ATTR_KEY_SHORTCUTS;
      return true;
    case extensions::mojom::AXStringAttribute::LANGUAGE:
      *out = ui::AX_ATTR_LANGUAGE;
      return true;
    case extensions::mojom::AXStringAttribute::NAME:
      *out = ui::AX_ATTR_NAME;
      return true;
    case extensions::mojom::AXStringAttribute::LIVE_RELEVANT:
      *out = ui::AX_ATTR_LIVE_RELEVANT;
      return true;
    case extensions::mojom::AXStringAttribute::LIVE_STATUS:
      *out = ui::AX_ATTR_LIVE_STATUS;
      return true;
    case extensions::mojom::AXStringAttribute::PLACEHOLDER:
      *out = ui::AX_ATTR_PLACEHOLDER;
      return true;
    case extensions::mojom::AXStringAttribute::ROLE:
      *out = ui::AX_ATTR_ROLE;
      return true;
    case extensions::mojom::AXStringAttribute::ROLE_DESCRIPTION:
      *out = ui::AX_ATTR_ROLE_DESCRIPTION;
      return true;
    case extensions::mojom::AXStringAttribute::SHORTCUT:
      *out = ui::AX_ATTR_SHORTCUT;
      return true;
    case extensions::mojom::AXStringAttribute::URL:
      *out = ui::AX_ATTR_URL;
      return true;
    case extensions::mojom::AXStringAttribute::VALUE:
      *out = ui::AX_ATTR_VALUE;
      return true;
    case extensions::mojom::AXStringAttribute::STRING_ATTRIBUTE_LAST:
      *out = ui::AX_STRING_ATTRIBUTE_LAST;
      return true;
  }
  return false;
}

// static
extensions::mojom::AXIntAttribute
EnumTraits<extensions::mojom::AXIntAttribute, ui::AXIntAttribute>::ToMojom(
    ui::AXIntAttribute ax_int_attribute) {
  using extensions::mojom::AXIntAttribute;
  switch (ax_int_attribute) {
    case ui::AX_INT_ATTRIBUTE_NONE:
      return extensions::mojom::AXIntAttribute::NONE;
    case ui::AX_ATTR_DEFAULT_ACTION_VERB:
      return extensions::mojom::AXIntAttribute::DEFAULT_ACTION_VERB;
    case ui::AX_ATTR_SCROLL_X:
      return extensions::mojom::AXIntAttribute::SCROLL_X;
    case ui::AX_ATTR_SCROLL_X_MIN:
      return extensions::mojom::AXIntAttribute::SCROLL_X_MIN;
    case ui::AX_ATTR_SCROLL_X_MAX:
      return extensions::mojom::AXIntAttribute::SCROLL_X_MAX;
    case ui::AX_ATTR_SCROLL_Y:
      return extensions::mojom::AXIntAttribute::SCROLL_Y;
    case ui::AX_ATTR_SCROLL_Y_MIN:
      return extensions::mojom::AXIntAttribute::SCROLL_Y_MIN;
    case ui::AX_ATTR_SCROLL_Y_MAX:
      return extensions::mojom::AXIntAttribute::SCROLL_Y_MAX;
    case ui::AX_ATTR_TEXT_SEL_START:
      return extensions::mojom::AXIntAttribute::TEXT_SEL_START;
    case ui::AX_ATTR_TEXT_SEL_END:
      return extensions::mojom::AXIntAttribute::TEXT_SEL_END;
    case ui::AX_ATTR_ARIA_COLUMN_COUNT:
      return extensions::mojom::AXIntAttribute::ARIA_COLUMN_COUNT;
    case ui::AX_ATTR_ARIA_CELL_COLUMN_INDEX:
      return extensions::mojom::AXIntAttribute::ARIA_CELL_COLUMN_INDEX;
    case ui::AX_ATTR_ARIA_ROW_COUNT:
      return extensions::mojom::AXIntAttribute::ARIA_ROW_COUNT;
    case ui::AX_ATTR_ARIA_CELL_ROW_INDEX:
      return extensions::mojom::AXIntAttribute::ARIA_CELL_ROW_INDEX;
    case ui::AX_ATTR_TABLE_ROW_COUNT:
      return extensions::mojom::AXIntAttribute::TABLE_ROW_COUNT;
    case ui::AX_ATTR_TABLE_COLUMN_COUNT:
      return extensions::mojom::AXIntAttribute::TABLE_COLUMN_COUNT;
    case ui::AX_ATTR_TABLE_HEADER_ID:
      return extensions::mojom::AXIntAttribute::TABLE_HEADER_ID;
    case ui::AX_ATTR_TABLE_ROW_INDEX:
      return extensions::mojom::AXIntAttribute::TABLE_ROW_INDEX;
    case ui::AX_ATTR_TABLE_ROW_HEADER_ID:
      return extensions::mojom::AXIntAttribute::TABLE_ROW_HEADER_ID;
    case ui::AX_ATTR_TABLE_COLUMN_INDEX:
      return extensions::mojom::AXIntAttribute::TABLE_COLUMN_INDEX;
    case ui::AX_ATTR_TABLE_COLUMN_HEADER_ID:
      return extensions::mojom::AXIntAttribute::TABLE_COLUMN_HEADER_ID;
    case ui::AX_ATTR_TABLE_CELL_COLUMN_INDEX:
      return extensions::mojom::AXIntAttribute::TABLE_CELL_COLUMN_INDEX;
    case ui::AX_ATTR_TABLE_CELL_COLUMN_SPAN:
      return extensions::mojom::AXIntAttribute::TABLE_CELL_COLUMN_SPAN;
    case ui::AX_ATTR_TABLE_CELL_ROW_INDEX:
      return extensions::mojom::AXIntAttribute::TABLE_CELL_ROW_INDEX;
    case ui::AX_ATTR_TABLE_CELL_ROW_SPAN:
      return extensions::mojom::AXIntAttribute::TABLE_CELL_ROW_SPAN;
    case ui::AX_ATTR_SORT_DIRECTION:
      return extensions::mojom::AXIntAttribute::SORT_DIRECTION;
    case ui::AX_ATTR_HIERARCHICAL_LEVEL:
      return extensions::mojom::AXIntAttribute::HIERARCHICAL_LEVEL;
    case ui::AX_ATTR_NAME_FROM:
      return extensions::mojom::AXIntAttribute::NAME_FROM;
    case ui::AX_ATTR_DESCRIPTION_FROM:
      return extensions::mojom::AXIntAttribute::DESCRIPTION_FROM;
    case ui::AX_ATTR_ACTIVEDESCENDANT_ID:
      return extensions::mojom::AXIntAttribute::ACTIVEDESCENDANT_ID;
    case ui::AX_ATTR_ERRORMESSAGE_ID:
      return extensions::mojom::AXIntAttribute::ERRORMESSAGE_ID;
    case ui::AX_ATTR_IN_PAGE_LINK_TARGET_ID:
      return extensions::mojom::AXIntAttribute::IN_PAGE_LINK_TARGET_ID;
    case ui::AX_ATTR_MEMBER_OF_ID:
      return extensions::mojom::AXIntAttribute::MEMBER_OF_ID;
    case ui::AX_ATTR_NEXT_ON_LINE_ID:
      return extensions::mojom::AXIntAttribute::NEXT_ON_LINE_ID;
    case ui::AX_ATTR_PREVIOUS_ON_LINE_ID:
      return extensions::mojom::AXIntAttribute::PREVIOUS_ON_LINE_ID;
    case ui::AX_ATTR_CHILD_TREE_ID:
      return extensions::mojom::AXIntAttribute::CHILD_TREE_ID;
    case ui::AX_ATTR_SET_SIZE:
      return extensions::mojom::AXIntAttribute::SET_SIZE;
    case ui::AX_ATTR_POS_IN_SET:
      return extensions::mojom::AXIntAttribute::POS_IN_SET;
    case ui::AX_ATTR_COLOR_VALUE:
      return extensions::mojom::AXIntAttribute::COLOR_VALUE;
    case ui::AX_ATTR_ARIA_CURRENT_STATE:
      return extensions::mojom::AXIntAttribute::ARIA_CURRENT_STATE;
    case ui::AX_ATTR_BACKGROUND_COLOR:
      return extensions::mojom::AXIntAttribute::BACKGROUND_COLOR;
    case ui::AX_ATTR_COLOR:
      return extensions::mojom::AXIntAttribute::COLOR;
    case ui::AX_ATTR_INVALID_STATE:
      return extensions::mojom::AXIntAttribute::INVALID_STATE;
    case ui::AX_ATTR_CHECKED_STATE:
      return extensions::mojom::AXIntAttribute::CHECKED_STATE;
    case ui::AX_ATTR_TEXT_DIRECTION:
      return extensions::mojom::AXIntAttribute::TEXT_DIRECTION;
    case ui::AX_ATTR_TEXT_STYLE:
      return extensions::mojom::AXIntAttribute::TEXT_STYLE;
  }
  NOTREACHED();
  return extensions::mojom::AXIntAttribute::INT_ATTRIBUTE_LAST;
}

// static
bool EnumTraits<extensions::mojom::AXIntAttribute,
                ui::AXIntAttribute>::FromMojom(extensions::mojom::AXIntAttribute
                                                   ax_int_attribute,
                                               ui::AXIntAttribute* out) {
  using extensions::mojom::AXIntAttribute;
  switch (ax_int_attribute) {
    case extensions::mojom::AXIntAttribute::NONE:
      *out = ui::AX_INT_ATTRIBUTE_NONE;
      return true;
    case extensions::mojom::AXIntAttribute::DEFAULT_ACTION_VERB:
      *out = ui::AX_ATTR_DEFAULT_ACTION_VERB;
      return true;
    case extensions::mojom::AXIntAttribute::SCROLL_X:
      *out = ui::AX_ATTR_SCROLL_X;
      return true;
    case extensions::mojom::AXIntAttribute::SCROLL_X_MIN:
      *out = ui::AX_ATTR_SCROLL_X_MIN;
      return true;
    case extensions::mojom::AXIntAttribute::SCROLL_X_MAX:
      *out = ui::AX_ATTR_SCROLL_X_MAX;
      return true;
    case extensions::mojom::AXIntAttribute::SCROLL_Y:
      *out = ui::AX_ATTR_SCROLL_Y;
      return true;
    case extensions::mojom::AXIntAttribute::SCROLL_Y_MIN:
      *out = ui::AX_ATTR_SCROLL_Y_MIN;
      return true;
    case extensions::mojom::AXIntAttribute::SCROLL_Y_MAX:
      *out = ui::AX_ATTR_SCROLL_Y_MAX;
      return true;
    case extensions::mojom::AXIntAttribute::TEXT_SEL_START:
      *out = ui::AX_ATTR_TEXT_SEL_START;
      return true;
    case extensions::mojom::AXIntAttribute::TEXT_SEL_END:
      *out = ui::AX_ATTR_TEXT_SEL_END;
      return true;
    case extensions::mojom::AXIntAttribute::ARIA_COLUMN_COUNT:
      *out = ui::AX_ATTR_ARIA_COLUMN_COUNT;
      return true;
    case extensions::mojom::AXIntAttribute::ARIA_CELL_COLUMN_INDEX:
      *out = ui::AX_ATTR_ARIA_CELL_COLUMN_INDEX;
      return true;
    case extensions::mojom::AXIntAttribute::ARIA_ROW_COUNT:
      *out = ui::AX_ATTR_ARIA_ROW_COUNT;
      return true;
    case extensions::mojom::AXIntAttribute::ARIA_CELL_ROW_INDEX:
      *out = ui::AX_ATTR_ARIA_CELL_ROW_INDEX;
      return true;
    case extensions::mojom::AXIntAttribute::TABLE_ROW_COUNT:
      *out = ui::AX_ATTR_TABLE_ROW_COUNT;
      return true;
    case extensions::mojom::AXIntAttribute::TABLE_COLUMN_COUNT:
      *out = ui::AX_ATTR_TABLE_COLUMN_COUNT;
      return true;
    case extensions::mojom::AXIntAttribute::TABLE_HEADER_ID:
      *out = ui::AX_ATTR_TABLE_HEADER_ID;
      return true;
    case extensions::mojom::AXIntAttribute::TABLE_ROW_INDEX:
      *out = ui::AX_ATTR_TABLE_ROW_INDEX;
      return true;
    case extensions::mojom::AXIntAttribute::TABLE_ROW_HEADER_ID:
      *out = ui::AX_ATTR_TABLE_ROW_HEADER_ID;
      return true;
    case extensions::mojom::AXIntAttribute::TABLE_COLUMN_INDEX:
      *out = ui::AX_ATTR_TABLE_COLUMN_INDEX;
      return true;
    case extensions::mojom::AXIntAttribute::TABLE_COLUMN_HEADER_ID:
      *out = ui::AX_ATTR_TABLE_COLUMN_HEADER_ID;
      return true;
    case extensions::mojom::AXIntAttribute::TABLE_CELL_COLUMN_INDEX:
      *out = ui::AX_ATTR_TABLE_CELL_COLUMN_INDEX;
      return true;
    case extensions::mojom::AXIntAttribute::TABLE_CELL_COLUMN_SPAN:
      *out = ui::AX_ATTR_TABLE_CELL_COLUMN_SPAN;
      return true;
    case extensions::mojom::AXIntAttribute::TABLE_CELL_ROW_INDEX:
      *out = ui::AX_ATTR_TABLE_CELL_ROW_INDEX;
      return true;
    case extensions::mojom::AXIntAttribute::TABLE_CELL_ROW_SPAN:
      *out = ui::AX_ATTR_TABLE_CELL_ROW_SPAN;
      return true;
    case extensions::mojom::AXIntAttribute::SORT_DIRECTION:
      *out = ui::AX_ATTR_SORT_DIRECTION;
      return true;
    case extensions::mojom::AXIntAttribute::HIERARCHICAL_LEVEL:
      *out = ui::AX_ATTR_HIERARCHICAL_LEVEL;
      return true;
    case extensions::mojom::AXIntAttribute::NAME_FROM:
      *out = ui::AX_ATTR_NAME_FROM;
      return true;
    case extensions::mojom::AXIntAttribute::DESCRIPTION_FROM:
      *out = ui::AX_ATTR_DESCRIPTION_FROM;
      return true;
    case extensions::mojom::AXIntAttribute::ACTIVEDESCENDANT_ID:
      *out = ui::AX_ATTR_ACTIVEDESCENDANT_ID;
      return true;
    case extensions::mojom::AXIntAttribute::ERRORMESSAGE_ID:
      *out = ui::AX_ATTR_ERRORMESSAGE_ID;
      return true;
    case extensions::mojom::AXIntAttribute::IN_PAGE_LINK_TARGET_ID:
      *out = ui::AX_ATTR_IN_PAGE_LINK_TARGET_ID;
      return true;
    case extensions::mojom::AXIntAttribute::MEMBER_OF_ID:
      *out = ui::AX_ATTR_MEMBER_OF_ID;
      return true;
    case extensions::mojom::AXIntAttribute::NEXT_ON_LINE_ID:
      *out = ui::AX_ATTR_NEXT_ON_LINE_ID;
      return true;
    case extensions::mojom::AXIntAttribute::PREVIOUS_ON_LINE_ID:
      *out = ui::AX_ATTR_PREVIOUS_ON_LINE_ID;
      return true;
    case extensions::mojom::AXIntAttribute::CHILD_TREE_ID:
      *out = ui::AX_ATTR_CHILD_TREE_ID;
      return true;
    case extensions::mojom::AXIntAttribute::SET_SIZE:
      *out = ui::AX_ATTR_SET_SIZE;
      return true;
    case extensions::mojom::AXIntAttribute::POS_IN_SET:
      *out = ui::AX_ATTR_POS_IN_SET;
      return true;
    case extensions::mojom::AXIntAttribute::COLOR_VALUE:
      *out = ui::AX_ATTR_COLOR_VALUE;
      return true;
    case extensions::mojom::AXIntAttribute::ARIA_CURRENT_STATE:
      *out = ui::AX_ATTR_ARIA_CURRENT_STATE;
      return true;
    case extensions::mojom::AXIntAttribute::BACKGROUND_COLOR:
      *out = ui::AX_ATTR_BACKGROUND_COLOR;
      return true;
    case extensions::mojom::AXIntAttribute::COLOR:
      *out = ui::AX_ATTR_COLOR;
      return true;
    case extensions::mojom::AXIntAttribute::INVALID_STATE:
      *out = ui::AX_ATTR_INVALID_STATE;
      return true;
    case extensions::mojom::AXIntAttribute::CHECKED_STATE:
      *out = ui::AX_ATTR_CHECKED_STATE;
      return true;
    case extensions::mojom::AXIntAttribute::TEXT_DIRECTION:
      *out = ui::AX_ATTR_TEXT_DIRECTION;
      return true;
    case extensions::mojom::AXIntAttribute::TEXT_STYLE:
      *out = ui::AX_ATTR_TEXT_STYLE;
      return true;
    case extensions::mojom::AXIntAttribute::INT_ATTRIBUTE_LAST:
      *out = ui::AX_INT_ATTRIBUTE_LAST;
      return true;
  }
  return false;
}

// static
extensions::mojom::AXFloatAttribute
EnumTraits<extensions::mojom::AXFloatAttribute, ui::AXFloatAttribute>::ToMojom(
    ui::AXFloatAttribute float_attribute) {
  using extensions::mojom::AXFloatAttribute;
  switch (float_attribute) {
    case ui::AX_FLOAT_ATTRIBUTE_NONE:
      return extensions::mojom::AXFloatAttribute::AX_FLOAT_ATTRIBUTE_NONE;
    case ui::AX_ATTR_VALUE_FOR_RANGE:
      return extensions::mojom::AXFloatAttribute::VALUE_FOR_RANGE;
    case ui::AX_ATTR_MIN_VALUE_FOR_RANGE:
      return extensions::mojom::AXFloatAttribute::MIN_VALUE_FOR_RANGE;
    case ui::AX_ATTR_MAX_VALUE_FOR_RANGE:
      return extensions::mojom::AXFloatAttribute::MAX_VALUE_FOR_RANGE;
    case ui::AX_ATTR_FONT_SIZE:
      return extensions::mojom::AXFloatAttribute::FONT_SIZE;
  }
  NOTREACHED();
  return extensions::mojom::AXFloatAttribute::AX_FLOAT_ATTRIBUTE_LAST;
}

// static
bool EnumTraits<extensions::mojom::AXFloatAttribute, ui::AXFloatAttribute>::
    FromMojom(extensions::mojom::AXFloatAttribute float_attribute,
              ui::AXFloatAttribute* out) {
  using extensions::mojom::AXFloatAttribute;
  switch (float_attribute) {
    case extensions::mojom::AXFloatAttribute::AX_FLOAT_ATTRIBUTE_NONE:
      *out = ui::AX_FLOAT_ATTRIBUTE_NONE;
      return true;
    case extensions::mojom::AXFloatAttribute::VALUE_FOR_RANGE:
      *out = ui::AX_ATTR_VALUE_FOR_RANGE;
      return true;
    case extensions::mojom::AXFloatAttribute::MIN_VALUE_FOR_RANGE:
      *out = ui::AX_ATTR_MIN_VALUE_FOR_RANGE;
      return true;
    case extensions::mojom::AXFloatAttribute::MAX_VALUE_FOR_RANGE:
      *out = ui::AX_ATTR_MAX_VALUE_FOR_RANGE;
      return true;
    case extensions::mojom::AXFloatAttribute::FONT_SIZE:
      *out = ui::AX_ATTR_FONT_SIZE;
      return true;
    case extensions::mojom::AXFloatAttribute::AX_FLOAT_ATTRIBUTE_LAST:
      *out = ui::AX_FLOAT_ATTRIBUTE_LAST;
      return true;
  }
  return false;
}

// static
extensions::mojom::AXBoolAttribute
EnumTraits<extensions::mojom::AXBoolAttribute, ui::AXBoolAttribute>::ToMojom(
    ui::AXBoolAttribute bool_attribute) {
  using extensions::mojom::AXBoolAttribute;
  switch (bool_attribute) {
    case ui::AX_BOOL_ATTRIBUTE_NONE:
      return extensions::mojom::AXBoolAttribute::AX_BOOL_ATTRIBUTE_NONE;
    case ui::AX_ATTR_CONTAINER_LIVE_ATOMIC:
      return extensions::mojom::AXBoolAttribute::CONTAINER_LIVE_ATOMIC;
    case ui::AX_ATTR_CONTAINER_LIVE_BUSY:
      return extensions::mojom::AXBoolAttribute::CONTAINER_LIVE_BUSY;
    case ui::AX_ATTR_LIVE_ATOMIC:
      return extensions::mojom::AXBoolAttribute::LIVE_ATOMIC;
    case ui::AX_ATTR_LIVE_BUSY:
      return extensions::mojom::AXBoolAttribute::LIVE_BUSY;
    case ui::AX_ATTR_MODAL:
      return extensions::mojom::AXBoolAttribute::MODAL;
    case ui::AX_ATTR_ARIA_READONLY:
      return extensions::mojom::AXBoolAttribute::ARIA_READONLY;
    case ui::AX_ATTR_UPDATE_LOCATION_ONLY:
      return extensions::mojom::AXBoolAttribute::UPDATE_LOCATION_ONLY;
    case ui::AX_ATTR_CANVAS_HAS_FALLBACK:
      return extensions::mojom::AXBoolAttribute::CANVAS_HAS_FALLBACK;
  }
  NOTREACHED();
  return extensions::mojom::AXBoolAttribute::AX_BOOL_ATTRIBUTE_LAST;
}

// static
bool EnumTraits<extensions::mojom::AXBoolAttribute, ui::AXBoolAttribute>::
    FromMojom(extensions::mojom::AXBoolAttribute bool_attribute,
              ui::AXBoolAttribute* out) {
  using extensions::mojom::AXBoolAttribute;
  switch (bool_attribute) {
    case extensions::mojom::AXBoolAttribute::AX_BOOL_ATTRIBUTE_NONE:
      *out = ui::AX_BOOL_ATTRIBUTE_NONE;
      return true;
    case extensions::mojom::AXBoolAttribute::CONTAINER_LIVE_ATOMIC:
      *out = ui::AX_ATTR_CONTAINER_LIVE_ATOMIC;
      return true;
    case extensions::mojom::AXBoolAttribute::CONTAINER_LIVE_BUSY:
      *out = ui::AX_ATTR_CONTAINER_LIVE_BUSY;
      return true;
    case extensions::mojom::AXBoolAttribute::LIVE_ATOMIC:
      *out = ui::AX_ATTR_LIVE_ATOMIC;
      return true;
    case extensions::mojom::AXBoolAttribute::LIVE_BUSY:
      *out = ui::AX_ATTR_LIVE_BUSY;
      return true;
    case extensions::mojom::AXBoolAttribute::MODAL:
      *out = ui::AX_ATTR_MODAL;
      return true;
    case extensions::mojom::AXBoolAttribute::ARIA_READONLY:
      *out = ui::AX_ATTR_ARIA_READONLY;
      return true;
    case extensions::mojom::AXBoolAttribute::UPDATE_LOCATION_ONLY:
      *out = ui::AX_ATTR_UPDATE_LOCATION_ONLY;
      return true;
    case extensions::mojom::AXBoolAttribute::CANVAS_HAS_FALLBACK:
      *out = ui::AX_ATTR_CANVAS_HAS_FALLBACK;
      return true;
    case extensions::mojom::AXBoolAttribute::AX_BOOL_ATTRIBUTE_LAST:
      *out = ui::AX_BOOL_ATTRIBUTE_LAST;
      return true;
  }
  return false;
}

// static
extensions::mojom::AXIntListAttribute
EnumTraits<extensions::mojom::AXIntListAttribute, ui::AXIntListAttribute>::
    ToMojom(ui::AXIntListAttribute int_list_attribute) {
  using extensions::mojom::AXIntListAttribute;
  switch (int_list_attribute) {
    case ui::AX_INT_LIST_ATTRIBUTE_NONE:
      return extensions::mojom::AXIntListAttribute::AX_INT_LIST_ATTRIBUTE_NONE;
    case ui::AX_ATTR_INDIRECT_CHILD_IDS:
      return extensions::mojom::AXIntListAttribute::INDIRECT_CHILD_IDS;
    case ui::AX_ATTR_CONTROLS_IDS:
      return extensions::mojom::AXIntListAttribute::CONTROLS_IDS;
    case ui::AX_ATTR_DESCRIBEDBY_IDS:
      return extensions::mojom::AXIntListAttribute::DESCRIBEDBY_IDS;
    case ui::AX_ATTR_DETAILS_IDS:
      return extensions::mojom::AXIntListAttribute::DETAILS_IDS;
    case ui::AX_ATTR_FLOWTO_IDS:
      return extensions::mojom::AXIntListAttribute::FLOWTO_IDS;
    case ui::AX_ATTR_LABELLEDBY_IDS:
      return extensions::mojom::AXIntListAttribute::LABELLEDBY_IDS;
    case ui::AX_ATTR_RADIO_GROUP_IDS:
      return extensions::mojom::AXIntListAttribute::RADIO_GROUP_IDS;
    case ui::AX_ATTR_LINE_BREAKS:
      return extensions::mojom::AXIntListAttribute::LINE_BREAKS;
    case ui::AX_ATTR_MARKER_TYPES:
      return extensions::mojom::AXIntListAttribute::MARKER_TYPES;
    case ui::AX_ATTR_MARKER_STARTS:
      return extensions::mojom::AXIntListAttribute::MARKER_STARTS;
    case ui::AX_ATTR_MARKER_ENDS:
      return extensions::mojom::AXIntListAttribute::MARKER_ENDS;
    case ui::AX_ATTR_CELL_IDS:
      return extensions::mojom::AXIntListAttribute::CELL_IDS;
    case ui::AX_ATTR_UNIQUE_CELL_IDS:
      return extensions::mojom::AXIntListAttribute::UNIQUE_CELL_IDS;
    case ui::AX_ATTR_CHARACTER_OFFSETS:
      return extensions::mojom::AXIntListAttribute::CHARACTER_OFFSETS;
    case ui::AX_ATTR_CACHED_LINE_STARTS:
      return extensions::mojom::AXIntListAttribute::CACHED_LINE_STARTS;
    case ui::AX_ATTR_WORD_STARTS:
      return extensions::mojom::AXIntListAttribute::WORD_STARTS;
    case ui::AX_ATTR_WORD_ENDS:
      return extensions::mojom::AXIntListAttribute::WORD_ENDS;
  }
  NOTREACHED();
  return extensions::mojom::AXIntListAttribute::AX_INT_LIST_ATTRIBUTE_LAST;
}

// static
bool EnumTraits<extensions::mojom::AXIntListAttribute, ui::AXIntListAttribute>::
    FromMojom(extensions::mojom::AXIntListAttribute int_list_attribute,
              ui::AXIntListAttribute* out) {
  using extensions::mojom::AXIntListAttribute;
  switch (int_list_attribute) {
    case extensions::mojom::AXIntListAttribute::AX_INT_LIST_ATTRIBUTE_NONE:
      *out = ui::AX_INT_LIST_ATTRIBUTE_NONE;
      return true;
    case extensions::mojom::AXIntListAttribute::INDIRECT_CHILD_IDS:
      *out = ui::AX_ATTR_INDIRECT_CHILD_IDS;
      return true;
    case extensions::mojom::AXIntListAttribute::CONTROLS_IDS:
      *out = ui::AX_ATTR_CONTROLS_IDS;
      return true;
    case extensions::mojom::AXIntListAttribute::DESCRIBEDBY_IDS:
      *out = ui::AX_ATTR_DESCRIBEDBY_IDS;
      return true;
    case extensions::mojom::AXIntListAttribute::DETAILS_IDS:
      *out = ui::AX_ATTR_DETAILS_IDS;
      return true;
    case extensions::mojom::AXIntListAttribute::FLOWTO_IDS:
      *out = ui::AX_ATTR_FLOWTO_IDS;
      return true;
    case extensions::mojom::AXIntListAttribute::LABELLEDBY_IDS:
      *out = ui::AX_ATTR_LABELLEDBY_IDS;
      return true;
    case extensions::mojom::AXIntListAttribute::RADIO_GROUP_IDS:
      *out = ui::AX_ATTR_RADIO_GROUP_IDS;
      return true;
    case extensions::mojom::AXIntListAttribute::LINE_BREAKS:
      *out = ui::AX_ATTR_LINE_BREAKS;
      return true;
    case extensions::mojom::AXIntListAttribute::MARKER_TYPES:
      *out = ui::AX_ATTR_MARKER_TYPES;
      return true;
    case extensions::mojom::AXIntListAttribute::MARKER_STARTS:
      *out = ui::AX_ATTR_MARKER_STARTS;
      return true;
    case extensions::mojom::AXIntListAttribute::MARKER_ENDS:
      *out = ui::AX_ATTR_MARKER_ENDS;
      return true;
    case extensions::mojom::AXIntListAttribute::CELL_IDS:
      *out = ui::AX_ATTR_CELL_IDS;
      return true;
    case extensions::mojom::AXIntListAttribute::UNIQUE_CELL_IDS:
      *out = ui::AX_ATTR_UNIQUE_CELL_IDS;
      return true;
    case extensions::mojom::AXIntListAttribute::CHARACTER_OFFSETS:
      *out = ui::AX_ATTR_CHARACTER_OFFSETS;
      return true;
    case extensions::mojom::AXIntListAttribute::CACHED_LINE_STARTS:
      *out = ui::AX_ATTR_CACHED_LINE_STARTS;
      return true;
    case extensions::mojom::AXIntListAttribute::WORD_STARTS:
      *out = ui::AX_ATTR_WORD_STARTS;
      return true;
    case extensions::mojom::AXIntListAttribute::WORD_ENDS:
      *out = ui::AX_ATTR_WORD_ENDS;
      return true;
    case extensions::mojom::AXIntListAttribute::AX_INT_LIST_ATTRIBUTE_LAST:
      *out = ui::AX_INT_LIST_ATTRIBUTE_LAST;
      return true;
  }
  return false;
}

// static
extensions::mojom::AXRole
EnumTraits<extensions::mojom::AXRole, ui::AXRole>::ToMojom(ui::AXRole ax_role) {
  using extensions::mojom::AXRole;
  switch (ax_role) {
    case ui::AX_ROLE_NONE:
      return extensions::mojom::AXRole::NONE;
    case ui::AX_ROLE_ABBR:
      return extensions::mojom::AXRole::ABBR;
    case ui::AX_ROLE_ALERT_DIALOG:
      return extensions::mojom::AXRole::ALERT_DIALOG;
    case ui::AX_ROLE_ALERT:
      return extensions::mojom::AXRole::ALERT;
    case ui::AX_ROLE_ANCHOR:
      return extensions::mojom::AXRole::ANCHOR;
    case ui::AX_ROLE_ANNOTATION:
      return extensions::mojom::AXRole::ANNOTATION;
    case ui::AX_ROLE_APPLICATION:
      return extensions::mojom::AXRole::APPLICATION;
    case ui::AX_ROLE_ARTICLE:
      return extensions::mojom::AXRole::ARTICLE;
    case ui::AX_ROLE_AUDIO:
      return extensions::mojom::AXRole::AUDIO;
    case ui::AX_ROLE_BANNER:
      return extensions::mojom::AXRole::BANNER;
    case ui::AX_ROLE_BLOCKQUOTE:
      return extensions::mojom::AXRole::BLOCKQUOTE;
    case ui::AX_ROLE_BUSY_INDICATOR:
      return extensions::mojom::AXRole::BUSY_INDICATOR;
    case ui::AX_ROLE_BUTTON:
      return extensions::mojom::AXRole::BUTTON;
    case ui::AX_ROLE_BUTTON_DROP_DOWN:
      return extensions::mojom::AXRole::BUTTON_DROP_DOWN;
    case ui::AX_ROLE_CANVAS:
      return extensions::mojom::AXRole::CANVAS;
    case ui::AX_ROLE_CAPTION:
      return extensions::mojom::AXRole::CAPTION;
    case ui::AX_ROLE_CARET:
      return extensions::mojom::AXRole::CARET;
    case ui::AX_ROLE_CELL:
      return extensions::mojom::AXRole::CELL;
    case ui::AX_ROLE_CHECK_BOX:
      return extensions::mojom::AXRole::CHECK_BOX;
    case ui::AX_ROLE_CLIENT:
      return extensions::mojom::AXRole::CLIENT;
    case ui::AX_ROLE_COLOR_WELL:
      return extensions::mojom::AXRole::COLOR_WELL;
    case ui::AX_ROLE_COLUMN_HEADER:
      return extensions::mojom::AXRole::COLUMN_HEADER;
    case ui::AX_ROLE_COLUMN:
      return extensions::mojom::AXRole::COLUMN;
    case ui::AX_ROLE_COMBO_BOX:
      return extensions::mojom::AXRole::COMBO_BOX;
    case ui::AX_ROLE_COMPLEMENTARY:
      return extensions::mojom::AXRole::COMPLEMENTARY;
    case ui::AX_ROLE_CONTENT_INFO:
      return extensions::mojom::AXRole::CONTENT_INFO;
    case ui::AX_ROLE_DATE:
      return extensions::mojom::AXRole::DATE;
    case ui::AX_ROLE_DATE_TIME:
      return extensions::mojom::AXRole::DATE_TIME;
    case ui::AX_ROLE_DEFINITION:
      return extensions::mojom::AXRole::DEFINITION;
    case ui::AX_ROLE_DESCRIPTION_LIST_DETAIL:
      return extensions::mojom::AXRole::DESCRIPTION_LIST_DETAIL;
    case ui::AX_ROLE_DESCRIPTION_LIST:
      return extensions::mojom::AXRole::DESCRIPTION_LIST;
    case ui::AX_ROLE_DESCRIPTION_LIST_TERM:
      return extensions::mojom::AXRole::DESCRIPTION_LIST_TERM;
    case ui::AX_ROLE_DESKTOP:
      return extensions::mojom::AXRole::DESKTOP;
    case ui::AX_ROLE_DETAILS:
      return extensions::mojom::AXRole::DETAILS;
    case ui::AX_ROLE_DIALOG:
      return extensions::mojom::AXRole::DIALOG;
    case ui::AX_ROLE_DIRECTORY:
      return extensions::mojom::AXRole::DIRECTORY;
    case ui::AX_ROLE_DISCLOSURE_TRIANGLE:
      return extensions::mojom::AXRole::DISCLOSURE_TRIANGLE;
    case ui::AX_ROLE_DOCUMENT:
      return extensions::mojom::AXRole::DOCUMENT;
    case ui::AX_ROLE_EMBEDDED_OBJECT:
      return extensions::mojom::AXRole::EMBEDDED_OBJECT;
    case ui::AX_ROLE_FEED:
      return extensions::mojom::AXRole::FEED;
    case ui::AX_ROLE_FIGCAPTION:
      return extensions::mojom::AXRole::FIGCAPTION;
    case ui::AX_ROLE_FIGURE:
      return extensions::mojom::AXRole::FIGURE;
    case ui::AX_ROLE_FOOTER:
      return extensions::mojom::AXRole::FOOTER;
    case ui::AX_ROLE_FORM:
      return extensions::mojom::AXRole::FORM;
    case ui::AX_ROLE_GENERIC_CONTAINER:
      return extensions::mojom::AXRole::GENERIC_CONTAINER;
    case ui::AX_ROLE_GRID:
      return extensions::mojom::AXRole::GRID;
    case ui::AX_ROLE_GROUP:
      return extensions::mojom::AXRole::GROUP;
    case ui::AX_ROLE_HEADING:
      return extensions::mojom::AXRole::HEADING;
    case ui::AX_ROLE_IFRAME:
      return extensions::mojom::AXRole::IFRAME;
    case ui::AX_ROLE_IFRAME_PRESENTATIONAL:
      return extensions::mojom::AXRole::IFRAME_PRESENTATIONAL;
    case ui::AX_ROLE_IGNORED:
      return extensions::mojom::AXRole::IGNORED;
    case ui::AX_ROLE_IMAGE_MAP_LINK:
      return extensions::mojom::AXRole::IMAGE_MAP_LINK;
    case ui::AX_ROLE_IMAGE_MAP:
      return extensions::mojom::AXRole::IMAGE_MAP;
    case ui::AX_ROLE_IMAGE:
      return extensions::mojom::AXRole::IMAGE;
    case ui::AX_ROLE_INLINE_TEXT_BOX:
      return extensions::mojom::AXRole::INLINE_TEXT_BOX;
    case ui::AX_ROLE_INPUT_TIME:
      return extensions::mojom::AXRole::INPUT_TIME;
    case ui::AX_ROLE_LABEL_TEXT:
      return extensions::mojom::AXRole::LABEL_TEXT;
    case ui::AX_ROLE_LEGEND:
      return extensions::mojom::AXRole::LEGEND;
    case ui::AX_ROLE_LINE_BREAK:
      return extensions::mojom::AXRole::LINE_BREAK;
    case ui::AX_ROLE_LINK:
      return extensions::mojom::AXRole::LINK;
    case ui::AX_ROLE_LIST_BOX_OPTION:
      return extensions::mojom::AXRole::LIST_BOX_OPTION;
    case ui::AX_ROLE_LIST_BOX:
      return extensions::mojom::AXRole::LIST_BOX;
    case ui::AX_ROLE_LIST_ITEM:
      return extensions::mojom::AXRole::LIST_ITEM;
    case ui::AX_ROLE_LIST_MARKER:
      return extensions::mojom::AXRole::LIST_MARKER;
    case ui::AX_ROLE_LIST:
      return extensions::mojom::AXRole::LIST;
    case ui::AX_ROLE_LOCATION_BAR:
      return extensions::mojom::AXRole::LOCATION_BAR;
    case ui::AX_ROLE_LOG:
      return extensions::mojom::AXRole::LOG;
    case ui::AX_ROLE_MAIN:
      return extensions::mojom::AXRole::MAIN;
    case ui::AX_ROLE_MARK:
      return extensions::mojom::AXRole::MARK;
    case ui::AX_ROLE_MARQUEE:
      return extensions::mojom::AXRole::MARQUEE;
    case ui::AX_ROLE_MATH:
      return extensions::mojom::AXRole::MATH;
    case ui::AX_ROLE_MENU:
      return extensions::mojom::AXRole::MENU;
    case ui::AX_ROLE_MENU_BAR:
      return extensions::mojom::AXRole::MENU_BAR;
    case ui::AX_ROLE_MENU_BUTTON:
      return extensions::mojom::AXRole::MENU_BUTTON;
    case ui::AX_ROLE_MENU_ITEM:
      return extensions::mojom::AXRole::MENU_ITEM;
    case ui::AX_ROLE_MENU_ITEM_CHECK_BOX:
      return extensions::mojom::AXRole::MENU_ITEM_CHECK_BOX;
    case ui::AX_ROLE_MENU_ITEM_RADIO:
      return extensions::mojom::AXRole::MENU_ITEM_RADIO;
    case ui::AX_ROLE_MENU_LIST_OPTION:
      return extensions::mojom::AXRole::MENU_LIST_OPTION;
    case ui::AX_ROLE_MENU_LIST_POPUP:
      return extensions::mojom::AXRole::MENU_LIST_POPUP;
    case ui::AX_ROLE_METER:
      return extensions::mojom::AXRole::METER;
    case ui::AX_ROLE_NAVIGATION:
      return extensions::mojom::AXRole::NAVIGATION;
    case ui::AX_ROLE_NOTE:
      return extensions::mojom::AXRole::NOTE;
    case ui::AX_ROLE_OUTLINE:
      return extensions::mojom::AXRole::OUTLINE;
    case ui::AX_ROLE_PANE:
      return extensions::mojom::AXRole::PANE;
    case ui::AX_ROLE_PARAGRAPH:
      return extensions::mojom::AXRole::PARAGRAPH;
    case ui::AX_ROLE_POP_UP_BUTTON:
      return extensions::mojom::AXRole::POP_UP_BUTTON;
    case ui::AX_ROLE_PRE:
      return extensions::mojom::AXRole::PRE;
    case ui::AX_ROLE_PRESENTATIONAL:
      return extensions::mojom::AXRole::PRESENTATIONAL;
    case ui::AX_ROLE_PROGRESS_INDICATOR:
      return extensions::mojom::AXRole::PROGRESS_INDICATOR;
    case ui::AX_ROLE_RADIO_BUTTON:
      return extensions::mojom::AXRole::RADIO_BUTTON;
    case ui::AX_ROLE_RADIO_GROUP:
      return extensions::mojom::AXRole::RADIO_GROUP;
    case ui::AX_ROLE_REGION:
      return extensions::mojom::AXRole::REGION;
    case ui::AX_ROLE_ROOT_WEB_AREA:
      return extensions::mojom::AXRole::ROOT_WEB_AREA;
    case ui::AX_ROLE_ROW_HEADER:
      return extensions::mojom::AXRole::ROW_HEADER;
    case ui::AX_ROLE_ROW:
      return extensions::mojom::AXRole::ROW;
    case ui::AX_ROLE_RUBY:
      return extensions::mojom::AXRole::RUBY;
    case ui::AX_ROLE_RULER:
      return extensions::mojom::AXRole::RULER;
    case ui::AX_ROLE_SVG_ROOT:
      return extensions::mojom::AXRole::SVG_ROOT;
    case ui::AX_ROLE_SCROLL_AREA:
      return extensions::mojom::AXRole::SCROLL_AREA;
    case ui::AX_ROLE_SCROLL_BAR:
      return extensions::mojom::AXRole::SCROLL_BAR;
    case ui::AX_ROLE_SEAMLESS_WEB_AREA:
      return extensions::mojom::AXRole::SEAMLESS_WEB_AREA;
    case ui::AX_ROLE_SEARCH:
      return extensions::mojom::AXRole::SEARCH;
    case ui::AX_ROLE_SEARCH_BOX:
      return extensions::mojom::AXRole::SEARCH_BOX;
    case ui::AX_ROLE_SLIDER:
      return extensions::mojom::AXRole::SLIDER;
    case ui::AX_ROLE_SLIDER_THUMB:
      return extensions::mojom::AXRole::SLIDER_THUMB;
    case ui::AX_ROLE_SPIN_BUTTON_PART:
      return extensions::mojom::AXRole::SPIN_BUTTON_PART;
    case ui::AX_ROLE_SPIN_BUTTON:
      return extensions::mojom::AXRole::SPIN_BUTTON;
    case ui::AX_ROLE_SPLITTER:
      return extensions::mojom::AXRole::SPLITTER;
    case ui::AX_ROLE_STATIC_TEXT:
      return extensions::mojom::AXRole::STATIC_TEXT;
    case ui::AX_ROLE_STATUS:
      return extensions::mojom::AXRole::STATUS;
    case ui::AX_ROLE_SWITCH:
      return extensions::mojom::AXRole::SWITCH;
    case ui::AX_ROLE_TAB_GROUP:
      return extensions::mojom::AXRole::TAB_GROUP;
    case ui::AX_ROLE_TAB_LIST:
      return extensions::mojom::AXRole::TAB_LIST;
    case ui::AX_ROLE_TAB_PANEL:
      return extensions::mojom::AXRole::TAB_PANEL;
    case ui::AX_ROLE_TAB:
      return extensions::mojom::AXRole::TAB;
    case ui::AX_ROLE_TABLE_HEADER_CONTAINER:
      return extensions::mojom::AXRole::TABLE_HEADER_CONTAINER;
    case ui::AX_ROLE_TABLE:
      return extensions::mojom::AXRole::TABLE;
    case ui::AX_ROLE_TERM:
      return extensions::mojom::AXRole::TERM;
    case ui::AX_ROLE_TEXT_FIELD:
      return extensions::mojom::AXRole::TEXT_FIELD;
    case ui::AX_ROLE_TIME:
      return extensions::mojom::AXRole::TIME;
    case ui::AX_ROLE_TIMER:
      return extensions::mojom::AXRole::TIMER;
    case ui::AX_ROLE_TITLE_BAR:
      return extensions::mojom::AXRole::TITLE_BAR;
    case ui::AX_ROLE_TOGGLE_BUTTON:
      return extensions::mojom::AXRole::TOGGLE_BUTTON;
    case ui::AX_ROLE_TOOLBAR:
      return extensions::mojom::AXRole::TOOLBAR;
    case ui::AX_ROLE_TREE_GRID:
      return extensions::mojom::AXRole::TREE_GRID;
    case ui::AX_ROLE_TREE_ITEM:
      return extensions::mojom::AXRole::TREE_ITEM;
    case ui::AX_ROLE_TREE:
      return extensions::mojom::AXRole::TREE;
    case ui::AX_ROLE_UNKNOWN:
      return extensions::mojom::AXRole::UNKNOWN;
    case ui::AX_ROLE_TOOLTIP:
      return extensions::mojom::AXRole::TOOLTIP;
    case ui::AX_ROLE_VIDEO:
      return extensions::mojom::AXRole::VIDEO;
    case ui::AX_ROLE_WEB_AREA:
      return extensions::mojom::AXRole::WEB_AREA;
    case ui::AX_ROLE_WEB_VIEW:
      return extensions::mojom::AXRole::WEB_VIEW;
    case ui::AX_ROLE_WINDOW:
      return extensions::mojom::AXRole::WINDOW;
  }
  NOTREACHED();
  return extensions::mojom::AXRole::LAST;
}

// static
bool EnumTraits<extensions::mojom::AXRole, ui::AXRole>::FromMojom(
    extensions::mojom::AXRole ax_role,
    ui::AXRole* out) {
  using extensions::mojom::AXRole;
  switch (ax_role) {
    case extensions::mojom::AXRole::NONE:
      *out = ui::AX_ROLE_NONE;
      return true;
    case extensions::mojom::AXRole::ABBR:
      *out = ui::AX_ROLE_ABBR;
      return true;
    case extensions::mojom::AXRole::ALERT_DIALOG:
      *out = ui::AX_ROLE_ALERT_DIALOG;
      return true;
    case extensions::mojom::AXRole::ALERT:
      *out = ui::AX_ROLE_ALERT;
      return true;
    case extensions::mojom::AXRole::ANCHOR:
      *out = ui::AX_ROLE_ANCHOR;
      return true;
    case extensions::mojom::AXRole::ANNOTATION:
      *out = ui::AX_ROLE_ANNOTATION;
      return true;
    case extensions::mojom::AXRole::APPLICATION:
      *out = ui::AX_ROLE_APPLICATION;
      return true;
    case extensions::mojom::AXRole::ARTICLE:
      *out = ui::AX_ROLE_ARTICLE;
      return true;
    case extensions::mojom::AXRole::AUDIO:
      *out = ui::AX_ROLE_AUDIO;
      return true;
    case extensions::mojom::AXRole::BANNER:
      *out = ui::AX_ROLE_BANNER;
      return true;
    case extensions::mojom::AXRole::BLOCKQUOTE:
      *out = ui::AX_ROLE_BLOCKQUOTE;
      return true;
    case extensions::mojom::AXRole::BUSY_INDICATOR:
      *out = ui::AX_ROLE_BUSY_INDICATOR;
      return true;
    case extensions::mojom::AXRole::BUTTON:
      *out = ui::AX_ROLE_BUTTON;
      return true;
    case extensions::mojom::AXRole::BUTTON_DROP_DOWN:
      *out = ui::AX_ROLE_BUTTON_DROP_DOWN;
      return true;
    case extensions::mojom::AXRole::CANVAS:
      *out = ui::AX_ROLE_CANVAS;
      return true;
    case extensions::mojom::AXRole::CAPTION:
      *out = ui::AX_ROLE_CAPTION;
      return true;
    case extensions::mojom::AXRole::CARET:
      *out = ui::AX_ROLE_CARET;
      return true;
    case extensions::mojom::AXRole::CELL:
      *out = ui::AX_ROLE_CELL;
      return true;
    case extensions::mojom::AXRole::CHECK_BOX:
      *out = ui::AX_ROLE_CHECK_BOX;
      return true;
    case extensions::mojom::AXRole::CLIENT:
      *out = ui::AX_ROLE_CLIENT;
      return true;
    case extensions::mojom::AXRole::COLOR_WELL:
      *out = ui::AX_ROLE_COLOR_WELL;
      return true;
    case extensions::mojom::AXRole::COLUMN_HEADER:
      *out = ui::AX_ROLE_COLUMN_HEADER;
      return true;
    case extensions::mojom::AXRole::COLUMN:
      *out = ui::AX_ROLE_COLUMN;
      return true;
    case extensions::mojom::AXRole::COMBO_BOX:
      *out = ui::AX_ROLE_COMBO_BOX;
      return true;
    case extensions::mojom::AXRole::COMPLEMENTARY:
      *out = ui::AX_ROLE_COMPLEMENTARY;
      return true;
    case extensions::mojom::AXRole::CONTENT_INFO:
      *out = ui::AX_ROLE_CONTENT_INFO;
      return true;
    case extensions::mojom::AXRole::DATE:
      *out = ui::AX_ROLE_DATE;
      return true;
    case extensions::mojom::AXRole::DATE_TIME:
      *out = ui::AX_ROLE_DATE_TIME;
      return true;
    case extensions::mojom::AXRole::DEFINITION:
      *out = ui::AX_ROLE_DEFINITION;
      return true;
    case extensions::mojom::AXRole::DESCRIPTION_LIST_DETAIL:
      *out = ui::AX_ROLE_DESCRIPTION_LIST_DETAIL;
      return true;
    case extensions::mojom::AXRole::DESCRIPTION_LIST:
      *out = ui::AX_ROLE_DESCRIPTION_LIST;
      return true;
    case extensions::mojom::AXRole::DESCRIPTION_LIST_TERM:
      *out = ui::AX_ROLE_DESCRIPTION_LIST_TERM;
      return true;
    case extensions::mojom::AXRole::DESKTOP:
      *out = ui::AX_ROLE_DESKTOP;
      return true;
    case extensions::mojom::AXRole::DETAILS:
      *out = ui::AX_ROLE_DETAILS;
      return true;
    case extensions::mojom::AXRole::DIALOG:
      *out = ui::AX_ROLE_DIALOG;
      return true;
    case extensions::mojom::AXRole::DIRECTORY:
      *out = ui::AX_ROLE_DIRECTORY;
      return true;
    case extensions::mojom::AXRole::DISCLOSURE_TRIANGLE:
      *out = ui::AX_ROLE_DISCLOSURE_TRIANGLE;
      return true;
    case extensions::mojom::AXRole::DOCUMENT:
      *out = ui::AX_ROLE_DOCUMENT;
      return true;
    case extensions::mojom::AXRole::EMBEDDED_OBJECT:
      *out = ui::AX_ROLE_EMBEDDED_OBJECT;
      return true;
    case extensions::mojom::AXRole::FEED:
      *out = ui::AX_ROLE_FEED;
      return true;
    case extensions::mojom::AXRole::FIGCAPTION:
      *out = ui::AX_ROLE_FIGCAPTION;
      return true;
    case extensions::mojom::AXRole::FIGURE:
      *out = ui::AX_ROLE_FIGURE;
      return true;
    case extensions::mojom::AXRole::FOOTER:
      *out = ui::AX_ROLE_FOOTER;
      return true;
    case extensions::mojom::AXRole::FORM:
      *out = ui::AX_ROLE_FORM;
      return true;
    case extensions::mojom::AXRole::GENERIC_CONTAINER:
      *out = ui::AX_ROLE_GENERIC_CONTAINER;
      return true;
    case extensions::mojom::AXRole::GRID:
      *out = ui::AX_ROLE_GRID;
      return true;
    case extensions::mojom::AXRole::GROUP:
      *out = ui::AX_ROLE_GROUP;
      return true;
    case extensions::mojom::AXRole::HEADING:
      *out = ui::AX_ROLE_HEADING;
      return true;
    case extensions::mojom::AXRole::IFRAME:
      *out = ui::AX_ROLE_IFRAME;
      return true;
    case extensions::mojom::AXRole::IFRAME_PRESENTATIONAL:
      *out = ui::AX_ROLE_IFRAME_PRESENTATIONAL;
      return true;
    case extensions::mojom::AXRole::IGNORED:
      *out = ui::AX_ROLE_IGNORED;
      return true;
    case extensions::mojom::AXRole::IMAGE_MAP_LINK:
      *out = ui::AX_ROLE_IMAGE_MAP_LINK;
      return true;
    case extensions::mojom::AXRole::IMAGE_MAP:
      *out = ui::AX_ROLE_IMAGE_MAP;
      return true;
    case extensions::mojom::AXRole::IMAGE:
      *out = ui::AX_ROLE_IMAGE;
      return true;
    case extensions::mojom::AXRole::INLINE_TEXT_BOX:
      *out = ui::AX_ROLE_INLINE_TEXT_BOX;
      return true;
    case extensions::mojom::AXRole::INPUT_TIME:
      *out = ui::AX_ROLE_INPUT_TIME;
      return true;
    case extensions::mojom::AXRole::LABEL_TEXT:
      *out = ui::AX_ROLE_LABEL_TEXT;
      return true;
    case extensions::mojom::AXRole::LEGEND:
      *out = ui::AX_ROLE_LEGEND;
      return true;
    case extensions::mojom::AXRole::LINE_BREAK:
      *out = ui::AX_ROLE_LINE_BREAK;
      return true;
    case extensions::mojom::AXRole::LINK:
      *out = ui::AX_ROLE_LINK;
      return true;
    case extensions::mojom::AXRole::LIST_BOX_OPTION:
      *out = ui::AX_ROLE_LIST_BOX_OPTION;
      return true;
    case extensions::mojom::AXRole::LIST_BOX:
      *out = ui::AX_ROLE_LIST_BOX;
      return true;
    case extensions::mojom::AXRole::LIST_ITEM:
      *out = ui::AX_ROLE_LIST_ITEM;
      return true;
    case extensions::mojom::AXRole::LIST_MARKER:
      *out = ui::AX_ROLE_LIST_MARKER;
      return true;
    case extensions::mojom::AXRole::LIST:
      *out = ui::AX_ROLE_LIST;
      return true;
    case extensions::mojom::AXRole::LOCATION_BAR:
      *out = ui::AX_ROLE_LOCATION_BAR;
      return true;
    case extensions::mojom::AXRole::LOG:
      *out = ui::AX_ROLE_LOG;
      return true;
    case extensions::mojom::AXRole::MAIN:
      *out = ui::AX_ROLE_MAIN;
      return true;
    case extensions::mojom::AXRole::MARK:
      *out = ui::AX_ROLE_MARK;
      return true;
    case extensions::mojom::AXRole::MARQUEE:
      *out = ui::AX_ROLE_MARQUEE;
      return true;
    case extensions::mojom::AXRole::MATH:
      *out = ui::AX_ROLE_MATH;
      return true;
    case extensions::mojom::AXRole::MENU:
      *out = ui::AX_ROLE_MENU;
      return true;
    case extensions::mojom::AXRole::MENU_BAR:
      *out = ui::AX_ROLE_MENU_BAR;
      return true;
    case extensions::mojom::AXRole::MENU_BUTTON:
      *out = ui::AX_ROLE_MENU_BUTTON;
      return true;
    case extensions::mojom::AXRole::MENU_ITEM:
      *out = ui::AX_ROLE_MENU_ITEM;
      return true;
    case extensions::mojom::AXRole::MENU_ITEM_CHECK_BOX:
      *out = ui::AX_ROLE_MENU_ITEM_CHECK_BOX;
      return true;
    case extensions::mojom::AXRole::MENU_ITEM_RADIO:
      *out = ui::AX_ROLE_MENU_ITEM_RADIO;
      return true;
    case extensions::mojom::AXRole::MENU_LIST_OPTION:
      *out = ui::AX_ROLE_MENU_LIST_OPTION;
      return true;
    case extensions::mojom::AXRole::MENU_LIST_POPUP:
      *out = ui::AX_ROLE_MENU_LIST_POPUP;
      return true;
    case extensions::mojom::AXRole::METER:
      *out = ui::AX_ROLE_METER;
      return true;
    case extensions::mojom::AXRole::NAVIGATION:
      *out = ui::AX_ROLE_NAVIGATION;
      return true;
    case extensions::mojom::AXRole::NOTE:
      *out = ui::AX_ROLE_NOTE;
      return true;
    case extensions::mojom::AXRole::OUTLINE:
      *out = ui::AX_ROLE_OUTLINE;
      return true;
    case extensions::mojom::AXRole::PANE:
      *out = ui::AX_ROLE_PANE;
      return true;
    case extensions::mojom::AXRole::PARAGRAPH:
      *out = ui::AX_ROLE_PARAGRAPH;
      return true;
    case extensions::mojom::AXRole::POP_UP_BUTTON:
      *out = ui::AX_ROLE_POP_UP_BUTTON;
      return true;
    case extensions::mojom::AXRole::PRE:
      *out = ui::AX_ROLE_PRE;
      return true;
    case extensions::mojom::AXRole::PRESENTATIONAL:
      *out = ui::AX_ROLE_PRESENTATIONAL;
      return true;
    case extensions::mojom::AXRole::PROGRESS_INDICATOR:
      *out = ui::AX_ROLE_PROGRESS_INDICATOR;
      return true;
    case extensions::mojom::AXRole::RADIO_BUTTON:
      *out = ui::AX_ROLE_RADIO_BUTTON;
      return true;
    case extensions::mojom::AXRole::RADIO_GROUP:
      *out = ui::AX_ROLE_RADIO_GROUP;
      return true;
    case extensions::mojom::AXRole::REGION:
      *out = ui::AX_ROLE_REGION;
      return true;
    case extensions::mojom::AXRole::ROOT_WEB_AREA:
      *out = ui::AX_ROLE_ROOT_WEB_AREA;
      return true;
    case extensions::mojom::AXRole::ROW_HEADER:
      *out = ui::AX_ROLE_ROW_HEADER;
      return true;
    case extensions::mojom::AXRole::ROW:
      *out = ui::AX_ROLE_ROW;
      return true;
    case extensions::mojom::AXRole::RUBY:
      *out = ui::AX_ROLE_RUBY;
      return true;
    case extensions::mojom::AXRole::RULER:
      *out = ui::AX_ROLE_RULER;
      return true;
    case extensions::mojom::AXRole::SVG_ROOT:
      *out = ui::AX_ROLE_SVG_ROOT;
      return true;
    case extensions::mojom::AXRole::SCROLL_AREA:
      *out = ui::AX_ROLE_SCROLL_AREA;
      return true;
    case extensions::mojom::AXRole::SCROLL_BAR:
      *out = ui::AX_ROLE_SCROLL_BAR;
      return true;
    case extensions::mojom::AXRole::SEAMLESS_WEB_AREA:
      *out = ui::AX_ROLE_SEAMLESS_WEB_AREA;
      return true;
    case extensions::mojom::AXRole::SEARCH:
      *out = ui::AX_ROLE_SEARCH;
      return true;
    case extensions::mojom::AXRole::SEARCH_BOX:
      *out = ui::AX_ROLE_SEARCH_BOX;
      return true;
    case extensions::mojom::AXRole::SLIDER:
      *out = ui::AX_ROLE_SLIDER;
      return true;
    case extensions::mojom::AXRole::SLIDER_THUMB:
      *out = ui::AX_ROLE_SLIDER_THUMB;
      return true;
    case extensions::mojom::AXRole::SPIN_BUTTON_PART:
      *out = ui::AX_ROLE_SPIN_BUTTON_PART;
      return true;
    case extensions::mojom::AXRole::SPIN_BUTTON:
      *out = ui::AX_ROLE_SPIN_BUTTON;
      return true;
    case extensions::mojom::AXRole::SPLITTER:
      *out = ui::AX_ROLE_SPLITTER;
      return true;
    case extensions::mojom::AXRole::STATIC_TEXT:
      *out = ui::AX_ROLE_STATIC_TEXT;
      return true;
    case extensions::mojom::AXRole::STATUS:
      *out = ui::AX_ROLE_STATUS;
      return true;
    case extensions::mojom::AXRole::SWITCH:
      *out = ui::AX_ROLE_SWITCH;
      return true;
    case extensions::mojom::AXRole::TAB_GROUP:
      *out = ui::AX_ROLE_TAB_GROUP;
      return true;
    case extensions::mojom::AXRole::TAB_LIST:
      *out = ui::AX_ROLE_TAB_LIST;
      return true;
    case extensions::mojom::AXRole::TAB_PANEL:
      *out = ui::AX_ROLE_TAB_PANEL;
      return true;
    case extensions::mojom::AXRole::TAB:
      *out = ui::AX_ROLE_TAB;
      return true;
    case extensions::mojom::AXRole::TABLE_HEADER_CONTAINER:
      *out = ui::AX_ROLE_TABLE_HEADER_CONTAINER;
      return true;
    case extensions::mojom::AXRole::TABLE:
      *out = ui::AX_ROLE_TABLE;
      return true;
    case extensions::mojom::AXRole::TERM:
      *out = ui::AX_ROLE_TERM;
      return true;
    case extensions::mojom::AXRole::TEXT_FIELD:
      *out = ui::AX_ROLE_TEXT_FIELD;
      return true;
    case extensions::mojom::AXRole::TIME:
      *out = ui::AX_ROLE_TIME;
      return true;
    case extensions::mojom::AXRole::TIMER:
      *out = ui::AX_ROLE_TIMER;
      return true;
    case extensions::mojom::AXRole::TITLE_BAR:
      *out = ui::AX_ROLE_TITLE_BAR;
      return true;
    case extensions::mojom::AXRole::TOGGLE_BUTTON:
      *out = ui::AX_ROLE_TOGGLE_BUTTON;
      return true;
    case extensions::mojom::AXRole::TOOLBAR:
      *out = ui::AX_ROLE_TOOLBAR;
      return true;
    case extensions::mojom::AXRole::TREE_GRID:
      *out = ui::AX_ROLE_TREE_GRID;
      return true;
    case extensions::mojom::AXRole::TREE_ITEM:
      *out = ui::AX_ROLE_TREE_ITEM;
      return true;
    case extensions::mojom::AXRole::TREE:
      *out = ui::AX_ROLE_TREE;
      return true;
    case extensions::mojom::AXRole::UNKNOWN:
      *out = ui::AX_ROLE_UNKNOWN;
      return true;
    case extensions::mojom::AXRole::TOOLTIP:
      *out = ui::AX_ROLE_TOOLTIP;
      return true;
    case extensions::mojom::AXRole::VIDEO:
      *out = ui::AX_ROLE_VIDEO;
      return true;
    case extensions::mojom::AXRole::WEB_AREA:
      *out = ui::AX_ROLE_WEB_AREA;
      return true;
    case extensions::mojom::AXRole::WEB_VIEW:
      *out = ui::AX_ROLE_WEB_VIEW;
      return true;
    case extensions::mojom::AXRole::WINDOW:
      *out = ui::AX_ROLE_WINDOW;
      return true;
    case extensions::mojom::AXRole::LAST:
      *out = ui::AX_ROLE_LAST;
      return true;
  }
  return false;
}

// static
extensions::mojom::AXTextAffinity
EnumTraits<extensions::mojom::AXTextAffinity, ui::AXTextAffinity>::ToMojom(
    ui::AXTextAffinity ax_text_affinity) {
  using extensions::mojom::AXTextAffinity;
  switch (ax_text_affinity) {
    case ui::AX_TEXT_AFFINITY_NONE:
      return extensions::mojom::AXTextAffinity::NONE;
    case ui::AX_TEXT_AFFINITY_DOWNSTREAM:
      return extensions::mojom::AXTextAffinity::DOWNSTREAM;
    case ui::AX_TEXT_AFFINITY_UPSTREAM:
      return extensions::mojom::AXTextAffinity::UPSTREAM;
  }
  NOTREACHED();
  return extensions::mojom::AXTextAffinity::LAST;
}

// static
bool EnumTraits<extensions::mojom::AXTextAffinity,
                ui::AXTextAffinity>::FromMojom(extensions::mojom::AXTextAffinity
                                                   ax_text_affinity,
                                               ui::AXTextAffinity* out) {
  using extensions::mojom::AXTextAffinity;
  switch (ax_text_affinity) {
    case extensions::mojom::AXTextAffinity::NONE:
      *out = ui::AX_TEXT_AFFINITY_NONE;
      return true;
    case extensions::mojom::AXTextAffinity::DOWNSTREAM:
      *out = ui::AX_TEXT_AFFINITY_DOWNSTREAM;
      return true;
    case extensions::mojom::AXTextAffinity::UPSTREAM:
      *out = ui::AX_TEXT_AFFINITY_UPSTREAM;
      return true;
    case extensions::mojom::AXTextAffinity::LAST:
      *out = ui::AX_TEXT_AFFINITY_LAST;
      return true;
  }
  return false;
}

}  // namespace mojo
