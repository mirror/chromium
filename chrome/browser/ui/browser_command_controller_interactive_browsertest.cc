// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_util.h"
#include "build/build_config.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_list_observer.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/interactive_test_utils.h"
#include "content/public/browser/notification_service.h"
#include "content/public/common/browser_side_navigation_policy.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "ui/events/keycodes/dom/keycode_converter.h"
#include "ui/events/keycodes/keyboard_code_conversion.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "url/gurl.h"
#include "url/url_constants.h"

#if defined(OS_MACOSX)
#include "base/mac/mac_util.h"
#endif

namespace {
// The html file to receive key events, prevent defaults and export all the
// events with "getKeyEventReport()" function. It has two magic keys: pressing
// "S" to enter fullscreen mode; pressing "X" to indicate the end of all the
// keys (see FinishTestAndVerifyResult() function).
constexpr char kFullscreenKeyboardLockHTML[] = "/fullscreen_keyboardlock.html";

// The html file to set its body element to fullscreen mode when "S" is
// received.
constexpr char kFullscreenTriggerHTML[] = "/fullscreen_trigger.html";

// On MacOSX command key is used for most of the shortcuts, so replace it with
// control to reduce the complexity of comparison of the results.
void NormalizeMetaKeyForMacOS(std::string* output) {
#if defined(OS_MACOSX)
  base::ReplaceSubstringsAfterOffset(output, 0, "MetaLeft", "ControlLeft");
#endif
}

// Observes the removal of a Browser instance.
class BrowserRemovedObserver : public chrome::BrowserListObserver {
 public:
  // Tracks the removal of |browser|.
  explicit BrowserRemovedObserver(Browser* browser);
  // Waits until |browser_| is removed.
  ~BrowserRemovedObserver() override;

 private:
  // BrowserListObserver implementation.
  void OnBrowserRemoved(Browser* browser) override;

  Browser* const browser_;
  bool removed_ = false;
};

BrowserRemovedObserver::BrowserRemovedObserver(Browser* browser)
    : browser_(browser) {
  BrowserList::GetInstance()->AddObserver(this);
}

BrowserRemovedObserver::~BrowserRemovedObserver() {
  while (!removed_)
    content::RunAllPendingInMessageLoop();
  BrowserList::GetInstance()->RemoveObserver(this);
}

void BrowserRemovedObserver::OnBrowserRemoved(Browser* browser) {
  if (browser == browser_)
    removed_ = true;
}

// Observes the count of tabs in a Browser instance. This class is an assertion
// if IsBrowserSideNavigationEnabled() is false: the creation and destruction of
// tabs should be synchronized if browser side navigation is not enabled.
class TabCountObserver : public TabStripModelObserver {
 public:
  // Tracks the count of tabs in |browser|.
  TabCountObserver(Browser* browser, int expected);
  // Waits until |browser_| has exact |expected_| tabs.
  ~TabCountObserver() override;

 private:
  // TabStripModelObserver implementations.
  void TabInsertedAt(TabStripModel* tab_strip_model,
                     content::WebContents* contents,
                     int index,
                     bool foreground) override;

  void TabClosingAt(TabStripModel* tab_strip_model,
                    content::WebContents* contents,
                    int index) override;

  const bool enable_observer_;
  Browser* const browser_;
  const int expected_;
  bool seen_ = false;
};

TabCountObserver::TabCountObserver(Browser* browser, int expected)
    : enable_observer_(content::IsBrowserSideNavigationEnabled()),
      browser_(browser),
      expected_(expected) {
  if (enable_observer_) {
    seen_ = (browser_->tab_strip_model()->count() == expected_);
    browser_->tab_strip_model()->AddObserver(this);
  }
}

TabCountObserver::~TabCountObserver() {
  if (enable_observer_) {
    while (!seen_)
      content::RunAllPendingInMessageLoop();
    browser_->tab_strip_model()->RemoveObserver(this);
    return;
  }

  CHECK_EQ(expected_, browser_->tab_strip_model()->count());
}

void TabCountObserver::TabInsertedAt(TabStripModel* tab_strip_model,
                                     content::WebContents* contents,
                                     int index,
                                     bool foreground) {
  if (tab_strip_model->count() == expected_)
    seen_ = true;
}

void TabCountObserver::TabClosingAt(TabStripModel* tab_strip_model,
                                    content::WebContents* contents,
                                    int index) {
  if (tab_strip_model->count() == expected_)
    seen_ = true;
}

}  // namespace

