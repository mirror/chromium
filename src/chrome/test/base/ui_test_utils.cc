// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/base/ui_test_utils.h"

#if defined(OS_WIN)
#include <windows.h>
#endif

#include <vector>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/command_line.h"
#include "base/file_path.h"
#include "base/json/json_reader.h"
#include "base/memory/scoped_ptr.h"
#include "base/message_loop.h"
#include "base/path_service.h"
#include "base/process_util.h"
#include "base/rand_util.h"
#include "base/string_number_conversions.h"
#include "base/test/test_timeouts.h"
#include "base/time.h"
#include "base/utf_string_conversions.h"
#include "base/values.h"
#include "chrome/browser/bookmarks/bookmark_model.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/history/history.h"
#include "chrome/browser/infobars/infobar_tab_helper.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service.h"
#include "chrome/browser/search_engines/template_url_service_test_util.h"
#include "chrome/browser/tab_contents/thumbnail_generator.h"
#include "chrome/browser/ui/app_modal_dialogs/app_modal_dialog.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_navigator.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/find_bar/find_notification_details.h"
#include "chrome/browser/ui/find_bar/find_tab_helper.h"
#include "chrome/browser/ui/tab_contents/tab_contents.h"
#include "chrome/common/chrome_notification_types.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/extensions/extension_action.h"
#include "chrome/test/automation/javascript_execution_controller.h"
#include "chrome/test/base/bookmark_load_observer.h"
#include "content/public/browser/dom_operation_notification_details.h"
#include "content/public/browser/download_item.h"
#include "content/public/browser/download_manager.h"
#include "content/public/browser/geolocation.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_view.h"
#include "content/public/common/geoposition.h"
#include "content/public/test/test_navigation_observer.h"
#include "googleurl/src/gurl.h"
#include "net/base/net_util.h"
#include "net/test/python_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/size.h"
#include "ui/ui_controls/ui_controls.h"

#if defined(TOOLKIT_VIEWS)
#include "ui/views/focus/accelerator_handler.h"
#endif

#if defined(USE_AURA)
#include "ash/shell.h"
#include "ui/aura/root_window.h"
#endif

using content::DomOperationNotificationDetails;
using content::NativeWebKeyboardEvent;
using content::NavigationController;
using content::NavigationEntry;
using content::OpenURLParams;
using content::RenderViewHost;
using content::RenderWidgetHost;
using content::Referrer;
using content::WebContents;

static const int kDefaultWsPort = 8880;

// Number of times to repost a Quit task so that the MessageLoop finishes up
// pending tasks and tasks posted by those pending tasks without risking the
// potential hang behavior of MessageLoop::QuitWhenIdle.
// The criteria for choosing this number: it should be high enough to make the
// quit act like QuitWhenIdle, while taking into account that any page which is
// animating may be rendering another frame for each quit deferral. For an
// animating page, the potential delay to quitting the RunLoop would be
// kNumQuitDeferrals * frame_render_time. Some perf tests run slow, such as
// 200ms/frame.
static const int kNumQuitDeferrals = 10;

namespace ui_test_utils {

namespace {

class DOMOperationObserver : public content::NotificationObserver,
                             public content::WebContentsObserver {
 public:
  explicit DOMOperationObserver(RenderViewHost* render_view_host)
      : content::WebContentsObserver(
            WebContents::FromRenderViewHost(render_view_host)),
        did_respond_(false) {
    registrar_.Add(this, content::NOTIFICATION_DOM_OPERATION_RESPONSE,
                   content::Source<RenderViewHost>(render_view_host));
    message_loop_runner_ = new MessageLoopRunner;
    message_loop_runner_->Run();
  }

  virtual void Observe(int type,
                       const content::NotificationSource& source,
                       const content::NotificationDetails& details) OVERRIDE {
    DCHECK(type == content::NOTIFICATION_DOM_OPERATION_RESPONSE);
    content::Details<DomOperationNotificationDetails> dom_op_details(details);
    response_ = dom_op_details->json;
    did_respond_ = true;
    message_loop_runner_->Quit();
  }

  // Overridden from content::WebContentsObserver:
  virtual void RenderViewGone(base::TerminationStatus status) OVERRIDE {
    message_loop_runner_->Quit();
  }

  bool GetResponse(std::string* response) WARN_UNUSED_RESULT {
    *response = response_;
    return did_respond_;
  }

 private:
  content::NotificationRegistrar registrar_;
  std::string response_;
  bool did_respond_;
  scoped_refptr<MessageLoopRunner> message_loop_runner_;

  DISALLOW_COPY_AND_ASSIGN(DOMOperationObserver);
};

class FindInPageNotificationObserver : public content::NotificationObserver {
 public:
  explicit FindInPageNotificationObserver(TabContents* parent_tab)
      : parent_tab_(parent_tab),
        active_match_ordinal_(-1),
        number_of_matches_(0) {
    current_find_request_id_ =
        parent_tab->find_tab_helper()->current_find_request_id();
    registrar_.Add(this, chrome::NOTIFICATION_FIND_RESULT_AVAILABLE,
                   content::Source<WebContents>(parent_tab_->web_contents()));
    message_loop_runner_ = new MessageLoopRunner;
    message_loop_runner_->Run();
  }

  int active_match_ordinal() const { return active_match_ordinal_; }

  int number_of_matches() const { return number_of_matches_; }

  virtual void Observe(int type, const content::NotificationSource& source,
                       const content::NotificationDetails& details) {
    if (type == chrome::NOTIFICATION_FIND_RESULT_AVAILABLE) {
      content::Details<FindNotificationDetails> find_details(details);
      if (find_details->request_id() == current_find_request_id_) {
        // We get multiple responses and one of those will contain the ordinal.
        // This message comes to us before the final update is sent.
        if (find_details->active_match_ordinal() > -1)
          active_match_ordinal_ = find_details->active_match_ordinal();
        if (find_details->final_update()) {
          number_of_matches_ = find_details->number_of_matches();
          message_loop_runner_->Quit();
        } else {
          DVLOG(1) << "Ignoring, since we only care about the final message";
        }
      }
    } else {
      NOTREACHED();
    }
  }

