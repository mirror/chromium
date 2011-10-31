// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/input_method/xkeyboard.h"

#include <algorithm>
#include <set>
#include <string>

#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "base/message_loop.h"
#include "chrome/browser/chromeos/input_method/input_method_util.h"
#include "content/test/test_browser_thread.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/x/x11_util.h"

#include <X11/Xlib.h>

#if defined(TOUCH_UI)
// Since TOUCH_UI build only supports a few keyboard layouts, we skip the tests
// for now.
#define TestCreateFullXkbLayoutNameKeepAlt \
  DISABLED_TestCreateFullXkbLayoutNameKeepAlt
#define TestCreateFullXkbLayoutNameKeepCapsLock \
  DISABLED_TestCreateFullXkbLayoutNameKeepCapsLock
#endif  // TOUCH_UI

namespace chromeos {
namespace input_method {

namespace {

class TestableXKeyboard : public XKeyboard {
 public:
  explicit TestableXKeyboard(const InputMethodUtil& util) : XKeyboard(util) {
  }

  // Change access rights.
  using XKeyboard::CreateFullXkbLayoutName;
  using XKeyboard::ContainsModifierKeyAsReplacement;
};

class XKeyboardTest : public testing::Test {
 public:
  XKeyboardTest()
      : controller_(IBusController::Create()),
        util_(controller_->GetSupportedInputMethods()),
        xkey_(util_),
        ui_thread_(BrowserThread::UI, &message_loop_) {
  }

  static void SetUpTestCase() {
  }

  scoped_ptr<IBusController> controller_;
  InputMethodUtil util_;
  TestableXKeyboard xkey_;