class BrowserCommandControllerInteractiveTest : public InProcessBrowserTest {
 public:
  BrowserCommandControllerInteractiveTest() = default;
  ~BrowserCommandControllerInteractiveTest() override = default;

 protected:
  // Starts the |kFullscreenKeyboardLockHTML| in a new tab and waits for it to
  // be loaded.
  void StartFullscreenLockPage();

  // Starts the |kFullscreenTriggerHTML| page in current tab and waits for it to
  // be loaded.
  void StartFullscreenTriggerPage();

  // Sends a control or command + |key| shortcut to the focused window. Shift
  // modifier will be added if |shift| is true.
  void SendShortcut(ui::KeyboardCode key, bool shift = false);

  // Sends a control or command + shift + |key| shortcut to the focused window.
  void SendShiftShortcut(ui::KeyboardCode key);

  // Sends a control + shift + tab shortcut to the focused window.
  void SendPreviousTabShortcut();

  // Sends a control + tab shortcut to the focused window.
  void SendNextTabShortcut();

  // Sends a fullscreen shortcut to the focused window and wait for the
  // operation to take effect.
  void SendFullscreenShortcutAndWait();

  // Sends a KeyS to the focused window to trigger JavaScript fullscreen and
  // wait for the operation to take effect.
  void SendJsFullscreenShortcutAndWait();

  // Sends an ESC to the focused window.
  void SendEscape();

  // Sends an ESC to the focused window to exit JavaScript fullscreen and wait
  // for the operation to take effect.
  void SendEscapeAndWaitForExitingFullscreen();

  // Sends a set of preventable shortcuts to the web page and expects them to be
  // prevented.
  void SendShortcutsAndExpectPrevented();

  // Sends a set of preventable shortcuts to the web page and expects them to
  // not be prevented. If |js_fullscreen| is true, the test will use
  // SendJsFullscreenShortcutAndWait() to trigger the fullscreen mode. Otherwise
  // SendFullscreenShortcutAndWait() will be used.
  void SendShortcutsAndExpectNotPrevented(bool js_fullscreen);

  // Sends a magic KeyX to the focused window to stop the test case, receives
  // the result and verifies if it is equal to |expected_result_|.
  void FinishTestAndVerifyResult();

  // Returns whether the active tab is in html fullscreen mode.
  bool IsActiveTabFullscreen() const {
    auto* contents = active_web_contents();
    return contents->GetDelegate()->IsFullscreenForTabOrPending(contents);
  }

  // Returns whether the last_active_browser() is in browser fullscreen mode.
  bool IsInBrowserFullscreen() const {
    return last_active_browser()
               ->exclusive_access_manager()
               ->fullscreen_controller()
               ->IsFullscreenForBrowser();
  }

  content::WebContents* active_web_contents() const {
    return last_active_browser()->tab_strip_model()->GetActiveWebContents();
  }

  // Gets the current active tab index.
  int active_tab_index() const {
    return last_active_browser()->tab_strip_model()->active_index();
  }

  // Gets the count of tabs in current browser.
  int tab_count() const {
    return last_active_browser()->tab_strip_model()->count();
  }

  // Gets the count of browser instances.
  size_t browser_count() const {
    return BrowserList::GetInstance()->size();
  }

  // Get the last active Browser instance.
  Browser* last_active_browser() const {
    return BrowserList::GetInstance()->GetLastActive();
  };