 private:
  content::NotificationRegistrar registrar_;
  TabContents* parent_tab_;
  // We will at some point (before final update) be notified of the ordinal and
  // we need to preserve it so we can send it later.
  int active_match_ordinal_;
  int number_of_matches_;
  // The id of the current find request, obtained from WebContents. Allows us
  // to monitor when the search completes.
  int current_find_request_id_;
  scoped_refptr<MessageLoopRunner> message_loop_runner_;

  DISALLOW_COPY_AND_ASSIGN(FindInPageNotificationObserver);
};

class InProcessJavaScriptExecutionController
    : public base::RefCounted<InProcessJavaScriptExecutionController>,
      public JavaScriptExecutionController {
 public:
  explicit InProcessJavaScriptExecutionController(
      RenderViewHost* render_view_host)
      : render_view_host_(render_view_host) {}

 protected:
  // Executes |script| and sets the JSON response |json|.
  virtual bool ExecuteJavaScriptAndGetJSON(const std::string& script,
                                           std::string* json) {
    render_view_host_->ExecuteJavascriptInWebFrame(string16(),
                                                   UTF8ToUTF16(script));
    DOMOperationObserver dom_op_observer(render_view_host_);
    return dom_op_observer.GetResponse(json);
  }

  virtual void FirstObjectAdded() {
    AddRef();
  }

  virtual void LastObjectRemoved() {
    Release();
  }

 private:
  friend class base::RefCounted<InProcessJavaScriptExecutionController>;
  virtual ~InProcessJavaScriptExecutionController() {}

  // Weak pointer to the associated RenderViewHost.
  RenderViewHost* render_view_host_;
};

// Specifying a prototype so that we can add the WARN_UNUSED_RESULT attribute.
bool ExecuteJavaScriptHelper(RenderViewHost* render_view_host,
                             const std::wstring& frame_xpath,
                             const std::wstring& original_script,
                             scoped_ptr<Value>* result) WARN_UNUSED_RESULT;

// Executes the passed |original_script| in the frame pointed to by
// |frame_xpath|.  If |result| is not NULL, stores the value that the evaluation
// of the script in |result|.  Returns true on success.
bool ExecuteJavaScriptHelper(RenderViewHost* render_view_host,
                             const std::wstring& frame_xpath,
                             const std::wstring& original_script,
                             scoped_ptr<Value>* result) {
  // TODO(jcampan): we should make the domAutomationController not require an
  //                automation id.
  std::wstring script = L"window.domAutomationController.setAutomationId(0);" +
      original_script;
  render_view_host->ExecuteJavascriptInWebFrame(WideToUTF16Hack(frame_xpath),
                                                WideToUTF16Hack(script));
  DOMOperationObserver dom_op_observer(render_view_host);
  std::string json;
  if (!dom_op_observer.GetResponse(&json)) {
    DLOG(ERROR) << "Cannot communicate with DOMOperationObserver.";
    return false;
  }

  // Nothing more to do for callers that ignore the returned JS value.
  if (!result)
    return true;

  base::JSONReader reader(base::JSON_ALLOW_TRAILING_COMMAS);
  result->reset(reader.ReadToValue(json));
  if (!result->get()) {
    DLOG(ERROR) << reader.GetErrorMessage();
    return false;
  }

  return true;
}

void RunAllPendingMessageAndSendQuit(content::BrowserThread::ID thread_id,
                                     const base::Closure& quit_task) {
  MessageLoop::current()->PostTask(FROM_HERE,
                                   MessageLoop::QuitWhenIdleClosure());
  RunMessageLoop();
  content::BrowserThread::PostTask(thread_id, FROM_HERE, quit_task);
}

}  // namespace

void RunMessageLoop() {
  base::RunLoop run_loop;
  RunThisRunLoop(&run_loop);
}

void RunThisRunLoop(base::RunLoop* run_loop) {
  MessageLoop::ScopedNestableTaskAllower allow(MessageLoop::current());
#if !defined(USE_AURA) && defined(TOOLKIT_VIEWS)
  scoped_ptr<views::AcceleratorHandler> handler;
  if (content::BrowserThread::CurrentlyOn(content::BrowserThread::UI)) {
    handler.reset(new views::AcceleratorHandler);
    run_loop->set_dispatcher(handler.get());
  }
#endif
  run_loop->Run();
}

// TODO(jbates) move this to a new test_utils.cc in content/test/
static void DeferredQuitRunLoop(const base::Closure& quit_task,
                                int num_quit_deferrals) {
  if (num_quit_deferrals <= 0) {
    quit_task.Run();
  } else {
    MessageLoop::current()->PostTask(FROM_HERE,
        base::Bind(&DeferredQuitRunLoop, quit_task, num_quit_deferrals - 1));
  }
}

base::Closure GetQuitTaskForRunLoop(base::RunLoop* run_loop) {
  return base::Bind(&DeferredQuitRunLoop, run_loop->QuitClosure(),
                    kNumQuitDeferrals);
}

void RunAllPendingInMessageLoop() {
  MessageLoop::current()->PostTask(FROM_HERE,
                                   MessageLoop::QuitWhenIdleClosure());
  ui_test_utils::RunMessageLoop();
}

void RunAllPendingInMessageLoop(content::BrowserThread::ID thread_id) {
  if (content::BrowserThread::CurrentlyOn(thread_id)) {
    RunAllPendingInMessageLoop();
    return;
  }
  content::BrowserThread::ID current_thread_id;
  if (!content::BrowserThread::GetCurrentThreadIdentifier(&current_thread_id)) {
    NOTREACHED();
    return;
  }

  base::RunLoop run_loop;
  content::BrowserThread::PostTask(thread_id, FROM_HERE,
      base::Bind(&RunAllPendingMessageAndSendQuit, current_thread_id,
                 run_loop.QuitClosure()));
  ui_test_utils::RunThisRunLoop(&run_loop);
}

bool GetCurrentTabTitle(const Browser* browser, string16* title) {
  WebContents* web_contents = chrome::GetActiveWebContents(browser);
  if (!web_contents)
    return false;
  NavigationEntry* last_entry = web_contents->GetController().GetActiveEntry();
  if (!last_entry)
    return false;
  title->assign(last_entry->GetTitleForDisplay(""));
  return true;
}

void WaitForNavigations(NavigationController* controller,
                        int number_of_navigations) {
  content::TestNavigationObserver observer(
      content::Source<NavigationController>(controller), NULL,
      number_of_navigations);
  base::RunLoop run_loop;
  observer.WaitForObservation(
      base::Bind(&ui_test_utils::RunThisRunLoop, base::Unretained(&run_loop)),
      ui_test_utils::GetQuitTaskForRunLoop(&run_loop));
}

void WaitForNewTab(Browser* browser) {
  WindowedNotificationObserver observer(
      chrome::NOTIFICATION_TAB_ADDED,
      content::Source<content::WebContentsDelegate>(browser));
  observer.Wait();
}

void WaitForLoadStop(WebContents* tab) {
  WindowedNotificationObserver load_stop_observer(
      content::NOTIFICATION_LOAD_STOP,
      content::Source<NavigationController>(&tab->GetController()));
  // In many cases, the load may have finished before we get here.  Only wait if
  // the tab still has a pending navigation.
  if (!tab->IsLoading())
    return;
  load_stop_observer.Wait();
}

Browser* WaitForBrowserNotInSet(std::set<Browser*> excluded_browsers) {
  Browser* new_browser = GetBrowserNotInSet(excluded_browsers);
  if (new_browser == NULL) {
    BrowserAddedObserver observer;
    new_browser = observer.WaitForSingleNewBrowser();
    // The new browser should never be in |excluded_browsers|.
    DCHECK(!ContainsKey(excluded_browsers, new_browser));
  }
  return new_browser;
}

void OpenURLOffTheRecord(Profile* profile, const GURL& url) {
  chrome::OpenURLOffTheRecord(profile, url);
  Browser* browser = browser::FindTabbedBrowser(
      profile->GetOffTheRecordProfile(), false);
  WaitForNavigations(&chrome::GetActiveWebContents(browser)->GetController(),
                     1);
}

void NavigateToURL(browser::NavigateParams* params) {
  content::TestNavigationObserver observer(
      content::NotificationService::AllSources(), NULL, 1);
  browser::Navigate(params);
  base::RunLoop run_loop;
  observer.WaitForObservation(
      base::Bind(&ui_test_utils::RunThisRunLoop, base::Unretained(&run_loop)),
      ui_test_utils::GetQuitTaskForRunLoop(&run_loop));
}

void NavigateToURL(Browser* browser, const GURL& url) {
  NavigateToURLWithDisposition(browser, url, CURRENT_TAB,
                               BROWSER_TEST_WAIT_FOR_NAVIGATION);
}

// Navigates the specified tab (via |disposition|) of |browser| to |url|,
// blocking until the |number_of_navigations| specified complete.
// |disposition| indicates what tab the download occurs in, and
// |browser_test_flags| controls what to wait for before continuing.
static void NavigateToURLWithDispositionBlockUntilNavigationsComplete(
    Browser* browser,
    const GURL& url,
    int number_of_navigations,
    WindowOpenDisposition disposition,
    int browser_test_flags) {
  if (disposition == CURRENT_TAB && chrome::GetActiveWebContents(browser))
    WaitForLoadStop(chrome::GetActiveWebContents(browser));
  NavigationController* controller =
      chrome::GetActiveWebContents(browser) ?
      &chrome::GetActiveWebContents(browser)->GetController() : NULL;
  content::TestNavigationObserver same_tab_observer(
      content::Source<NavigationController>(controller),
      NULL,
      number_of_navigations);

  std::set<Browser*> initial_browsers;
  for (std::vector<Browser*>::const_iterator iter = BrowserList::begin();
       iter != BrowserList::end();
       ++iter) {
    initial_browsers.insert(*iter);
  }

  WindowedNotificationObserver tab_added_observer(
      chrome::NOTIFICATION_TAB_ADDED,
      content::NotificationService::AllSources());

  WindowedNotificationObserver auth_observer(
      chrome::NOTIFICATION_AUTH_NEEDED,
      content::NotificationService::AllSources());

  browser->OpenURL(OpenURLParams(
      url, Referrer(), disposition, content::PAGE_TRANSITION_TYPED, false));
  if (browser_test_flags & BROWSER_TEST_WAIT_FOR_BROWSER)
    browser = WaitForBrowserNotInSet(initial_browsers);
  if (browser_test_flags & BROWSER_TEST_WAIT_FOR_TAB)
    tab_added_observer.Wait();
  if (browser_test_flags & BROWSER_TEST_WAIT_FOR_AUTH)
    auth_observer.Wait();
  if (!(browser_test_flags & BROWSER_TEST_WAIT_FOR_NAVIGATION)) {
    // Some other flag caused the wait prior to this.
    return;
  }
  WebContents* web_contents = NULL;
  if (disposition == NEW_BACKGROUND_TAB) {
    // We've opened up a new tab, but not selected it.
    web_contents =
        chrome::GetWebContentsAt(browser, browser->active_index() + 1);
    EXPECT_TRUE(web_contents != NULL)
        << " Unable to wait for navigation to \"" << url.spec()
        << "\" because the new tab is not available yet";
    return;
  } else if ((disposition == CURRENT_TAB) ||
      (disposition == NEW_FOREGROUND_TAB) ||
      (disposition == SINGLETON_TAB)) {
    // The currently selected tab is the right one.
    web_contents = chrome::GetActiveWebContents(browser);
  }
  if (disposition == CURRENT_TAB) {
    base::RunLoop run_loop;
    same_tab_observer.WaitForObservation(
        base::Bind(&ui_test_utils::RunThisRunLoop, base::Unretained(&run_loop)),
        ui_test_utils::GetQuitTaskForRunLoop(&run_loop));
    return;
  } else if (web_contents) {
    NavigationController* controller = &web_contents->GetController();
    WaitForNavigations(controller, number_of_navigations);
    return;
  }
  EXPECT_TRUE(NULL != web_contents) << " Unable to wait for navigation to \""
                                    << url.spec() << "\""
                                    << " because we can't get the tab contents";
}

void NavigateToURLWithDisposition(Browser* browser,
                                  const GURL& url,
                                  WindowOpenDisposition disposition,
                                  int browser_test_flags) {
  NavigateToURLWithDispositionBlockUntilNavigationsComplete(
      browser,
      url,
      1,
      disposition,
      browser_test_flags);
}

void NavigateToURLBlockUntilNavigationsComplete(Browser* browser,
                                                const GURL& url,
                                                int number_of_navigations) {
  NavigateToURLWithDispositionBlockUntilNavigationsComplete(
      browser,
      url,
      number_of_navigations,
      CURRENT_TAB,
      BROWSER_TEST_WAIT_FOR_NAVIGATION);
}

DOMElementProxyRef GetActiveDOMDocument(Browser* browser) {
  JavaScriptExecutionController* executor =
      new InProcessJavaScriptExecutionController(
          chrome::GetActiveWebContents(browser)->GetRenderViewHost());
  int element_handle;
  executor->ExecuteJavaScriptAndGetReturn("document;", &element_handle);
  return executor->GetObjectProxy<DOMElementProxy>(element_handle);
}

bool ExecuteJavaScript(RenderViewHost* render_view_host,
                       const std::wstring& frame_xpath,
                       const std::wstring& original_script) {
  std::wstring script =
      original_script + L";window.domAutomationController.send(0);";
  return ExecuteJavaScriptHelper(render_view_host, frame_xpath, script, NULL);
}

bool ExecuteJavaScriptAndExtractInt(RenderViewHost* render_view_host,
                                    const std::wstring& frame_xpath,
                                    const std::wstring& script,
                                    int* result) {
  DCHECK(result);
  scoped_ptr<Value> value;
  if (!ExecuteJavaScriptHelper(render_view_host, frame_xpath, script, &value) ||
      !value.get())
    return false;

  return value->GetAsInteger(result);
}

bool ExecuteJavaScriptAndExtractBool(RenderViewHost* render_view_host,
                                     const std::wstring& frame_xpath,
                                     const std::wstring& script,
                                     bool* result) {
  DCHECK(result);
  scoped_ptr<Value> value;
  if (!ExecuteJavaScriptHelper(render_view_host, frame_xpath, script, &value) ||
      !value.get())
    return false;

  return value->GetAsBoolean(result);
}

bool ExecuteJavaScriptAndExtractString(RenderViewHost* render_view_host,
                                       const std::wstring& frame_xpath,
                                       const std::wstring& script,
                                       std::string* result) {
  DCHECK(result);
  scoped_ptr<Value> value;
  if (!ExecuteJavaScriptHelper(render_view_host, frame_xpath, script, &value) ||
      !value.get())
    return false;

  return value->GetAsString(result);
}

FilePath GetTestFilePath(const FilePath& dir, const FilePath& file) {
  FilePath path;
  PathService::Get(chrome::DIR_TEST_DATA, &path);
  return path.Append(dir).Append(file);
}

GURL GetTestUrl(const FilePath& dir, const FilePath& file) {
  return net::FilePathToFileURL(GetTestFilePath(dir, file));
}

GURL GetFileUrlWithQuery(const FilePath& path,
                         const std::string& query_string) {
  GURL url = net::FilePathToFileURL(path);
  if (!query_string.empty()) {
    GURL::Replacements replacements;
    replacements.SetQueryStr(query_string);
    return url.ReplaceComponents(replacements);
  }
  return url;
}

AppModalDialog* WaitForAppModalDialog() {
  WindowedNotificationObserver observer(
      chrome::NOTIFICATION_APP_MODAL_DIALOG_SHOWN,
      content::NotificationService::AllSources());
  observer.Wait();
  return content::Source<AppModalDialog>(observer.source()).ptr();
}

void WaitForAppModalDialogAndCloseIt() {
  AppModalDialog* dialog = WaitForAppModalDialog();
  dialog->CloseModalDialog();
}

void CrashTab(WebContents* tab) {
  content::RenderProcessHost* rph = tab->GetRenderProcessHost();
  WindowedNotificationObserver observer(
      content::NOTIFICATION_RENDERER_PROCESS_CLOSED,
      content::Source<content::RenderProcessHost>(rph));
  base::KillProcess(rph->GetHandle(), 0, false);
  observer.Wait();
}

int FindInPage(TabContents* tab_contents, const string16& search_string,
               bool forward, bool match_case, int* ordinal) {
  tab_contents->
      find_tab_helper()->StartFinding(search_string, forward, match_case);
  FindInPageNotificationObserver observer(tab_contents);
  if (ordinal)
    *ordinal = observer.active_match_ordinal();
  return observer.number_of_matches();
}

void CloseAllInfoBars(TabContents* tab) {
  InfoBarTabHelper* infobar_helper = tab->infobar_tab_helper();
  while (infobar_helper->infobar_count() > 0)
    infobar_helper->RemoveInfoBar(infobar_helper->GetInfoBarDelegateAt(0));
}

void SimulateMouseClick(content::WebContents* tab) {
  int x = tab->GetView()->GetContainerSize().width() / 2;
  int y = tab->GetView()->GetContainerSize().height() / 2;
  WebKit::WebMouseEvent mouse_event;
  mouse_event.type = WebKit::WebInputEvent::MouseDown;
  mouse_event.button = WebKit::WebMouseEvent::ButtonLeft;
  mouse_event.x = x;
  mouse_event.y = y;
  // Mac needs globalX/globalY for events to plugins.
  gfx::Rect offset;
  tab->GetView()->GetContainerBounds(&offset);
  mouse_event.globalX = x + offset.x();
  mouse_event.globalY = y + offset.y();
  mouse_event.clickCount = 1;
  tab->GetRenderViewHost()->ForwardMouseEvent(mouse_event);
  mouse_event.type = WebKit::WebInputEvent::MouseUp;
  tab->GetRenderViewHost()->ForwardMouseEvent(mouse_event);
}

void SimulateMouseEvent(content::WebContents* tab,
                        WebKit::WebInputEvent::Type type,
                        const gfx::Point& point) {
  WebKit::WebMouseEvent mouse_event;
  mouse_event.type = type;
  mouse_event.x = point.x();
  mouse_event.y = point.y();
  tab->GetRenderViewHost()->ForwardMouseEvent(mouse_event);
}

void BuildSimpleWebKeyEvent(WebKit::WebInputEvent::Type type,
                            ui::KeyboardCode key,
                            bool control,
                            bool shift,
                            bool alt,
                            bool command,
                            NativeWebKeyboardEvent* event) {
  event->nativeKeyCode = 0;
  event->windowsKeyCode = key;
  event->setKeyIdentifierFromWindowsKeyCode();
  event->type = type;
  event->modifiers = 0;
  event->isSystemKey = false;
  event->timeStampSeconds = base::Time::Now().ToDoubleT();
  event->skip_in_browser = true;

  if (type == WebKit::WebInputEvent::Char ||
      type == WebKit::WebInputEvent::RawKeyDown) {
    event->text[0] = key;
    event->unmodifiedText[0] = key;
  }

  if (control)
    event->modifiers |= WebKit::WebInputEvent::ControlKey;

  if (shift)
    event->modifiers |= WebKit::WebInputEvent::ShiftKey;

  if (alt)
    event->modifiers |= WebKit::WebInputEvent::AltKey;

  if (command)
    event->modifiers |= WebKit::WebInputEvent::MetaKey;
}

void SimulateKeyPress(content::WebContents* tab,
                      ui::KeyboardCode key,
                      bool control,
                      bool shift,
                      bool alt,
                      bool command) {
  NativeWebKeyboardEvent event_down;
  BuildSimpleWebKeyEvent(
      WebKit::WebInputEvent::RawKeyDown, key, control, shift, alt, command,
      &event_down);
  tab->GetRenderViewHost()->ForwardKeyboardEvent(event_down);

  NativeWebKeyboardEvent char_event;
  BuildSimpleWebKeyEvent(
      WebKit::WebInputEvent::Char, key, control, shift, alt, command,
      &char_event);
  tab->GetRenderViewHost()->ForwardKeyboardEvent(char_event);

  NativeWebKeyboardEvent event_up;
  BuildSimpleWebKeyEvent(
      WebKit::WebInputEvent::KeyUp, key, control, shift, alt, command,
      &event_up);
  tab->GetRenderViewHost()->ForwardKeyboardEvent(event_up);
}

void RegisterAndWait(content::NotificationObserver* observer,
                     int type,
                     const content::NotificationSource& source) {
  content::NotificationRegistrar registrar;
  registrar.Add(observer, type, source);
  RunMessageLoop();
}

void WaitForBookmarkModelToLoad(BookmarkModel* model) {
  if (model->IsLoaded())
    return;
  base::RunLoop run_loop;
  BookmarkLoadObserver observer(GetQuitTaskForRunLoop(&run_loop));
  model->AddObserver(&observer);
  RunThisRunLoop(&run_loop);
  model->RemoveObserver(&observer);
  ASSERT_TRUE(model->IsLoaded());
}

void WaitForTemplateURLServiceToLoad(TemplateURLService* service) {
  if (service->loaded())
    return;
  service->Load();
  TemplateURLServiceTestUtil::BlockTillServiceProcessesRequests();
  ASSERT_TRUE(service->loaded());
}

void WaitForHistoryToLoad(HistoryService* history_service) {
  WindowedNotificationObserver history_loaded_observer(
      chrome::NOTIFICATION_HISTORY_LOADED,
      content::NotificationService::AllSources());
  if (!history_service->BackendLoaded())
    history_loaded_observer.Wait();
}

bool GetNativeWindow(const Browser* browser, gfx::NativeWindow* native_window) {
  BrowserWindow* window = browser->window();
  if (!window)
    return false;

  *native_window = window->GetNativeWindow();
  return *native_window;
}

bool BringBrowserWindowToFront(const Browser* browser) {
  gfx::NativeWindow window = NULL;
  if (!GetNativeWindow(browser, &window))
    return false;

  return ui_test_utils::ShowAndFocusNativeWindow(window);
}

Browser* GetBrowserNotInSet(std::set<Browser*> excluded_browsers) {
  for (BrowserList::const_iterator iter = BrowserList::begin();
       iter != BrowserList::end();
       ++iter) {
    if (excluded_browsers.find(*iter) == excluded_browsers.end())
      return *iter;
  }

  return NULL;
}

bool SendKeyPressSync(const Browser* browser,
                      ui::KeyboardCode key,
                      bool control,
                      bool shift,
                      bool alt,
                      bool command) {
  gfx::NativeWindow window = NULL;
  if (!GetNativeWindow(browser, &window))
    return false;
  scoped_refptr<MessageLoopRunner> runner = new MessageLoopRunner;
  bool result;
  result = ui_controls::SendKeyPressNotifyWhenDone(
      window, key, control, shift, alt, command, runner->QuitClosure());
#if defined(OS_WIN)
  if (!result && BringBrowserWindowToFront(browser)) {
    result = ui_controls::SendKeyPressNotifyWhenDone(
        window, key, control, shift, alt, command, runner->QuitClosure());
  }
#endif
  if (!result) {
    LOG(ERROR) << "ui_controls::SendKeyPressNotifyWhenDone failed";
    return false;
  }

  // Run the message loop. It'll stop running when either the key was received
  // or the test timed out (in which case testing::Test::HasFatalFailure should
  // be set).
  runner->Run();
  return !testing::Test::HasFatalFailure();
}

bool SendKeyPressAndWait(const Browser* browser,
                         ui::KeyboardCode key,
                         bool control,
                         bool shift,
                         bool alt,
                         bool command,
                         int type,
                         const content::NotificationSource& source) {
  WindowedNotificationObserver observer(type, source);

  if (!SendKeyPressSync(browser, key, control, shift, alt, command))
    return false;

  observer.Wait();
  return !testing::Test::HasFatalFailure();
}

bool SendMouseMoveSync(const gfx::Point& location) {
  scoped_refptr<MessageLoopRunner> runner = new MessageLoopRunner;
  if (!ui_controls::SendMouseMoveNotifyWhenDone(
          location.x(), location.y(), runner->QuitClosure())) {
    return false;
  }
  runner->Run();
  return !testing::Test::HasFatalFailure();
}

bool SendMouseEventsSync(ui_controls::MouseButton type, int state) {
  scoped_refptr<MessageLoopRunner> runner = new MessageLoopRunner;
  if (!ui_controls::SendMouseEventsNotifyWhenDone(
          type, state, runner->QuitClosure())) {
    return false;
  }
  runner->Run();
  return !testing::Test::HasFatalFailure();
}

MessageLoopRunner::MessageLoopRunner() {
}

MessageLoopRunner::~MessageLoopRunner() {
}

void MessageLoopRunner::Run() {
  ui_test_utils::RunThisRunLoop(&run_loop_);
}

base::Closure MessageLoopRunner::QuitClosure() {
  return base::Bind(&MessageLoopRunner::Quit, this);
}

void MessageLoopRunner::Quit() {
  ui_test_utils::GetQuitTaskForRunLoop(&run_loop_).Run();
}

TestWebSocketServer::TestWebSocketServer()
    : started_(false),
      port_(kDefaultWsPort),
      secure_(false) {
#if defined(OS_POSIX)
  process_group_id_ = base::kNullProcessHandle;
#endif
}

int TestWebSocketServer::UseRandomPort() {
  port_ = base::RandInt(1024, 65535);
  return port_;
}

void TestWebSocketServer::UseTLS() {
  secure_ = true;
}

bool TestWebSocketServer::Start(const FilePath& root_directory) {
  if (started_)
    return true;
  // Append CommandLine arguments after the server script, switches won't work.
  scoped_ptr<CommandLine> cmd_line(CreateWebSocketServerCommandLine());
  cmd_line->AppendArg("--server=start");
  cmd_line->AppendArg("--chromium");
  cmd_line->AppendArg("--register_cygwin");
  cmd_line->AppendArgNative(FILE_PATH_LITERAL("--root=") +
                            root_directory.value());
  cmd_line->AppendArg("--port=" + base::IntToString(port_));
  if (secure_)
    cmd_line->AppendArg("--tls");
  if (!temp_dir_.CreateUniqueTempDir()) {
    LOG(ERROR) << "Unable to create a temporary directory.";
    return false;
  }
  cmd_line->AppendArgNative(FILE_PATH_LITERAL("--output-dir=") +
                            temp_dir_.path().value());
  websocket_pid_file_ = temp_dir_.path().AppendASCII("websocket.pid");
  cmd_line->AppendArgNative(FILE_PATH_LITERAL("--pidfile=") +
                            websocket_pid_file_.value());
  SetPythonPath();

  base::LaunchOptions options;
  base::ProcessHandle process_handle;

#if defined(OS_POSIX)
  options.new_process_group = true;
#elif defined(OS_WIN)
  job_handle_.Set(CreateJobObject(NULL, NULL));
  if (!job_handle_.IsValid()) {
    LOG(ERROR) << "Could not create JobObject.";
    return false;
  }

  if (!base::SetJobObjectAsKillOnJobClose(job_handle_.Get())) {
    LOG(ERROR) << "Could not SetInformationJobObject.";
    return false;
  }

  options.inherit_handles = true;
  options.job_handle = job_handle_.Get();
#endif

  // Launch a new WebSocket server process.
  if (!base::LaunchProcess(*cmd_line.get(), options, &process_handle)) {
    LOG(ERROR) << "Unable to launch websocket server.";
    return false;
  }
#if defined(OS_POSIX)
  process_group_id_ = process_handle;
#endif
  int exit_code;
  bool wait_success = base::WaitForExitCodeWithTimeout(
      process_handle,
      &exit_code,
      TestTimeouts::action_max_timeout_ms());
  base::CloseProcessHandle(process_handle);

  if (!wait_success || exit_code != 0) {
    LOG(ERROR) << "Failed to run new-run-webkit-websocketserver: "
               << "wait_success = " << wait_success << ", "
               << "exit_code = " << exit_code;
    return false;
  }

  started_ = true;
  return true;
}

CommandLine* TestWebSocketServer::CreatePythonCommandLine() {
  // Note: Python's first argument must be the script; do not append CommandLine
  // switches, as they would precede the script path and break this CommandLine.
  FilePath path;
  CHECK(GetPythonRunTime(&path));
  return new CommandLine(path);
}

void TestWebSocketServer::SetPythonPath() {
  FilePath scripts_path;
  PathService::Get(base::DIR_SOURCE_ROOT, &scripts_path);

  scripts_path = scripts_path
      .Append(FILE_PATH_LITERAL("third_party"))
      .Append(FILE_PATH_LITERAL("WebKit"))
      .Append(FILE_PATH_LITERAL("Tools"))
      .Append(FILE_PATH_LITERAL("Scripts"));
  AppendToPythonPath(scripts_path);
}

CommandLine* TestWebSocketServer::CreateWebSocketServerCommandLine() {
  FilePath src_path;
  // Get to 'src' dir.
  PathService::Get(base::DIR_SOURCE_ROOT, &src_path);

  FilePath script_path(src_path);
  script_path = script_path.AppendASCII("third_party");
  script_path = script_path.AppendASCII("WebKit");
  script_path = script_path.AppendASCII("Tools");
  script_path = script_path.AppendASCII("Scripts");
  script_path = script_path.AppendASCII("new-run-webkit-websocketserver");

  CommandLine* cmd_line = CreatePythonCommandLine();
  cmd_line->AppendArgPath(script_path);
  return cmd_line;
}

TestWebSocketServer::~TestWebSocketServer() {
  if (!started_)
    return;
  // Append CommandLine arguments after the server script, switches won't work.
  scoped_ptr<CommandLine> cmd_line(CreateWebSocketServerCommandLine());
  cmd_line->AppendArg("--server=stop");
  cmd_line->AppendArg("--chromium");
  cmd_line->AppendArgNative(FILE_PATH_LITERAL("--pidfile=") +
                            websocket_pid_file_.value());
  base::LaunchOptions options;
  options.wait = true;
  base::LaunchProcess(*cmd_line.get(), options, NULL);

#if defined(OS_POSIX)
  // Just to make sure that the server process terminates certainly.
  if (process_group_id_ != base::kNullProcessHandle)
    base::KillProcessGroup(process_group_id_);
#endif
}

WindowedNotificationObserver::WindowedNotificationObserver(
    int notification_type,
    const content::NotificationSource& source)
    : seen_(false),
      running_(false),
      source_(content::NotificationService::AllSources()) {
  registrar_.Add(this, notification_type, source);
}

WindowedNotificationObserver::~WindowedNotificationObserver() {}

void WindowedNotificationObserver::Wait() {
  if (seen_)
    return;

  running_ = true;
  message_loop_runner_ = new MessageLoopRunner;
  message_loop_runner_->Run();
  EXPECT_TRUE(seen_);
}

void WindowedNotificationObserver::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  source_ = source;
  details_ = details;
  seen_ = true;
  if (!running_)
    return;