  MessageLoopForUI message_loop_;
  content::TestBrowserThread ui_thread_;
};

// Returns a ModifierMap object that contains the following mapping:
// - kSearchKey is mapped to |search|.
// - kLeftControl key is mapped to |control|.
// - kLeftAlt key is mapped to |alt|.
ModifierMap GetMap(ModifierKey search, ModifierKey control, ModifierKey alt) {
  ModifierMap modifier_key;
  // Use the Search key as |search|.
  modifier_key.push_back(ModifierKeyPair(kSearchKey, search));
  modifier_key.push_back(ModifierKeyPair(kLeftControlKey, control));
  modifier_key.push_back(ModifierKeyPair(kLeftAltKey, alt));
  return modifier_key;
}

// Checks |modifier_map| and returns true if the following conditions are met:
// - kSearchKey is mapped to |search|.
// - kLeftControl key is mapped to |control|.
// - kLeftAlt key is mapped to |alt|.
bool CheckMap(const ModifierMap& modifier_map,
              ModifierKey search, ModifierKey control, ModifierKey alt) {
  ModifierMap::const_iterator begin = modifier_map.begin();
  ModifierMap::const_iterator end = modifier_map.end();
  if ((std::count(begin, end, ModifierKeyPair(kSearchKey, search)) == 1) &&
      (std::count(begin, end,
                  ModifierKeyPair(kLeftControlKey, control)) == 1) &&
      (std::count(begin, end, ModifierKeyPair(kLeftAltKey, alt)) == 1)) {
    return true;
  }
  return false;
}

// Returns true if X display is available.
bool DisplayAvailable() {
  return ui::GetXDisplay() ? true : false;
}

}  // namespace

// Tests CreateFullXkbLayoutName() function.
TEST_F(XKeyboardTest, TestCreateFullXkbLayoutNameBasic) {
  // CreateFullXkbLayoutName should not accept an empty |layout_name|.
  EXPECT_STREQ("", xkey_.CreateFullXkbLayoutName(
      "", GetMap(kVoidKey, kVoidKey, kVoidKey)).c_str());

  // CreateFullXkbLayoutName should not accept an empty ModifierMap.
  EXPECT_STREQ("", xkey_.CreateFullXkbLayoutName(
      "us", ModifierMap()).c_str());

  // CreateFullXkbLayoutName should not accept an incomplete ModifierMap.
  ModifierMap tmp_map = GetMap(kVoidKey, kVoidKey, kVoidKey);
  tmp_map.pop_back();
  EXPECT_STREQ("", xkey_.CreateFullXkbLayoutName("us", tmp_map).c_str());

  // CreateFullXkbLayoutName should not accept redundant ModifierMaps.
  tmp_map = GetMap(kVoidKey, kVoidKey, kVoidKey);
  tmp_map.push_back(ModifierKeyPair(kSearchKey, kVoidKey));  // two search maps
  EXPECT_STREQ("", xkey_.CreateFullXkbLayoutName("us", tmp_map).c_str());
  tmp_map = GetMap(kVoidKey, kVoidKey, kVoidKey);
  tmp_map.push_back(ModifierKeyPair(kLeftControlKey, kVoidKey));  // two ctrls
  EXPECT_STREQ("", xkey_.CreateFullXkbLayoutName("us", tmp_map).c_str());
  tmp_map = GetMap(kVoidKey, kVoidKey, kVoidKey);
  tmp_map.push_back(ModifierKeyPair(kLeftAltKey, kVoidKey));  // two alts.
  EXPECT_STREQ("", xkey_.CreateFullXkbLayoutName("us", tmp_map).c_str());

  // CreateFullXkbLayoutName should not accept invalid ModifierMaps.
  tmp_map = GetMap(kVoidKey, kVoidKey, kVoidKey);
  tmp_map.push_back(ModifierKeyPair(kVoidKey, kSearchKey));  // can't remap void
  EXPECT_STREQ("", xkey_.CreateFullXkbLayoutName("us", tmp_map).c_str());
  tmp_map = GetMap(kVoidKey, kVoidKey, kVoidKey);
  tmp_map.push_back(ModifierKeyPair(kCapsLockKey, kSearchKey));  // ditto
  EXPECT_STREQ("", xkey_.CreateFullXkbLayoutName("us", tmp_map).c_str());

  // CreateFullXkbLayoutName can remap Search/Ctrl/Alt to CapsLock.
  EXPECT_STREQ("us+chromeos(capslock_disabled_disabled)",
               xkey_.CreateFullXkbLayoutName(
                   "us",
                   GetMap(kCapsLockKey, kVoidKey, kVoidKey)).c_str());
  EXPECT_STREQ("us+chromeos(disabled_capslock_disabled)",
               xkey_.CreateFullXkbLayoutName(
                   "us",
                   GetMap(kVoidKey, kCapsLockKey, kVoidKey)).c_str());
  EXPECT_STREQ("us+chromeos(disabled_disabled_capslock)",
               xkey_.CreateFullXkbLayoutName(
                   "us",
                   GetMap(kVoidKey, kVoidKey, kCapsLockKey)).c_str());

  // CreateFullXkbLayoutName should not accept non-alphanumeric characters
  // except "()-_".
  EXPECT_STREQ("", xkey_.CreateFullXkbLayoutName(
      "us!", GetMap(kVoidKey, kVoidKey, kVoidKey)).c_str());
  EXPECT_STREQ("", xkey_.CreateFullXkbLayoutName(
      "us; /bin/sh", GetMap(kVoidKey, kVoidKey, kVoidKey)).c_str());
  EXPECT_STREQ("ab-c_12+chromeos(disabled_disabled_disabled),us",
               xkey_.CreateFullXkbLayoutName(
                   "ab-c_12",
                   GetMap(kVoidKey, kVoidKey, kVoidKey)).c_str());

  // CreateFullXkbLayoutName should not accept upper-case ascii characters.
  EXPECT_STREQ("", xkey_.CreateFullXkbLayoutName(
      "US", GetMap(kVoidKey, kVoidKey, kVoidKey)).c_str());

  // CreateFullXkbLayoutName should accept lower-case ascii characters.
  for (int c = 'a'; c <= 'z'; ++c) {
    EXPECT_STRNE("", xkey_.CreateFullXkbLayoutName(
        std::string(3, c),
        GetMap(kVoidKey, kVoidKey, kVoidKey)).c_str());
  }

  // CreateFullXkbLayoutName should accept numbers.
  for (int c = '0'; c <= '9'; ++c) {
    EXPECT_STRNE("", xkey_.CreateFullXkbLayoutName(
        std::string(3, c),
        GetMap(kVoidKey, kVoidKey, kVoidKey)).c_str());
  }

  // CreateFullXkbLayoutName should accept a layout with a variant name.
  EXPECT_STREQ("us(dvorak)+chromeos(disabled_disabled_disabled)",
               xkey_.CreateFullXkbLayoutName(
                   "us(dvorak)",
                   GetMap(kVoidKey, kVoidKey, kVoidKey)).c_str());
  EXPECT_STREQ("jp+chromeos(disabled_disabled_disabled),us",
               xkey_.CreateFullXkbLayoutName(
                   "jp",  // does not use AltGr, therefore no _keepralt.
                   GetMap(kVoidKey, kVoidKey, kVoidKey)).c_str());

  // When the layout name is not "us", the second layout should be added.
  EXPECT_EQ(std::string::npos, xkey_.CreateFullXkbLayoutName(
      "us", GetMap(kVoidKey, kVoidKey, kVoidKey)).find(",us"));
  EXPECT_EQ(std::string::npos, xkey_.CreateFullXkbLayoutName(
      "us(dvorak)", GetMap(kVoidKey, kVoidKey, kVoidKey)).find(",us"));
  EXPECT_NE(std::string::npos, xkey_.CreateFullXkbLayoutName(
      "gb(extd)", GetMap(kVoidKey, kVoidKey, kVoidKey)).find(",us"));
  EXPECT_NE(std::string::npos, xkey_.CreateFullXkbLayoutName(
      "jp", GetMap(kVoidKey, kVoidKey, kVoidKey)).find(",us"));
}

TEST_F(XKeyboardTest, TestCreateFullXkbLayoutNameKeepCapsLock) {
  EXPECT_STREQ("us(colemak)+chromeos(search_disabled_disabled)",
               xkey_.CreateFullXkbLayoutName(
                   "us(colemak)",
                   // The 1st kVoidKey should be ignored.
                   GetMap(kVoidKey, kVoidKey, kVoidKey)).c_str());
  EXPECT_STREQ("de(neo)+"
               "chromeos(search_leftcontrol_leftcontrol_keepralt),us",
               xkey_.CreateFullXkbLayoutName(
                   // The 1st kLeftControlKey should be ignored.
                   "de(neo)", GetMap(kLeftControlKey,
                                     kLeftControlKey,
                                     kLeftControlKey)).c_str());
  EXPECT_STREQ("gb(extd)+chromeos(disabled_disabled_disabled_keepralt),us",
                xkey_.CreateFullXkbLayoutName(
                    "gb(extd)",
                    GetMap(kVoidKey, kVoidKey, kVoidKey)).c_str());
}

TEST_F(XKeyboardTest, TestCreateFullXkbLayoutNameKeepAlt) {
  EXPECT_STREQ("us(intl)+chromeos(disabled_disabled_disabled_keepralt)",
               xkey_.CreateFullXkbLayoutName(
                   "us(intl)", GetMap(kVoidKey, kVoidKey, kVoidKey)).c_str());
  EXPECT_STREQ("kr(kr104)+"
               "chromeos(leftcontrol_leftcontrol_leftcontrol_keepralt),us",
               xkey_.CreateFullXkbLayoutName(
                   "kr(kr104)", GetMap(kLeftControlKey,
                                       kLeftControlKey,
                                       kLeftControlKey)).c_str());
}

// Tests if CreateFullXkbLayoutName and ExtractLayoutNameFromFullXkbLayoutName
// functions could handle all combinations of modifier remapping.
TEST_F(XKeyboardTest, TestCreateFullXkbLayoutNameModifierKeys) {
  std::set<std::string> layouts;
  for (int i = 0; i < static_cast<int>(kNumModifierKeys); ++i) {
    for (int j = 0; j < static_cast<int>(kNumModifierKeys); ++j) {
      for (int k = 0; k < static_cast<int>(kNumModifierKeys); ++k) {
        const std::string layout = xkey_.CreateFullXkbLayoutName(
            "us", GetMap(ModifierKey(i), ModifierKey(j), ModifierKey(k)));
        // CreateFullXkbLayoutName should succeed (i.e. should not return "".)
        EXPECT_STREQ("us+", layout.substr(0, 3).c_str())
            << "layout: " << layout;
        // All 4*3*3 layouts should be different.
        EXPECT_TRUE(layouts.insert(layout).second) << "layout: " << layout;
      }
    }
  }
}

TEST_F(XKeyboardTest, TestSetCapsLockIsEnabled) {
  if (!DisplayAvailable()) {
    return;
  }
  const bool initial_lock_state = XKeyboard::CapsLockIsEnabled();
  XKeyboard::SetCapsLockEnabled(true);
  EXPECT_TRUE(XKeyboard::CapsLockIsEnabled());
  XKeyboard::SetCapsLockEnabled(false);
  EXPECT_FALSE(XKeyboard::CapsLockIsEnabled());
  XKeyboard::SetCapsLockEnabled(true);
  EXPECT_TRUE(XKeyboard::CapsLockIsEnabled());
  XKeyboard::SetCapsLockEnabled(false);
  EXPECT_FALSE(XKeyboard::CapsLockIsEnabled());
  XKeyboard::SetCapsLockEnabled(initial_lock_state);
}

TEST_F(XKeyboardTest, TestContainsModifierKeyAsReplacement) {
  EXPECT_FALSE(TestableXKeyboard::ContainsModifierKeyAsReplacement(
      GetMap(kVoidKey, kVoidKey, kVoidKey), kCapsLockKey));
  EXPECT_TRUE(TestableXKeyboard::ContainsModifierKeyAsReplacement(
      GetMap(kCapsLockKey, kVoidKey, kVoidKey), kCapsLockKey));
  EXPECT_TRUE(TestableXKeyboard::ContainsModifierKeyAsReplacement(
      GetMap(kVoidKey, kCapsLockKey, kVoidKey), kCapsLockKey));
  EXPECT_TRUE(TestableXKeyboard::ContainsModifierKeyAsReplacement(
      GetMap(kVoidKey, kVoidKey, kCapsLockKey), kCapsLockKey));
  EXPECT_TRUE(TestableXKeyboard::ContainsModifierKeyAsReplacement(
      GetMap(kCapsLockKey, kCapsLockKey, kVoidKey), kCapsLockKey));
  EXPECT_TRUE(TestableXKeyboard::ContainsModifierKeyAsReplacement(
      GetMap(kCapsLockKey, kCapsLockKey, kCapsLockKey), kCapsLockKey));
  EXPECT_TRUE(TestableXKeyboard::ContainsModifierKeyAsReplacement(
      GetMap(kSearchKey, kVoidKey, kVoidKey), kSearchKey));
}

}  // namespace input_method
}  // namespace chromeos