  // Ensure last_active_browser() is focused.
  void FocusOnLastActiveBrowser() {
    ASSERT_TRUE(ui_test_utils::BringBrowserWindowToFront(
        last_active_browser()));
  }

 private:
  // Starts the test page |file| in current tab and waits for it to be loaded.
  void StartTestPage(const char* file);

  void SetUpOnMainThread() override;

  // The expected output from the web page. This string is generated by
  // appending key presses from Send* functions above.
  std::string expected_result_;

  DISALLOW_COPY_AND_ASSIGN(BrowserCommandControllerInteractiveTest);
};

void BrowserCommandControllerInteractiveTest::StartFullscreenLockPage() {
  // Ensures the initial states.
  ASSERT_EQ(1, tab_count());
  ASSERT_EQ(0, active_tab_index());
  ASSERT_EQ(1U, browser_count());
  // Add a second tab for counting and focus purposes.
  AddTabAtIndex(1, GURL(url::kAboutBlankURL), ui::PAGE_TRANSITION_LINK);
  ASSERT_EQ(2, tab_count());
  ASSERT_EQ(1U, browser_count());

  ASSERT_NO_FATAL_FAILURE(StartTestPage(kFullscreenKeyboardLockHTML));
}

void BrowserCommandControllerInteractiveTest::StartFullscreenTriggerPage() {
  ASSERT_NO_FATAL_FAILURE(StartTestPage(kFullscreenTriggerHTML));
}

void BrowserCommandControllerInteractiveTest::SendShortcut(
    ui::KeyboardCode key,
    bool shift /* = false */) {
#if defined(OS_MACOSX)
  const bool control_modifier = false;
  const bool command_modifier = true;
#else
  const bool control_modifier = true;
  const bool command_modifier = false;
#endif
  ASSERT_TRUE(ui_test_utils::SendKeyPressSync(
      last_active_browser(), key, control_modifier, shift, false,
      command_modifier));

  expected_result_ += ui::KeycodeConverter::DomCodeToCodeString(
      ui::UsLayoutKeyboardCodeToDomCode(key));
  expected_result_ += " ctrl:";
  expected_result_ += control_modifier ? "true" : "false";
  expected_result_ += " shift:";
  expected_result_ += shift ? "true" : "false";
  expected_result_ += " alt:false";
  expected_result_ += " meta:";
  expected_result_ += command_modifier ? "true" : "false";
  expected_result_ += '\n';
}

void BrowserCommandControllerInteractiveTest::SendShiftShortcut(
    ui::KeyboardCode key) {
  ASSERT_NO_FATAL_FAILURE(SendShortcut(key, true));
}

void BrowserCommandControllerInteractiveTest::SendPreviousTabShortcut() {
  expected_result_ += "Tab ctrl:true shift:true alt:false meta:false\n";
  ASSERT_TRUE(ui_test_utils::SendKeyPressSync(
      last_active_browser(), ui::VKEY_TAB, true, true, false, false));
}

void BrowserCommandControllerInteractiveTest::SendNextTabShortcut() {
  expected_result_ += "Tab ctrl:true shift:false alt:false meta:false\n";
  ASSERT_TRUE(ui_test_utils::SendKeyPressSync(
      last_active_browser(), ui::VKEY_TAB, true, false, false, false));
}

void BrowserCommandControllerInteractiveTest::SendFullscreenShortcutAndWait() {
  // On MacOSX, entering and exiting fullscreen are not synchronous. So we wait
  // for the observer to notice the change of fullscreen state.
  content::WindowedNotificationObserver observer(
      chrome::NOTIFICATION_FULLSCREEN_CHANGED,
      content::NotificationService::AllSources());
  // Enter fullscreen.
#if defined(OS_MACOSX)
  // On MACOSX, Command + Control + F is used.
  ASSERT_TRUE(ui_test_utils::SendKeyPressSync(
      last_active_browser(), ui::VKEY_F, true, false, false, true));
#elif defined(OS_CHROMEOS)
  // A dedicated fullscreen key is used on Chrome OS, so send a fullscreen
  // command directly instead, to avoid constructing the key press.
  ASSERT_TRUE(chrome::ExecuteCommand(last_active_browser(), IDC_FULLSCREEN));
#else
  ASSERT_TRUE(ui_test_utils::SendKeyPressSync(
      last_active_browser(), ui::VKEY_F11, false, false, false, false));
#endif

  observer.Wait();
}