  message_loop_runner_->Quit();
  running_ = false;
}

WindowedTabAddedNotificationObserver::WindowedTabAddedNotificationObserver(
    const content::NotificationSource& source)
    : WindowedNotificationObserver(chrome::NOTIFICATION_TAB_ADDED, source),
      added_tab_(NULL) {
}

void WindowedTabAddedNotificationObserver::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  added_tab_ = content::Details<content::WebContents>(details).ptr();
  WindowedNotificationObserver::Observe(type, source, details);
}

TitleWatcher::TitleWatcher(WebContents* web_contents,
                           const string16& expected_title)
    : web_contents_(web_contents),
      expected_title_observed_(false),
      quit_loop_on_observation_(false) {
  EXPECT_TRUE(web_contents != NULL);
  expected_titles_.push_back(expected_title);
  notification_registrar_.Add(this,
                              content::NOTIFICATION_WEB_CONTENTS_TITLE_UPDATED,
                              content::Source<WebContents>(web_contents));

  // When navigating through the history, the restored NavigationEntry's title
  // will be used. If the entry ends up having the same title after we return
  // to it, as will usually be the case, the
  // NOTIFICATION_WEB_CONTENTS_TITLE_UPDATED will then be suppressed, since the
  // NavigationEntry's title hasn't changed.
  notification_registrar_.Add(
      this,
      content::NOTIFICATION_LOAD_STOP,
      content::Source<NavigationController>(&web_contents->GetController()));
}

