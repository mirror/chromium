// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/accessibility/ax_enum_util.h"

namespace ui {

const char* ToString(ax::mojom::Event event) {
  switch (event) {
    case ax::mojom::Event::NONE:
      return "none";
    case ax::mojom::Event::ACTIVEDESCENDANTCHANGED:
      return "activedescendantchanged";
    case ax::mojom::Event::ALERT:
      return "alert";
    case ax::mojom::Event::ARIA_ATTRIBUTE_CHANGED:
      return "ariaAttributeChanged";
    case ax::mojom::Event::AUTOCORRECTION_OCCURED:
      return "autocorrectionOccured";
    case ax::mojom::Event::BLUR:
      return "blur";
    case ax::mojom::Event::CHECKED_STATE_CHANGED:
      return "checkedStateChanged";
    case ax::mojom::Event::CHILDREN_CHANGED:
      return "childrenChanged";
    case ax::mojom::Event::CLICKED:
      return "clicked";
    case ax::mojom::Event::DOCUMENT_SELECTION_CHANGED:
      return "documentSelectionChanged";
    case ax::mojom::Event::EXPANDED_CHANGED:
      return "expandedChanged";
    case ax::mojom::Event::FOCUS:
      return "focus";
    case ax::mojom::Event::HIDE:
      return "hide";
    case ax::mojom::Event::HIT_TEST_RESULT:
      return "hitTestResult";
    case ax::mojom::Event::HOVER:
      return "hover";
    case ax::mojom::Event::IMAGE_FRAME_UPDATED:
      return "imageFrameUpdated";
    case ax::mojom::Event::INVALID_STATUS_CHANGED:
      return "invalidStatusChanged";
    case ax::mojom::Event::LAYOUT_COMPLETE:
      return "layoutComplete";
    case ax::mojom::Event::LIVE_REGION_CREATED:
      return "liveRegionCreated";
    case ax::mojom::Event::LIVE_REGION_CHANGED:
      return "liveRegionChanged";
    case ax::mojom::Event::LOAD_COMPLETE:
      return "loadComplete";
    case ax::mojom::Event::LOCATION_CHANGED:
      return "locationChanged";
    case ax::mojom::Event::MEDIA_STARTED_PLAYING:
      return "mediaStartedPlaying";
    case ax::mojom::Event::MEDIA_STOPPED_PLAYING:
      return "mediaStoppedPlaying";
    case ax::mojom::Event::MENU_END:
      return "menuEnd";
    case ax::mojom::Event::MENU_LIST_ITEM_SELECTED:
      return "menuListItemSelected";
    case ax::mojom::Event::MENU_LIST_VALUE_CHANGED:
      return "menuListValueChanged";
    case ax::mojom::Event::MENU_POPUP_END:
      return "menuPopupEnd";
    case ax::mojom::Event::MENU_POPUP_START:
      return "menuPopupStart";
    case ax::mojom::Event::MENU_START:
      return "menuStart";
    case ax::mojom::Event::MOUSE_CANCELED:
      return "mouseCanceled";
    case ax::mojom::Event::MOUSE_DRAGGED:
      return "mouseDragged";
    case ax::mojom::Event::MOUSE_MOVED_VALUE:
      return "mouseMoved";
    case ax::mojom::Event::MOUSE_PRESSED:
      return "mousePressed";
    case ax::mojom::Event::MOUSE_RELEASED:
      return "mouseReleased";
    case ax::mojom::Event::ROW_COLLAPSED:
      return "rowCollapsed";
    case ax::mojom::Event::ROW_COUNT_CHANGED:
      return "rowCountChanged";
    case ax::mojom::Event::ROW_EXPANDED:
      return "rowExpanded";
    case ax::mojom::Event::SCROLL_POSITION_CHANGED:
      return "scrollPositionChanged";
    case ax::mojom::Event::SCROLLED_TO_ANCHOR:
      return "scrolledToAnchor";
    case ax::mojom::Event::SELECTED_CHILDREN_CHANGED:
      return "selectedChildrenChanged";
    case ax::mojom::Event::SELECTION:
      return "selection";
    case ax::mojom::Event::SELECTION_ADD:
      return "selectionAdd";
    case ax::mojom::Event::SELECTION_REMOVE:
      return "selectionRemove";
    case ax::mojom::Event::SHOW:
      return "show";
    case ax::mojom::Event::TEXT_CHANGED:
      return "textChanged";
    case ax::mojom::Event::TEXT_SELECTION_CHANGED:
      return "textSelectionChanged";
    case ax::mojom::Event::TREE_CHANGED:
      return "treeChanged";
    case ax::mojom::Event::VALUE_CHANGED:
      return "valueChanged";
  }
}

ax::mojom::Event ParseEvent(const char *event) {
  if (0 == strcmp(event, "none"))
    return ax::mojom::Event::NONE;
  if (0 == strcmp(event, "activedescendantchanged"))
    return ax::mojom::Event::ACTIVEDESCENDANTCHANGED;
  if (0 == strcmp(event, "alert"))
    return ax::mojom::Event::ALERT;
  if (0 == strcmp(event, "ariaAttributeChanged"))
    return ax::mojom::Event::ARIA_ATTRIBUTE_CHANGED;
  if (0 == strcmp(event, "autocorrectionOccured"))
    return ax::mojom::Event::AUTOCORRECTION_OCCURED;
  if (0 == strcmp(event, "blur"))
    return ax::mojom::Event::BLUR;
  if (0 == strcmp(event, "checkedStateChanged"))
    return ax::mojom::Event::CHECKED_STATE_CHANGED;
  if (0 == strcmp(event, "childrenChanged"))
    return ax::mojom::Event::CHILDREN_CHANGED;
  if (0 == strcmp(event, "clicked"))
    return ax::mojom::Event::CLICKED;
  if (0 == strcmp(event, "documentSelectionChanged"))
    return ax::mojom::Event::DOCUMENT_SELECTION_CHANGED;
  if (0 == strcmp(event, "expandedChanged"))
    return ax::mojom::Event::EXPANDED_CHANGED;
  if (0 == strcmp(event, "focus"))
    return ax::mojom::Event::FOCUS;
  if (0 == strcmp(event, "hide"))
    return ax::mojom::Event::HIDE;
  if (0 == strcmp(event, "hitTestResult"))
    return ax::mojom::Event::HIT_TEST_RESULT;
  if (0 == strcmp(event, "hover"))
    return ax::mojom::Event::HOVER;
  if (0 == strcmp(event, "imageFrameUpdated"))
    return ax::mojom::Event::IMAGE_FRAME_UPDATED;
  if (0 == strcmp(event, "invalidStatusChanged"))
    return ax::mojom::Event::INVALID_STATUS_CHANGED;
  if (0 == strcmp(event, "layoutComplete"))
    return ax::mojom::Event::LAYOUT_COMPLETE;
  if (0 == strcmp(event, "liveRegionCreated"))
    return ax::mojom::Event::LIVE_REGION_CREATED;
  if (0 == strcmp(event, "liveRegionChanged"))
    return ax::mojom::Event::LIVE_REGION_CHANGED;
  if (0 == strcmp(event, "loadComplete"))
    return ax::mojom::Event::LOAD_COMPLETE;
  if (0 == strcmp(event, "locationChanged"))
    return ax::mojom::Event::LOCATION_CHANGED;
  if (0 == strcmp(event, "mediaStartedPlaying"))
    return ax::mojom::Event::MEDIA_STARTED_PLAYING;
  if (0 == strcmp(event, "mediaStoppedPlaying"))
    return ax::mojom::Event::MEDIA_STOPPED_PLAYING;
  if (0 == strcmp(event, "menuEnd"))
    return ax::mojom::Event::MENU_END;
  if (0 == strcmp(event, "menuListItemSelected"))
    return ax::mojom::Event::MENU_LIST_ITEM_SELECTED;
  if (0 == strcmp(event, "menuListValueChanged"))
    return ax::mojom::Event::MENU_LIST_VALUE_CHANGED;
  if (0 == strcmp(event, "menuPopupEnd"))
    return ax::mojom::Event::MENU_POPUP_END;
  if (0 == strcmp(event, "menuPopupStart"))
    return ax::mojom::Event::MENU_POPUP_START;
  if (0 == strcmp(event, "menuStart"))
    return ax::mojom::Event::MENU_START;
  if (0 == strcmp(event, "mouseCanceled"))
    return ax::mojom::Event::MOUSE_CANCELED;
  if (0 == strcmp(event, "mouseDragged"))
    return ax::mojom::Event::MOUSE_DRAGGED;
  if (0 == strcmp(event, "mouseMoved"))
    return ax::mojom::Event::MOUSE_MOVED_VALUE;
  if (0 == strcmp(event, "mousePressed"))
    return ax::mojom::Event::MOUSE_PRESSED;
  if (0 == strcmp(event, "mouseReleased"))
    return ax::mojom::Event::MOUSE_RELEASED;
  if (0 == strcmp(event, "rowCollapsed"))
    return ax::mojom::Event::ROW_COLLAPSED;
  if (0 == strcmp(event, "rowCountChanged"))
    return ax::mojom::Event::ROW_COUNT_CHANGED;
  if (0 == strcmp(event, "rowExpanded"))
    return ax::mojom::Event::ROW_EXPANDED;
  if (0 == strcmp(event, "scrollPositionChanged"))
    return ax::mojom::Event::SCROLL_POSITION_CHANGED;
  if (0 == strcmp(event, "scrolledToAnchor"))
    return ax::mojom::Event::SCROLLED_TO_ANCHOR;
  if (0 == strcmp(event, "selectedChildrenChanged"))
    return ax::mojom::Event::SELECTED_CHILDREN_CHANGED;
  if (0 == strcmp(event, "selection"))
    return ax::mojom::Event::SELECTION;
  if (0 == strcmp(event, "selectionAdd"))
    return ax::mojom::Event::SELECTION_ADD;
  if (0 == strcmp(event, "selectionRemove"))
    return ax::mojom::Event::SELECTION_REMOVE;
  if (0 == strcmp(event, "show"))
    return ax::mojom::Event::SHOW;
  if (0 == strcmp(event, "textChanged"))
    return ax::mojom::Event::TEXT_CHANGED;
  if (0 == strcmp(event, "textSelectionChanged"))
    return ax::mojom::Event::TEXT_SELECTION_CHANGED;
  if (0 == strcmp(event, "treeChanged"))
    return ax::mojom::Event::TREE_CHANGED;
  if (0 == strcmp(event, "valueChanged"))
    return ax::mojom::Event::VALUE_CHANGED;
  return ax::mojom::Event::NONE;
}

const char* ToString(ax::mojom::Role role) {
  switch (role) {
    case ax::mojom::Role::NONE:
      return "none";
    case ax::mojom::Role::ABBR:
      return "abbr";
    case ax::mojom::Role::ALERT_DIALOG:
      return "alertDialog";
    case ax::mojom::Role::ALERT:
      return "alert";
    case ax::mojom::Role::ANCHOR:
      return "anchor";
    case ax::mojom::Role::ANNOTATION:
      return "annotation";
    case ax::mojom::Role::APPLICATION:
      return "application";
    case ax::mojom::Role::ARTICLE:
      return "article";
    case ax::mojom::Role::AUDIO:
      return "audio";
    case ax::mojom::Role::BANNER:
      return "banner";
    case ax::mojom::Role::BLOCKQUOTE:
      return "blockquote";
    case ax::mojom::Role::BUTTON:
      return "button";
    case ax::mojom::Role::BUTTON_DROP_DOWN:
      return "buttonDropDown";
    case ax::mojom::Role::CANVAS:
      return "canvas";
    case ax::mojom::Role::CAPTION:
      return "caption";
    case ax::mojom::Role::CARET:
      return "caret";
    case ax::mojom::Role::CELL:
      return "cell";
    case ax::mojom::Role::CHECK_BOX:
      return "checkBox";
    case ax::mojom::Role::CLIENT:
      return "client";
    case ax::mojom::Role::COLOR_WELL:
      return "colorWell";
    case ax::mojom::Role::COLUMN_HEADER:
      return "columnHeader";
    case ax::mojom::Role::COLUMN:
      return "column";
    case ax::mojom::Role::COMBO_BOX_GROUPING:
      return "comboBoxGrouping";
    case ax::mojom::Role::COMBO_BOX_MENU_BUTTON:
      return "comboBoxMenuButton";
    case ax::mojom::Role::COMPLEMENTARY:
      return "complementary";
    case ax::mojom::Role::CONTENT_INFO:
      return "contentInfo";
    case ax::mojom::Role::DATE:
      return "date";
    case ax::mojom::Role::DATE_TIME:
      return "dateTime";
    case ax::mojom::Role::DEFINITION:
      return "definition";
    case ax::mojom::Role::DESCRIPTION_LIST_DETAIL:
      return "descriptionListDetail";
    case ax::mojom::Role::DESCRIPTION_LIST:
      return "descriptionList";
    case ax::mojom::Role::DESCRIPTION_LIST_TERM:
      return "descriptionListTerm";
    case ax::mojom::Role::DESKTOP:
      return "desktop";
    case ax::mojom::Role::DETAILS:
      return "details";
    case ax::mojom::Role::DIALOG:
      return "dialog";
    case ax::mojom::Role::DIRECTORY:
      return "directory";
    case ax::mojom::Role::DISCLOSURE_TRIANGLE:
      return "disclosureTriangle";
    case ax::mojom::Role::DOCUMENT:
      return "document";
    case ax::mojom::Role::EMBEDDED_OBJECT:
      return "embeddedObject";
    case ax::mojom::Role::FEED:
      return "feed";
    case ax::mojom::Role::FIGCAPTION:
      return "figcaption";
    case ax::mojom::Role::FIGURE:
      return "figure";
    case ax::mojom::Role::FOOTER:
      return "footer";
    case ax::mojom::Role::FORM:
      return "form";
    case ax::mojom::Role::GENERIC_CONTAINER:
      return "genericContainer";
    case ax::mojom::Role::GRID:
      return "grid";
    case ax::mojom::Role::GROUP:
      return "group";
    case ax::mojom::Role::HEADING:
      return "heading";
    case ax::mojom::Role::IFRAME:
      return "iframe";
    case ax::mojom::Role::IFRAME_PRESENTATIONAL:
      return "iframePresentational";
    case ax::mojom::Role::IGNORED:
      return "ignored";
    case ax::mojom::Role::IMAGE_MAP:
      return "imageMap";
    case ax::mojom::Role::IMAGE:
      return "image";
    case ax::mojom::Role::INLINE_TEXT_BOX:
      return "inlineTextBox";
    case ax::mojom::Role::INPUT_TIME:
      return "inputTime";
    case ax::mojom::Role::LABEL_TEXT:
      return "labelText";
    case ax::mojom::Role::LEGEND:
      return "legend";
    case ax::mojom::Role::LINE_BREAK:
      return "lineBreak";
    case ax::mojom::Role::LINK:
      return "link";
    case ax::mojom::Role::LIST_BOX_OPTION:
      return "listBoxOption";
    case ax::mojom::Role::LIST_BOX:
      return "listBox";
    case ax::mojom::Role::LIST_ITEM:
      return "listItem";
    case ax::mojom::Role::LIST_MARKER:
      return "listMarker";
    case ax::mojom::Role::LIST:
      return "list";
    case ax::mojom::Role::LOCATION_BAR:
      return "locationBar";
    case ax::mojom::Role::LOG:
      return "log";
    case ax::mojom::Role::MAIN:
      return "main";
    case ax::mojom::Role::MARK:
      return "mark";
    case ax::mojom::Role::MARQUEE:
      return "marquee";
    case ax::mojom::Role::MATH:
      return "math";
    case ax::mojom::Role::MENU:
      return "menu";
    case ax::mojom::Role::MENU_BAR:
      return "menuBar";
    case ax::mojom::Role::MENU_BUTTON:
      return "menuButton";
    case ax::mojom::Role::MENU_ITEM:
      return "menuItem";
    case ax::mojom::Role::MENU_ITEM_CHECK_BOX:
      return "menuItemCheckBox";
    case ax::mojom::Role::MENU_ITEM_RADIO:
      return "menuItemRadio";
    case ax::mojom::Role::MENU_LIST_OPTION:
      return "menuListOption";
    case ax::mojom::Role::MENU_LIST_POPUP:
      return "menuListPopup";
    case ax::mojom::Role::METER:
      return "meter";
    case ax::mojom::Role::NAVIGATION:
      return "navigation";
    case ax::mojom::Role::NOTE:
      return "note";
    case ax::mojom::Role::PANE:
      return "pane";
    case ax::mojom::Role::PARAGRAPH:
      return "paragraph";
    case ax::mojom::Role::POP_UP_BUTTON:
      return "popUpButton";
    case ax::mojom::Role::PRE:
      return "pre";
    case ax::mojom::Role::PRESENTATIONAL:
      return "presentational";
    case ax::mojom::Role::PROGRESS_INDICATOR:
      return "progressIndicator";
    case ax::mojom::Role::RADIO_BUTTON:
      return "radioButton";
    case ax::mojom::Role::RADIO_GROUP:
      return "radioGroup";
    case ax::mojom::Role::REGION:
      return "region";
    case ax::mojom::Role::ROOT_WEB_AREA:
      return "rootWebArea";
    case ax::mojom::Role::ROW_HEADER:
      return "rowHeader";
    case ax::mojom::Role::ROW:
      return "row";
    case ax::mojom::Role::RUBY:
      return "ruby";
    case ax::mojom::Role::SVG_ROOT:
      return "svgRoot";
    case ax::mojom::Role::SCROLL_BAR:
      return "scrollBar";
    case ax::mojom::Role::SEARCH:
      return "search";
    case ax::mojom::Role::SEARCH_BOX:
      return "searchBox";
    case ax::mojom::Role::SLIDER:
      return "slider";
    case ax::mojom::Role::SLIDER_THUMB:
      return "sliderThumb";
    case ax::mojom::Role::SPIN_BUTTON_PART:
      return "spinButtonPart";
    case ax::mojom::Role::SPIN_BUTTON:
      return "spinButton";
    case ax::mojom::Role::SPLITTER:
      return "splitter";
    case ax::mojom::Role::STATIC_TEXT:
      return "staticText";
    case ax::mojom::Role::STATUS:
      return "status";
    case ax::mojom::Role::SWITCH:
      return "switch";
    case ax::mojom::Role::TAB_LIST:
      return "tabList";
    case ax::mojom::Role::TAB_PANEL:
      return "tabPanel";
    case ax::mojom::Role::TAB:
      return "tab";
    case ax::mojom::Role::TABLE_HEADER_CONTAINER:
      return "tableHeaderContainer";
    case ax::mojom::Role::TABLE:
      return "table";
    case ax::mojom::Role::TERM:
      return "term";
    case ax::mojom::Role::TEXT_FIELD:
      return "textField";
    case ax::mojom::Role::TEXT_FIELD_WITH_COMBO_BOX:
      return "textFieldWithComboBox";
    case ax::mojom::Role::TIME:
      return "time";
    case ax::mojom::Role::TIMER:
      return "timer";
    case ax::mojom::Role::TITLE_BAR:
      return "titleBar";
    case ax::mojom::Role::TOGGLE_BUTTON:
      return "toggleButton";
    case ax::mojom::Role::TOOLBAR:
      return "toolbar";
    case ax::mojom::Role::TREE_GRID:
      return "treeGrid";
    case ax::mojom::Role::TREE_ITEM:
      return "treeItem";
    case ax::mojom::Role::TREE:
      return "tree";
    case ax::mojom::Role::UNKNOWN:
      return "unknown";
    case ax::mojom::Role::TOOLTIP:
      return "tooltip";
    case ax::mojom::Role::VIDEO:
      return "video";
    case ax::mojom::Role::WEB_AREA:
      return "webArea";
    case ax::mojom::Role::WEB_VIEW:
      return "webView";
    case ax::mojom::Role::WINDOW:
      return "window";
  }
}

ax::mojom::Role ParseRole(const char *role) {
  if (0 == strcmp(role, "none"))
    return ax::mojom::Role::NONE;
  if (0 == strcmp(role, "abbr"))
    return ax::mojom::Role::ABBR;
  if (0 == strcmp(role, "alertDialog"))
    return ax::mojom::Role::ALERT_DIALOG;
  if (0 == strcmp(role, "alert"))
    return ax::mojom::Role::ALERT;
  if (0 == strcmp(role, "anchor"))
    return ax::mojom::Role::ANCHOR;
  if (0 == strcmp(role, "annotation"))
    return ax::mojom::Role::ANNOTATION;
  if (0 == strcmp(role, "application"))
    return ax::mojom::Role::APPLICATION;
  if (0 == strcmp(role, "article"))
    return ax::mojom::Role::ARTICLE;
  if (0 == strcmp(role, "audio"))
    return ax::mojom::Role::AUDIO;
  if (0 == strcmp(role, "banner"))
    return ax::mojom::Role::BANNER;
  if (0 == strcmp(role, "blockquote"))
    return ax::mojom::Role::BLOCKQUOTE;
  if (0 == strcmp(role, "button"))
    return ax::mojom::Role::BUTTON;
  if (0 == strcmp(role, "buttonDropDown"))
    return ax::mojom::Role::BUTTON_DROP_DOWN;
  if (0 == strcmp(role, "canvas"))
    return ax::mojom::Role::CANVAS;
  if (0 == strcmp(role, "caption"))
    return ax::mojom::Role::CAPTION;
  if (0 == strcmp(role, "caret"))
    return ax::mojom::Role::CARET;
  if (0 == strcmp(role, "cell"))
    return ax::mojom::Role::CELL;
  if (0 == strcmp(role, "checkBox"))
    return ax::mojom::Role::CHECK_BOX;
  if (0 == strcmp(role, "client"))
    return ax::mojom::Role::CLIENT;
  if (0 == strcmp(role, "colorWell"))
    return ax::mojom::Role::COLOR_WELL;
  if (0 == strcmp(role, "columnHeader"))
    return ax::mojom::Role::COLUMN_HEADER;
  if (0 == strcmp(role, "column"))
    return ax::mojom::Role::COLUMN;
  if (0 == strcmp(role, "comboBoxGrouping"))
    return ax::mojom::Role::COMBO_BOX_GROUPING;
  if (0 == strcmp(role, "comboBoxMenuButton"))
    return ax::mojom::Role::COMBO_BOX_MENU_BUTTON;
  if (0 == strcmp(role, "complementary"))
    return ax::mojom::Role::COMPLEMENTARY;
  if (0 == strcmp(role, "contentInfo"))
    return ax::mojom::Role::CONTENT_INFO;
  if (0 == strcmp(role, "date"))
    return ax::mojom::Role::DATE;
  if (0 == strcmp(role, "dateTime"))
    return ax::mojom::Role::DATE_TIME;
  if (0 == strcmp(role, "definition"))
    return ax::mojom::Role::DEFINITION;
  if (0 == strcmp(role, "descriptionListDetail"))
    return ax::mojom::Role::DESCRIPTION_LIST_DETAIL;
  if (0 == strcmp(role, "descriptionList"))
    return ax::mojom::Role::DESCRIPTION_LIST;
  if (0 == strcmp(role, "descriptionListTerm"))
    return ax::mojom::Role::DESCRIPTION_LIST_TERM;
  if (0 == strcmp(role, "desktop"))
    return ax::mojom::Role::DESKTOP;
  if (0 == strcmp(role, "details"))
    return ax::mojom::Role::DETAILS;
  if (0 == strcmp(role, "dialog"))
    return ax::mojom::Role::DIALOG;
  if (0 == strcmp(role, "directory"))
    return ax::mojom::Role::DIRECTORY;
  if (0 == strcmp(role, "disclosureTriangle"))
    return ax::mojom::Role::DISCLOSURE_TRIANGLE;
  if (0 == strcmp(role, "document"))
    return ax::mojom::Role::DOCUMENT;
  if (0 == strcmp(role, "embeddedObject"))
    return ax::mojom::Role::EMBEDDED_OBJECT;
  if (0 == strcmp(role, "feed"))
    return ax::mojom::Role::FEED;
  if (0 == strcmp(role, "figcaption"))
    return ax::mojom::Role::FIGCAPTION;
  if (0 == strcmp(role, "figure"))
    return ax::mojom::Role::FIGURE;
  if (0 == strcmp(role, "footer"))
    return ax::mojom::Role::FOOTER;
  if (0 == strcmp(role, "form"))
    return ax::mojom::Role::FORM;
  if (0 == strcmp(role, "genericContainer"))
    return ax::mojom::Role::GENERIC_CONTAINER;
  if (0 == strcmp(role, "grid"))
    return ax::mojom::Role::GRID;
  if (0 == strcmp(role, "group"))
    return ax::mojom::Role::GROUP;
  if (0 == strcmp(role, "heading"))
    return ax::mojom::Role::HEADING;
  if (0 == strcmp(role, "iframe"))
    return ax::mojom::Role::IFRAME;
  if (0 == strcmp(role, "iframePresentational"))
    return ax::mojom::Role::IFRAME_PRESENTATIONAL;
  if (0 == strcmp(role, "ignored"))
    return ax::mojom::Role::IGNORED;
  if (0 == strcmp(role, "imageMap"))
    return ax::mojom::Role::IMAGE_MAP;
  if (0 == strcmp(role, "image"))
    return ax::mojom::Role::IMAGE;
  if (0 == strcmp(role, "inlineTextBox"))
    return ax::mojom::Role::INLINE_TEXT_BOX;
  if (0 == strcmp(role, "inputTime"))
    return ax::mojom::Role::INPUT_TIME;
  if (0 == strcmp(role, "labelText"))
    return ax::mojom::Role::LABEL_TEXT;
  if (0 == strcmp(role, "legend"))
    return ax::mojom::Role::LEGEND;
  if (0 == strcmp(role, "lineBreak"))
    return ax::mojom::Role::LINE_BREAK;
  if (0 == strcmp(role, "link"))
    return ax::mojom::Role::LINK;
  if (0 == strcmp(role, "listBoxOption"))
    return ax::mojom::Role::LIST_BOX_OPTION;
  if (0 == strcmp(role, "listBox"))
    return ax::mojom::Role::LIST_BOX;
  if (0 == strcmp(role, "listItem"))
    return ax::mojom::Role::LIST_ITEM;
  if (0 == strcmp(role, "listMarker"))
    return ax::mojom::Role::LIST_MARKER;
  if (0 == strcmp(role, "list"))
    return ax::mojom::Role::LIST;
  if (0 == strcmp(role, "locationBar"))
    return ax::mojom::Role::LOCATION_BAR;
  if (0 == strcmp(role, "log"))
    return ax::mojom::Role::LOG;
  if (0 == strcmp(role, "main"))
    return ax::mojom::Role::MAIN;
  if (0 == strcmp(role, "mark"))
    return ax::mojom::Role::MARK;
  if (0 == strcmp(role, "marquee"))
    return ax::mojom::Role::MARQUEE;
  if (0 == strcmp(role, "math"))
    return ax::mojom::Role::MATH;
  if (0 == strcmp(role, "menu"))
    return ax::mojom::Role::MENU;
  if (0 == strcmp(role, "menuBar"))
    return ax::mojom::Role::MENU_BAR;
  if (0 == strcmp(role, "menuButton"))
    return ax::mojom::Role::MENU_BUTTON;
  if (0 == strcmp(role, "menuItem"))
    return ax::mojom::Role::MENU_ITEM;
  if (0 == strcmp(role, "menuItemCheckBox"))
    return ax::mojom::Role::MENU_ITEM_CHECK_BOX;
  if (0 == strcmp(role, "menuItemRadio"))
    return ax::mojom::Role::MENU_ITEM_RADIO;
  if (0 == strcmp(role, "menuListOption"))
    return ax::mojom::Role::MENU_LIST_OPTION;
  if (0 == strcmp(role, "menuListPopup"))
    return ax::mojom::Role::MENU_LIST_POPUP;
  if (0 == strcmp(role, "meter"))
    return ax::mojom::Role::METER;
  if (0 == strcmp(role, "navigation"))
    return ax::mojom::Role::NAVIGATION;
  if (0 == strcmp(role, "note"))
    return ax::mojom::Role::NOTE;
  if (0 == strcmp(role, "pane"))
    return ax::mojom::Role::PANE;
  if (0 == strcmp(role, "paragraph"))
    return ax::mojom::Role::PARAGRAPH;
  if (0 == strcmp(role, "popUpButton"))
    return ax::mojom::Role::POP_UP_BUTTON;
  if (0 == strcmp(role, "pre"))
    return ax::mojom::Role::PRE;
  if (0 == strcmp(role, "presentational"))
    return ax::mojom::Role::PRESENTATIONAL;
  if (0 == strcmp(role, "progressIndicator"))
    return ax::mojom::Role::PROGRESS_INDICATOR;
  if (0 == strcmp(role, "radioButton"))
    return ax::mojom::Role::RADIO_BUTTON;
  if (0 == strcmp(role, "radioGroup"))
    return ax::mojom::Role::RADIO_GROUP;
  if (0 == strcmp(role, "region"))
    return ax::mojom::Role::REGION;
  if (0 == strcmp(role, "rootWebArea"))
    return ax::mojom::Role::ROOT_WEB_AREA;
  if (0 == strcmp(role, "rowHeader"))
    return ax::mojom::Role::ROW_HEADER;
  if (0 == strcmp(role, "row"))
    return ax::mojom::Role::ROW;
  if (0 == strcmp(role, "ruby"))
    return ax::mojom::Role::RUBY;
  if (0 == strcmp(role, "svgRoot"))
    return ax::mojom::Role::SVG_ROOT;
  if (0 == strcmp(role, "scrollBar"))
    return ax::mojom::Role::SCROLL_BAR;
  if (0 == strcmp(role, "search"))
    return ax::mojom::Role::SEARCH;
  if (0 == strcmp(role, "searchBox"))
    return ax::mojom::Role::SEARCH_BOX;
  if (0 == strcmp(role, "slider"))
    return ax::mojom::Role::SLIDER;
  if (0 == strcmp(role, "sliderThumb"))
    return ax::mojom::Role::SLIDER_THUMB;
  if (0 == strcmp(role, "spinButtonPart"))
    return ax::mojom::Role::SPIN_BUTTON_PART;
  if (0 == strcmp(role, "spinButton"))
    return ax::mojom::Role::SPIN_BUTTON;
  if (0 == strcmp(role, "splitter"))
    return ax::mojom::Role::SPLITTER;
  if (0 == strcmp(role, "staticText"))
    return ax::mojom::Role::STATIC_TEXT;
  if (0 == strcmp(role, "status"))
    return ax::mojom::Role::STATUS;
  if (0 == strcmp(role, "switch"))
    return ax::mojom::Role::SWITCH;
  if (0 == strcmp(role, "tabList"))
    return ax::mojom::Role::TAB_LIST;
  if (0 == strcmp(role, "tabPanel"))
    return ax::mojom::Role::TAB_PANEL;
  if (0 == strcmp(role, "tab"))
    return ax::mojom::Role::TAB;
  if (0 == strcmp(role, "tableHeaderContainer"))
    return ax::mojom::Role::TABLE_HEADER_CONTAINER;
  if (0 == strcmp(role, "table"))
    return ax::mojom::Role::TABLE;
  if (0 == strcmp(role, "term"))
    return ax::mojom::Role::TERM;
  if (0 == strcmp(role, "textField"))
    return ax::mojom::Role::TEXT_FIELD;
  if (0 == strcmp(role, "textFieldWithComboBox"))
    return ax::mojom::Role::TEXT_FIELD_WITH_COMBO_BOX;
  if (0 == strcmp(role, "time"))
    return ax::mojom::Role::TIME;
  if (0 == strcmp(role, "timer"))
    return ax::mojom::Role::TIMER;
  if (0 == strcmp(role, "titleBar"))
    return ax::mojom::Role::TITLE_BAR;
  if (0 == strcmp(role, "toggleButton"))
    return ax::mojom::Role::TOGGLE_BUTTON;
  if (0 == strcmp(role, "toolbar"))
    return ax::mojom::Role::TOOLBAR;
  if (0 == strcmp(role, "treeGrid"))
    return ax::mojom::Role::TREE_GRID;
  if (0 == strcmp(role, "treeItem"))
    return ax::mojom::Role::TREE_ITEM;
  if (0 == strcmp(role, "tree"))
    return ax::mojom::Role::TREE;
  if (0 == strcmp(role, "unknown"))
    return ax::mojom::Role::UNKNOWN;
  if (0 == strcmp(role, "tooltip"))
    return ax::mojom::Role::TOOLTIP;
  if (0 == strcmp(role, "video"))
    return ax::mojom::Role::VIDEO;
  if (0 == strcmp(role, "webArea"))
    return ax::mojom::Role::WEB_AREA;
  if (0 == strcmp(role, "webView"))
    return ax::mojom::Role::WEB_VIEW;
  if (0 == strcmp(role, "window"))
    return ax::mojom::Role::WINDOW;
  return ax::mojom::Role::NONE;
}

const char* ToString(ax::mojom::State state) {
  switch (state) {
    case ax::mojom::State::NONE:
      return "none";
    case ax::mojom::State::COLLAPSED:
      return "collapsed";
    case ax::mojom::State::DEFAULT:
      return "default";
    case ax::mojom::State::EDITABLE:
      return "editable";
    case ax::mojom::State::EXPANDED:
      return "expanded";
    case ax::mojom::State::FOCUSABLE:
      return "focusable";
    case ax::mojom::State::HASPOPUP:
      return "haspopup";
    case ax::mojom::State::HORIZONTAL:
      return "horizontal";
    case ax::mojom::State::HOVERED:
      return "hovered";
    case ax::mojom::State::INVISIBLE:
      return "invisible";
    case ax::mojom::State::LINKED:
      return "linked";
    case ax::mojom::State::MULTILINE:
      return "multiline";
    case ax::mojom::State::MULTISELECTABLE:
      return "multiselectable";
    case ax::mojom::State::PROTECTED:
      return "protected";
    case ax::mojom::State::REQUIRED:
      return "required";
    case ax::mojom::State::RICHLY_EDITABLE:
      return "richlyEditable";
    case ax::mojom::State::SELECTABLE:
      return "selectable";
    case ax::mojom::State::SELECTED:
      return "selected";
    case ax::mojom::State::VERTICAL:
      return "vertical";
    case ax::mojom::State::VISITED:
      return "visited";
  }
}

ax::mojom::State ParseState(const char *state) {
  if (0 == strcmp(state, "none"))
    return ax::mojom::State::NONE;
  if (0 == strcmp(state, "collapsed"))
    return ax::mojom::State::COLLAPSED;
  if (0 == strcmp(state, "default"))
    return ax::mojom::State::DEFAULT;
  if (0 == strcmp(state, "editable"))
    return ax::mojom::State::EDITABLE;
  if (0 == strcmp(state, "expanded"))
    return ax::mojom::State::EXPANDED;
  if (0 == strcmp(state, "focusable"))
    return ax::mojom::State::FOCUSABLE;
  if (0 == strcmp(state, "haspopup"))
    return ax::mojom::State::HASPOPUP;
  if (0 == strcmp(state, "horizontal"))
    return ax::mojom::State::HORIZONTAL;
  if (0 == strcmp(state, "hovered"))
    return ax::mojom::State::HOVERED;
  if (0 == strcmp(state, "invisible"))
    return ax::mojom::State::INVISIBLE;
  if (0 == strcmp(state, "linked"))
    return ax::mojom::State::LINKED;
  if (0 == strcmp(state, "multiline"))
    return ax::mojom::State::MULTILINE;
  if (0 == strcmp(state, "multiselectable"))
    return ax::mojom::State::MULTISELECTABLE;
  if (0 == strcmp(state, "protected"))
    return ax::mojom::State::PROTECTED;
  if (0 == strcmp(state, "required"))
    return ax::mojom::State::REQUIRED;
  if (0 == strcmp(state, "richlyEditable"))
    return ax::mojom::State::RICHLY_EDITABLE;
  if (0 == strcmp(state, "selectable"))
    return ax::mojom::State::SELECTABLE;
  if (0 == strcmp(state, "selected"))
    return ax::mojom::State::SELECTED;
  if (0 == strcmp(state, "vertical"))
    return ax::mojom::State::VERTICAL;
  if (0 == strcmp(state, "visited"))
    return ax::mojom::State::VISITED;
  return ax::mojom::State::NONE;
}

const char* ToString(ax::mojom::Action action) {
  switch (action) {
    case ax::mojom::Action::NONE:
      return "none";
    case ax::mojom::Action::BLUR:
      return "blur";
    case ax::mojom::Action::CUSTOM_ACTION:
      return "customAction";
    case ax::mojom::Action::DECREMENT:
      return "decrement";
    case ax::mojom::Action::DO_DEFAULT:
      return "doDefault";
    case ax::mojom::Action::FOCUS:
      return "focus";
    case ax::mojom::Action::GET_IMAGE_DATA:
      return "getImageData";
    case ax::mojom::Action::HIT_TEST:
      return "hitTest";
    case ax::mojom::Action::INCREMENT:
      return "increment";
    case ax::mojom::Action::LOAD_INLINE_TEXT_BOXES:
      return "loadInlineTextBoxes";
    case ax::mojom::Action::REPLACE_SELECTED_TEXT:
      return "replaceSelectedText";
    case ax::mojom::Action::SCROLL_BACKWARD:
      return "scrollBackward";
    case ax::mojom::Action::SCROLL_FORWARD:
      return "scrollForward";
    case ax::mojom::Action::SCROLL_UP:
      return "scrollUp";
    case ax::mojom::Action::SCROLL_DOWN:
      return "scrollDown";
    case ax::mojom::Action::SCROLL_LEFT:
      return "scrollLeft";
    case ax::mojom::Action::SCROLL_RIGHT:
      return "scrollRight";
    case ax::mojom::Action::SCROLL_TO_MAKE_VISIBLE:
      return "scrollToMakeVisible";
    case ax::mojom::Action::SCROLL_TO_POINT:
      return "scrollToPoint";
    case ax::mojom::Action::SET_SCROLL_OFFSET:
      return "setScrollOffset";
    case ax::mojom::Action::SET_SELECTION:
      return "setSelection";
    case ax::mojom::Action::SET_SEQUENTIAL_FOCUS_NAVIGATION_STARTING_POINT:
      return "setSequentialFocusNavigationStartingPoint";
    case ax::mojom::Action::SET_VALUE:
      return "setValue";
    case ax::mojom::Action::SHOW_CONTEXT_MENU:
      return "showContextMenu";
  }
}

ax::mojom::Action ParseAction(const char *action) {
  if (0 == strcmp(action, "none"))
    return ax::mojom::Action::NONE;
  if (0 == strcmp(action, "blur"))
    return ax::mojom::Action::BLUR;
  if (0 == strcmp(action, "customAction"))
    return ax::mojom::Action::CUSTOM_ACTION;
  if (0 == strcmp(action, "decrement"))
    return ax::mojom::Action::DECREMENT;
  if (0 == strcmp(action, "doDefault"))
    return ax::mojom::Action::DO_DEFAULT;
  if (0 == strcmp(action, "focus"))
    return ax::mojom::Action::FOCUS;
  if (0 == strcmp(action, "getImageData"))
    return ax::mojom::Action::GET_IMAGE_DATA;
  if (0 == strcmp(action, "hitTest"))
    return ax::mojom::Action::HIT_TEST;
  if (0 == strcmp(action, "increment"))
    return ax::mojom::Action::INCREMENT;
  if (0 == strcmp(action, "loadInlineTextBoxes"))
    return ax::mojom::Action::LOAD_INLINE_TEXT_BOXES;
  if (0 == strcmp(action, "replaceSelectedText"))
    return ax::mojom::Action::REPLACE_SELECTED_TEXT;
  if (0 == strcmp(action, "scrollBackward"))
    return ax::mojom::Action::SCROLL_BACKWARD;
  if (0 == strcmp(action, "scrollForward"))
    return ax::mojom::Action::SCROLL_FORWARD;
  if (0 == strcmp(action, "scrollUp"))
    return ax::mojom::Action::SCROLL_UP;
  if (0 == strcmp(action, "scrollDown"))
    return ax::mojom::Action::SCROLL_DOWN;
  if (0 == strcmp(action, "scrollLeft"))
    return ax::mojom::Action::SCROLL_LEFT;
  if (0 == strcmp(action, "scrollRight"))
    return ax::mojom::Action::SCROLL_RIGHT;
  if (0 == strcmp(action, "scrollToMakeVisible"))
    return ax::mojom::Action::SCROLL_TO_MAKE_VISIBLE;
  if (0 == strcmp(action, "scrollToPoint"))
    return ax::mojom::Action::SCROLL_TO_POINT;
  if (0 == strcmp(action, "setScrollOffset"))
    return ax::mojom::Action::SET_SCROLL_OFFSET;
  if (0 == strcmp(action, "setSelection"))
    return ax::mojom::Action::SET_SELECTION;
  if (0 == strcmp(action, "setSequentialFocusNavigationStartingPoint"))
    return ax::mojom::Action::SET_SEQUENTIAL_FOCUS_NAVIGATION_STARTING_POINT;
  if (0 == strcmp(action, "setValue"))
    return ax::mojom::Action::SET_VALUE;
  if (0 == strcmp(action, "showContextMenu"))
    return ax::mojom::Action::SHOW_CONTEXT_MENU;
  return ax::mojom::Action::NONE;
}

const char* ToString(ax::mojom::ActionFlags action_flags) {
  switch (action_flags) {
    case ax::mojom::ActionFlags::NONE:
      return "none";
    case ax::mojom::ActionFlags::REQUEST_IMAGES:
      return "requestImages";
    case ax::mojom::ActionFlags::REQUEST_INLINE_TEXT_BOXES:
      return "requestInlineTextBoxes";
  }
}

ax::mojom::ActionFlags ParseActionFlags(const char *action_flags) {
  if (0 == strcmp(action_flags, "none"))
    return ax::mojom::ActionFlags::NONE;
  if (0 == strcmp(action_flags, "requestImages"))
    return ax::mojom::ActionFlags::REQUEST_IMAGES;
  if (0 == strcmp(action_flags, "requestInlineTextBoxes"))
    return ax::mojom::ActionFlags::REQUEST_INLINE_TEXT_BOXES;
  return ax::mojom::ActionFlags::NONE;
}

const char* ToString(ax::mojom::DefaultActionVerb default_action_verb) {
  switch (default_action_verb) {
    case ax::mojom::DefaultActionVerb::NONE:
      return "none";
    case ax::mojom::DefaultActionVerb::ACTIVATE:
      return "activate";
    case ax::mojom::DefaultActionVerb::CHECK:
      return "check";
    case ax::mojom::DefaultActionVerb::CLICK:
      return "click";
    case ax::mojom::DefaultActionVerb::CLICK_ANCESTOR:
      return "clickAncestor";
    case ax::mojom::DefaultActionVerb::JUMP:
      return "jump";
    case ax::mojom::DefaultActionVerb::OPEN:
      return "open";
    case ax::mojom::DefaultActionVerb::PRESS:
      return "press";
    case ax::mojom::DefaultActionVerb::SELECT:
      return "select";
    case ax::mojom::DefaultActionVerb::UNCHECK:
      return "uncheck";
  }
}

ax::mojom::DefaultActionVerb ParseDefaultActionVerb(const char *default_action_verb) {
  if (0 == strcmp(default_action_verb, "none"))
    return ax::mojom::DefaultActionVerb::NONE;
  if (0 == strcmp(default_action_verb, "activate"))
    return ax::mojom::DefaultActionVerb::ACTIVATE;
  if (0 == strcmp(default_action_verb, "check"))
    return ax::mojom::DefaultActionVerb::CHECK;
  if (0 == strcmp(default_action_verb, "click"))
    return ax::mojom::DefaultActionVerb::CLICK;
  if (0 == strcmp(default_action_verb, "clickAncestor"))
    return ax::mojom::DefaultActionVerb::CLICK_ANCESTOR;
  if (0 == strcmp(default_action_verb, "jump"))
    return ax::mojom::DefaultActionVerb::JUMP;
  if (0 == strcmp(default_action_verb, "open"))
    return ax::mojom::DefaultActionVerb::OPEN;
  if (0 == strcmp(default_action_verb, "press"))
    return ax::mojom::DefaultActionVerb::PRESS;
  if (0 == strcmp(default_action_verb, "select"))
    return ax::mojom::DefaultActionVerb::SELECT;
  if (0 == strcmp(default_action_verb, "uncheck"))
    return ax::mojom::DefaultActionVerb::UNCHECK;
  return ax::mojom::DefaultActionVerb::NONE;
}

const char* ToString(ax::mojom::Mutation mutation) {
  switch (mutation) {
    case ax::mojom::Mutation::NONE:
      return "none";
    case ax::mojom::Mutation::NODE_CREATED:
      return "nodeCreated";
    case ax::mojom::Mutation::SUBTREE_CREATED:
      return "subtreeCreated";
    case ax::mojom::Mutation::NODE_CHANGED:
      return "nodeChanged";
    case ax::mojom::Mutation::NODE_REMOVED:
      return "nodeRemoved";
  }
}

ax::mojom::Mutation ParseMutation(const char *mutation) {
  if (0 == strcmp(mutation, "none"))
    return ax::mojom::Mutation::NONE;
  if (0 == strcmp(mutation, "nodeCreated"))
    return ax::mojom::Mutation::NODE_CREATED;
  if (0 == strcmp(mutation, "subtreeCreated"))
    return ax::mojom::Mutation::SUBTREE_CREATED;
  if (0 == strcmp(mutation, "nodeChanged"))
    return ax::mojom::Mutation::NODE_CHANGED;
  if (0 == strcmp(mutation, "nodeRemoved"))
    return ax::mojom::Mutation::NODE_REMOVED;
  return ax::mojom::Mutation::NONE;
}

const char* ToString(ax::mojom::StringAttribute string_attribute) {
  switch (string_attribute) {
    case ax::mojom::StringAttribute::NONE:
      return "none";
    case ax::mojom::StringAttribute::ACCESS_KEY:
      return "accessKey";
    case ax::mojom::StringAttribute::ARIA_INVALID_VALUE:
      return "ariaInvalidValue";
    case ax::mojom::StringAttribute::AUTO_COMPLETE:
      return "autoComplete";
    case ax::mojom::StringAttribute::CHROME_CHANNEL:
      return "chromeChannel";
    case ax::mojom::StringAttribute::CLASS_NAME:
      return "className";
    case ax::mojom::StringAttribute::CONTAINER_LIVE_RELEVANT:
      return "containerLiveRelevant";
    case ax::mojom::StringAttribute::CONTAINER_LIVE_STATUS:
      return "containerLiveStatus";
    case ax::mojom::StringAttribute::DESCRIPTION:
      return "description";
    case ax::mojom::StringAttribute::DISPLAY:
      return "display";
    case ax::mojom::StringAttribute::FONT_FAMILY:
      return "fontFamily";
    case ax::mojom::StringAttribute::HTML_TAG:
      return "htmlTag";
    case ax::mojom::StringAttribute::IMAGE_DATA_URL:
      return "imageDataUrl";
    case ax::mojom::StringAttribute::INNER_HTML:
      return "innerHtml";
    case ax::mojom::StringAttribute::KEY_SHORTCUTS:
      return "keyShortcuts";
    case ax::mojom::StringAttribute::LANGUAGE:
      return "language";
    case ax::mojom::StringAttribute::NAME:
      return "name";
    case ax::mojom::StringAttribute::LIVE_RELEVANT:
      return "liveRelevant";
    case ax::mojom::StringAttribute::LIVE_STATUS:
      return "liveStatus";
    case ax::mojom::StringAttribute::PLACEHOLDER:
      return "placeholder";
    case ax::mojom::StringAttribute::ROLE:
      return "role";
    case ax::mojom::StringAttribute::ROLE_DESCRIPTION:
      return "roleDescription";
    case ax::mojom::StringAttribute::URL:
      return "url";
    case ax::mojom::StringAttribute::VALUE:
      return "value";
  }
}

ax::mojom::StringAttribute ParseStringAttribute(const char *string_attribute) {
  if (0 == strcmp(string_attribute, "none"))
    return ax::mojom::StringAttribute::NONE;
  if (0 == strcmp(string_attribute, "accessKey"))
    return ax::mojom::StringAttribute::ACCESS_KEY;
  if (0 == strcmp(string_attribute, "ariaInvalidValue"))
    return ax::mojom::StringAttribute::ARIA_INVALID_VALUE;
  if (0 == strcmp(string_attribute, "autoComplete"))
    return ax::mojom::StringAttribute::AUTO_COMPLETE;
  if (0 == strcmp(string_attribute, "chromeChannel"))
    return ax::mojom::StringAttribute::CHROME_CHANNEL;
  if (0 == strcmp(string_attribute, "className"))
    return ax::mojom::StringAttribute::CLASS_NAME;
  if (0 == strcmp(string_attribute, "containerLiveRelevant"))
    return ax::mojom::StringAttribute::CONTAINER_LIVE_RELEVANT;
  if (0 == strcmp(string_attribute, "containerLiveStatus"))
    return ax::mojom::StringAttribute::CONTAINER_LIVE_STATUS;
  if (0 == strcmp(string_attribute, "description"))
    return ax::mojom::StringAttribute::DESCRIPTION;
  if (0 == strcmp(string_attribute, "display"))
    return ax::mojom::StringAttribute::DISPLAY;
  if (0 == strcmp(string_attribute, "fontFamily"))
    return ax::mojom::StringAttribute::FONT_FAMILY;
  if (0 == strcmp(string_attribute, "htmlTag"))
    return ax::mojom::StringAttribute::HTML_TAG;
  if (0 == strcmp(string_attribute, "imageDataUrl"))
    return ax::mojom::StringAttribute::IMAGE_DATA_URL;
  if (0 == strcmp(string_attribute, "innerHtml"))
    return ax::mojom::StringAttribute::INNER_HTML;
  if (0 == strcmp(string_attribute, "keyShortcuts"))
    return ax::mojom::StringAttribute::KEY_SHORTCUTS;
  if (0 == strcmp(string_attribute, "language"))
    return ax::mojom::StringAttribute::LANGUAGE;
  if (0 == strcmp(string_attribute, "name"))
    return ax::mojom::StringAttribute::NAME;
  if (0 == strcmp(string_attribute, "liveRelevant"))
    return ax::mojom::StringAttribute::LIVE_RELEVANT;
  if (0 == strcmp(string_attribute, "liveStatus"))
    return ax::mojom::StringAttribute::LIVE_STATUS;
  if (0 == strcmp(string_attribute, "placeholder"))
    return ax::mojom::StringAttribute::PLACEHOLDER;
  if (0 == strcmp(string_attribute, "role"))
    return ax::mojom::StringAttribute::ROLE;
  if (0 == strcmp(string_attribute, "roleDescription"))
    return ax::mojom::StringAttribute::ROLE_DESCRIPTION;
  if (0 == strcmp(string_attribute, "url"))
    return ax::mojom::StringAttribute::URL;
  if (0 == strcmp(string_attribute, "value"))
    return ax::mojom::StringAttribute::VALUE;
  return ax::mojom::StringAttribute::NONE;
}

const char* ToString(ax::mojom::IntAttribute int_attribute) {
  switch (int_attribute) {
    case ax::mojom::IntAttribute::NONE:
      return "none";
    case ax::mojom::IntAttribute::DEFAULT_ACTION_VERB:
      return "defaultActionVerb";
    case ax::mojom::IntAttribute::SCROLL_X:
      return "scrollX";
    case ax::mojom::IntAttribute::SCROLL_X_MIN:
      return "scrollXMin";
    case ax::mojom::IntAttribute::SCROLL_X_MAX:
      return "scrollXMax";
    case ax::mojom::IntAttribute::SCROLL_Y:
      return "scrollY";
    case ax::mojom::IntAttribute::SCROLL_Y_MIN:
      return "scrollYMin";
    case ax::mojom::IntAttribute::SCROLL_Y_MAX:
      return "scrollYMax";
    case ax::mojom::IntAttribute::TEXT_SEL_START:
      return "textSelStart";
    case ax::mojom::IntAttribute::TEXT_SEL_END:
      return "textSelEnd";
    case ax::mojom::IntAttribute::ARIA_COLUMN_COUNT:
      return "ariaColumnCount";
    case ax::mojom::IntAttribute::ARIA_CELL_COLUMN_INDEX:
      return "ariaCellColumnIndex";
    case ax::mojom::IntAttribute::ARIA_ROW_COUNT:
      return "ariaRowCount";
    case ax::mojom::IntAttribute::ARIA_CELL_ROW_INDEX:
      return "ariaCellRowIndex";
    case ax::mojom::IntAttribute::TABLE_ROW_COUNT:
      return "tableRowCount";
    case ax::mojom::IntAttribute::TABLE_COLUMN_COUNT:
      return "tableColumnCount";
    case ax::mojom::IntAttribute::TABLE_HEADER_ID:
      return "tableHeaderId";
    case ax::mojom::IntAttribute::TABLE_ROW_INDEX:
      return "tableRowIndex";
    case ax::mojom::IntAttribute::TABLE_ROW_HEADER_ID:
      return "tableRowHeaderId";
    case ax::mojom::IntAttribute::TABLE_COLUMN_INDEX:
      return "tableColumnIndex";
    case ax::mojom::IntAttribute::TABLE_COLUMN_HEADER_ID:
      return "tableColumnHeaderId";
    case ax::mojom::IntAttribute::TABLE_CELL_COLUMN_INDEX:
      return "tableCellColumnIndex";
    case ax::mojom::IntAttribute::TABLE_CELL_COLUMN_SPAN:
      return "tableCellColumnSpan";
    case ax::mojom::IntAttribute::TABLE_CELL_ROW_INDEX:
      return "tableCellRowIndex";
    case ax::mojom::IntAttribute::TABLE_CELL_ROW_SPAN:
      return "tableCellRowSpan";
    case ax::mojom::IntAttribute::SORT_DIRECTION:
      return "sortDirection";
    case ax::mojom::IntAttribute::HIERARCHICAL_LEVEL:
      return "hierarchicalLevel";
    case ax::mojom::IntAttribute::NAME_FROM:
      return "nameFrom";
    case ax::mojom::IntAttribute::DESCRIPTION_FROM:
      return "descriptionFrom";
    case ax::mojom::IntAttribute::ACTIVEDESCENDANT_ID:
      return "activedescendantId";
    case ax::mojom::IntAttribute::DETAILS_ID:
      return "detailsId";
    case ax::mojom::IntAttribute::ERRORMESSAGE_ID:
      return "errormessageId";
    case ax::mojom::IntAttribute::IN_PAGE_LINK_TARGET_ID:
      return "inPageLinkTargetId";
    case ax::mojom::IntAttribute::MEMBER_OF_ID:
      return "memberOfId";
    case ax::mojom::IntAttribute::NEXT_ON_LINE_ID:
      return "nextOnLineId";
    case ax::mojom::IntAttribute::PREVIOUS_ON_LINE_ID:
      return "previousOnLineId";
    case ax::mojom::IntAttribute::CHILD_TREE_ID:
      return "childTreeId";
    case ax::mojom::IntAttribute::RESTRICTION:
      return "restriction";
    case ax::mojom::IntAttribute::SET_SIZE:
      return "setSize";
    case ax::mojom::IntAttribute::POS_IN_SET:
      return "posInSet";
    case ax::mojom::IntAttribute::COLOR_VALUE:
      return "colorValue";
    case ax::mojom::IntAttribute::ARIA_CURRENT_STATE:
      return "ariaCurrentState";
    case ax::mojom::IntAttribute::BACKGROUND_COLOR:
      return "backgroundColor";
    case ax::mojom::IntAttribute::COLOR:
      return "color";
    case ax::mojom::IntAttribute::INVALID_STATE:
      return "invalidState";
    case ax::mojom::IntAttribute::CHECKED_STATE:
      return "checkedState";
    case ax::mojom::IntAttribute::TEXT_DIRECTION:
      return "textDirection";
    case ax::mojom::IntAttribute::TEXT_STYLE:
      return "textStyle";
    case ax::mojom::IntAttribute::PREVIOUS_FOCUS_ID:
      return "previousFocusId";
    case ax::mojom::IntAttribute::NEXT_FOCUS_ID:
      return "nextFocusId";
  }
}

ax::mojom::IntAttribute ParseIntAttribute(const char *int_attribute) {
  if (0 == strcmp(int_attribute, "none"))
    return ax::mojom::IntAttribute::NONE;
  if (0 == strcmp(int_attribute, "defaultActionVerb"))
    return ax::mojom::IntAttribute::DEFAULT_ACTION_VERB;
  if (0 == strcmp(int_attribute, "scrollX"))
    return ax::mojom::IntAttribute::SCROLL_X;
  if (0 == strcmp(int_attribute, "scrollXMin"))
    return ax::mojom::IntAttribute::SCROLL_X_MIN;
  if (0 == strcmp(int_attribute, "scrollXMax"))
    return ax::mojom::IntAttribute::SCROLL_X_MAX;
  if (0 == strcmp(int_attribute, "scrollY"))
    return ax::mojom::IntAttribute::SCROLL_Y;
  if (0 == strcmp(int_attribute, "scrollYMin"))
    return ax::mojom::IntAttribute::SCROLL_Y_MIN;
  if (0 == strcmp(int_attribute, "scrollYMax"))
    return ax::mojom::IntAttribute::SCROLL_Y_MAX;
  if (0 == strcmp(int_attribute, "textSelStart"))
    return ax::mojom::IntAttribute::TEXT_SEL_START;
  if (0 == strcmp(int_attribute, "textSelEnd"))
    return ax::mojom::IntAttribute::TEXT_SEL_END;
  if (0 == strcmp(int_attribute, "ariaColumnCount"))
    return ax::mojom::IntAttribute::ARIA_COLUMN_COUNT;
  if (0 == strcmp(int_attribute, "ariaCellColumnIndex"))
    return ax::mojom::IntAttribute::ARIA_CELL_COLUMN_INDEX;
  if (0 == strcmp(int_attribute, "ariaRowCount"))
    return ax::mojom::IntAttribute::ARIA_ROW_COUNT;
  if (0 == strcmp(int_attribute, "ariaCellRowIndex"))
    return ax::mojom::IntAttribute::ARIA_CELL_ROW_INDEX;
  if (0 == strcmp(int_attribute, "tableRowCount"))
    return ax::mojom::IntAttribute::TABLE_ROW_COUNT;
  if (0 == strcmp(int_attribute, "tableColumnCount"))
    return ax::mojom::IntAttribute::TABLE_COLUMN_COUNT;
  if (0 == strcmp(int_attribute, "tableHeaderId"))
    return ax::mojom::IntAttribute::TABLE_HEADER_ID;
  if (0 == strcmp(int_attribute, "tableRowIndex"))
    return ax::mojom::IntAttribute::TABLE_ROW_INDEX;
  if (0 == strcmp(int_attribute, "tableRowHeaderId"))
    return ax::mojom::IntAttribute::TABLE_ROW_HEADER_ID;
  if (0 == strcmp(int_attribute, "tableColumnIndex"))
    return ax::mojom::IntAttribute::TABLE_COLUMN_INDEX;
  if (0 == strcmp(int_attribute, "tableColumnHeaderId"))
    return ax::mojom::IntAttribute::TABLE_COLUMN_HEADER_ID;
  if (0 == strcmp(int_attribute, "tableCellColumnIndex"))
    return ax::mojom::IntAttribute::TABLE_CELL_COLUMN_INDEX;
  if (0 == strcmp(int_attribute, "tableCellColumnSpan"))
    return ax::mojom::IntAttribute::TABLE_CELL_COLUMN_SPAN;
  if (0 == strcmp(int_attribute, "tableCellRowIndex"))
    return ax::mojom::IntAttribute::TABLE_CELL_ROW_INDEX;
  if (0 == strcmp(int_attribute, "tableCellRowSpan"))
    return ax::mojom::IntAttribute::TABLE_CELL_ROW_SPAN;
  if (0 == strcmp(int_attribute, "sortDirection"))
    return ax::mojom::IntAttribute::SORT_DIRECTION;
  if (0 == strcmp(int_attribute, "hierarchicalLevel"))
    return ax::mojom::IntAttribute::HIERARCHICAL_LEVEL;
  if (0 == strcmp(int_attribute, "nameFrom"))
    return ax::mojom::IntAttribute::NAME_FROM;
  if (0 == strcmp(int_attribute, "descriptionFrom"))
    return ax::mojom::IntAttribute::DESCRIPTION_FROM;
  if (0 == strcmp(int_attribute, "activedescendantId"))
    return ax::mojom::IntAttribute::ACTIVEDESCENDANT_ID;
  if (0 == strcmp(int_attribute, "detailsId"))
    return ax::mojom::IntAttribute::DETAILS_ID;
  if (0 == strcmp(int_attribute, "errormessageId"))
    return ax::mojom::IntAttribute::ERRORMESSAGE_ID;
  if (0 == strcmp(int_attribute, "inPageLinkTargetId"))
    return ax::mojom::IntAttribute::IN_PAGE_LINK_TARGET_ID;
  if (0 == strcmp(int_attribute, "memberOfId"))
    return ax::mojom::IntAttribute::MEMBER_OF_ID;
  if (0 == strcmp(int_attribute, "nextOnLineId"))
    return ax::mojom::IntAttribute::NEXT_ON_LINE_ID;
  if (0 == strcmp(int_attribute, "previousOnLineId"))
    return ax::mojom::IntAttribute::PREVIOUS_ON_LINE_ID;
  if (0 == strcmp(int_attribute, "childTreeId"))
    return ax::mojom::IntAttribute::CHILD_TREE_ID;
  if (0 == strcmp(int_attribute, "restriction"))
    return ax::mojom::IntAttribute::RESTRICTION;
  if (0 == strcmp(int_attribute, "setSize"))
    return ax::mojom::IntAttribute::SET_SIZE;
  if (0 == strcmp(int_attribute, "posInSet"))
    return ax::mojom::IntAttribute::POS_IN_SET;
  if (0 == strcmp(int_attribute, "colorValue"))
    return ax::mojom::IntAttribute::COLOR_VALUE;
  if (0 == strcmp(int_attribute, "ariaCurrentState"))
    return ax::mojom::IntAttribute::ARIA_CURRENT_STATE;
  if (0 == strcmp(int_attribute, "backgroundColor"))
    return ax::mojom::IntAttribute::BACKGROUND_COLOR;
  if (0 == strcmp(int_attribute, "color"))
    return ax::mojom::IntAttribute::COLOR;
  if (0 == strcmp(int_attribute, "invalidState"))
    return ax::mojom::IntAttribute::INVALID_STATE;
  if (0 == strcmp(int_attribute, "checkedState"))
    return ax::mojom::IntAttribute::CHECKED_STATE;
  if (0 == strcmp(int_attribute, "textDirection"))
    return ax::mojom::IntAttribute::TEXT_DIRECTION;
  if (0 == strcmp(int_attribute, "textStyle"))
    return ax::mojom::IntAttribute::TEXT_STYLE;
  if (0 == strcmp(int_attribute, "previousFocusId"))
    return ax::mojom::IntAttribute::PREVIOUS_FOCUS_ID;
  if (0 == strcmp(int_attribute, "nextFocusId"))
    return ax::mojom::IntAttribute::NEXT_FOCUS_ID;
  return ax::mojom::IntAttribute::NONE;
}

const char* ToString(ax::mojom::FloatAttribute float_attribute) {
  switch (float_attribute) {
    case ax::mojom::FloatAttribute::NONE:
      return "none";
    case ax::mojom::FloatAttribute::VALUE_FOR_RANGE:
      return "valueForRange";
    case ax::mojom::FloatAttribute::MIN_VALUE_FOR_RANGE:
      return "minValueForRange";
    case ax::mojom::FloatAttribute::MAX_VALUE_FOR_RANGE:
      return "maxValueForRange";
    case ax::mojom::FloatAttribute::STEP_VALUE_FOR_RANGE:
      return "stepValueForRange";
    case ax::mojom::FloatAttribute::FONT_SIZE:
      return "fontSize";
  }
}

ax::mojom::FloatAttribute ParseFloatAttribute(const char *float_attribute) {
  if (0 == strcmp(float_attribute, "none"))
    return ax::mojom::FloatAttribute::NONE;
  if (0 == strcmp(float_attribute, "valueForRange"))
    return ax::mojom::FloatAttribute::VALUE_FOR_RANGE;
  if (0 == strcmp(float_attribute, "minValueForRange"))
    return ax::mojom::FloatAttribute::MIN_VALUE_FOR_RANGE;
  if (0 == strcmp(float_attribute, "maxValueForRange"))
    return ax::mojom::FloatAttribute::MAX_VALUE_FOR_RANGE;
  if (0 == strcmp(float_attribute, "stepValueForRange"))
    return ax::mojom::FloatAttribute::STEP_VALUE_FOR_RANGE;
  if (0 == strcmp(float_attribute, "fontSize"))
    return ax::mojom::FloatAttribute::FONT_SIZE;
  return ax::mojom::FloatAttribute::NONE;
}

const char* ToString(ax::mojom::BoolAttribute bool_attribute) {
  switch (bool_attribute) {
    case ax::mojom::BoolAttribute::NONE:
      return "none";
    case ax::mojom::BoolAttribute::BUSY:
      return "busy";
    case ax::mojom::BoolAttribute::EDITABLE_ROOT:
      return "editableRoot";
    case ax::mojom::BoolAttribute::CONTAINER_LIVE_ATOMIC:
      return "containerLiveAtomic";
    case ax::mojom::BoolAttribute::CONTAINER_LIVE_BUSY:
      return "containerLiveBusy";
    case ax::mojom::BoolAttribute::LIVE_ATOMIC:
      return "liveAtomic";
    case ax::mojom::BoolAttribute::MODAL:
      return "modal";
    case ax::mojom::BoolAttribute::UPDATE_LOCATION_ONLY:
      return "updateLocationOnly";
    case ax::mojom::BoolAttribute::CANVAS_HAS_FALLBACK:
      return "canvasHasFallback";
    case ax::mojom::BoolAttribute::SCROLLABLE:
      return "scrollable";
    case ax::mojom::BoolAttribute::CLICKABLE:
      return "clickable";
    case ax::mojom::BoolAttribute::CLIPS_CHILDREN:
      return "clipsChildren";
  }
}

ax::mojom::BoolAttribute ParseBoolAttribute(const char *bool_attribute) {
  if (0 == strcmp(bool_attribute, "none"))
    return ax::mojom::BoolAttribute::NONE;
  if (0 == strcmp(bool_attribute, "busy"))
    return ax::mojom::BoolAttribute::BUSY;
  if (0 == strcmp(bool_attribute, "editableRoot"))
    return ax::mojom::BoolAttribute::EDITABLE_ROOT;
  if (0 == strcmp(bool_attribute, "containerLiveAtomic"))
    return ax::mojom::BoolAttribute::CONTAINER_LIVE_ATOMIC;
  if (0 == strcmp(bool_attribute, "containerLiveBusy"))
    return ax::mojom::BoolAttribute::CONTAINER_LIVE_BUSY;
  if (0 == strcmp(bool_attribute, "liveAtomic"))
    return ax::mojom::BoolAttribute::LIVE_ATOMIC;
  if (0 == strcmp(bool_attribute, "modal"))
    return ax::mojom::BoolAttribute::MODAL;
  if (0 == strcmp(bool_attribute, "updateLocationOnly"))
    return ax::mojom::BoolAttribute::UPDATE_LOCATION_ONLY;
  if (0 == strcmp(bool_attribute, "canvasHasFallback"))
    return ax::mojom::BoolAttribute::CANVAS_HAS_FALLBACK;
  if (0 == strcmp(bool_attribute, "scrollable"))
    return ax::mojom::BoolAttribute::SCROLLABLE;
  if (0 == strcmp(bool_attribute, "clickable"))
    return ax::mojom::BoolAttribute::CLICKABLE;
  if (0 == strcmp(bool_attribute, "clipsChildren"))
    return ax::mojom::BoolAttribute::CLIPS_CHILDREN;
  return ax::mojom::BoolAttribute::NONE;
}

const char* ToString(ax::mojom::IntListAttribute int_list_attribute) {
  switch (int_list_attribute) {
    case ax::mojom::IntListAttribute::NONE:
      return "none";
    case ax::mojom::IntListAttribute::INDIRECT_CHILD_IDS:
      return "indirectChildIds";
    case ax::mojom::IntListAttribute::CONTROLS_IDS:
      return "controlsIds";
    case ax::mojom::IntListAttribute::DESCRIBEDBY_IDS:
      return "describedbyIds";
    case ax::mojom::IntListAttribute::FLOWTO_IDS:
      return "flowtoIds";
    case ax::mojom::IntListAttribute::LABELLEDBY_IDS:
      return "labelledbyIds";
    case ax::mojom::IntListAttribute::RADIO_GROUP_IDS:
      return "radioGroupIds";
    case ax::mojom::IntListAttribute::LINE_BREAKS:
      return "lineBreaks";
    case ax::mojom::IntListAttribute::MARKER_TYPES:
      return "markerTypes";
    case ax::mojom::IntListAttribute::MARKER_STARTS:
      return "markerStarts";
    case ax::mojom::IntListAttribute::MARKER_ENDS:
      return "markerEnds";
    case ax::mojom::IntListAttribute::CELL_IDS:
      return "cellIds";
    case ax::mojom::IntListAttribute::UNIQUE_CELL_IDS:
      return "uniqueCellIds";
    case ax::mojom::IntListAttribute::CHARACTER_OFFSETS:
      return "characterOffsets";
    case ax::mojom::IntListAttribute::CACHED_LINE_STARTS:
      return "cachedLineStarts";
    case ax::mojom::IntListAttribute::WORD_STARTS:
      return "wordStarts";
    case ax::mojom::IntListAttribute::WORD_ENDS:
      return "wordEnds";
    case ax::mojom::IntListAttribute::CUSTOM_ACTION_IDS:
      return "customActionIds";
  }
}

ax::mojom::IntListAttribute ParseIntListAttribute(const char *int_list_attribute) {
  if (0 == strcmp(int_list_attribute, "none"))
    return ax::mojom::IntListAttribute::NONE;
  if (0 == strcmp(int_list_attribute, "indirectChildIds"))
    return ax::mojom::IntListAttribute::INDIRECT_CHILD_IDS;
  if (0 == strcmp(int_list_attribute, "controlsIds"))
    return ax::mojom::IntListAttribute::CONTROLS_IDS;
  if (0 == strcmp(int_list_attribute, "describedbyIds"))
    return ax::mojom::IntListAttribute::DESCRIBEDBY_IDS;
  if (0 == strcmp(int_list_attribute, "flowtoIds"))
    return ax::mojom::IntListAttribute::FLOWTO_IDS;
  if (0 == strcmp(int_list_attribute, "labelledbyIds"))
    return ax::mojom::IntListAttribute::LABELLEDBY_IDS;
  if (0 == strcmp(int_list_attribute, "radioGroupIds"))
    return ax::mojom::IntListAttribute::RADIO_GROUP_IDS;
  if (0 == strcmp(int_list_attribute, "lineBreaks"))
    return ax::mojom::IntListAttribute::LINE_BREAKS;
  if (0 == strcmp(int_list_attribute, "markerTypes"))
    return ax::mojom::IntListAttribute::MARKER_TYPES;
  if (0 == strcmp(int_list_attribute, "markerStarts"))
    return ax::mojom::IntListAttribute::MARKER_STARTS;
  if (0 == strcmp(int_list_attribute, "markerEnds"))
    return ax::mojom::IntListAttribute::MARKER_ENDS;
  if (0 == strcmp(int_list_attribute, "cellIds"))
    return ax::mojom::IntListAttribute::CELL_IDS;
  if (0 == strcmp(int_list_attribute, "uniqueCellIds"))
    return ax::mojom::IntListAttribute::UNIQUE_CELL_IDS;
  if (0 == strcmp(int_list_attribute, "characterOffsets"))
    return ax::mojom::IntListAttribute::CHARACTER_OFFSETS;
  if (0 == strcmp(int_list_attribute, "cachedLineStarts"))
    return ax::mojom::IntListAttribute::CACHED_LINE_STARTS;
  if (0 == strcmp(int_list_attribute, "wordStarts"))
    return ax::mojom::IntListAttribute::WORD_STARTS;
  if (0 == strcmp(int_list_attribute, "wordEnds"))
    return ax::mojom::IntListAttribute::WORD_ENDS;
  if (0 == strcmp(int_list_attribute, "customActionIds"))
    return ax::mojom::IntListAttribute::CUSTOM_ACTION_IDS;
  return ax::mojom::IntListAttribute::NONE;
}

const char* ToString(ax::mojom::StringListAttribute string_list_attribute) {
  switch (string_list_attribute) {
    case ax::mojom::StringListAttribute::NONE:
      return "none";
    case ax::mojom::StringListAttribute::CUSTOM_ACTION_DESCRIPTIONS:
      return "customActionDescriptions";
  }
}

ax::mojom::StringListAttribute ParseStringListAttribute(const char *string_list_attribute) {
  if (0 == strcmp(string_list_attribute, "none"))
    return ax::mojom::StringListAttribute::NONE;
  if (0 == strcmp(string_list_attribute, "customActionDescriptions"))
    return ax::mojom::StringListAttribute::CUSTOM_ACTION_DESCRIPTIONS;
  return ax::mojom::StringListAttribute::NONE;
}

const char* ToString(ax::mojom::MarkerType marker_type) {
  switch (marker_type) {
    case ax::mojom::MarkerType::NONE:
      return "none";
    case ax::mojom::MarkerType::SPELLING:
      return "spelling";
    case ax::mojom::MarkerType::GRAMMAR:
      return "grammar";
    case ax::mojom::MarkerType::SPELLING_GRAMMAR:
      return "spellingGrammar";
    case ax::mojom::MarkerType::TEXT_MATCH:
      return "textMatch";
    case ax::mojom::MarkerType::SPELLING_TEXT_MATCH:
      return "spellingTextMatch";
    case ax::mojom::MarkerType::GRAMMAR_TEXT_MATCH:
      return "grammarTextMatch";
    case ax::mojom::MarkerType::SPELLING_GRAMMAR_TEXT_MATCH:
      return "spellingGrammarTextMatch";
    case ax::mojom::MarkerType::ACTIVE_SUGGESTION:
      return "activeSuggestion";
    case ax::mojom::MarkerType::SPELLING_ACTIVE_SUGGESTION:
      return "spellingActiveSuggestion";
    case ax::mojom::MarkerType::GRAMMAR_ACTIVE_SUGGESTION:
      return "grammarActiveSuggestion";
    case ax::mojom::MarkerType::SPELLING_GRAMMAR_ACTIVE_SUGGESTION:
      return "spellingGrammarActiveSuggestion";
    case ax::mojom::MarkerType::TEXT_MATCH_ACTIVE_SUGGESTION:
      return "textMatchActiveSuggestion";
    case ax::mojom::MarkerType::SPELLING_TEXT_MATCH_ACTIVE_SUGGESTION:
      return "spellingTextMatchActiveSuggestion";
    case ax::mojom::MarkerType::GRAMMAR_TEXT_MATCH_ACTIVE_SUGGESTION:
      return "grammarTextMatchActiveSuggestion";
    case ax::mojom::MarkerType::SPELLING_GRAMMAR_TEXT_MATCH_ACTIVE_SUGGESTION:
      return "spellingGrammarTextMatchActiveSuggestion";
    case ax::mojom::MarkerType::SUGGESTION:
      return "suggestion";
    case ax::mojom::MarkerType::SPELLING_SUGGESTION:
      return "spellingSuggestion";
    case ax::mojom::MarkerType::GRAMMAR_SUGGESTION:
      return "grammarSuggestion";
    case ax::mojom::MarkerType::SPELLING_GRAMMAR_SUGGESTION:
      return "spellingGrammarSuggestion";
    case ax::mojom::MarkerType::TEXT_MATCH_SUGGESTION:
      return "textMatchSuggestion";
    case ax::mojom::MarkerType::SPELLING_TEXT_MATCH_SUGGESTION:
      return "spellingTextMatchSuggestion";
    case ax::mojom::MarkerType::GRAMMAR_TEXT_MATCH_SUGGESTION:
      return "grammarTextMatchSuggestion";
    case ax::mojom::MarkerType::SPELLING_GRAMMAR_TEXT_MATCH_SUGGESTION:
      return "spellingGrammarTextMatchSuggestion";
    case ax::mojom::MarkerType::ACTIVE_SUGGESTION_SUGGESTION:
      return "activeSuggestionSuggestion";
    case ax::mojom::MarkerType::SPELLING_ACTIVE_SUGGESTION_SUGGESTION:
      return "spellingActiveSuggestionSuggestion";
    case ax::mojom::MarkerType::GRAMMAR_ACTIVE_SUGGESTION_SUGGESTION:
      return "grammarActiveSuggestionSuggestion";
    case ax::mojom::MarkerType::SPELLING_GRAMMAR_ACTIVE_SUGGESTION_SUGGESTION:
      return "spellingGrammarActiveSuggestionSuggestion";
    case ax::mojom::MarkerType::TEXT_MATCH_ACTIVE_SUGGESTION_SUGGESTION:
      return "textMatchActiveSuggestionSuggestion";
    case ax::mojom::MarkerType::SPELLING_TEXT_MATCH_ACTIVE_SUGGESTION_SUGGESTION:
      return "spellingTextMatchActiveSuggestionSuggestion";
    case ax::mojom::MarkerType::GRAMMAR_TEXT_MATCH_ACTIVE_SUGGESTION_SUGGESTION:
      return "grammarTextMatchActiveSuggestionSuggestion";
    case ax::mojom::MarkerType::SPELLING_GRAMMAR_TEXT_MATCH_ACTIVE_SUGGESTION_SUGGESTION:
      return "spellingGrammarTextMatchActiveSuggestionSuggestion";
  }
}

ax::mojom::MarkerType ParseMarkerType(const char *marker_type) {
  if (0 == strcmp(marker_type, "none"))
    return ax::mojom::MarkerType::NONE;
  if (0 == strcmp(marker_type, "spelling"))
    return ax::mojom::MarkerType::SPELLING;
  if (0 == strcmp(marker_type, "grammar"))
    return ax::mojom::MarkerType::GRAMMAR;
  if (0 == strcmp(marker_type, "spellingGrammar"))
    return ax::mojom::MarkerType::SPELLING_GRAMMAR;
  if (0 == strcmp(marker_type, "textMatch"))
    return ax::mojom::MarkerType::TEXT_MATCH;
  if (0 == strcmp(marker_type, "spellingTextMatch"))
    return ax::mojom::MarkerType::SPELLING_TEXT_MATCH;
  if (0 == strcmp(marker_type, "grammarTextMatch"))
    return ax::mojom::MarkerType::GRAMMAR_TEXT_MATCH;
  if (0 == strcmp(marker_type, "spellingGrammarTextMatch"))
    return ax::mojom::MarkerType::SPELLING_GRAMMAR_TEXT_MATCH;
  if (0 == strcmp(marker_type, "activeSuggestion"))
    return ax::mojom::MarkerType::ACTIVE_SUGGESTION;
  if (0 == strcmp(marker_type, "spellingActiveSuggestion"))
    return ax::mojom::MarkerType::SPELLING_ACTIVE_SUGGESTION;
  if (0 == strcmp(marker_type, "grammarActiveSuggestion"))
    return ax::mojom::MarkerType::GRAMMAR_ACTIVE_SUGGESTION;
  if (0 == strcmp(marker_type, "spellingGrammarActiveSuggestion"))
    return ax::mojom::MarkerType::SPELLING_GRAMMAR_ACTIVE_SUGGESTION;
  if (0 == strcmp(marker_type, "textMatchActiveSuggestion"))
    return ax::mojom::MarkerType::TEXT_MATCH_ACTIVE_SUGGESTION;
  if (0 == strcmp(marker_type, "spellingTextMatchActiveSuggestion"))
    return ax::mojom::MarkerType::SPELLING_TEXT_MATCH_ACTIVE_SUGGESTION;
  if (0 == strcmp(marker_type, "grammarTextMatchActiveSuggestion"))
    return ax::mojom::MarkerType::GRAMMAR_TEXT_MATCH_ACTIVE_SUGGESTION;
  if (0 == strcmp(marker_type, "spellingGrammarTextMatchActiveSuggestion"))
    return ax::mojom::MarkerType::SPELLING_GRAMMAR_TEXT_MATCH_ACTIVE_SUGGESTION;
  if (0 == strcmp(marker_type, "suggestion"))
    return ax::mojom::MarkerType::SUGGESTION;
  if (0 == strcmp(marker_type, "spellingSuggestion"))
    return ax::mojom::MarkerType::SPELLING_SUGGESTION;
  if (0 == strcmp(marker_type, "grammarSuggestion"))
    return ax::mojom::MarkerType::GRAMMAR_SUGGESTION;
  if (0 == strcmp(marker_type, "spellingGrammarSuggestion"))
    return ax::mojom::MarkerType::SPELLING_GRAMMAR_SUGGESTION;
  if (0 == strcmp(marker_type, "textMatchSuggestion"))
    return ax::mojom::MarkerType::TEXT_MATCH_SUGGESTION;
  if (0 == strcmp(marker_type, "spellingTextMatchSuggestion"))
    return ax::mojom::MarkerType::SPELLING_TEXT_MATCH_SUGGESTION;
  if (0 == strcmp(marker_type, "grammarTextMatchSuggestion"))
    return ax::mojom::MarkerType::GRAMMAR_TEXT_MATCH_SUGGESTION;
  if (0 == strcmp(marker_type, "spellingGrammarTextMatchSuggestion"))
    return ax::mojom::MarkerType::SPELLING_GRAMMAR_TEXT_MATCH_SUGGESTION;
  if (0 == strcmp(marker_type, "activeSuggestionSuggestion"))
    return ax::mojom::MarkerType::ACTIVE_SUGGESTION_SUGGESTION;
  if (0 == strcmp(marker_type, "spellingActiveSuggestionSuggestion"))
    return ax::mojom::MarkerType::SPELLING_ACTIVE_SUGGESTION_SUGGESTION;
  if (0 == strcmp(marker_type, "grammarActiveSuggestionSuggestion"))
    return ax::mojom::MarkerType::GRAMMAR_ACTIVE_SUGGESTION_SUGGESTION;
  if (0 == strcmp(marker_type, "spellingGrammarActiveSuggestionSuggestion"))
    return ax::mojom::MarkerType::SPELLING_GRAMMAR_ACTIVE_SUGGESTION_SUGGESTION;
  if (0 == strcmp(marker_type, "textMatchActiveSuggestionSuggestion"))
    return ax::mojom::MarkerType::TEXT_MATCH_ACTIVE_SUGGESTION_SUGGESTION;
  if (0 == strcmp(marker_type, "spellingTextMatchActiveSuggestionSuggestion"))
    return ax::mojom::MarkerType::SPELLING_TEXT_MATCH_ACTIVE_SUGGESTION_SUGGESTION;
  if (0 == strcmp(marker_type, "grammarTextMatchActiveSuggestionSuggestion"))
    return ax::mojom::MarkerType::GRAMMAR_TEXT_MATCH_ACTIVE_SUGGESTION_SUGGESTION;
  if (0 == strcmp(marker_type, "spellingGrammarTextMatchActiveSuggestionSuggestion"))
    return ax::mojom::MarkerType::SPELLING_GRAMMAR_TEXT_MATCH_ACTIVE_SUGGESTION_SUGGESTION;
  return ax::mojom::MarkerType::NONE;
}

const char* ToString(ax::mojom::TextDirection text_direction) {
  switch (text_direction) {
    case ax::mojom::TextDirection::NONE:
      return "none";
    case ax::mojom::TextDirection::LTR:
      return "ltr";
    case ax::mojom::TextDirection::RTL:
      return "rtl";
    case ax::mojom::TextDirection::TTB:
      return "ttb";
    case ax::mojom::TextDirection::BTT:
      return "btt";
  }
}

ax::mojom::TextDirection ParseTextDirection(const char *text_direction) {
  if (0 == strcmp(text_direction, "none"))
    return ax::mojom::TextDirection::NONE;
  if (0 == strcmp(text_direction, "ltr"))
    return ax::mojom::TextDirection::LTR;
  if (0 == strcmp(text_direction, "rtl"))
    return ax::mojom::TextDirection::RTL;
  if (0 == strcmp(text_direction, "ttb"))
    return ax::mojom::TextDirection::TTB;
  if (0 == strcmp(text_direction, "btt"))
    return ax::mojom::TextDirection::BTT;
  return ax::mojom::TextDirection::NONE;
}

const char* ToString(ax::mojom::TextStyle text_style) {
  switch (text_style) {
    case ax::mojom::TextStyle::NONE:
      return "none";
    case ax::mojom::TextStyle::TEXT_STYLE_BOLD:
      return "textStyleBold";
    case ax::mojom::TextStyle::TEXT_STYLE_ITALIC:
      return "textStyleItalic";
    case ax::mojom::TextStyle::TEXT_STYLE_BOLD_ITALIC:
      return "textStyleBoldItalic";
    case ax::mojom::TextStyle::TEXT_STYLE_UNDERLINE:
      return "textStyleUnderline";
    case ax::mojom::TextStyle::TEXT_STYLE_BOLD_UNDERLINE:
      return "textStyleBoldUnderline";
    case ax::mojom::TextStyle::TEXT_STYLE_ITALIC_UNDERLINE:
      return "textStyleItalicUnderline";
    case ax::mojom::TextStyle::TEXT_STYLE_BOLD_ITALIC_UNDERLINE:
      return "textStyleBoldItalicUnderline";
    case ax::mojom::TextStyle::TEXT_STYLE_LINE_THROUGH:
      return "textStyleLineThrough";
    case ax::mojom::TextStyle::TEXT_STYLE_BOLD_LINE_THROUGH:
      return "textStyleBoldLineThrough";
    case ax::mojom::TextStyle::TEXT_STYLE_ITALIC_LINE_THROUGH:
      return "textStyleItalicLineThrough";
    case ax::mojom::TextStyle::TEXT_STYLE_BOLD_ITALIC_LINE_THROUGH:
      return "textStyleBoldItalicLineThrough";
    case ax::mojom::TextStyle::TEXT_STYLE_UNDERLINE_LINE_THROUGH:
      return "textStyleUnderlineLineThrough";
    case ax::mojom::TextStyle::TEXT_STYLE_BOLD_UNDERLINE_LINE_THROUGH:
      return "textStyleBoldUnderlineLineThrough";
    case ax::mojom::TextStyle::TEXT_STYLE_ITALIC_UNDERLINE_LINE_THROUGH:
      return "textStyleItalicUnderlineLineThrough";
    case ax::mojom::TextStyle::TEXT_STYLE_BOLD_ITALIC_UNDERLINE_LINE_THROUGH:
      return "textStyleBoldItalicUnderlineLineThrough";
  }
}

ax::mojom::TextStyle ParseTextStyle(const char *text_style) {
  if (0 == strcmp(text_style, "none"))
    return ax::mojom::TextStyle::NONE;
  if (0 == strcmp(text_style, "textStyleBold"))
    return ax::mojom::TextStyle::TEXT_STYLE_BOLD;
  if (0 == strcmp(text_style, "textStyleItalic"))
    return ax::mojom::TextStyle::TEXT_STYLE_ITALIC;
  if (0 == strcmp(text_style, "textStyleBoldItalic"))
    return ax::mojom::TextStyle::TEXT_STYLE_BOLD_ITALIC;
  if (0 == strcmp(text_style, "textStyleUnderline"))
    return ax::mojom::TextStyle::TEXT_STYLE_UNDERLINE;
  if (0 == strcmp(text_style, "textStyleBoldUnderline"))
    return ax::mojom::TextStyle::TEXT_STYLE_BOLD_UNDERLINE;
  if (0 == strcmp(text_style, "textStyleItalicUnderline"))
    return ax::mojom::TextStyle::TEXT_STYLE_ITALIC_UNDERLINE;
  if (0 == strcmp(text_style, "textStyleBoldItalicUnderline"))
    return ax::mojom::TextStyle::TEXT_STYLE_BOLD_ITALIC_UNDERLINE;
  if (0 == strcmp(text_style, "textStyleLineThrough"))
    return ax::mojom::TextStyle::TEXT_STYLE_LINE_THROUGH;
  if (0 == strcmp(text_style, "textStyleBoldLineThrough"))
    return ax::mojom::TextStyle::TEXT_STYLE_BOLD_LINE_THROUGH;
  if (0 == strcmp(text_style, "textStyleItalicLineThrough"))
    return ax::mojom::TextStyle::TEXT_STYLE_ITALIC_LINE_THROUGH;
  if (0 == strcmp(text_style, "textStyleBoldItalicLineThrough"))
    return ax::mojom::TextStyle::TEXT_STYLE_BOLD_ITALIC_LINE_THROUGH;
  if (0 == strcmp(text_style, "textStyleUnderlineLineThrough"))
    return ax::mojom::TextStyle::TEXT_STYLE_UNDERLINE_LINE_THROUGH;
  if (0 == strcmp(text_style, "textStyleBoldUnderlineLineThrough"))
    return ax::mojom::TextStyle::TEXT_STYLE_BOLD_UNDERLINE_LINE_THROUGH;
  if (0 == strcmp(text_style, "textStyleItalicUnderlineLineThrough"))
    return ax::mojom::TextStyle::TEXT_STYLE_ITALIC_UNDERLINE_LINE_THROUGH;
  if (0 == strcmp(text_style, "textStyleBoldItalicUnderlineLineThrough"))
    return ax::mojom::TextStyle::TEXT_STYLE_BOLD_ITALIC_UNDERLINE_LINE_THROUGH;
  return ax::mojom::TextStyle::NONE;
}

const char* ToString(ax::mojom::AriaCurrentState aria_current_state) {
  switch (aria_current_state) {
    case ax::mojom::AriaCurrentState::NONE:
      return "none";
    case ax::mojom::AriaCurrentState::FALSE_VALUE:
      return "false";
    case ax::mojom::AriaCurrentState::TRUE_VALUE:
      return "true";
    case ax::mojom::AriaCurrentState::PAGE:
      return "page";
    case ax::mojom::AriaCurrentState::STEP:
      return "step";
    case ax::mojom::AriaCurrentState::LOCATION:
      return "location";
    case ax::mojom::AriaCurrentState::UNCLIPPED_LOCATION:
      return "unclippedLocation";
    case ax::mojom::AriaCurrentState::DATE:
      return "date";
    case ax::mojom::AriaCurrentState::TIME:
      return "time";
  }
}

ax::mojom::AriaCurrentState ParseAriaCurrentState(const char *aria_current_state) {
  if (0 == strcmp(aria_current_state, "none"))
    return ax::mojom::AriaCurrentState::NONE;
  if (0 == strcmp(aria_current_state, "false"))
    return ax::mojom::AriaCurrentState::FALSE_VALUE;
  if (0 == strcmp(aria_current_state, "true"))
    return ax::mojom::AriaCurrentState::TRUE_VALUE;
  if (0 == strcmp(aria_current_state, "page"))
    return ax::mojom::AriaCurrentState::PAGE;
  if (0 == strcmp(aria_current_state, "step"))
    return ax::mojom::AriaCurrentState::STEP;
  if (0 == strcmp(aria_current_state, "location"))
    return ax::mojom::AriaCurrentState::LOCATION;
  if (0 == strcmp(aria_current_state, "unclippedLocation"))
    return ax::mojom::AriaCurrentState::UNCLIPPED_LOCATION;
  if (0 == strcmp(aria_current_state, "date"))
    return ax::mojom::AriaCurrentState::DATE;
  if (0 == strcmp(aria_current_state, "time"))
    return ax::mojom::AriaCurrentState::TIME;
  return ax::mojom::AriaCurrentState::NONE;
}

const char* ToString(ax::mojom::InvalidState invalid_state) {
  switch (invalid_state) {
    case ax::mojom::InvalidState::NONE:
      return "none";
    case ax::mojom::InvalidState::FALSE_VALUE:
      return "false";
    case ax::mojom::InvalidState::TRUE_VALUE:
      return "true";
    case ax::mojom::InvalidState::SPELLING:
      return "spelling";
    case ax::mojom::InvalidState::GRAMMAR:
      return "grammar";
    case ax::mojom::InvalidState::OTHER:
      return "other";
  }
}

ax::mojom::InvalidState ParseInvalidState(const char *invalid_state) {
  if (0 == strcmp(invalid_state, "none"))
    return ax::mojom::InvalidState::NONE;
  if (0 == strcmp(invalid_state, "false"))
    return ax::mojom::InvalidState::FALSE_VALUE;
  if (0 == strcmp(invalid_state, "true"))
    return ax::mojom::InvalidState::TRUE_VALUE;
  if (0 == strcmp(invalid_state, "spelling"))
    return ax::mojom::InvalidState::SPELLING;
  if (0 == strcmp(invalid_state, "grammar"))
    return ax::mojom::InvalidState::GRAMMAR;
  if (0 == strcmp(invalid_state, "other"))
    return ax::mojom::InvalidState::OTHER;
  return ax::mojom::InvalidState::NONE;
}

const char* ToString(ax::mojom::Restriction restriction) {
  switch (restriction) {
    case ax::mojom::Restriction::NONE:
      return "none";
    case ax::mojom::Restriction::READ_ONLY:
      return "readOnly";
    case ax::mojom::Restriction::DISABLED:
      return "disabled";
  }
}

ax::mojom::Restriction ParseRestriction(const char *restriction) {
  if (0 == strcmp(restriction, "none"))
    return ax::mojom::Restriction::NONE;
  if (0 == strcmp(restriction, "readOnly"))
    return ax::mojom::Restriction::READ_ONLY;
  if (0 == strcmp(restriction, "disabled"))
    return ax::mojom::Restriction::DISABLED;
  return ax::mojom::Restriction::NONE;
}

const char* ToString(ax::mojom::CheckedState checked_state) {
  switch (checked_state) {
    case ax::mojom::CheckedState::NONE:
      return "none";
    case ax::mojom::CheckedState::FALSE_VALUE:
      return "false";
    case ax::mojom::CheckedState::TRUE_VALUE:
      return "true";
    case ax::mojom::CheckedState::MIXED:
      return "mixed";
  }
}

ax::mojom::CheckedState ParseCheckedState(const char *checked_state) {
  if (0 == strcmp(checked_state, "none"))
    return ax::mojom::CheckedState::NONE;
  if (0 == strcmp(checked_state, "false"))
    return ax::mojom::CheckedState::FALSE_VALUE;
  if (0 == strcmp(checked_state, "true"))
    return ax::mojom::CheckedState::TRUE_VALUE;
  if (0 == strcmp(checked_state, "mixed"))
    return ax::mojom::CheckedState::MIXED;
  return ax::mojom::CheckedState::NONE;
}

const char* ToString(ax::mojom::SortDirection sort_direction) {
  switch (sort_direction) {
    case ax::mojom::SortDirection::NONE:
      return "none";
    case ax::mojom::SortDirection::UNSORTED:
      return "unsorted";
    case ax::mojom::SortDirection::ASCENDING:
      return "ascending";
    case ax::mojom::SortDirection::DESCENDING:
      return "descending";
    case ax::mojom::SortDirection::OTHER:
      return "other";
  }
}

ax::mojom::SortDirection ParseSortDirection(const char *sort_direction) {
  if (0 == strcmp(sort_direction, "none"))
    return ax::mojom::SortDirection::NONE;
  if (0 == strcmp(sort_direction, "unsorted"))
    return ax::mojom::SortDirection::UNSORTED;
  if (0 == strcmp(sort_direction, "ascending"))
    return ax::mojom::SortDirection::ASCENDING;
  if (0 == strcmp(sort_direction, "descending"))
    return ax::mojom::SortDirection::DESCENDING;
  if (0 == strcmp(sort_direction, "other"))
    return ax::mojom::SortDirection::OTHER;
  return ax::mojom::SortDirection::NONE;
}

const char* ToString(ax::mojom::NameFrom name_from) {
  switch (name_from) {
    case ax::mojom::NameFrom::NONE:
      return "none";
    case ax::mojom::NameFrom::UNINITIALIZED:
      return "uninitialized";
    case ax::mojom::NameFrom::ATTRIBUTE:
      return "attribute";
    case ax::mojom::NameFrom::ATTRIBUTE_EXPLICITLY_EMPTY:
      return "attributeExplicitlyEmpty";
    case ax::mojom::NameFrom::CONTENTS:
      return "contents";
    case ax::mojom::NameFrom::PLACEHOLDER:
      return "placeholder";
    case ax::mojom::NameFrom::RELATED_ELEMENT:
      return "relatedElement";
    case ax::mojom::NameFrom::VALUE:
      return "value";
  }
}

ax::mojom::NameFrom ParseNameFrom(const char *name_from) {
  if (0 == strcmp(name_from, "none"))
    return ax::mojom::NameFrom::NONE;
  if (0 == strcmp(name_from, "uninitialized"))
    return ax::mojom::NameFrom::UNINITIALIZED;
  if (0 == strcmp(name_from, "attribute"))
    return ax::mojom::NameFrom::ATTRIBUTE;
  if (0 == strcmp(name_from, "attributeExplicitlyEmpty"))
    return ax::mojom::NameFrom::ATTRIBUTE_EXPLICITLY_EMPTY;
  if (0 == strcmp(name_from, "contents"))
    return ax::mojom::NameFrom::CONTENTS;
  if (0 == strcmp(name_from, "placeholder"))
    return ax::mojom::NameFrom::PLACEHOLDER;
  if (0 == strcmp(name_from, "relatedElement"))
    return ax::mojom::NameFrom::RELATED_ELEMENT;
  if (0 == strcmp(name_from, "value"))
    return ax::mojom::NameFrom::VALUE;
  return ax::mojom::NameFrom::NONE;
}

const char* ToString(ax::mojom::DescriptionFrom description_from) {
  switch (description_from) {
    case ax::mojom::DescriptionFrom::NONE:
      return "none";
    case ax::mojom::DescriptionFrom::UNINITIALIZED:
      return "uninitialized";
    case ax::mojom::DescriptionFrom::ATTRIBUTE:
      return "attribute";
    case ax::mojom::DescriptionFrom::CONTENTS:
      return "contents";
    case ax::mojom::DescriptionFrom::PLACEHOLDER:
      return "placeholder";
    case ax::mojom::DescriptionFrom::RELATED_ELEMENT:
      return "relatedElement";
  }
}

ax::mojom::DescriptionFrom ParseDescriptionFrom(const char *description_from) {
  if (0 == strcmp(description_from, "none"))
    return ax::mojom::DescriptionFrom::NONE;
  if (0 == strcmp(description_from, "uninitialized"))
    return ax::mojom::DescriptionFrom::UNINITIALIZED;
  if (0 == strcmp(description_from, "attribute"))
    return ax::mojom::DescriptionFrom::ATTRIBUTE;
  if (0 == strcmp(description_from, "contents"))
    return ax::mojom::DescriptionFrom::CONTENTS;
  if (0 == strcmp(description_from, "placeholder"))
    return ax::mojom::DescriptionFrom::PLACEHOLDER;
  if (0 == strcmp(description_from, "relatedElement"))
    return ax::mojom::DescriptionFrom::RELATED_ELEMENT;
  return ax::mojom::DescriptionFrom::NONE;
}

const char* ToString(ax::mojom::EventFrom event_from) {
  switch (event_from) {
    case ax::mojom::EventFrom::NONE:
      return "none";
    case ax::mojom::EventFrom::USER:
      return "user";
    case ax::mojom::EventFrom::PAGE:
      return "page";
    case ax::mojom::EventFrom::ACTION:
      return "action";
  }
}

ax::mojom::EventFrom ParseEventFrom(const char *event_from) {
  if (0 == strcmp(event_from, "none"))
    return ax::mojom::EventFrom::NONE;
  if (0 == strcmp(event_from, "user"))
    return ax::mojom::EventFrom::USER;
  if (0 == strcmp(event_from, "page"))
    return ax::mojom::EventFrom::PAGE;
  if (0 == strcmp(event_from, "action"))
    return ax::mojom::EventFrom::ACTION;
  return ax::mojom::EventFrom::NONE;
}

const char* ToString(ax::mojom::Gesture gesture) {
  switch (gesture) {
    case ax::mojom::Gesture::NONE:
      return "none";
    case ax::mojom::Gesture::CLICK:
      return "click";
    case ax::mojom::Gesture::SWIPE_LEFT_1:
      return "swipeLeft1";
    case ax::mojom::Gesture::SWIPE_UP_1:
      return "swipeUp1";
    case ax::mojom::Gesture::SWIPE_RIGHT_1:
      return "swipeRight1";
    case ax::mojom::Gesture::SWIPE_DOWN_1:
      return "swipeDown1";
    case ax::mojom::Gesture::SWIPE_LEFT_2:
      return "swipeLeft2";
    case ax::mojom::Gesture::SWIPE_UP_2:
      return "swipeUp2";
    case ax::mojom::Gesture::SWIPE_RIGHT_2:
      return "swipeRight2";
    case ax::mojom::Gesture::SWIPE_DOWN_2:
      return "swipeDown2";
    case ax::mojom::Gesture::SWIPE_LEFT_3:
      return "swipeLeft3";
    case ax::mojom::Gesture::SWIPE_UP_3:
      return "swipeUp3";
    case ax::mojom::Gesture::SWIPE_RIGHT_3:
      return "swipeRight3";
    case ax::mojom::Gesture::SWIPE_DOWN_3:
      return "swipeDown3";
    case ax::mojom::Gesture::SWIPE_LEFT_4:
      return "swipeLeft4";
    case ax::mojom::Gesture::SWIPE_UP_4:
      return "swipeUp4";
    case ax::mojom::Gesture::SWIPE_RIGHT_4:
      return "swipeRight4";
    case ax::mojom::Gesture::SWIPE_DOWN_4:
      return "swipeDown4";
    case ax::mojom::Gesture::TAP_2:
      return "tap2";
  }
}

ax::mojom::Gesture ParseGesture(const char *gesture) {
  if (0 == strcmp(gesture, "none"))
    return ax::mojom::Gesture::NONE;
  if (0 == strcmp(gesture, "click"))
    return ax::mojom::Gesture::CLICK;
  if (0 == strcmp(gesture, "swipeLeft1"))
    return ax::mojom::Gesture::SWIPE_LEFT_1;
  if (0 == strcmp(gesture, "swipeUp1"))
    return ax::mojom::Gesture::SWIPE_UP_1;
  if (0 == strcmp(gesture, "swipeRight1"))
    return ax::mojom::Gesture::SWIPE_RIGHT_1;
  if (0 == strcmp(gesture, "swipeDown1"))
    return ax::mojom::Gesture::SWIPE_DOWN_1;
  if (0 == strcmp(gesture, "swipeLeft2"))
    return ax::mojom::Gesture::SWIPE_LEFT_2;
  if (0 == strcmp(gesture, "swipeUp2"))
    return ax::mojom::Gesture::SWIPE_UP_2;
  if (0 == strcmp(gesture, "swipeRight2"))
    return ax::mojom::Gesture::SWIPE_RIGHT_2;
  if (0 == strcmp(gesture, "swipeDown2"))
    return ax::mojom::Gesture::SWIPE_DOWN_2;
  if (0 == strcmp(gesture, "swipeLeft3"))
    return ax::mojom::Gesture::SWIPE_LEFT_3;
  if (0 == strcmp(gesture, "swipeUp3"))
    return ax::mojom::Gesture::SWIPE_UP_3;
  if (0 == strcmp(gesture, "swipeRight3"))
    return ax::mojom::Gesture::SWIPE_RIGHT_3;
  if (0 == strcmp(gesture, "swipeDown3"))
    return ax::mojom::Gesture::SWIPE_DOWN_3;
  if (0 == strcmp(gesture, "swipeLeft4"))
    return ax::mojom::Gesture::SWIPE_LEFT_4;
  if (0 == strcmp(gesture, "swipeUp4"))
    return ax::mojom::Gesture::SWIPE_UP_4;
  if (0 == strcmp(gesture, "swipeRight4"))
    return ax::mojom::Gesture::SWIPE_RIGHT_4;
  if (0 == strcmp(gesture, "swipeDown4"))
    return ax::mojom::Gesture::SWIPE_DOWN_4;
  if (0 == strcmp(gesture, "tap2"))
    return ax::mojom::Gesture::TAP_2;
  return ax::mojom::Gesture::NONE;
}

const char* ToString(ax::mojom::TextAffinity text_affinity) {
  switch (text_affinity) {
    case ax::mojom::TextAffinity::NONE:
      return "none";
    case ax::mojom::TextAffinity::DOWNSTREAM:
      return "downstream";
    case ax::mojom::TextAffinity::UPSTREAM:
      return "upstream";
  }
}

ax::mojom::TextAffinity ParseTextAffinity(const char *text_affinity) {
  if (0 == strcmp(text_affinity, "none"))
    return ax::mojom::TextAffinity::NONE;
  if (0 == strcmp(text_affinity, "downstream"))
    return ax::mojom::TextAffinity::DOWNSTREAM;
  if (0 == strcmp(text_affinity, "upstream"))
    return ax::mojom::TextAffinity::UPSTREAM;
  return ax::mojom::TextAffinity::NONE;
}

const char* ToString(ax::mojom::TreeOrder tree_order) {
  switch (tree_order) {
    case ax::mojom::TreeOrder::NONE:
      return "none";
    case ax::mojom::TreeOrder::UNDEFINED:
      return "undefined";
    case ax::mojom::TreeOrder::BEFORE:
      return "before";
    case ax::mojom::TreeOrder::EQUAL:
      return "equal";
    case ax::mojom::TreeOrder::AFTER:
      return "after";
  }
}

ax::mojom::TreeOrder ParseTreeOrder(const char *tree_order) {
  if (0 == strcmp(tree_order, "none"))
    return ax::mojom::TreeOrder::NONE;
  if (0 == strcmp(tree_order, "undefined"))
    return ax::mojom::TreeOrder::UNDEFINED;
  if (0 == strcmp(tree_order, "before"))
    return ax::mojom::TreeOrder::BEFORE;
  if (0 == strcmp(tree_order, "equal"))
    return ax::mojom::TreeOrder::EQUAL;
  if (0 == strcmp(tree_order, "after"))
    return ax::mojom::TreeOrder::AFTER;
  return ax::mojom::TreeOrder::NONE;
}

}  // namespace ui