void BrowserCommandControllerInteractiveTest::
    SendJsFullscreenShortcutAndWait() {
  content::WindowedNotificationObserver observer(
      chrome::NOTIFICATION_FULLSCREEN_CHANGED,
      content::NotificationService::AllSources());
  ASSERT_TRUE(ui_test_utils::SendKeyPressSync(
      last_active_browser(), ui::VKEY_S, false, false, false, false));
  expected_result_ += "KeyS ctrl:false shift:false alt:false meta:false\n";
  observer.Wait();
  ASSERT_TRUE(IsActiveTabFullscreen());
}

void BrowserCommandControllerInteractiveTest::SendEscape() {
  ASSERT_TRUE(ui_test_utils::SendKeyPressSync(
      last_active_browser(), ui::VKEY_ESCAPE, false, false, false, false));
  expected_result_ += "Escape ctrl:false shift:false alt:false meta:false\n";
}

void BrowserCommandControllerInteractiveTest ::
    SendEscapeAndWaitForExitingFullscreen() {
  content::WindowedNotificationObserver observer(
      chrome::NOTIFICATION_FULLSCREEN_CHANGED,
      content::NotificationService::AllSources());
  ASSERT_TRUE(ui_test_utils::SendKeyPressSync(
      last_active_browser(), ui::VKEY_ESCAPE, false, false, false, false));
  observer.Wait();
  ASSERT_FALSE(IsActiveTabFullscreen());
}

void BrowserCommandControllerInteractiveTest::
    SendShortcutsAndExpectPrevented() {
  const int initial_active_index = active_tab_index();
  const int initial_tab_count = tab_count();
  const size_t initial_browser_count = browser_count();
  // The tab should not be closed.
  ASSERT_NO_FATAL_FAILURE(SendShortcut(ui::VKEY_W));
  ASSERT_EQ(initial_tab_count, tab_count());
  // The window should not be closed.
  ASSERT_NO_FATAL_FAILURE(SendShiftShortcut(ui::VKEY_W));
  ASSERT_EQ(initial_browser_count, browser_count());
  // TODO(zijiehe): ChromeOS incorrectly handles these;
  // see http://crbug.com/737307.
#if !defined(OS_CHROMEOS)
  // A new tab should not be created.
  ASSERT_NO_FATAL_FAILURE(SendShortcut(ui::VKEY_T));
  ASSERT_EQ(initial_tab_count, tab_count());
  // A new window should not be created.
  ASSERT_NO_FATAL_FAILURE(SendShortcut(ui::VKEY_N));
  ASSERT_EQ(initial_browser_count, browser_count());
  // A new incognito window should not be created.
  ASSERT_NO_FATAL_FAILURE(SendShiftShortcut(ui::VKEY_N));
  ASSERT_EQ(initial_browser_count, browser_count());
  // Last closed tab should not be restored.
  ASSERT_NO_FATAL_FAILURE(SendShiftShortcut(ui::VKEY_T));
  ASSERT_EQ(initial_tab_count, tab_count());
#endif
  // Browser should not switch to the next tab.
  ASSERT_NO_FATAL_FAILURE(SendShortcut(ui::VKEY_TAB));
  ASSERT_EQ(initial_active_index, active_tab_index());
  // Browser should not switch to the previous tab.
  ASSERT_NO_FATAL_FAILURE(SendShiftShortcut(ui::VKEY_TAB));
  ASSERT_EQ(initial_active_index, active_tab_index());
}

