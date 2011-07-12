// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/sessions_ui.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sync/engine/syncapi.h"
#include "chrome/browser/sync/glue/session_model_associator.h"
#include "chrome/browser/sync/profile_sync_service.h"
#include "chrome/browser/ui/webui/chrome_url_data_manager.h"
#include "chrome/browser/ui/webui/ntp/new_tab_ui.h"
#include "chrome/common/chrome_version_info.h"
#include "chrome/common/jstemplate_builder.h"
#include "chrome/common/url_constants.h"
#include "content/browser/webui/web_ui.h"
#include "grit/browser_resources.h"
#include "grit/chromium_strings.h"
#include "grit/generated_resources.h"
#include "grit/theme_resources.h"
#include "grit/theme_resources_standard.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"


namespace {

///////////////////////////////////////////////////////////////////////////////
//
// SessionsUIHTMLSource
//
///////////////////////////////////////////////////////////////////////////////

class SessionsUIHTMLSource : public ChromeURLDataManager::DataSource {
 public:
  SessionsUIHTMLSource()
      : DataSource(chrome::kChromeUISessionsHost, MessageLoop::current()) {}

  // Called when the network layer has requested a resource underneath
  // the path we registered.
  virtual void StartDataRequest(const std::string& path,
                                bool is_incognito,
                                int request_id);
  virtual std::string GetMimeType(const std::string&) const {
    return "text/html";
  }

 private:
  ~SessionsUIHTMLSource() {}

  DISALLOW_COPY_AND_ASSIGN(SessionsUIHTMLSource);
};

void SessionsUIHTMLSource::StartDataRequest(const std::string& path,
                                            bool is_incognito,
                                            int request_id) {
  DictionaryValue localized_strings;
  localized_strings.SetString("sessionsTitle",
      l10n_util::GetStringUTF16(IDS_SESSIONS_TITLE));
  localized_strings.SetString("sessionsCountFormat",
      l10n_util::GetStringUTF16(IDS_SESSIONS_SESSION_COUNT_BANNER_FORMAT));
  localized_strings.SetString("noSessionsMessage",
      l10n_util::GetStringUTF16(IDS_SESSIONS_NO_SESSIONS_MESSAGE));

  localized_strings.SetString("magicCountFormat",
      l10n_util::GetStringUTF16(IDS_SESSIONS_MAGIC_LIST_BANNER_FORMAT));
  localized_strings.SetString("noMagicMessage",
      l10n_util::GetStringUTF16(IDS_SESSIONS_NO_MAGIC_MESSAGE));

  ChromeURLDataManager::DataSource::SetFontAndTextDirection(&localized_strings);

  static const base::StringPiece sessions_html(
      ResourceBundle::GetSharedInstance().
      GetRawDataResource(IDR_SESSIONS_HTML));
  std::string full_html =
      jstemplate_builder::GetI18nTemplateHtml(sessions_html,
                                              &localized_strings);
  jstemplate_builder::AppendJsTemplateSourceHtml(&full_html);

  scoped_refptr<RefCountedBytes> html_bytes(new RefCountedBytes);
  html_bytes->data.resize(full_html.size());
  std::copy(full_html.begin(), full_html.end(), html_bytes->data.begin());

  SendResponse(request_id, html_bytes);
}

////////////////////////////////////////////////////////////////////////////////
//
// SessionsDOMHandler
//
////////////////////////////////////////////////////////////////////////////////

// The handler for Javascript messages for the chrome://sessions/ page.
class SessionsDOMHandler : public WebUIMessageHandler {
 public:
  SessionsDOMHandler();
  virtual ~SessionsDOMHandler();

  // WebUIMessageHandler implementation.
  virtual WebUIMessageHandler* Attach(WebUI* web_ui);
  virtual void RegisterMessages();

 private:
  // Asynchronously fetches the session list. Called from JS.
  void HandleRequestSessions(const ListValue* args);

  // Sends the recent session list to JS.
  void UpdateUI();

  // Returns the model associated for getting the list of sessions from sync.
  browser_sync::SessionModelAssociator* GetModelAssociator();

  // Appends each entry in |tabs| to |tab_list| as a DictonaryValue.
  void GetTabList(const std::vector<SessionTab*>& tabs, ListValue* tab_list);

  // Appends each entry in |windows| to |window_list| as a DictonaryValue.
  void GetWindowList(const std::vector<SessionWindow*>& windows,
                     ListValue* window_list);

  // Appends each entry in |sessions| to |session_list| as a DictonaryValue.
  void GetSessionList(const std::vector<const SyncedSession*>& sessions,
                      ListValue* session_list);

  // Traverses all tabs in |sessions| and adds them to |all_tabs|.
  void GetAllTabs(const std::vector<const SyncedSession*>& sessions,
                  std::vector<SessionTab*>* all_tabs);

  // Creates a "magic" list of tabs from all the sessions.
  void CreateMagicTabList(const std::vector<const SyncedSession*>& sessions,
                          ListValue* tab_list);

