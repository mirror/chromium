// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/accessibility/browser_accessibility_android.h"

#include "base/containers/hash_tables.h"
#include "base/i18n/break_iterator.h"
#include "base/lazy_instance.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "content/app/strings/grit/content_strings.h"
#include "content/browser/accessibility/browser_accessibility_manager_android.h"
#include "content/common/accessibility_messages.h"
#include "content/public/common/content_client.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/accessibility/ax_role_properties.h"
#include "ui/accessibility/platform/ax_android_constants.h"
#include "ui/accessibility/platform/ax_snapshot_node_android_platform.h"
#include "ui/accessibility/platform/ax_unique_id.h"

namespace {

// These are enums from android.text.InputType in Java:
enum {
  ANDROID_TEXT_INPUTTYPE_TYPE_NULL = 0,
  ANDROID_TEXT_INPUTTYPE_TYPE_DATETIME = 0x4,
  ANDROID_TEXT_INPUTTYPE_TYPE_DATETIME_DATE = 0x14,
  ANDROID_TEXT_INPUTTYPE_TYPE_DATETIME_TIME = 0x24,
  ANDROID_TEXT_INPUTTYPE_TYPE_NUMBER = 0x2,
  ANDROID_TEXT_INPUTTYPE_TYPE_PHONE = 0x3,
  ANDROID_TEXT_INPUTTYPE_TYPE_TEXT = 0x1,
  ANDROID_TEXT_INPUTTYPE_TYPE_TEXT_URI = 0x11,
  ANDROID_TEXT_INPUTTYPE_TYPE_TEXT_WEB_EDIT_TEXT = 0xa1,
  ANDROID_TEXT_INPUTTYPE_TYPE_TEXT_WEB_EMAIL = 0xd1,
  ANDROID_TEXT_INPUTTYPE_TYPE_TEXT_WEB_PASSWORD = 0xe1
};

// These are enums from android.view.View in Java:
enum {
  ANDROID_VIEW_VIEW_ACCESSIBILITY_LIVE_REGION_NONE = 0,
  ANDROID_VIEW_VIEW_ACCESSIBILITY_LIVE_REGION_POLITE = 1,
  ANDROID_VIEW_VIEW_ACCESSIBILITY_LIVE_REGION_ASSERTIVE = 2
};

// These are enums from
// android.view.accessibility.AccessibilityNodeInfo.RangeInfo in Java:
enum {
  ANDROID_VIEW_ACCESSIBILITY_RANGE_TYPE_FLOAT = 1
};

}  // namespace