void TitleWatcher::AlsoWaitForTitle(const string16& expected_title) {
  expected_titles_.push_back(expected_title);
}

TitleWatcher::~TitleWatcher() {
}

const string16& TitleWatcher::WaitAndGetTitle() {
  if (expected_title_observed_)
    return observed_title_;
  quit_loop_on_observation_ = true;
  message_loop_runner_ = new MessageLoopRunner;
  message_loop_runner_->Run();
  return observed_title_;
}

void TitleWatcher::Observe(int type,
                           const content::NotificationSource& source,
                           const content::NotificationDetails& details) {
  if (type == content::NOTIFICATION_WEB_CONTENTS_TITLE_UPDATED) {
    WebContents* source_contents = content::Source<WebContents>(source).ptr();
    ASSERT_EQ(web_contents_, source_contents);
  } else if (type == content::NOTIFICATION_LOAD_STOP) {
    NavigationController* controller =
        content::Source<NavigationController>(source).ptr();
    ASSERT_EQ(&web_contents_->GetController(), controller);
  } else {
    FAIL() << "Unexpected notification received.";
  }

  std::vector<string16>::const_iterator it =
      std::find(expected_titles_.begin(),
                expected_titles_.end(),
                web_contents_->GetTitle());
  if (it == expected_titles_.end())
    return;
  observed_title_ = *it;
  expected_title_observed_ = true;
  if (quit_loop_on_observation_) {
    // Only call Quit once, on first Observe:
    quit_loop_on_observation_ = false;
    message_loop_runner_->Quit();
  }
}

