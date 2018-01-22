// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/accessibility/blink_ax_enum_conversion.h"

#include "base/logging.h"

namespace content {

uint32_t AXStateFromBlink(const blink::WebAXObject& o) {
  uint32_t state = 0;

  blink::WebAXExpanded expanded = o.IsExpanded();
  if (expanded) {
    if (expanded == blink::kWebAXExpandedCollapsed)
      state |= (1 << static_cast<int32_t>(ax::mojom::State::COLLAPSED));
    else if (expanded == blink::kWebAXExpandedExpanded)
      state |= (1 << static_cast<int32_t>(ax::mojom::State::EXPANDED));
  }

  if (o.CanSetFocusAttribute())
    state |= (1 << static_cast<int32_t>(ax::mojom::State::FOCUSABLE));

  if (o.Role() == blink::kWebAXRolePopUpButton || o.AriaHasPopup())
    state |= (1 << static_cast<int32_t>(ax::mojom::State::HASPOPUP));

  if (o.IsHovered())
    state |= (1 << static_cast<int32_t>(ax::mojom::State::HOVERED));

  if (!o.IsVisible())
    state |= (1 << static_cast<int32_t>(ax::mojom::State::INVISIBLE));

  if (o.IsLinked())
    state |= (1 << static_cast<int32_t>(ax::mojom::State::LINKED));

  if (o.IsMultiline())
    state |= (1 << static_cast<int32_t>(ax::mojom::State::MULTILINE));

  if (o.IsMultiSelectable())
    state |= (1 << static_cast<int32_t>(ax::mojom::State::MULTISELECTABLE));

  if (o.IsPasswordField())
    state |= (1 << static_cast<int32_t>(ax::mojom::State::PROTECTED));

  if (o.IsRequired())
    state |= (1 << static_cast<int32_t>(ax::mojom::State::REQUIRED));

  if (o.IsSelected() != blink::kWebAXSelectedStateUndefined)
    state |= (1 << static_cast<int32_t>(ax::mojom::State::SELECTABLE));

  if (o.IsEditable())
    state |= (1 << static_cast<int32_t>(ax::mojom::State::EDITABLE));

  if (o.IsSelected() == blink::kWebAXSelectedStateTrue)
    state |= (1 << static_cast<int32_t>(ax::mojom::State::SELECTED));

  if (o.IsRichlyEditable())
    state |= (1 << static_cast<int32_t>(ax::mojom::State::RICHLY_EDITABLE));

  if (o.IsVisited())
    state |= (1 << static_cast<int32_t>(ax::mojom::State::VISITED));

  if (o.Orientation() == blink::kWebAXOrientationVertical)
    state |= (1 << static_cast<int32_t>(ax::mojom::State::VERTICAL));
  else if (o.Orientation() == blink::kWebAXOrientationHorizontal)
    state |= (1 << static_cast<int32_t>(ax::mojom::State::HORIZONTAL));

  if (o.IsVisited())
    state |= (1 << static_cast<int32_t>(ax::mojom::State::VISITED));

  return state;
}

ax::mojom::Role AXRoleFromBlink(blink::WebAXRole role) {
  switch (role) {
    case blink::kWebAXRoleAbbr:
      return ax::mojom::Role::ABBR;
    case blink::kWebAXRoleAlert:
      return ax::mojom::Role::ALERT;
    case blink::kWebAXRoleAlertDialog:
      return ax::mojom::Role::ALERT_DIALOG;
    case blink::kWebAXRoleAnchor:
      return ax::mojom::Role::ANCHOR;
    case blink::kWebAXRoleAnnotation:
      return ax::mojom::Role::ANNOTATION;
    case blink::kWebAXRoleApplication:
      return ax::mojom::Role::APPLICATION;
    case blink::kWebAXRoleArticle:
      return ax::mojom::Role::ARTICLE;
    case blink::kWebAXRoleAudio:
      return ax::mojom::Role::AUDIO;
    case blink::kWebAXRoleBanner:
      return ax::mojom::Role::BANNER;
    case blink::kWebAXRoleBlockquote:
      return ax::mojom::Role::BLOCKQUOTE;
    case blink::kWebAXRoleButton:
      return ax::mojom::Role::BUTTON;
    case blink::kWebAXRoleCanvas:
      return ax::mojom::Role::CANVAS;
    case blink::kWebAXRoleCaption:
      return ax::mojom::Role::CAPTION;
    case blink::kWebAXRoleCell:
      return ax::mojom::Role::CELL;
    case blink::kWebAXRoleCheckBox:
      return ax::mojom::Role::CHECK_BOX;
    case blink::kWebAXRoleColorWell:
      return ax::mojom::Role::COLOR_WELL;
    case blink::kWebAXRoleColumn:
      return ax::mojom::Role::COLUMN;
    case blink::kWebAXRoleColumnHeader:
      return ax::mojom::Role::COLUMN_HEADER;
    case blink::kWebAXRoleComboBoxGrouping:
      return ax::mojom::Role::COMBO_BOX_GROUPING;
    case blink::kWebAXRoleComboBoxMenuButton:
      return ax::mojom::Role::COMBO_BOX_MENU_BUTTON;
    case blink::kWebAXRoleComplementary:
      return ax::mojom::Role::COMPLEMENTARY;
    case blink::kWebAXRoleContentInfo:
      return ax::mojom::Role::CONTENT_INFO;
    case blink::kWebAXRoleDate:
      return ax::mojom::Role::DATE;
    case blink::kWebAXRoleDateTime:
      return ax::mojom::Role::DATE_TIME;
    case blink::kWebAXRoleDefinition:
      return ax::mojom::Role::DEFINITION;
    case blink::kWebAXRoleDescriptionListDetail:
      return ax::mojom::Role::DESCRIPTION_LIST_DETAIL;
    case blink::kWebAXRoleDescriptionList:
      return ax::mojom::Role::DESCRIPTION_LIST;
    case blink::kWebAXRoleDescriptionListTerm:
      return ax::mojom::Role::DESCRIPTION_LIST_TERM;
    case blink::kWebAXRoleDetails:
      return ax::mojom::Role::DETAILS;
    case blink::kWebAXRoleDialog:
      return ax::mojom::Role::DIALOG;
    case blink::kWebAXRoleDirectory:
      return ax::mojom::Role::DIRECTORY;
    case blink::kWebAXRoleDisclosureTriangle:
      return ax::mojom::Role::DISCLOSURE_TRIANGLE;
    case blink::kWebAXRoleDocument:
      return ax::mojom::Role::DOCUMENT;
    case blink::kWebAXRoleEmbeddedObject:
      return ax::mojom::Role::EMBEDDED_OBJECT;
    case blink::kWebAXRoleFeed:
      return ax::mojom::Role::FEED;
    case blink::kWebAXRoleFigcaption:
      return ax::mojom::Role::FIGCAPTION;
    case blink::kWebAXRoleFigure:
      return ax::mojom::Role::FIGURE;
    case blink::kWebAXRoleFooter:
      return ax::mojom::Role::FOOTER;
    case blink::kWebAXRoleForm:
      return ax::mojom::Role::FORM;
    case blink::kWebAXRoleGenericContainer:
      return ax::mojom::Role::GENERIC_CONTAINER;
    case blink::kWebAXRoleGrid:
      return ax::mojom::Role::GRID;
    case blink::kWebAXRoleGroup:
      return ax::mojom::Role::GROUP;
    case blink::kWebAXRoleHeading:
      return ax::mojom::Role::HEADING;
    case blink::kWebAXRoleIframe:
      return ax::mojom::Role::IFRAME;
    case blink::kWebAXRoleIframePresentational:
      return ax::mojom::Role::IFRAME_PRESENTATIONAL;
    case blink::kWebAXRoleIgnored:
      return ax::mojom::Role::IGNORED;
    case blink::kWebAXRoleImage:
      return ax::mojom::Role::IMAGE;
    case blink::kWebAXRoleImageMap:
      return ax::mojom::Role::IMAGE_MAP;
    case blink::kWebAXRoleInlineTextBox:
      return ax::mojom::Role::INLINE_TEXT_BOX;
    case blink::kWebAXRoleInputTime:
      return ax::mojom::Role::INPUT_TIME;
    case blink::kWebAXRoleLabel:
      return ax::mojom::Role::LABEL_TEXT;
    case blink::kWebAXRoleLegend:
      return ax::mojom::Role::LEGEND;
    case blink::kWebAXRoleLink:
      return ax::mojom::Role::LINK;
    case blink::kWebAXRoleList:
      return ax::mojom::Role::LIST;
    case blink::kWebAXRoleListBox:
      return ax::mojom::Role::LIST_BOX;
    case blink::kWebAXRoleListBoxOption:
      return ax::mojom::Role::LIST_BOX_OPTION;
    case blink::kWebAXRoleListItem:
      return ax::mojom::Role::LIST_ITEM;
    case blink::kWebAXRoleListMarker:
      return ax::mojom::Role::LIST_MARKER;
    case blink::kWebAXRoleLog:
      return ax::mojom::Role::LOG;
    case blink::kWebAXRoleMain:
      return ax::mojom::Role::MAIN;
    case blink::kWebAXRoleMarquee:
      return ax::mojom::Role::MARQUEE;
    case blink::kWebAXRoleMark:
      return ax::mojom::Role::MARK;
    case blink::kWebAXRoleMath:
      return ax::mojom::Role::MATH;
    case blink::kWebAXRoleMenu:
      return ax::mojom::Role::MENU;
    case blink::kWebAXRoleMenuBar:
      return ax::mojom::Role::MENU_BAR;
    case blink::kWebAXRoleMenuButton:
      return ax::mojom::Role::MENU_BUTTON;
    case blink::kWebAXRoleMenuItem:
      return ax::mojom::Role::MENU_ITEM;
    case blink::kWebAXRoleMenuItemCheckBox:
      return ax::mojom::Role::MENU_ITEM_CHECK_BOX;
    case blink::kWebAXRoleMenuItemRadio:
      return ax::mojom::Role::MENU_ITEM_RADIO;
    case blink::kWebAXRoleMenuListOption:
      return ax::mojom::Role::MENU_LIST_OPTION;
    case blink::kWebAXRoleMenuListPopup:
      return ax::mojom::Role::MENU_LIST_POPUP;
    case blink::kWebAXRoleMeter:
      return ax::mojom::Role::METER;
    case blink::kWebAXRoleNavigation:
      return ax::mojom::Role::NAVIGATION;
    case blink::kWebAXRoleNone:
      return ax::mojom::Role::NONE;
    case blink::kWebAXRoleNote:
      return ax::mojom::Role::NOTE;
    case blink::kWebAXRoleParagraph:
      return ax::mojom::Role::PARAGRAPH;
    case blink::kWebAXRolePopUpButton:
      return ax::mojom::Role::POP_UP_BUTTON;
    case blink::kWebAXRolePre:
      return ax::mojom::Role::PRE;
    case blink::kWebAXRolePresentational:
      return ax::mojom::Role::PRESENTATIONAL;
    case blink::kWebAXRoleProgressIndicator:
      return ax::mojom::Role::PROGRESS_INDICATOR;
    case blink::kWebAXRoleRadioButton:
      return ax::mojom::Role::RADIO_BUTTON;
    case blink::kWebAXRoleRadioGroup:
      return ax::mojom::Role::RADIO_GROUP;
    case blink::kWebAXRoleRegion:
      return ax::mojom::Role::REGION;
    case blink::kWebAXRoleRow:
      return ax::mojom::Role::ROW;
    case blink::kWebAXRoleRuby:
      return ax::mojom::Role::RUBY;
    case blink::kWebAXRoleRowHeader:
      return ax::mojom::Role::ROW_HEADER;
    case blink::kWebAXRoleSVGRoot:
      return ax::mojom::Role::SVG_ROOT;
    case blink::kWebAXRoleScrollBar:
      return ax::mojom::Role::SCROLL_BAR;
    case blink::kWebAXRoleSearch:
      return ax::mojom::Role::SEARCH;
    case blink::kWebAXRoleSearchBox:
      return ax::mojom::Role::SEARCH_BOX;
    case blink::kWebAXRoleSlider:
      return ax::mojom::Role::SLIDER;
    case blink::kWebAXRoleSliderThumb:
      return ax::mojom::Role::SLIDER_THUMB;
    case blink::kWebAXRoleSpinButton:
      return ax::mojom::Role::SPIN_BUTTON;
    case blink::kWebAXRoleSpinButtonPart:
      return ax::mojom::Role::SPIN_BUTTON_PART;
    case blink::kWebAXRoleSplitter:
      return ax::mojom::Role::SPLITTER;
    case blink::kWebAXRoleStaticText:
      return ax::mojom::Role::STATIC_TEXT;
    case blink::kWebAXRoleStatus:
      return ax::mojom::Role::STATUS;
    case blink::kWebAXRoleSwitch:
      return ax::mojom::Role::SWITCH;
    case blink::kWebAXRoleTab:
      return ax::mojom::Role::TAB;
    case blink::kWebAXRoleTabList:
      return ax::mojom::Role::TAB_LIST;
    case blink::kWebAXRoleTabPanel:
      return ax::mojom::Role::TAB_PANEL;
    case blink::kWebAXRoleTable:
      return ax::mojom::Role::TABLE;
    case blink::kWebAXRoleTableHeaderContainer:
      return ax::mojom::Role::TABLE_HEADER_CONTAINER;
    case blink::kWebAXRoleTerm:
      return ax::mojom::Role::TERM;
    case blink::kWebAXRoleTextField:
      return ax::mojom::Role::TEXT_FIELD;
    case blink::kWebAXRoleTextFieldWithComboBox:
      return ax::mojom::Role::TEXT_FIELD_WITH_COMBO_BOX;
    case blink::kWebAXRoleTime:
      return ax::mojom::Role::TIME;
    case blink::kWebAXRoleTimer:
      return ax::mojom::Role::TIMER;
    case blink::kWebAXRoleToggleButton:
      return ax::mojom::Role::TOGGLE_BUTTON;
    case blink::kWebAXRoleToolbar:
      return ax::mojom::Role::TOOLBAR;
    case blink::kWebAXRoleTree:
      return ax::mojom::Role::TREE;
    case blink::kWebAXRoleTreeGrid:
      return ax::mojom::Role::TREE_GRID;
    case blink::kWebAXRoleTreeItem:
      return ax::mojom::Role::TREE_ITEM;
    case blink::kWebAXRoleUnknown:
      return ax::mojom::Role::UNKNOWN;
    case blink::kWebAXRoleUserInterfaceTooltip:
      return ax::mojom::Role::TOOLTIP;
    case blink::kWebAXRoleVideo:
      return ax::mojom::Role::VIDEO;
    case blink::kWebAXRoleWebArea:
      return ax::mojom::Role::ROOT_WEB_AREA;
    case blink::kWebAXRoleLineBreak:
      return ax::mojom::Role::LINE_BREAK;
    default:
      return ax::mojom::Role::UNKNOWN;
  }
}

ax::mojom::Event AXEventFromBlink(blink::WebAXEvent event) {
  switch (event) {
    case blink::kWebAXEventActiveDescendantChanged:
      return ax::mojom::Event::ACTIVEDESCENDANTCHANGED;
    case blink::kWebAXEventAriaAttributeChanged:
      return ax::mojom::Event::ARIA_ATTRIBUTE_CHANGED;
    case blink::kWebAXEventAutocorrectionOccured:
      return ax::mojom::Event::AUTOCORRECTION_OCCURED;
    case blink::kWebAXEventBlur:
      return ax::mojom::Event::BLUR;
    case blink::kWebAXEventCheckedStateChanged:
      return ax::mojom::Event::CHECKED_STATE_CHANGED;
    case blink::kWebAXEventChildrenChanged:
      return ax::mojom::Event::CHILDREN_CHANGED;
    case blink::kWebAXEventClicked:
      return ax::mojom::Event::CLICKED;
    case blink::kWebAXEventDocumentSelectionChanged:
      return ax::mojom::Event::DOCUMENT_SELECTION_CHANGED;
    case blink::kWebAXEventExpandedChanged:
      return ax::mojom::Event::EXPANDED_CHANGED;
    case blink::kWebAXEventFocus:
      return ax::mojom::Event::FOCUS;
    case blink::kWebAXEventHover:
      return ax::mojom::Event::HOVER;
    case blink::kWebAXEventInvalidStatusChanged:
      return ax::mojom::Event::INVALID_STATUS_CHANGED;
    case blink::kWebAXEventLayoutComplete:
      return ax::mojom::Event::LAYOUT_COMPLETE;
    case blink::kWebAXEventLiveRegionChanged:
      return ax::mojom::Event::LIVE_REGION_CHANGED;
    case blink::kWebAXEventLoadComplete:
      return ax::mojom::Event::LOAD_COMPLETE;
    case blink::kWebAXEventLocationChanged:
      return ax::mojom::Event::LOCATION_CHANGED;
    case blink::kWebAXEventMenuListItemSelected:
      return ax::mojom::Event::MENU_LIST_ITEM_SELECTED;
    case blink::kWebAXEventMenuListItemUnselected:
      return ax::mojom::Event::MENU_LIST_ITEM_SELECTED;
    case blink::kWebAXEventMenuListValueChanged:
      return ax::mojom::Event::MENU_LIST_VALUE_CHANGED;
    case blink::kWebAXEventRowCollapsed:
      return ax::mojom::Event::ROW_COLLAPSED;
    case blink::kWebAXEventRowCountChanged:
      return ax::mojom::Event::ROW_COUNT_CHANGED;
    case blink::kWebAXEventRowExpanded:
      return ax::mojom::Event::ROW_EXPANDED;
    case blink::kWebAXEventScrollPositionChanged:
      return ax::mojom::Event::SCROLL_POSITION_CHANGED;
    case blink::kWebAXEventScrolledToAnchor:
      return ax::mojom::Event::SCROLLED_TO_ANCHOR;
    case blink::kWebAXEventSelectedChildrenChanged:
      return ax::mojom::Event::SELECTED_CHILDREN_CHANGED;
    case blink::kWebAXEventSelectedTextChanged:
      return ax::mojom::Event::TEXT_SELECTION_CHANGED;
    case blink::kWebAXEventTextChanged:
      return ax::mojom::Event::TEXT_CHANGED;
    case blink::kWebAXEventValueChanged:
      return ax::mojom::Event::VALUE_CHANGED;
    default:
      // We can't add an assertion here, that prevents us
      // from adding new event enums in Blink.
      return ax::mojom::Event::NONE;
  }
}

ax::mojom::DefaultActionVerb AXDefaultActionVerbFromBlink(
    blink::WebAXDefaultActionVerb action_verb) {
  switch (action_verb) {
    case blink::WebAXDefaultActionVerb::kNone:
      return ax::mojom::DefaultActionVerb::NONE;
    case blink::WebAXDefaultActionVerb::kActivate:
      return ax::mojom::DefaultActionVerb::ACTIVATE;
    case blink::WebAXDefaultActionVerb::kCheck:
      return ax::mojom::DefaultActionVerb::CHECK;
    case blink::WebAXDefaultActionVerb::kClick:
      return ax::mojom::DefaultActionVerb::CLICK;
    case blink::WebAXDefaultActionVerb::kClickAncestor:
      return ax::mojom::DefaultActionVerb::CLICK_ANCESTOR;
    case blink::WebAXDefaultActionVerb::kJump:
      return ax::mojom::DefaultActionVerb::JUMP;
    case blink::WebAXDefaultActionVerb::kOpen:
      return ax::mojom::DefaultActionVerb::OPEN;
    case blink::WebAXDefaultActionVerb::kPress:
      return ax::mojom::DefaultActionVerb::PRESS;
    case blink::WebAXDefaultActionVerb::kSelect:
      return ax::mojom::DefaultActionVerb::SELECT;
    case blink::WebAXDefaultActionVerb::kUncheck:
      return ax::mojom::DefaultActionVerb::UNCHECK;
  }
  NOTREACHED();
  return ax::mojom::DefaultActionVerb::NONE;
}

ax::mojom::MarkerType AXMarkerTypeFromBlink(blink::WebAXMarkerType marker_type) {
  switch (marker_type) {
    case blink::kWebAXMarkerTypeSpelling:
      return ax::mojom::MarkerType::SPELLING;
    case blink::kWebAXMarkerTypeGrammar:
      return ax::mojom::MarkerType::GRAMMAR;
    case blink::kWebAXMarkerTypeTextMatch:
      return ax::mojom::MarkerType::TEXT_MATCH;
    case blink::kWebAXMarkerTypeActiveSuggestion:
      return ax::mojom::MarkerType::ACTIVE_SUGGESTION;
    case blink::kWebAXMarkerTypeSuggestion:
      return ax::mojom::MarkerType::SUGGESTION;
  }
  NOTREACHED();
  return ax::mojom::MarkerType::NONE;
}

ax::mojom::TextDirection AXTextDirectionFromBlink(
    blink::WebAXTextDirection text_direction) {
  switch (text_direction) {
    case blink::kWebAXTextDirectionLR:
      return ax::mojom::TextDirection::LTR;
    case blink::kWebAXTextDirectionRL:
      return ax::mojom::TextDirection::RTL;
    case blink::kWebAXTextDirectionTB:
      return ax::mojom::TextDirection::TTB;
    case blink::kWebAXTextDirectionBT:
      return ax::mojom::TextDirection::BTT;
  }
  NOTREACHED();
  return ax::mojom::TextDirection::NONE;
}

ax::mojom::TextStyle AXTextStyleFromBlink(blink::WebAXTextStyle text_style) {
  uint32_t browser_text_style = static_cast<uint32_t>(ax::mojom::TextStyle::NONE);
  if (text_style & blink::kWebAXTextStyleBold)
    browser_text_style |= static_cast<int32_t>(ax::mojom::TextStyle::TEXT_STYLE_BOLD);
  if (text_style & blink::kWebAXTextStyleItalic)
    browser_text_style |= static_cast<int32_t>(ax::mojom::TextStyle::TEXT_STYLE_ITALIC);
  if (text_style & blink::kWebAXTextStyleUnderline)
    browser_text_style |= static_cast<int32_t>(ax::mojom::TextStyle::TEXT_STYLE_UNDERLINE);
  if (text_style & blink::kWebAXTextStyleLineThrough)
    browser_text_style |= static_cast<int32_t>(ax::mojom::TextStyle::TEXT_STYLE_LINE_THROUGH);
  return static_cast<ax::mojom::TextStyle>(browser_text_style);
}

ax::mojom::AriaCurrentState AXAriaCurrentStateFromBlink(
    blink::WebAXAriaCurrentState aria_current_state) {
  switch (aria_current_state) {
    case blink::kWebAXAriaCurrentStateUndefined:
      return ax::mojom::AriaCurrentState::NONE;
    case blink::kWebAXAriaCurrentStateFalse:
      return ax::mojom::AriaCurrentState::FALSE_VALUE;
    case blink::kWebAXAriaCurrentStateTrue:
      return ax::mojom::AriaCurrentState::TRUE_VALUE;
    case blink::kWebAXAriaCurrentStatePage:
      return ax::mojom::AriaCurrentState::PAGE;
    case blink::kWebAXAriaCurrentStateStep:
      return ax::mojom::AriaCurrentState::STEP;
    case blink::kWebAXAriaCurrentStateLocation:
      return ax::mojom::AriaCurrentState::LOCATION;
    case blink::kWebAXAriaCurrentStateDate:
      return ax::mojom::AriaCurrentState::DATE;
    case blink::kWebAXAriaCurrentStateTime:
      return ax::mojom::AriaCurrentState::TIME;
  }

  NOTREACHED();
  return ax::mojom::AriaCurrentState::NONE;
}

ax::mojom::InvalidState AXInvalidStateFromBlink(
    blink::WebAXInvalidState invalid_state) {
  switch (invalid_state) {
    case blink::kWebAXInvalidStateUndefined:
      return ax::mojom::InvalidState::NONE;
    case blink::kWebAXInvalidStateFalse:
      return ax::mojom::InvalidState::FALSE_VALUE;
    case blink::kWebAXInvalidStateTrue:
      return ax::mojom::InvalidState::TRUE_VALUE;
    case blink::kWebAXInvalidStateSpelling:
      return ax::mojom::InvalidState::SPELLING;
    case blink::kWebAXInvalidStateGrammar:
      return ax::mojom::InvalidState::GRAMMAR;
    case blink::kWebAXInvalidStateOther:
      return ax::mojom::InvalidState::OTHER;
  }
  NOTREACHED();
  return ax::mojom::InvalidState::NONE;
}

ax::mojom::CheckedState AXCheckedStateFromBlink(
    blink::WebAXCheckedState checked_state) {
  switch (checked_state) {
    case blink::kWebAXCheckedUndefined:
      return ax::mojom::CheckedState::NONE;
    case blink::kWebAXCheckedTrue:
      return ax::mojom::CheckedState::TRUE_VALUE;
    case blink::kWebAXCheckedMixed:
      return ax::mojom::CheckedState::MIXED;
    case blink::kWebAXCheckedFalse:
      return ax::mojom::CheckedState::FALSE_VALUE;
  }
  NOTREACHED();
  return ax::mojom::CheckedState::NONE;
}

ax::mojom::SortDirection AXSortDirectionFromBlink(
    blink::WebAXSortDirection sort_direction) {
  switch (sort_direction) {
    case blink::kWebAXSortDirectionUndefined:
      return ax::mojom::SortDirection::NONE;
    case blink::kWebAXSortDirectionNone:
      return ax::mojom::SortDirection::UNSORTED;
    case blink::kWebAXSortDirectionAscending:
      return ax::mojom::SortDirection::ASCENDING;
    case blink::kWebAXSortDirectionDescending:
      return ax::mojom::SortDirection::DESCENDING;
    case blink::kWebAXSortDirectionOther:
      return ax::mojom::SortDirection::OTHER;
  }
  NOTREACHED();
  return ax::mojom::SortDirection::NONE;
}

ax::mojom::NameFrom AXNameFromFromBlink(blink::WebAXNameFrom name_from) {
  switch (name_from) {
    case blink::kWebAXNameFromUninitialized:
      return ax::mojom::NameFrom::UNINITIALIZED;
    case blink::kWebAXNameFromAttribute:
      return ax::mojom::NameFrom::ATTRIBUTE;
    case blink::kWebAXNameFromAttributeExplicitlyEmpty:
      return ax::mojom::NameFrom::ATTRIBUTE_EXPLICITLY_EMPTY;
    case blink::kWebAXNameFromCaption:
      return ax::mojom::NameFrom::RELATED_ELEMENT;
    case blink::kWebAXNameFromContents:
      return ax::mojom::NameFrom::CONTENTS;
    case blink::kWebAXNameFromPlaceholder:
      return ax::mojom::NameFrom::PLACEHOLDER;
    case blink::kWebAXNameFromRelatedElement:
      return ax::mojom::NameFrom::RELATED_ELEMENT;
    case blink::kWebAXNameFromValue:
      return ax::mojom::NameFrom::VALUE;
    case blink::kWebAXNameFromTitle:
      return ax::mojom::NameFrom::ATTRIBUTE;
  }
  NOTREACHED();
  return ax::mojom::NameFrom::NONE;
}

ax::mojom::DescriptionFrom AXDescriptionFromFromBlink(
    blink::WebAXDescriptionFrom description_from) {
  switch (description_from) {
    case blink::kWebAXDescriptionFromUninitialized:
      return ax::mojom::DescriptionFrom::UNINITIALIZED;
    case blink::kWebAXDescriptionFromAttribute:
      return ax::mojom::DescriptionFrom::ATTRIBUTE;
    case blink::kWebAXDescriptionFromContents:
      return ax::mojom::DescriptionFrom::CONTENTS;
    case blink::kWebAXDescriptionFromRelatedElement:
      return ax::mojom::DescriptionFrom::RELATED_ELEMENT;
  }
  NOTREACHED();
  return ax::mojom::DescriptionFrom::NONE;
}

ax::mojom::TextAffinity AXTextAffinityFromBlink(blink::WebAXTextAffinity affinity) {
  switch (affinity) {
    case blink::kWebAXTextAffinityUpstream:
      return ax::mojom::TextAffinity::UPSTREAM;
    case blink::kWebAXTextAffinityDownstream:
      return ax::mojom::TextAffinity::DOWNSTREAM;
  }
  NOTREACHED();
  return ax::mojom::TextAffinity::DOWNSTREAM;
}

}  // namespace content.