void BrowserCommandControllerInteractiveTest::
    SendShortcutsAndExpectNotPrevented(bool js_fullscreen) {
  const int initial_active_index = active_tab_index();
  const int initial_tab_count = tab_count();
  const size_t initial_browser_count = browser_count();
  const auto enter_fullscreen = [this, js_fullscreen]() {
    ASSERT_NO_FATAL_FAILURE(this->FocusOnLastActiveBrowser());
    if (js_fullscreen) {
      ASSERT_NO_FATAL_FAILURE(this->StartFullscreenTriggerPage());
      if (!this->IsActiveTabFullscreen()) {
        ASSERT_NO_FATAL_FAILURE(this->SendJsFullscreenShortcutAndWait());
      }
    } else {
      if (!this->IsInBrowserFullscreen()) {
        ASSERT_NO_FATAL_FAILURE(this->SendFullscreenShortcutAndWait());
      }
      ASSERT_TRUE(this->IsInBrowserFullscreen());
    }
  };

  ASSERT_NO_FATAL_FAILURE(enter_fullscreen());

  // A new tab should be created and focused.
  {
    TabCountObserver observer(last_active_browser(), initial_tab_count + 1);
    ASSERT_NO_FATAL_FAILURE(SendShortcut(ui::VKEY_T));
  }

  ASSERT_NO_FATAL_FAILURE(enter_fullscreen());

  // The newly created tab should be closed.
  {
    TabCountObserver observer(last_active_browser(), initial_tab_count);
    ASSERT_NO_FATAL_FAILURE(SendShortcut(ui::VKEY_W));
  }
  ASSERT_EQ(initial_active_index, active_tab_index());

  ASSERT_NO_FATAL_FAILURE(enter_fullscreen());

  // A new tab should be created and focused.
  {
    TabCountObserver observer(last_active_browser(), initial_tab_count + 1);
    ASSERT_NO_FATAL_FAILURE(SendShortcut(ui::VKEY_T));
  }

  ASSERT_NO_FATAL_FAILURE(enter_fullscreen());

  // The previous tab should be focused.
  ASSERT_NO_FATAL_FAILURE(SendPreviousTabShortcut());
  ASSERT_EQ(initial_active_index, active_tab_index());

  ASSERT_NO_FATAL_FAILURE(enter_fullscreen());

  // The newly created tab should be focused.
  ASSERT_NO_FATAL_FAILURE(SendNextTabShortcut());

  ASSERT_NO_FATAL_FAILURE(enter_fullscreen());

  // The newly created tab should be closed.
  {
    TabCountObserver observer(last_active_browser(), initial_tab_count);
    ASSERT_NO_FATAL_FAILURE(SendShortcut(ui::VKEY_W));
  }
  ASSERT_EQ(initial_active_index, active_tab_index());

  ASSERT_NO_FATAL_FAILURE(enter_fullscreen());

  // A new window should be created and focused.
  ASSERT_NO_FATAL_FAILURE(SendShortcut(ui::VKEY_N));
  ASSERT_EQ(initial_browser_count + 1, browser_count());

  ASSERT_NO_FATAL_FAILURE(enter_fullscreen());

  {
    // Closing browser is not synchronized.
    BrowserRemovedObserver observer(last_active_browser());
    // The newly created window should be closed.
    ASSERT_NO_FATAL_FAILURE(SendShiftShortcut(ui::VKEY_W));
  }

  ASSERT_EQ(initial_browser_count, browser_count());
  ASSERT_EQ(initial_active_index, active_tab_index());
}

void BrowserCommandControllerInteractiveTest::FinishTestAndVerifyResult() {
  // The renderer process receives key events through IPC channel,
  // SendKeyPressSync() cannot guarantee the JS has processed the key event it
  // sent. So we sent a KeyX to the webpage to indicate the end of the test
  // case. After processing this key event, web page is safe to send the record
  // back through window.domAutomationController.
  EXPECT_TRUE(ui_test_utils::SendKeyPressSync(
      last_active_browser(), ui::VKEY_X, false, false, false, false));
  expected_result_ += "KeyX ctrl:false shift:false alt:false meta:false";
  std::string result;
  EXPECT_TRUE(content::ExecuteScriptAndExtractString(
      active_web_contents()->GetRenderViewHost(),
      "getKeyEventReport();", &result));
  NormalizeMetaKeyForMacOS(&result);
  NormalizeMetaKeyForMacOS(&expected_result_);
  base::TrimWhitespaceASCII(result, base::TRIM_ALL, &result);
  ASSERT_EQ(expected_result_, result);
}