BrowserAddedObserver::BrowserAddedObserver()
    : notification_observer_(
          chrome::NOTIFICATION_BROWSER_OPENED,
          content::NotificationService::AllSources()) {
  original_browsers_.insert(BrowserList::begin(), BrowserList::end());
}

BrowserAddedObserver::~BrowserAddedObserver() {
}

Browser* BrowserAddedObserver::WaitForSingleNewBrowser() {
  notification_observer_.Wait();
  // Ensure that only a single new browser has appeared.
  EXPECT_EQ(original_browsers_.size() + 1, BrowserList::size());
  return GetBrowserNotInSet(original_browsers_);
}

DOMMessageQueue::DOMMessageQueue() : waiting_for_message_(false) {
  registrar_.Add(this, content::NOTIFICATION_DOM_OPERATION_RESPONSE,
                 content::NotificationService::AllSources());
}

DOMMessageQueue::~DOMMessageQueue() {}

void DOMMessageQueue::Observe(int type,
                              const content::NotificationSource& source,
                              const content::NotificationDetails& details) {
  content::Details<DomOperationNotificationDetails> dom_op_details(details);
  content::Source<RenderViewHost> sender(source);
  message_queue_.push(dom_op_details->json);
  if (waiting_for_message_) {
    waiting_for_message_ = false;
    message_loop_runner_->Quit();
  }
}