namespace content {

// static
BrowserAccessibility* BrowserAccessibility::Create() {
  return new BrowserAccessibilityAndroid();
}

using UniqueIdMap = base::hash_map<int32_t, BrowserAccessibilityAndroid*>;
// Map from each AXPlatformNode's unique id to its instance.
base::LazyInstance<UniqueIdMap>::Leaky g_unique_id_map =
    LAZY_INSTANCE_INITIALIZER;

// static
BrowserAccessibilityAndroid* BrowserAccessibilityAndroid::GetFromUniqueId(
    int32_t unique_id) {
  UniqueIdMap* unique_ids = g_unique_id_map.Pointer();
  auto iter = unique_ids->find(unique_id);
  if (iter != unique_ids->end())
    return iter->second;

  return nullptr;
}

BrowserAccessibilityAndroid::BrowserAccessibilityAndroid() {
  g_unique_id_map.Get()[unique_id()] = this;
}

BrowserAccessibilityAndroid::~BrowserAccessibilityAndroid() {
  if (unique_id())
    g_unique_id_map.Get().erase(unique_id());
}

bool BrowserAccessibilityAndroid::IsNative() const {
  return true;
}

void BrowserAccessibilityAndroid::OnLocationChanged() {
  auto* manager =
      static_cast<BrowserAccessibilityManagerAndroid*>(this->manager());
  manager->FireLocationChanged(this);
}

base::string16 BrowserAccessibilityAndroid::GetValue() const {
  base::string16 value = BrowserAccessibility::GetValue();

  // Optionally replace entered password text with bullet characters
  // based on a user preference.
  if (IsPassword()) {
    auto* manager =
        static_cast<BrowserAccessibilityManagerAndroid*>(this->manager());
    if (manager->ShouldRespectDisplayedPasswordText()) {
      // In the Chrome accessibility tree, the value of a password node is
      // unobscured. However, if ShouldRespectDisplayedPasswordText() returns
      // true we should try to expose whatever's actually visually displayed,
      // whether that's the actual password or dots or whatever. To do this
      // we rely on the password field's shadow dom.
      value = base::UTF8ToUTF16(ComputeAccessibleNameFromDescendants());
    } else if (!manager->ShouldExposePasswordText()) {
      value = base::string16(value.size(), ui::kSecurePasswordBullet);
    }
  }

  return value;
}

bool BrowserAccessibilityAndroid::PlatformIsLeaf() const {
  if (BrowserAccessibility::PlatformIsLeaf())
    return true;

  // Iframes are always allowed to contain children.
  if (IsIframe() ||
      GetRole() == ax::mojom::Role::ROOT_WEB_AREA ||
      GetRole() == ax::mojom::Role::WEB_AREA) {
    return false;
  }

  // Date and time controls should drop their children.
  switch (GetRole()) {
    case ax::mojom::Role::DATE:
    case ax::mojom::Role::DATE_TIME:
    case ax::mojom::Role::INPUT_TIME:
      return true;
    default:
      break;
  }

  // If it has a focusable child, we definitely can't leave out children.
  if (HasFocusableNonOptionChild())
    return false;

  BrowserAccessibilityManagerAndroid* manager_android =
      static_cast<BrowserAccessibilityManagerAndroid*>(manager());
  if (manager_android->prune_tree_for_screen_reader()) {
    // Headings with text can drop their children.
    base::string16 name = GetText();
    if (GetRole() == ax::mojom::Role::HEADING && !name.empty())
      return true;

    // Focusable nodes with text can drop their children.
    if (HasState(ax::mojom::State::FOCUSABLE) && !name.empty())
      return true;

    // Nodes with only static text as children can drop their children.
    if (HasOnlyTextChildren())
      return true;
  }

  return false;
}

bool BrowserAccessibilityAndroid::IsCheckable() const {
  return HasIntAttribute(ax::mojom::IntAttribute::CHECKED_STATE);
}

bool BrowserAccessibilityAndroid::IsChecked() const {
  return GetIntAttribute(ax::mojom::IntAttribute::CHECKED_STATE) ==
         ax::mojom::CheckedState::TRUE_VALUE;
}

bool BrowserAccessibilityAndroid::IsClickable() const {
  // If it has a custom default action verb except for
  // ax::mojom::DefaultActionVerb::CLICK_ANCESTOR, it's definitely clickable.
  // ax::mojom::DefaultActionVerb::CLICK_ANCESTOR is used when an element with a click
  // listener is present in its ancestry chain.
  if (HasIntAttribute(ax::mojom::IntAttribute::DEFAULT_ACTION_VERB) &&
      (GetIntAttribute(ax::mojom::IntAttribute::DEFAULT_ACTION_VERB) !=
       ax::mojom::DefaultActionVerb::CLICK_ANCESTOR)) {
    return true;
  }

  // Otherwise return true if it's focusable, but skip web areas and iframes.
  if (IsIframe() || (GetRole() == ax::mojom::Role::ROOT_WEB_AREA))
    return false;
  return IsFocusable();
}

bool BrowserAccessibilityAndroid::IsCollapsed() const {
  return HasState(ax::mojom::State::COLLAPSED);
}

// TODO(dougt) Move to ax_role_properties?
bool BrowserAccessibilityAndroid::IsCollection() const {
  return (ui::IsTableLikeRole(GetRole()) || GetRole() == ax::mojom::Role::LIST ||
          GetRole() == ax::mojom::Role::LIST_BOX ||
          GetRole() == ax::mojom::Role::DESCRIPTION_LIST ||
          GetRole() == ax::mojom::Role::TREE);
}

bool BrowserAccessibilityAndroid::IsCollectionItem() const {
  return (GetRole() == ax::mojom::Role::CELL ||
          GetRole() == ax::mojom::Role::COLUMN_HEADER ||
          GetRole() == ax::mojom::Role::DESCRIPTION_LIST_TERM ||
          GetRole() == ax::mojom::Role::LIST_BOX_OPTION ||
          GetRole() == ax::mojom::Role::LIST_ITEM ||
          GetRole() == ax::mojom::Role::ROW_HEADER ||
          GetRole() == ax::mojom::Role::TREE_ITEM);
}

bool BrowserAccessibilityAndroid::IsContentInvalid() const {
  return HasIntAttribute(ax::mojom::IntAttribute::INVALID_STATE) &&
         GetIntAttribute(ax::mojom::IntAttribute::INVALID_STATE) !=
             ax::mojom::InvalidState::FALSE_VALUE;
}

bool BrowserAccessibilityAndroid::IsDismissable() const {
  return false;  // No concept of "dismissable" on the web currently.
}

bool BrowserAccessibilityAndroid::IsEditableText() const {
  return IsPlainTextField() || IsRichTextField();
}

bool BrowserAccessibilityAndroid::IsEnabled() const {
  return GetIntAttribute(ax::mojom::IntAttribute::RESTRICTION) !=
         ax::mojom::Restriction::DISABLED;
}

bool BrowserAccessibilityAndroid::IsExpanded() const {
  return HasState(ax::mojom::State::EXPANDED);
}

bool BrowserAccessibilityAndroid::IsFocusable() const {
  // If it's an iframe element, or the root element of a child frame,
  // only mark it as focusable if the element has an explicit name.
  // Otherwise mark it as not focusable to avoid the user landing on
  // empty container elements in the tree.
  if (IsIframe() ||
      (GetRole() == ax::mojom::Role::ROOT_WEB_AREA && PlatformGetParent()))
    return HasStringAttribute(ax::mojom::StringAttribute::NAME);

  return HasState(ax::mojom::State::FOCUSABLE);
}

bool BrowserAccessibilityAndroid::IsFocused() const {
  return manager()->GetFocus() == this;
}

bool BrowserAccessibilityAndroid::IsHeading() const {
  BrowserAccessibilityAndroid* parent =
      static_cast<BrowserAccessibilityAndroid*>(PlatformGetParent());
  if (parent && parent->IsHeading())
    return true;

  return (GetRole() == ax::mojom::Role::COLUMN_HEADER ||
          GetRole() == ax::mojom::Role::HEADING ||
          GetRole() == ax::mojom::Role::ROW_HEADER);
}

bool BrowserAccessibilityAndroid::IsHierarchical() const {
  return (GetRole() == ax::mojom::Role::LIST ||
          GetRole() == ax::mojom::Role::DESCRIPTION_LIST ||
          GetRole() == ax::mojom::Role::TREE);
}

bool BrowserAccessibilityAndroid::IsLink() const {
  return ui::AXSnapshotNodeAndroid::AXRoleIsLink(GetRole());
}

bool BrowserAccessibilityAndroid::IsMultiLine() const {
  return HasState(ax::mojom::State::MULTILINE);
}

bool BrowserAccessibilityAndroid::IsPassword() const {
  return HasState(ax::mojom::State::PROTECTED);
}

bool BrowserAccessibilityAndroid::IsRangeType() const {
  return (GetRole() == ax::mojom::Role::PROGRESS_INDICATOR ||
          GetRole() == ax::mojom::Role::METER ||
          GetRole() == ax::mojom::Role::SCROLL_BAR ||
          GetRole() == ax::mojom::Role::SLIDER ||
          (GetRole() == ax::mojom::Role::SPLITTER && IsFocusable()));
}

bool BrowserAccessibilityAndroid::IsScrollable() const {
  return HasIntAttribute(ax::mojom::IntAttribute::SCROLL_X_MAX);
}

bool BrowserAccessibilityAndroid::IsSelected() const {
  return HasState(ax::mojom::State::SELECTED);
}

bool BrowserAccessibilityAndroid::IsSlider() const {
  return GetRole() == ax::mojom::Role::SLIDER;
}

bool BrowserAccessibilityAndroid::IsVisibleToUser() const {
  return !HasState(ax::mojom::State::INVISIBLE);
}

bool BrowserAccessibilityAndroid::IsInterestingOnAndroid() const {
  if (GetRole() == ax::mojom::Role::ROOT_WEB_AREA && GetText().empty())
    return true;

  // Focusable nodes are always interesting. Note that IsFocusable()
  // already skips over things like iframes and child frames that are
  // technically focusable but shouldn't be exposed as focusable on Android.
  if (IsFocusable())
    return true;

  // If it's not focusable but has a control role, then it's interesting.
  if (ui::IsControl(GetRole()))
    return true;

  // A non focusable child of a control is not interesting
  const BrowserAccessibility* parent = PlatformGetParent();
  while (parent != nullptr) {
    if (ui::IsControl(parent->GetRole()))
      return false;
    parent = parent->PlatformGetParent();
  }

  // Otherwise, the interesting nodes are leaf nodes with non-whitespace text.
  return PlatformIsLeaf() &&
      !base::ContainsOnlyChars(GetText(), base::kWhitespaceUTF16);
}

const BrowserAccessibilityAndroid*
    BrowserAccessibilityAndroid::GetSoleInterestingNodeFromSubtree() const {
  if (IsInterestingOnAndroid())
    return this;

  const BrowserAccessibilityAndroid* sole_interesting_node = nullptr;
  for (uint32_t i = 0; i < PlatformChildCount(); ++i) {
    const BrowserAccessibilityAndroid* interesting_node =
        static_cast<const BrowserAccessibilityAndroid*>(PlatformGetChild(i))->
            GetSoleInterestingNodeFromSubtree();
    if (interesting_node && sole_interesting_node) {
      // If there are two interesting nodes, return nullptr.
      return nullptr;
    } else if (interesting_node) {
      sole_interesting_node = interesting_node;
    }
  }

  return sole_interesting_node;
}

bool BrowserAccessibilityAndroid::AreInlineTextBoxesLoaded() const {
  if (GetRole() == ax::mojom::Role::STATIC_TEXT)
    return InternalChildCount() > 0;

  // Return false if any descendant needs to load inline text boxes.
  for (uint32_t i = 0; i < InternalChildCount(); ++i) {
    BrowserAccessibilityAndroid* child =
        static_cast<BrowserAccessibilityAndroid*>(InternalGetChild(i));
    if (!child->AreInlineTextBoxesLoaded())
      return false;
  }

  // Otherwise return true - either they're all loaded, or there aren't
  // any descendants that need to load inline text boxes.
  return true;
}

bool BrowserAccessibilityAndroid::CanOpenPopup() const {
  return HasState(ax::mojom::State::HASPOPUP);
}

const char* BrowserAccessibilityAndroid::GetClassName() const {
  return ui::AXSnapshotNodeAndroid::AXRoleToAndroidClassName(
      GetRole(), PlatformGetParent() != nullptr);
}

base::string16 BrowserAccessibilityAndroid::GetText() const {
  if (IsIframe() ||
      GetRole() == ax::mojom::Role::WEB_AREA) {
    return base::string16();
  }

  // First, always return the |value| attribute if this is an
  // input field.
  base::string16 value = GetValue();
  if (ShouldExposeValueAsName())
    return value;

  // For color wells, the color is stored in separate attributes.
  // Perhaps we could return color names in the future?
  if (GetRole() == ax::mojom::Role::COLOR_WELL) {
    unsigned int color =
        static_cast<unsigned int>(GetIntAttribute(ax::mojom::IntAttribute::COLOR_VALUE));
    unsigned int red = SkColorGetR(color);
    unsigned int green = SkColorGetG(color);
    unsigned int blue = SkColorGetB(color);
    return base::UTF8ToUTF16(
        base::StringPrintf("#%02X%02X%02X", red, green, blue));
  }

  base::string16 text = GetString16Attribute(ax::mojom::StringAttribute::NAME);
  if (text.empty())
    text = value;

  // If this is the root element, give up now, allow it to have no
  // accessible text. For almost all other focusable nodes we try to
  // get text from contents, but for the root element that's redundant
  // and often way too verbose.
  if (GetRole() == ax::mojom::Role::ROOT_WEB_AREA)
    return text;

  // This is called from PlatformIsLeaf, so don't call PlatformChildCount
  // from within this!
  if (text.empty() && (HasOnlyTextChildren() ||
                       (IsFocusable() && HasOnlyTextAndImageChildren()))) {
    for (uint32_t i = 0; i < InternalChildCount(); i++) {
      BrowserAccessibility* child = InternalGetChild(i);
      text += static_cast<BrowserAccessibilityAndroid*>(child)->GetText();
    }
  }

  if (text.empty() && (IsLink() || GetRole() == ax::mojom::Role::IMAGE) &&
      !HasExplicitlyEmptyName()) {
    base::string16 url = GetString16Attribute(ax::mojom::StringAttribute::URL);
    text = ui::AXSnapshotNodeAndroid::AXUrlBaseText(url);
  }

  return text;
}

base::string16 BrowserAccessibilityAndroid::GetHint() const {
  base::string16 description = GetString16Attribute(ax::mojom::StringAttribute::DESCRIPTION);

  // If we're returning the value as the main text, then return both the
  // accessible name and description as the hint.
  if (ShouldExposeValueAsName()) {
    base::string16 name = GetString16Attribute(ax::mojom::StringAttribute::NAME);
    if (!name.empty() && !description.empty())
      return name + base::ASCIIToUTF16(" ") + description;
    else if (!name.empty())
      return name;
  }

  return description;
}

std::string BrowserAccessibilityAndroid::GetRoleString() const {
  return ToString(GetRole());
}

base::string16 BrowserAccessibilityAndroid::GetRoleDescription() const {
  content::ContentClient* content_client = content::GetContentClient();

  // As a special case, if we have a heading level return a string like
  // "heading level 1", etc.
  if (GetRole() == ax::mojom::Role::HEADING) {
    int level = GetIntAttribute(ax::mojom::IntAttribute::HIERARCHICAL_LEVEL);
    if (level >= 1 && level <= 6) {
      std::vector<base::string16> values;
      values.push_back(base::IntToString16(level));
      return base::ReplaceStringPlaceholders(
          content_client->GetLocalizedString(IDS_AX_ROLE_HEADING_WITH_LEVEL),
          values, nullptr);
    }
  }

  int message_id = -1;
  switch (GetRole()) {
    case ax::mojom::Role::ABBR:
      // No role description.
      break;
    case ax::mojom::Role::ALERT_DIALOG:
      message_id = IDS_AX_ROLE_ALERT_DIALOG;
      break;
    case ax::mojom::Role::ALERT:
      message_id = IDS_AX_ROLE_ALERT;
      break;
    case ax::mojom::Role::ANCHOR:
      // No role description.
      break;
    case ax::mojom::Role::ANNOTATION:
      // No role description.
      break;
    case ax::mojom::Role::APPLICATION:
      message_id = IDS_AX_ROLE_APPLICATION;
      break;
    case ax::mojom::Role::ARTICLE:
      message_id = IDS_AX_ROLE_ARTICLE;
      break;
    case ax::mojom::Role::AUDIO:
      message_id = IDS_AX_MEDIA_AUDIO_ELEMENT;
      break;
    case ax::mojom::Role::BANNER:
      message_id = IDS_AX_ROLE_BANNER;
      break;
    case ax::mojom::Role::BLOCKQUOTE:
      message_id = IDS_AX_ROLE_BLOCKQUOTE;
      break;
    case ax::mojom::Role::BUTTON:
      message_id = IDS_AX_ROLE_BUTTON;
      break;
    case ax::mojom::Role::BUTTON_DROP_DOWN:
      message_id = IDS_AX_ROLE_BUTTON_DROP_DOWN;
      break;
    case ax::mojom::Role::CANVAS:
      // No role description.
      break;
    case ax::mojom::Role::CAPTION:
      // No role description.
       break;
    case ax::mojom::Role::CARET:
      // No role description.
      break;
    case ax::mojom::Role::CELL:
      message_id = IDS_AX_ROLE_CELL;
      break;
    case ax::mojom::Role::CHECK_BOX:
      message_id = IDS_AX_ROLE_CHECK_BOX;
      break;
    case ax::mojom::Role::CLIENT:
      // No role description.
      break;
    case ax::mojom::Role::COLOR_WELL:
      message_id = IDS_AX_ROLE_COLOR_WELL;
      break;
    case ax::mojom::Role::COLUMN_HEADER:
      message_id = IDS_AX_ROLE_COLUMN_HEADER;
      break;
    case ax::mojom::Role::COLUMN:
      // No role description.
      break;
    case ax::mojom::Role::COMBO_BOX_GROUPING:
      // No role descripotion.
      break;
    case ax::mojom::Role::COMBO_BOX_MENU_BUTTON:
      // No role descripotion.
      break;
    case ax::mojom::Role::COMPLEMENTARY:
      message_id = IDS_AX_ROLE_COMPLEMENTARY;
      break;
    case ax::mojom::Role::CONTENT_INFO:
      message_id = IDS_AX_ROLE_CONTENT_INFO;
      break;
    case ax::mojom::Role::DATE:
      message_id = IDS_AX_ROLE_DATE;
      break;
    case ax::mojom::Role::DATE_TIME:
      message_id = IDS_AX_ROLE_DATE_TIME;
      break;
    case ax::mojom::Role::DEFINITION:
      message_id = IDS_AX_ROLE_DEFINITION;
      break;
    case ax::mojom::Role::DESCRIPTION_LIST_DETAIL:
      message_id = IDS_AX_ROLE_DEFINITION;
      break;
    case ax::mojom::Role::DESCRIPTION_LIST:
      // No role description.
      break;
    case ax::mojom::Role::DESCRIPTION_LIST_TERM:
      // No role description.
      break;
    case ax::mojom::Role::DESKTOP:
      // No role description.
      break;
    case ax::mojom::Role::DETAILS:
      // No role description.
      break;
    case ax::mojom::Role::DIALOG:
      message_id = IDS_AX_ROLE_DIALOG;
      break;
    case ax::mojom::Role::DIRECTORY:
      message_id = IDS_AX_ROLE_DIRECTORY;
      break;
    case ax::mojom::Role::DISCLOSURE_TRIANGLE:
      message_id = IDS_AX_ROLE_DISCLOSURE_TRIANGLE;
      break;
    case ax::mojom::Role::DOCUMENT:
      message_id = IDS_AX_ROLE_DOCUMENT;
      break;
    case ax::mojom::Role::EMBEDDED_OBJECT:
      message_id = IDS_AX_ROLE_EMBEDDED_OBJECT;
      break;
    case ax::mojom::Role::FEED:
      message_id = IDS_AX_ROLE_FEED;
      break;
    case ax::mojom::Role::FIGCAPTION:
      // No role description.
      break;
    case ax::mojom::Role::FIGURE:
      message_id = IDS_AX_ROLE_GRAPHIC;
      break;
    case ax::mojom::Role::FOOTER:
      message_id = IDS_AX_ROLE_FOOTER;
      break;
    case ax::mojom::Role::FORM:
      // No role description.
      break;
    case ax::mojom::Role::GENERIC_CONTAINER:
      // No role description.
      break;
    case ax::mojom::Role::GRID:
      message_id = IDS_AX_ROLE_TABLE;
      break;
    case ax::mojom::Role::GROUP:
      // No role description.
      break;
    case ax::mojom::Role::HEADING:
      // Note that code above this switch statement handles headings with
      // a level, returning a string like "heading level 1", etc.
      message_id = IDS_AX_ROLE_HEADING;
      break;
    case ax::mojom::Role::IFRAME:
      // No role description.
      break;
    case ax::mojom::Role::IFRAME_PRESENTATIONAL:
      // No role description.
      break;
    case ax::mojom::Role::IGNORED:
      // No role description.
      break;
    case ax::mojom::Role::IMAGE_MAP:
      message_id = IDS_AX_ROLE_GRAPHIC;
      break;
    case ax::mojom::Role::IMAGE:
      message_id = IDS_AX_ROLE_GRAPHIC;
      break;
    case ax::mojom::Role::INLINE_TEXT_BOX:
      // No role description.
      break;
    case ax::mojom::Role::INPUT_TIME:
      message_id = IDS_AX_ROLE_INPUT_TIME;
      break;
    case ax::mojom::Role::LABEL_TEXT:
      // No role description.
      break;
    case ax::mojom::Role::LEGEND:
      // No role description.
      break;
    case ax::mojom::Role::LINE_BREAK:
      // No role description.
      break;
    case ax::mojom::Role::LINK:
      message_id = IDS_AX_ROLE_LINK;
      break;
    case ax::mojom::Role::LIST_BOX_OPTION:
      // No role description.
      break;
    case ax::mojom::Role::LIST_BOX:
      message_id = IDS_AX_ROLE_LIST_BOX;
      break;
    case ax::mojom::Role::LIST_ITEM:
      // No role description.
      break;
    case ax::mojom::Role::LIST_MARKER:
      // No role description.
      break;
    case ax::mojom::Role::LIST:
      // No role description.
      break;
    case ax::mojom::Role::LOCATION_BAR:
      // No role description.
      break;
    case ax::mojom::Role::LOG:
      message_id = IDS_AX_ROLE_LOG;
      break;
    case ax::mojom::Role::MAIN:
      message_id = IDS_AX_ROLE_MAIN_CONTENT;
      break;
    case ax::mojom::Role::MARK:
      message_id = IDS_AX_ROLE_MARK;
      break;
    case ax::mojom::Role::MARQUEE:
      message_id = IDS_AX_ROLE_MARQUEE;
      break;
    case ax::mojom::Role::MATH:
      message_id = IDS_AX_ROLE_MATH;
      break;
    case ax::mojom::Role::MENU:
      message_id = IDS_AX_ROLE_MENU;
      break;
    case ax::mojom::Role::MENU_BAR:
      message_id = IDS_AX_ROLE_MENU_BAR;
      break;
    case ax::mojom::Role::MENU_BUTTON:
      message_id = IDS_AX_ROLE_MENU_BUTTON;
      break;
    case ax::mojom::Role::MENU_ITEM:
      message_id = IDS_AX_ROLE_MENU_ITEM;
      break;
    case ax::mojom::Role::MENU_ITEM_CHECK_BOX:
      message_id = IDS_AX_ROLE_CHECK_BOX;
      break;
    case ax::mojom::Role::MENU_ITEM_RADIO:
      message_id = IDS_AX_ROLE_RADIO;
      break;
    case ax::mojom::Role::MENU_LIST_OPTION:
      // No role description.
      break;
    case ax::mojom::Role::MENU_LIST_POPUP:
      // No role description.
      break;
    case ax::mojom::Role::METER:
      message_id = IDS_AX_ROLE_METER;
      break;
    case ax::mojom::Role::NAVIGATION:
      message_id = IDS_AX_ROLE_NAVIGATIONAL_LINK;
      break;
    case ax::mojom::Role::NOTE:
      message_id = IDS_AX_ROLE_NOTE;
      break;
    case ax::mojom::Role::PANE:
      // No role description.
      break;
    case ax::mojom::Role::PARAGRAPH:
      // No role description.
      break;
    case ax::mojom::Role::POP_UP_BUTTON:
      message_id = IDS_AX_ROLE_POP_UP_BUTTON;
      break;
    case ax::mojom::Role::PRE:
      // No role description.
      break;
    case ax::mojom::Role::PRESENTATIONAL:
      // No role description.
      break;
    case ax::mojom::Role::PROGRESS_INDICATOR:
      message_id = IDS_AX_ROLE_PROGRESS_INDICATOR;
      break;
    case ax::mojom::Role::RADIO_BUTTON:
      message_id = IDS_AX_ROLE_RADIO;
      break;
    case ax::mojom::Role::RADIO_GROUP:
      message_id = IDS_AX_ROLE_RADIO_GROUP;
      break;
    case ax::mojom::Role::REGION:
      message_id = IDS_AX_ROLE_REGION;
      break;
    case ax::mojom::Role::ROOT_WEB_AREA:
      // No role description.
      break;
    case ax::mojom::Role::ROW_HEADER:
      message_id = IDS_AX_ROLE_ROW_HEADER;
      break;
    case ax::mojom::Role::ROW:
      // No role description.
      break;
    case ax::mojom::Role::RUBY:
      // No role description.
      break;
    case ax::mojom::Role::SVG_ROOT:
      message_id = IDS_AX_ROLE_GRAPHIC;
      break;
    case ax::mojom::Role::SCROLL_BAR:
      message_id = IDS_AX_ROLE_SCROLL_BAR;
      break;
    case ax::mojom::Role::SEARCH:
      message_id = IDS_AX_ROLE_SEARCH;
      break;
    case ax::mojom::Role::SEARCH_BOX:
      message_id = IDS_AX_ROLE_SEARCH_BOX;
      break;
    case ax::mojom::Role::SLIDER:
      message_id = IDS_AX_ROLE_SLIDER;
      break;
    case ax::mojom::Role::SLIDER_THUMB:
      // No role description.
      break;
    case ax::mojom::Role::SPIN_BUTTON_PART:
      // No role description.
      break;
    case ax::mojom::Role::SPIN_BUTTON:
      message_id = IDS_AX_ROLE_SPIN_BUTTON;
      break;
    case ax::mojom::Role::SPLITTER:
      message_id = IDS_AX_ROLE_SPLITTER;
      break;
    case ax::mojom::Role::STATIC_TEXT:
      // No role description.
      break;
    case ax::mojom::Role::STATUS:
      message_id = IDS_AX_ROLE_STATUS;
      break;
    case ax::mojom::Role::SWITCH:
      message_id = IDS_AX_ROLE_SWITCH;
      break;
    case ax::mojom::Role::TAB_LIST:
      message_id = IDS_AX_ROLE_TAB_LIST;
      break;
    case ax::mojom::Role::TAB_PANEL:
      message_id = IDS_AX_ROLE_TAB_PANEL;
      break;
    case ax::mojom::Role::TAB:
      message_id = IDS_AX_ROLE_TAB;
      break;
    case ax::mojom::Role::TABLE_HEADER_CONTAINER:
      // No role description.
      break;
    case ax::mojom::Role::TABLE:
      message_id = IDS_AX_ROLE_TABLE;
      break;
    case ax::mojom::Role::TERM:
      message_id = IDS_AX_ROLE_DESCRIPTION_TERM;
      break;
    case ax::mojom::Role::TEXT_FIELD:
      // No role description.
      break;
    case ax::mojom::Role::TEXT_FIELD_WITH_COMBO_BOX:
      // No role description.
      break;
    case ax::mojom::Role::TIME:
      message_id = IDS_AX_ROLE_TIME;
      break;
    case ax::mojom::Role::TIMER:
      message_id = IDS_AX_ROLE_TIMER;
      break;
    case ax::mojom::Role::TITLE_BAR:
      // No role description.
      break;
    case ax::mojom::Role::TOGGLE_BUTTON:
      message_id = IDS_AX_ROLE_TOGGLE_BUTTON;
      break;
    case ax::mojom::Role::TOOLBAR:
      message_id = IDS_AX_ROLE_TOOLBAR;
      break;
    case ax::mojom::Role::TREE_GRID:
      message_id = IDS_AX_ROLE_TREE_GRID;
      break;
    case ax::mojom::Role::TREE_ITEM:
      message_id = IDS_AX_ROLE_TREE_ITEM;
      break;
    case ax::mojom::Role::TREE:
      message_id = IDS_AX_ROLE_TREE;
      break;
    case ax::mojom::Role::UNKNOWN:
      // No role description.
      break;
    case ax::mojom::Role::TOOLTIP:
      message_id = IDS_AX_ROLE_TOOLTIP;
      break;
    case ax::mojom::Role::VIDEO:
      message_id = IDS_AX_MEDIA_VIDEO_ELEMENT;
      break;
    case ax::mojom::Role::WEB_AREA:
      // No role description.
      break;
    case ax::mojom::Role::WEB_VIEW:
      // No role description.
      break;
    case ax::mojom::Role::WINDOW:
      // No role description.
      break;
    case ax::mojom::Role::NONE:
      // No role description.
      break;
  }

  if (message_id != -1)
    return content_client->GetLocalizedString(message_id);

  return base::string16();
}

int BrowserAccessibilityAndroid::GetItemIndex() const {
  int index = 0;
  if (IsRangeType()) {
    // Return a percentage here for live feedback in an AccessibilityEvent.
    // The exact value is returned in RangeCurrentValue.
    float min = GetFloatAttribute(ax::mojom::FloatAttribute::MIN_VALUE_FOR_RANGE);
    float max = GetFloatAttribute(ax::mojom::FloatAttribute::MAX_VALUE_FOR_RANGE);
    float value = GetFloatAttribute(ax::mojom::FloatAttribute::VALUE_FOR_RANGE);
    if (max > min && value >= min && value <= max)
      index = static_cast<int>(((value - min)) * 100 / (max - min));
  } else {
    switch (GetRole()) {
      case ax::mojom::Role::LIST_ITEM:
      case ax::mojom::Role::LIST_BOX_OPTION:
      case ax::mojom::Role::TREE_ITEM:
        index = GetIntAttribute(ax::mojom::IntAttribute::POS_IN_SET) - 1;
        break;
      default:
        break;
    }
  }
  return index;
}

int BrowserAccessibilityAndroid::GetItemCount() const {
  int count = 0;
  if (IsRangeType()) {
    // An AccessibilityEvent can only return integer information about a
    // seek control, so we return a percentage. The real range is returned
    // in RangeMin and RangeMax.
    count = 100;
  } else {
    switch (GetRole()) {
      case ax::mojom::Role::LIST:
      case ax::mojom::Role::LIST_BOX:
      case ax::mojom::Role::DESCRIPTION_LIST:
        count = PlatformChildCount();
        break;
      default:
        break;
    }
  }
  return count;
}

bool BrowserAccessibilityAndroid::CanScrollForward() const {
  if (IsSlider()) {
    float value = GetFloatAttribute(ax::mojom::FloatAttribute::VALUE_FOR_RANGE);
    float max = GetFloatAttribute(ax::mojom::FloatAttribute::MAX_VALUE_FOR_RANGE);
    return value < max;
  } else {
    return GetScrollX() < GetMaxScrollX() ||
           GetScrollY() < GetMaxScrollY();
  }
}

bool BrowserAccessibilityAndroid::CanScrollBackward() const {
  if (IsSlider()) {
    float value = GetFloatAttribute(ax::mojom::FloatAttribute::VALUE_FOR_RANGE);
    float min = GetFloatAttribute(ax::mojom::FloatAttribute::MIN_VALUE_FOR_RANGE);
    return value > min;
  } else {
    return GetScrollX() > GetMinScrollX() ||
           GetScrollY() > GetMinScrollY();
  }
}

bool BrowserAccessibilityAndroid::CanScrollUp() const {
  return GetScrollY() > GetMinScrollY();
}

bool BrowserAccessibilityAndroid::CanScrollDown() const {
  return GetScrollY() < GetMaxScrollY();
}

bool BrowserAccessibilityAndroid::CanScrollLeft() const {
  return GetScrollX() > GetMinScrollX();
}

bool BrowserAccessibilityAndroid::CanScrollRight() const {
  return GetScrollX() < GetMaxScrollX();
}

int BrowserAccessibilityAndroid::GetScrollX() const {
  int value = 0;
  GetIntAttribute(ax::mojom::IntAttribute::SCROLL_X, &value);
  return value;
}

int BrowserAccessibilityAndroid::GetScrollY() const {
  int value = 0;
  GetIntAttribute(ax::mojom::IntAttribute::SCROLL_Y, &value);
  return value;
}

int BrowserAccessibilityAndroid::GetMinScrollX() const {
  return GetIntAttribute(ax::mojom::IntAttribute::SCROLL_X_MIN);
}

int BrowserAccessibilityAndroid::GetMinScrollY() const {
  return GetIntAttribute(ax::mojom::IntAttribute::SCROLL_Y_MIN);
}

int BrowserAccessibilityAndroid::GetMaxScrollX() const {
  return GetIntAttribute(ax::mojom::IntAttribute::SCROLL_X_MAX);
}

int BrowserAccessibilityAndroid::GetMaxScrollY() const {
  return GetIntAttribute(ax::mojom::IntAttribute::SCROLL_Y_MAX);
}

bool BrowserAccessibilityAndroid::Scroll(int direction) const {
  int x_initial = GetIntAttribute(ax::mojom::IntAttribute::SCROLL_X);
  int x_min = GetIntAttribute(ax::mojom::IntAttribute::SCROLL_X_MIN);
  int x_max = GetIntAttribute(ax::mojom::IntAttribute::SCROLL_X_MAX);
  int y_initial = GetIntAttribute(ax::mojom::IntAttribute::SCROLL_Y);
  int y_min = GetIntAttribute(ax::mojom::IntAttribute::SCROLL_Y_MIN);
  int y_max = GetIntAttribute(ax::mojom::IntAttribute::SCROLL_Y_MAX);

  // Figure out the bounding box of the visible portion of this scrollable
  // view so we know how much to scroll by.
  gfx::Rect bounds;
  if (GetRole() == ax::mojom::Role::ROOT_WEB_AREA && !PlatformGetParent()) {
    // If this is the root web area, use the bounds of the view to determine
    // how big one page is.
    if (!manager()->delegate())
      return false;
    bounds = manager()->delegate()->AccessibilityGetViewBounds();
  } else if (GetRole() == ax::mojom::Role::ROOT_WEB_AREA && PlatformGetParent()) {
    // If this is a web area inside of an iframe, try to use the bounds of
    // the containing element.
    BrowserAccessibility* parent = PlatformGetParent();
    while (parent && (parent->GetPageBoundsRect().width() == 0 ||
                      parent->GetPageBoundsRect().height() == 0)) {
      parent = parent->PlatformGetParent();
    }
    if (parent)
      bounds = parent->GetPageBoundsRect();
    else
      bounds = GetPageBoundsRect();
  } else {
    // Otherwise this is something like a scrollable div, just use the
    // bounds of this object itself.
    bounds = GetPageBoundsRect();
  }

  // Scroll by 80% of one page.
  int page_x = std::max(bounds.width() * 4 / 5, 1);
  int page_y = std::max(bounds.height() * 4 / 5, 1);

  if (direction == FORWARD)
    direction = y_max > y_min ? DOWN : RIGHT;
  if (direction == BACKWARD)
    direction = y_max > y_min ? UP : LEFT;

  int x = x_initial;
  int y = y_initial;
  switch (direction) {
    case UP:
      if (y_initial == y_min)
        return false;
      y = std::min(std::max(y_initial - page_y, y_min), y_max);
      break;
    case DOWN:
      if (y_initial == y_max)
        return false;
      y = std::min(std::max(y_initial + page_y, y_min), y_max);
      break;
    case LEFT:
      if (x_initial == x_min)
        return false;
      x = std::min(std::max(x_initial - page_x, x_min), x_max);
      break;
    case RIGHT:
      if (x_initial == x_max)
        return false;
      x = std::min(std::max(x_initial + page_x, x_min), x_max);
      break;
    default:
      NOTREACHED();
  }

  manager()->SetScrollOffset(*this, gfx::Point(x, y));
  return true;
}

// Given arbitrary old_value_ and new_value_, we must come up with reasonable
// edit metrics. Although edits like "apple" > "apples" are typical, anything
// is possible, such as "apple" > "applesauce", "apple" > "boot", or "" >
// "supercalifragilisticexpialidocious". So we consider old_value_ to be of the
// form AXB and new_value_ to be of the form AYB, where X and Y are the pieces
// that don't match. We take the X to be the "removed" characters and Y to be
// the "added" characters.

int BrowserAccessibilityAndroid::GetTextChangeFromIndex() const {
  // This is len(A)
  return CommonPrefixLength(old_value_, new_value_);
}

int BrowserAccessibilityAndroid::GetTextChangeAddedCount() const {
  // This is len(AYB) - (len(A) + len(B)), or len(Y), the added characters.
  return new_value_.length() - CommonEndLengths(old_value_, new_value_);
}

int BrowserAccessibilityAndroid::GetTextChangeRemovedCount() const {
  // This is len(AXB) - (len(A) + len(B)), or len(X), the removed characters.
  return old_value_.length() - CommonEndLengths(old_value_, new_value_);
}

// static
size_t BrowserAccessibilityAndroid::CommonPrefixLength(
    const base::string16 a,
    const base::string16 b) {
  size_t a_len = a.length();
  size_t b_len = b.length();
  size_t i = 0;
  while (i < a_len &&
         i < b_len &&
         a[i] == b[i]) {
    i++;
  }
  return i;
}

// static
size_t BrowserAccessibilityAndroid::CommonSuffixLength(
    const base::string16 a,
    const base::string16 b) {
  size_t a_len = a.length();
  size_t b_len = b.length();
  size_t i = 0;
  while (i < a_len &&
         i < b_len &&
         a[a_len - i - 1] == b[b_len - i - 1]) {
    i++;
  }
  return i;
}

// TODO(nektar): Merge this function with
// |BrowserAccessibilityCocoa::computeTextEdit|.
//
// static
size_t BrowserAccessibilityAndroid::CommonEndLengths(
    const base::string16 a,
    const base::string16 b) {
  size_t prefix_len = CommonPrefixLength(a, b);
  // Remove the matching prefix before finding the suffix. Otherwise, if
  // old_value_ is "a" and new_value_ is "aa", "a" will be double-counted as
  // both a prefix and a suffix of "aa".
  base::string16 a_body = a.substr(prefix_len, std::string::npos);
  base::string16 b_body = b.substr(prefix_len, std::string::npos);
  size_t suffix_len = CommonSuffixLength(a_body, b_body);
  return prefix_len + suffix_len;
}

base::string16 BrowserAccessibilityAndroid::GetTextChangeBeforeText() const {
  return old_value_;
}

int BrowserAccessibilityAndroid::GetSelectionStart() const {
  int sel_start = 0;
  GetIntAttribute(ax::mojom::IntAttribute::TEXT_SEL_START, &sel_start);
  return sel_start;
}

int BrowserAccessibilityAndroid::GetSelectionEnd() const {
  int sel_end = 0;
  GetIntAttribute(ax::mojom::IntAttribute::TEXT_SEL_END, &sel_end);
  return sel_end;
}

int BrowserAccessibilityAndroid::GetEditableTextLength() const {
  base::string16 value = GetValue();
  return value.length();
}

int BrowserAccessibilityAndroid::AndroidInputType() const {
  std::string html_tag = GetStringAttribute(
      ax::mojom::StringAttribute::HTML_TAG);
  if (html_tag != "input")
    return ANDROID_TEXT_INPUTTYPE_TYPE_NULL;

  std::string type;
  if (!GetHtmlAttribute("type", &type))
    return ANDROID_TEXT_INPUTTYPE_TYPE_TEXT;

  if (type.empty() || type == "text" || type == "search")
    return ANDROID_TEXT_INPUTTYPE_TYPE_TEXT;
  else if (type == "date")
    return ANDROID_TEXT_INPUTTYPE_TYPE_DATETIME_DATE;
  else if (type == "datetime" || type == "datetime-local")
    return ANDROID_TEXT_INPUTTYPE_TYPE_DATETIME;
  else if (type == "email")
    return ANDROID_TEXT_INPUTTYPE_TYPE_TEXT_WEB_EMAIL;
  else if (type == "month")
    return ANDROID_TEXT_INPUTTYPE_TYPE_DATETIME_DATE;
  else if (type == "number")
    return ANDROID_TEXT_INPUTTYPE_TYPE_NUMBER;
  else if (type == "password")
    return ANDROID_TEXT_INPUTTYPE_TYPE_TEXT_WEB_PASSWORD;
  else if (type == "tel")
    return ANDROID_TEXT_INPUTTYPE_TYPE_PHONE;
  else if (type == "time")
    return ANDROID_TEXT_INPUTTYPE_TYPE_DATETIME_TIME;
  else if (type == "url")
    return ANDROID_TEXT_INPUTTYPE_TYPE_TEXT_URI;
  else if (type == "week")
    return ANDROID_TEXT_INPUTTYPE_TYPE_DATETIME;

  return ANDROID_TEXT_INPUTTYPE_TYPE_NULL;
}

int BrowserAccessibilityAndroid::AndroidLiveRegionType() const {
  std::string live = GetStringAttribute(
      ax::mojom::StringAttribute::LIVE_STATUS);
  if (live == "polite")
    return ANDROID_VIEW_VIEW_ACCESSIBILITY_LIVE_REGION_POLITE;
  else if (live == "assertive")
    return ANDROID_VIEW_VIEW_ACCESSIBILITY_LIVE_REGION_ASSERTIVE;
  return ANDROID_VIEW_VIEW_ACCESSIBILITY_LIVE_REGION_NONE;
}

int BrowserAccessibilityAndroid::AndroidRangeType() const {
  return ANDROID_VIEW_ACCESSIBILITY_RANGE_TYPE_FLOAT;
}

int BrowserAccessibilityAndroid::RowCount() const {
  if (ui::IsTableLikeRole(GetRole())) {
    return CountChildrenWithRole(ax::mojom::Role::ROW);
  }

  if (GetRole() == ax::mojom::Role::LIST ||
      GetRole() == ax::mojom::Role::LIST_BOX ||
      GetRole() == ax::mojom::Role::DESCRIPTION_LIST ||
      GetRole() == ax::mojom::Role::TREE) {
    return PlatformChildCount();
  }

  return 0;
}

int BrowserAccessibilityAndroid::ColumnCount() const {
  if (ui::IsTableLikeRole(GetRole())) {
    return CountChildrenWithRole(ax::mojom::Role::COLUMN);
  }
  return 0;
}

int BrowserAccessibilityAndroid::RowIndex() const {
  if (GetRole() == ax::mojom::Role::LIST_ITEM ||
      GetRole() == ax::mojom::Role::LIST_BOX_OPTION ||
      GetRole() == ax::mojom::Role::TREE_ITEM) {
    return GetIndexInParent();
  }

  return GetIntAttribute(ax::mojom::IntAttribute::TABLE_CELL_ROW_INDEX);
}

int BrowserAccessibilityAndroid::RowSpan() const {
  return GetIntAttribute(ax::mojom::IntAttribute::TABLE_CELL_ROW_SPAN);
}

int BrowserAccessibilityAndroid::ColumnIndex() const {
  return GetIntAttribute(ax::mojom::IntAttribute::TABLE_CELL_COLUMN_INDEX);
}

int BrowserAccessibilityAndroid::ColumnSpan() const {
  return GetIntAttribute(ax::mojom::IntAttribute::TABLE_CELL_COLUMN_SPAN);
}

float BrowserAccessibilityAndroid::RangeMin() const {
  return GetFloatAttribute(ax::mojom::FloatAttribute::MIN_VALUE_FOR_RANGE);
}

float BrowserAccessibilityAndroid::RangeMax() const {
  return GetFloatAttribute(ax::mojom::FloatAttribute::MAX_VALUE_FOR_RANGE);
}

float BrowserAccessibilityAndroid::RangeCurrentValue() const {
  return GetFloatAttribute(ax::mojom::FloatAttribute::VALUE_FOR_RANGE);
}

void BrowserAccessibilityAndroid::GetGranularityBoundaries(
    int granularity,
    std::vector<int32_t>* starts,
    std::vector<int32_t>* ends,
    int offset) {
  switch (granularity) {
    case ANDROID_ACCESSIBILITY_NODE_INFO_MOVEMENT_GRANULARITY_LINE:
      GetLineBoundaries(starts, ends, offset);
      break;
    case ANDROID_ACCESSIBILITY_NODE_INFO_MOVEMENT_GRANULARITY_WORD:
      GetWordBoundaries(starts, ends, offset);
      break;
    default:
      NOTREACHED();
  }
}

void BrowserAccessibilityAndroid::GetLineBoundaries(
    std::vector<int32_t>* line_starts,
    std::vector<int32_t>* line_ends,
    int offset) {
  // If this node has no children, treat it as all one line.
  if (GetText().size() > 0 && !InternalChildCount()) {
    line_starts->push_back(offset);
    line_ends->push_back(offset + GetText().size());
  }

  // If this is a static text node, get the line boundaries from the
  // inline text boxes if possible.
  if (GetRole() == ax::mojom::Role::STATIC_TEXT) {
    int last_y = 0;
    for (uint32_t i = 0; i < InternalChildCount(); i++) {
      BrowserAccessibilityAndroid* child =
          static_cast<BrowserAccessibilityAndroid*>(InternalGetChild(i));
      CHECK_EQ(ax::mojom::Role::INLINE_TEXT_BOX, child->GetRole());
      // TODO(dmazzoni): replace this with a proper API to determine
      // if two inline text boxes are on the same line. http://crbug.com/421771
      int y = child->GetPageBoundsRect().y();
      if (i == 0) {
        line_starts->push_back(offset);
      } else if (y != last_y) {
        line_ends->push_back(offset);
        line_starts->push_back(offset);
      }
      offset += child->GetText().size();
      last_y = y;
    }
    line_ends->push_back(offset);
    return;
  }

  // Otherwise, call GetLineBoundaries recursively on the children.
  for (uint32_t i = 0; i < InternalChildCount(); i++) {
    BrowserAccessibilityAndroid* child =
        static_cast<BrowserAccessibilityAndroid*>(InternalGetChild(i));
    child->GetLineBoundaries(line_starts, line_ends, offset);
    offset += child->GetText().size();
  }
}

void BrowserAccessibilityAndroid::GetWordBoundaries(
    std::vector<int32_t>* word_starts,
    std::vector<int32_t>* word_ends,
    int offset) {
  if (GetRole() == ax::mojom::Role::INLINE_TEXT_BOX) {
    const std::vector<int32_t>& starts =
        GetIntListAttribute(ax::mojom::IntListAttribute::WORD_STARTS);
    const std::vector<int32_t>& ends =
        GetIntListAttribute(ax::mojom::IntListAttribute::WORD_ENDS);
    for (size_t i = 0; i < starts.size(); ++i) {
      word_starts->push_back(offset + starts[i]);
      word_ends->push_back(offset + ends[i]);
    }
    return;
  }

  base::string16 concatenated_text;
  for (uint32_t i = 0; i < InternalChildCount(); i++) {
    BrowserAccessibilityAndroid* child =
        static_cast<BrowserAccessibilityAndroid*>(InternalGetChild(i));
    base::string16 child_text = child->GetText();
    concatenated_text += child->GetText();
  }

  base::string16 text = GetText();
  if (text.empty() || concatenated_text == text) {
    // Great - this node is just the concatenation of its children, so
    // we can get the word boundaries recursively.
    for (uint32_t i = 0; i < InternalChildCount(); i++) {
      BrowserAccessibilityAndroid* child =
          static_cast<BrowserAccessibilityAndroid*>(InternalGetChild(i));
      child->GetWordBoundaries(word_starts, word_ends, offset);
      offset += child->GetText().size();
    }
  } else {
    // This node has its own accessible text that doesn't match its
    // visible text - like alt text for an image or something with an
    // aria-label, so split the text into words locally.
    base::i18n::BreakIterator iter(text, base::i18n::BreakIterator::BREAK_WORD);
    if (!iter.Init())
      return;
    while (iter.Advance()) {
      if (iter.IsWord()) {
        word_starts->push_back(iter.prev());
        word_ends->push_back(iter.pos());
      }
    }
  }
}

bool BrowserAccessibilityAndroid::HasFocusableNonOptionChild() const {
  // This is called from PlatformIsLeaf, so don't call PlatformChildCount
  // from within this!
  for (uint32_t i = 0; i < InternalChildCount(); i++) {
    BrowserAccessibility* child = InternalGetChild(i);
    if (child->HasState(ax::mojom::State::FOCUSABLE) &&
        child->GetRole() != ax::mojom::Role::MENU_LIST_OPTION)
      return true;
    if (static_cast<BrowserAccessibilityAndroid*>(child)
            ->HasFocusableNonOptionChild())
      return true;
  }
  return false;
}

bool BrowserAccessibilityAndroid::HasNonEmptyValue() const {
  return IsEditableText() && !GetValue().empty();
}

bool BrowserAccessibilityAndroid::HasOnlyTextChildren() const {
  // This is called from PlatformIsLeaf, so don't call PlatformChildCount
  // from within this!
  for (uint32_t i = 0; i < InternalChildCount(); i++) {
    BrowserAccessibility* child = InternalGetChild(i);
    if (!child->IsTextOnlyObject())
      return false;
  }
  return true;
}

bool BrowserAccessibilityAndroid::HasOnlyTextAndImageChildren() const {
  // This is called from PlatformIsLeaf, so don't call PlatformChildCount
  // from within this!
  for (uint32_t i = 0; i < InternalChildCount(); i++) {
    BrowserAccessibility* child = InternalGetChild(i);
    if (child->GetRole() != ax::mojom::Role::STATIC_TEXT &&
        child->GetRole() != ax::mojom::Role::IMAGE) {
      return false;
    }
  }
  return true;
}

bool BrowserAccessibilityAndroid::IsIframe() const {
  return (GetRole() == ax::mojom::Role::IFRAME ||
          GetRole() == ax::mojom::Role::IFRAME_PRESENTATIONAL);
}

bool BrowserAccessibilityAndroid::ShouldExposeValueAsName() const {
  base::string16 value = GetValue();
  if (value.empty())
    return false;

  if (HasState(ax::mojom::State::EDITABLE))
    return true;

  switch (GetRole()) {
    case ax::mojom::Role::POP_UP_BUTTON:
    case ax::mojom::Role::TEXT_FIELD:
    case ax::mojom::Role::TEXT_FIELD_WITH_COMBO_BOX:
      return true;
    default:
      break;
  }

  return false;
}

void BrowserAccessibilityAndroid::OnDataChanged() {
  BrowserAccessibility::OnDataChanged();

  if (IsEditableText()) {
    base::string16 value = GetValue();
    if (value != new_value_) {
      old_value_ = new_value_;
      new_value_ = value;
    }
  }
}

int BrowserAccessibilityAndroid::CountChildrenWithRole(ax::mojom::Role role) const {
  int count = 0;
  for (uint32_t i = 0; i < PlatformChildCount(); i++) {
    if (PlatformGetChild(i)->GetRole() == role)
      count++;
  }
  return count;
}

}  // namespace content