void BrowserCommandControllerInteractiveTest::SetUpOnMainThread() {
  ASSERT_TRUE(ui_test_utils::BringBrowserWindowToFront(last_active_browser()));
}

void BrowserCommandControllerInteractiveTest::StartTestPage(const char* file) {
  if (!embedded_test_server()->Started())
    ASSERT_TRUE(embedded_test_server()->Start());
  ui_test_utils::NavigateToURLWithDisposition(
      last_active_browser(), embedded_test_server()->GetURL(file),
      WindowOpenDisposition::CURRENT_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);
}

IN_PROC_BROWSER_TEST_F(BrowserCommandControllerInteractiveTest,
                       ShortcutsShouldTakeEffectInWindowMode) {
  ASSERT_EQ(1, tab_count());
  ASSERT_NO_FATAL_FAILURE(SendShortcut(ui::VKEY_T));
  ASSERT_EQ(2, tab_count());
  ASSERT_NO_FATAL_FAILURE(SendShortcut(ui::VKEY_T));
  ASSERT_EQ(3, tab_count());
  ASSERT_NO_FATAL_FAILURE(SendShortcut(ui::VKEY_W));
  ASSERT_EQ(2, tab_count());
  ASSERT_NO_FATAL_FAILURE(SendShortcut(ui::VKEY_W));
  ASSERT_EQ(1, tab_count());
  ASSERT_NO_FATAL_FAILURE(SendFullscreenShortcutAndWait());
  ASSERT_TRUE(IsInBrowserFullscreen());
  ASSERT_FALSE(IsActiveTabFullscreen());
}

IN_PROC_BROWSER_TEST_F(BrowserCommandControllerInteractiveTest,
                       UnpreservedShortcutsShouldBePreventable) {
  ASSERT_NO_FATAL_FAILURE(StartFullscreenLockPage());

  // The browser print function should be blocked by the web page.
  ASSERT_NO_FATAL_FAILURE(SendShortcut(ui::VKEY_P));
  // The system print function should be blocked by the web page.
  ASSERT_NO_FATAL_FAILURE(SendShiftShortcut(ui::VKEY_P));
  ASSERT_NO_FATAL_FAILURE(FinishTestAndVerifyResult());
}

#if defined(OS_MACOSX)
// TODO(zijiehe): Figure out why this test crashes on Mac OSX. The suspicious
// command is "SendFullscreenShortcutAndWait()". See, http://crbug.com/738949.
#define MAYBE_KeyEventsShouldBeConsumedByWebPageInBrowserFullscreen \
  DISABLED_KeyEventsShouldBeConsumedByWebPageInBrowserFullscreen
#else
#define MAYBE_KeyEventsShouldBeConsumedByWebPageInBrowserFullscreen \
  KeyEventsShouldBeConsumedByWebPageInBrowserFullscreen
#endif
IN_PROC_BROWSER_TEST_F(
    BrowserCommandControllerInteractiveTest,
    MAYBE_KeyEventsShouldBeConsumedByWebPageInBrowserFullscreen) {
  ASSERT_NO_FATAL_FAILURE(StartFullscreenLockPage());

  ASSERT_NO_FATAL_FAILURE(SendFullscreenShortcutAndWait());
  ASSERT_FALSE(IsActiveTabFullscreen());
  ASSERT_TRUE(IsInBrowserFullscreen());
  ASSERT_NO_FATAL_FAILURE(SendShortcutsAndExpectPrevented());
  // Current page should not exit browser fullscreen mode.
  ASSERT_NO_FATAL_FAILURE(SendEscape());

  ASSERT_NO_FATAL_FAILURE(FinishTestAndVerifyResult());

  ASSERT_NO_FATAL_FAILURE(SendFullscreenShortcutAndWait());
  ASSERT_FALSE(IsActiveTabFullscreen());
  ASSERT_FALSE(IsInBrowserFullscreen());
}