void DOMMessageQueue::ClearQueue() {
  message_queue_ = std::queue<std::string>();
}

bool DOMMessageQueue::WaitForMessage(std::string* message) {
  if (message_queue_.empty()) {
    waiting_for_message_ = true;
    // This will be quit when a new message comes in.
    message_loop_runner_ = new MessageLoopRunner;
    message_loop_runner_->Run();
  }
  // The queue should not be empty, unless we were quit because of a timeout.
  if (message_queue_.empty())
    return false;
  if (message)
    *message = message_queue_.front();
  return true;
}

// Coordinates taking snapshots of a |RenderWidget|.
class SnapshotTaker {
 public:
  SnapshotTaker() : bitmap_(NULL) {}

  bool TakeRenderWidgetSnapshot(RenderWidgetHost* rwh,
                                const gfx::Size& page_size,
                                const gfx::Size& desired_size,
                                SkBitmap* bitmap) WARN_UNUSED_RESULT {
    bitmap_ = bitmap;
    ThumbnailGenerator* generator = g_browser_process->GetThumbnailGenerator();
    generator->MonitorRenderer(rwh, true);
    snapshot_taken_ = false;
    generator->AskForSnapshot(
        rwh,
        base::Bind(&SnapshotTaker::OnSnapshotTaken, base::Unretained(this)),
        page_size,
        desired_size);
    message_loop_runner_ = new MessageLoopRunner;
    message_loop_runner_->Run();
    return snapshot_taken_;
  }