  DISALLOW_COPY_AND_ASSIGN(SessionsDOMHandler);
};

SessionsDOMHandler::SessionsDOMHandler() {
}

SessionsDOMHandler::~SessionsDOMHandler() {
}

WebUIMessageHandler* SessionsDOMHandler::Attach(WebUI* web_ui) {
  return WebUIMessageHandler::Attach(web_ui);
}

void SessionsDOMHandler::RegisterMessages() {
  web_ui_->RegisterMessageCallback("requestSessionList",
      NewCallback(this, &SessionsDOMHandler::HandleRequestSessions));
}

void SessionsDOMHandler::HandleRequestSessions(const ListValue* args) {
  UpdateUI();
}

browser_sync::SessionModelAssociator* SessionsDOMHandler::GetModelAssociator() {
  // We only want to get the model associator if there is one, and it is done
  // syncing sessions.
  Profile* profile = web_ui_->GetProfile();
  if (!profile->HasProfileSyncService())
    return NULL;
  ProfileSyncService* service = profile->GetProfileSyncService();
  if (!service->ShouldPushChanges())
    return NULL;
  return service->GetSessionModelAssociator();
}

void SessionsDOMHandler::GetTabList(
    const std::vector<SessionTab*>& tabs, ListValue* tab_list) {
  for (std::vector<SessionTab*>::const_iterator it = tabs.begin();
       it != tabs.end(); ++it) {
    const SessionTab* tab = *it;
    const int nav_index = tab->current_navigation_index;
    // Range-check |nav_index| as it has been observed to be invalid sometimes.
    if (nav_index < 0 ||
        static_cast<size_t>(nav_index) >= tab->navigations.size()) {
      continue;
    }
    const TabNavigation& nav = tab->navigations[nav_index];
    scoped_ptr<DictionaryValue> tab_data(new DictionaryValue());
    tab_data->SetInteger("id", tab->tab_id.id());
    tab_data->SetDouble("timestamp",
        static_cast<double> (tab->timestamp.ToInternalValue()));
    if (nav.title().empty())
      tab_data->SetString("title", nav.virtual_url().spec());
    else
      tab_data->SetString("title", nav.title());
    tab_data->SetString("url", nav.virtual_url().spec());
    tab_list->Append(tab_data.release());
  }
}

void SessionsDOMHandler::GetWindowList(
    const std::vector<SessionWindow*>& windows, ListValue* window_list) {
  for (std::vector<SessionWindow*>::const_iterator it =
      windows.begin(); it != windows.end(); ++it) {
    const SessionWindow* window = *it;
    scoped_ptr<DictionaryValue> window_data(new DictionaryValue());
    window_data->SetInteger("id", window->window_id.id());
    window_data->SetDouble("timestamp",
        static_cast<double> (window->timestamp.ToInternalValue()));
    scoped_ptr<ListValue> tab_list(new ListValue());
    GetTabList(window->tabs, tab_list.get());
    window_data->Set("tabs", tab_list.release());
    window_list->Append(window_data.release());
  }
}

void SessionsDOMHandler::GetSessionList(
    const std::vector<const SyncedSession*>& sessions,
    ListValue* session_list) {
  for (std::vector<const SyncedSession*>::const_iterator it =
       sessions.begin(); it != sessions.end(); ++it) {
    const SyncedSession* session = *it;
    scoped_ptr<DictionaryValue> session_data(new DictionaryValue());
    session_data->SetString("tag", session->session_tag);
    scoped_ptr<ListValue> window_list(new ListValue());
    GetWindowList(session->windows, window_list.get());
    session_data->Set("windows", window_list.release());
    session_list->Append(session_data.release());
  }
}

void SessionsDOMHandler::GetAllTabs(
    const std::vector<const SyncedSession*>& sessions,
    std::vector<SessionTab*>* all_tabs) {
  for (size_t i = 0; i < sessions.size(); i++) {
    const std::vector<SessionWindow*>& windows = sessions[i]->windows;
    for (size_t j = 0; j < windows.size(); j++) {
      const std::vector<SessionTab*>& tabs = windows[j]->tabs;
      all_tabs->insert(all_tabs->end(), tabs.begin(), tabs.end());
    }
  }
}

// Comparator function for sort() in CreateMagicTabList() below.
bool CompareTabsByTimestamp(SessionTab* lhs, SessionTab* rhs) {
  return lhs->timestamp.ToInternalValue() > rhs->timestamp.ToInternalValue();
}

void SessionsDOMHandler::CreateMagicTabList(
    const std::vector<const SyncedSession*>& sessions,
    ListValue* tab_list) {
  std::vector<SessionTab*> all_tabs;
  GetAllTabs(sessions, &all_tabs);

  // Sort the list by timestamp - newest first.
  std::sort(all_tabs.begin(), all_tabs.end(), CompareTabsByTimestamp);

  // Truncate the list if necessary.
  const size_t kMagicTabListMaxSize = 10;
  if (all_tabs.size() > kMagicTabListMaxSize)
    all_tabs.resize(kMagicTabListMaxSize);

  GetTabList(all_tabs, tab_list);
}

void SessionsDOMHandler::UpdateUI() {
  ListValue session_list;
  ListValue magic_list;

  browser_sync::SessionModelAssociator* associator = GetModelAssociator();
  // Make sure the associator has been created.
  if (associator) {
    std::vector<const SyncedSession*> sessions;
    if (associator->GetAllForeignSessions(&sessions)) {
      GetSessionList(sessions, &session_list);
      CreateMagicTabList(sessions, &magic_list);
    }
  }

  // Send the results to JavaScript, even if the lists are empty, so that the
  // UI can show a message that there is nothing.
  web_ui_->CallJavascriptFunction("updateSessionList",
                                  session_list,
                                  magic_list);
}

}  // namespace

///////////////////////////////////////////////////////////////////////////////
//
// SessionsUI
//
///////////////////////////////////////////////////////////////////////////////

SessionsUI::SessionsUI(TabContents* contents) : ChromeWebUI(contents) {
  AddMessageHandler((new SessionsDOMHandler())->Attach(this));

  SessionsUIHTMLSource* html_source = new SessionsUIHTMLSource();

  // Set up the chrome://sessions/ source.
  contents->profile()->GetChromeURLDataManager()->AddDataSource(html_source);
}

// static
RefCountedMemory* SessionsUI::GetFaviconResourceBytes() {
  return ResourceBundle::GetSharedInstance().
      LoadDataResourceBytes(IDR_HISTORY_FAVICON);
}