IN_PROC_BROWSER_TEST_F(
    BrowserCommandControllerInteractiveTest,
    KeyEventsShouldBeConsumedByWebPageInJsFullscreenExceptForEsc) {
  ASSERT_NO_FATAL_FAILURE(StartFullscreenLockPage());

  ASSERT_NO_FATAL_FAILURE(SendJsFullscreenShortcutAndWait());
  ASSERT_NO_FATAL_FAILURE(SendShortcutsAndExpectPrevented());
  // Current page should exit HTML fullscreen mode.
  ASSERT_NO_FATAL_FAILURE(SendEscapeAndWaitForExitingFullscreen());

  ASSERT_NO_FATAL_FAILURE(FinishTestAndVerifyResult());
}

IN_PROC_BROWSER_TEST_F(
    BrowserCommandControllerInteractiveTest,
    KeyEventsShouldBeConsumedByWebPageInJsFullscreenExceptForF11) {
  ASSERT_NO_FATAL_FAILURE(StartFullscreenLockPage());

  ASSERT_NO_FATAL_FAILURE(SendJsFullscreenShortcutAndWait());
  ASSERT_NO_FATAL_FAILURE(SendShortcutsAndExpectPrevented());
#if defined(OS_MACOSX)
  // On 10.9 or earlier, sending the exit fullscreen shortcut will crash the
  // binary. See http://crbug.com/740250.
  if (base::mac::IsAtLeastOS10_10()) {
    // Current page should exit browser fullscreen mode.
    ASSERT_NO_FATAL_FAILURE(SendFullscreenShortcutAndWait());
    ASSERT_FALSE(IsActiveTabFullscreen());
    ASSERT_FALSE(IsInBrowserFullscreen());
  }
#else
  // Current page should exit browser fullscreen mode.
  ASSERT_NO_FATAL_FAILURE(SendFullscreenShortcutAndWait());
  ASSERT_FALSE(IsActiveTabFullscreen());
  ASSERT_FALSE(IsInBrowserFullscreen());
#endif

  ASSERT_NO_FATAL_FAILURE(FinishTestAndVerifyResult());
}

IN_PROC_BROWSER_TEST_F(BrowserCommandControllerInteractiveTest,
                       ShortcutsShouldTakeEffectInBrowserFullscreen) {
#if defined(OS_MACOSX)
  // On 10.9 or earlier, sending the exit fullscreen shortcut will crash the
  // binary. See http://crbug.com/740250.
  if (base::mac::IsAtMostOS10_9())
    return;
#endif
  ASSERT_NO_FATAL_FAILURE(SendShortcutsAndExpectNotPrevented(false));
}

#if defined(OS_MACOSX)
// On MacOSX, HTML fullscreen uses appkit implementation, which triggers an
// animation. I.e. Most of the commands in the test case need to be protected by
// a fullscreen changed observer. But it means we will also test which commands
// will trigger the window to exit fullscreen, which should not be covered by
// this test case. So this test case should not be enabled on MacOSX.
#define MAYBE_ShortcutsShouldTakeEffectInJsFullscreen \
        DISABLED_ShortcutsShouldTakeEffectInJsFullscreen
#else
#define MAYBE_ShortcutsShouldTakeEffectInJsFullscreen \
        ShortcutsShouldTakeEffectInJsFullscreen
#endif
IN_PROC_BROWSER_TEST_F(BrowserCommandControllerInteractiveTest,
                       MAYBE_ShortcutsShouldTakeEffectInJsFullscreen) {
  ASSERT_NO_FATAL_FAILURE(SendShortcutsAndExpectNotPrevented(true));
}