  bool TakeEntirePageSnapshot(RenderViewHost* rvh,
                              SkBitmap* bitmap) WARN_UNUSED_RESULT {
    const wchar_t* script =
        L"window.domAutomationController.send("
        L"    JSON.stringify([document.width, document.height]))";
    std::string json;
    if (!ui_test_utils::ExecuteJavaScriptAndExtractString(
            rvh, L"", script, &json))
      return false;

    // Parse the JSON.
    std::vector<int> dimensions;
    scoped_ptr<Value> value(
        base::JSONReader::Read(json, base::JSON_ALLOW_TRAILING_COMMAS));
    if (!value->IsType(Value::TYPE_LIST))
      return false;
    ListValue* list = static_cast<ListValue*>(value.get());
    int width, height;
    if (!list->GetInteger(0, &width) || !list->GetInteger(1, &height))
      return false;

    // Take the snapshot.
    gfx::Size page_size(width, height);
    return TakeRenderWidgetSnapshot(rvh, page_size, page_size, bitmap);
  }

 private:
  // Called when the ThumbnailGenerator has taken the snapshot.
  void OnSnapshotTaken(const SkBitmap& bitmap) {
    *bitmap_ = bitmap;
    snapshot_taken_ = true;
    message_loop_runner_->Quit();
  }

  SkBitmap* bitmap_;
  // Whether the snapshot was actually taken and received by this SnapshotTaker.
  // This will be false if the test times out.
  bool snapshot_taken_;
  scoped_refptr<MessageLoopRunner> message_loop_runner_;

  DISALLOW_COPY_AND_ASSIGN(SnapshotTaker);
};

bool TakeRenderWidgetSnapshot(RenderWidgetHost* rwh,
                              const gfx::Size& page_size,
                              SkBitmap* bitmap) {
  DCHECK(bitmap);
  SnapshotTaker taker;
  return taker.TakeRenderWidgetSnapshot(rwh, page_size, page_size, bitmap);
}

bool TakeEntirePageSnapshot(RenderViewHost* rvh, SkBitmap* bitmap) {
  DCHECK(bitmap);
  SnapshotTaker taker;
  return taker.TakeEntirePageSnapshot(rvh, bitmap);
}

void OverrideGeolocation(double latitude, double longitude) {
  content::Geoposition position;
  position.latitude = latitude;
  position.longitude = longitude;
  position.altitude = 0.;
  position.accuracy = 0.;
  position.timestamp = base::Time::Now();
  scoped_refptr<MessageLoopRunner> runner = new MessageLoopRunner;
  content::OverrideLocationForTesting(position, runner->QuitClosure());
  runner->Run();
}

namespace internal {

void ClickTask(ui_controls::MouseButton button,
               int state,
               const base::Closure& followup) {
  if (!followup.is_null())
    ui_controls::SendMouseEventsNotifyWhenDone(button, state, followup);
  else
    ui_controls::SendMouseEvents(button, state);
}

}  // namespace internal
}  // namespace ui_test_utils
