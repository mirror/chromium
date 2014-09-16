// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Implementation of the SafeBrowsingBlockingPage class.

#include "chrome/browser/safe_browsing/safe_browsing_blocking_page.h"

#include <string>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/i18n/rtl.h"
#include "base/lazy_instance.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/histogram.h"
#include "base/prefs/pref_service.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_piece.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "base/values.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/renderer_preferences_util.h"
#include "chrome/browser/safe_browsing/malware_details.h"
#include "chrome/browser/safe_browsing/ui_manager.h"
#include "chrome/browser/tab_contents/tab_util.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/grit/locale_settings.h"
#include "components/google/core/browser/google_util.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/interstitial_page.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/user_metrics.h"
#include "content/public/browser/web_contents.h"
#include "grit/browser_resources.h"
#include "net/base/escape.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/webui/jstemplate_builder.h"
#include "ui/base/webui/web_ui_util.h"

#if defined(ENABLE_EXTENSIONS)
#include "chrome/browser/extensions/api/experience_sampling_private/experience_sampling.h"
#endif

using base::UserMetricsAction;
using content::BrowserThread;
using content::InterstitialPage;
using content::OpenURLParams;
using content::Referrer;
using content::WebContents;

#if defined(ENABLE_EXTENSIONS)
using extensions::ExperienceSamplingEvent;
#endif

namespace {

// For malware interstitial pages, we link the problematic URL to Google's
// diagnostic page.
#if defined(GOOGLE_CHROME_BUILD)
const char* const kSbDiagnosticUrl =
    "http://safebrowsing.clients.google.com/safebrowsing/diagnostic?site=%s&client=googlechrome";
#else
const char* const kSbDiagnosticUrl =
    "http://safebrowsing.clients.google.com/safebrowsing/diagnostic?site=%s&client=chromium";
#endif

// URL for malware and phishing, V2.
const char kLearnMoreMalwareUrlV2[] =
    "https://www.google.com/transparencyreport/safebrowsing/";
const char kLearnMorePhishingUrlV2[] =
    "https://www.google.com/transparencyreport/safebrowsing/";

const char kPrivacyLinkHtml[] =
    "<a id=\"privacy-link\" href=\"\" onclick=\"sendCommand('showPrivacy'); "
    "return false;\" onmousedown=\"return false;\">%s</a>";

// After a malware interstitial where the user opted-in to the report
// but clicked "proceed anyway", we delay the call to
// MalwareDetails::FinishCollection() by this much time (in
// milliseconds).
const int64 kMalwareDetailsProceedDelayMilliSeconds = 3000;

// The commands returned by the page when the user performs an action.
const char kDoReportCommand[] = "doReport";
const char kDontReportCommand[] = "dontReport";
const char kExpandedSeeMoreCommand[] = "expandedSeeMore";
const char kLearnMoreCommand[] = "learnMore2";
const char kProceedCommand[] = "proceed";
const char kShowDiagnosticCommand[] = "showDiagnostic";
const char kShowPrivacyCommand[] = "showPrivacy";
const char kTakeMeBackCommand[] = "takeMeBack";
// Special command that we use when the user navigated away from the
// page.  E.g., closed the tab or the window.  This is only used by
// RecordUserReactionTime.
const char kNavigatedAwayMetaCommand[] = "closed";

// Other constants used to communicate with the JavaScript.
const char kBoxChecked[] = "boxchecked";
const char kDisplayCheckBox[] = "displaycheckbox";

// Constants for the Experience Sampling instrumentation.
#if defined(ENABLE_EXTENSIONS)
const char kEventNameMalware[] = "safebrowsing_interstitial_";
const char kEventNamePhishing[] = "phishing_interstitial_";
const char kEventNameMalwareAndPhishing[] =
    "malware_and_phishing_interstitial_";
const char kEventNameOther[] = "safebrowsing_other_interstitial_";
#endif

base::LazyInstance<SafeBrowsingBlockingPage::UnsafeResourceMap>
    g_unsafe_resource_map = LAZY_INSTANCE_INITIALIZER;

// This enum is used for a histogram.  Don't reorder, delete, or insert
// elements.  New elements should be added before MAX_ACTION only.
enum DetailedDecision {
  MALWARE_SHOW_NEW_SITE = 0,
  MALWARE_PROCEED_NEW_SITE,
  MALWARE_SHOW_CROSS_SITE,
  MALWARE_PROCEED_CROSS_SITE,
  PHISHING_SHOW_NEW_SITE,
  PHISHING_PROCEED_NEW_SITE,
  PHISHING_SHOW_CROSS_SITE,
  PHISHING_PROCEED_CROSS_SITE,
  MAX_DETAILED_ACTION
};

void RecordDetailedUserAction(DetailedDecision decision) {
  UMA_HISTOGRAM_ENUMERATION("SB2.InterstitialActionDetails",
                            decision,
                            MAX_DETAILED_ACTION);
}

}  // namespace

// static
SafeBrowsingBlockingPageFactory* SafeBrowsingBlockingPage::factory_ = NULL;

// The default SafeBrowsingBlockingPageFactory.  Global, made a singleton so we
// don't leak it.
class SafeBrowsingBlockingPageFactoryImpl
    : public SafeBrowsingBlockingPageFactory {
 public:
  virtual SafeBrowsingBlockingPage* CreateSafeBrowsingPage(
      SafeBrowsingUIManager* ui_manager,
      WebContents* web_contents,
      const SafeBrowsingBlockingPage::UnsafeResourceList& unsafe_resources)
      OVERRIDE {
    return new SafeBrowsingBlockingPage(ui_manager, web_contents,
        unsafe_resources);
  }

 private:
  friend struct base::DefaultLazyInstanceTraits<
      SafeBrowsingBlockingPageFactoryImpl>;

  SafeBrowsingBlockingPageFactoryImpl() { }

  DISALLOW_COPY_AND_ASSIGN(SafeBrowsingBlockingPageFactoryImpl);
};

static base::LazyInstance<SafeBrowsingBlockingPageFactoryImpl>
    g_safe_browsing_blocking_page_factory_impl = LAZY_INSTANCE_INITIALIZER;

SafeBrowsingBlockingPage::SafeBrowsingBlockingPage(
    SafeBrowsingUIManager* ui_manager,
    WebContents* web_contents,
    const UnsafeResourceList& unsafe_resources)
    : malware_details_proceed_delay_ms_(
          kMalwareDetailsProceedDelayMilliSeconds),
      ui_manager_(ui_manager),
      report_loop_(NULL),
      is_main_frame_load_blocked_(IsMainPageLoadBlocked(unsafe_resources)),
      unsafe_resources_(unsafe_resources),
      proceeded_(false),
      web_contents_(web_contents),
      url_(unsafe_resources[0].url),
      interstitial_page_(NULL),
      has_expanded_see_more_section_(false),
      create_view_(true),
      num_visits_(-1) {
  bool malware = false;
  bool phishing = false;
  for (UnsafeResourceList::const_iterator iter = unsafe_resources_.begin();
       iter != unsafe_resources_.end(); ++iter) {
    const UnsafeResource& resource = *iter;
    SBThreatType threat_type = resource.threat_type;
    if (threat_type == SB_THREAT_TYPE_URL_MALWARE ||
        threat_type == SB_THREAT_TYPE_CLIENT_SIDE_MALWARE_URL) {
      malware = true;
    } else {
      DCHECK(threat_type == SB_THREAT_TYPE_URL_PHISHING ||
             threat_type == SB_THREAT_TYPE_CLIENT_SIDE_PHISHING_URL);
      phishing = true;
    }
  }
  DCHECK(phishing || malware);
  if (malware && phishing)
    interstitial_type_ = TYPE_MALWARE_AND_PHISHING;
  else if (malware)
    interstitial_type_ = TYPE_MALWARE;
  else
    interstitial_type_ = TYPE_PHISHING;

  RecordUserAction(SHOW);
  HistoryService* history_service = HistoryServiceFactory::GetForProfile(
          Profile::FromBrowserContext(web_contents->GetBrowserContext()),
          Profile::EXPLICIT_ACCESS);
  if (history_service) {
    history_service->GetVisibleVisitCountToHost(
        url_,
        base::Bind(&SafeBrowsingBlockingPage::OnGotHistoryCount,
                   base::Unretained(this)),
        &request_tracker_);
  }

  if (!is_main_frame_load_blocked_) {
    navigation_entry_index_to_remove_ =
        web_contents->GetController().GetLastCommittedEntryIndex();
  } else {
    navigation_entry_index_to_remove_ = -1;
  }

  // Start computing malware details. They will be sent only
  // if the user opts-in on the blocking page later.
  // If there's more than one malicious resources, it means the user
  // clicked through the first warning, so we don't prepare additional
  // reports.
  if (unsafe_resources.size() == 1 &&
      unsafe_resources[0].threat_type == SB_THREAT_TYPE_URL_MALWARE &&
      malware_details_.get() == NULL && CanShowMalwareDetailsOption()) {
    malware_details_ = MalwareDetails::NewMalwareDetails(
        ui_manager_, web_contents, unsafe_resources[0]);
  }

#if defined(ENABLE_EXTENSIONS)
  // ExperienceSampling: Set up new sampling event for this interstitial.
  // This needs to handle all types of warnings this interstitial can show.
  std::string event_name;
  switch (interstitial_type_) {
    case TYPE_MALWARE_AND_PHISHING:
      event_name = kEventNameMalwareAndPhishing;
      break;
    case TYPE_MALWARE:
      event_name = kEventNameMalware;
      break;
    case TYPE_PHISHING:
      event_name = kEventNamePhishing;
      break;
    default:
      event_name = kEventNameOther;
      break;
  }
  sampling_event_.reset(new ExperienceSamplingEvent(
      event_name,
      url_,
      web_contents_->GetLastCommittedURL(),
      web_contents_->GetBrowserContext()));
#endif

  // Creating interstitial_page_ without showing it leaks memory, so don't
  // create it here.
}

bool SafeBrowsingBlockingPage::CanShowMalwareDetailsOption() {
  return (!web_contents_->GetBrowserContext()->IsOffTheRecord() &&
          web_contents_->GetURL().SchemeIs(url::kHttpScheme));
}

SafeBrowsingBlockingPage::~SafeBrowsingBlockingPage() {
}

void SafeBrowsingBlockingPage::CommandReceived(const std::string& cmd) {
  std::string command(cmd);  // Make a local copy so we can modify it.
  // The Jasonified response has quotes, remove them.
  if (command.length() > 1 && command[0] == '"') {
    command = command.substr(1, command.length() - 2);
  }
  RecordUserReactionTime(command);
  if (command == kDoReportCommand) {
    SetReportingPreference(true);
    return;
  }

  if (command == kDontReportCommand) {
    SetReportingPreference(false);
    return;
  }

  if (command == kLearnMoreCommand) {
    // User pressed "Learn more".
#if defined(ENABLE_EXTENSIONS)
    if (sampling_event_.get())
      sampling_event_->set_has_viewed_learn_more(true);
#endif
    GURL learn_more_url(interstitial_type_ == TYPE_PHISHING ?
                        kLearnMorePhishingUrlV2 : kLearnMoreMalwareUrlV2);
    learn_more_url = google_util::AppendGoogleLocaleParam(
        learn_more_url, g_browser_process->GetApplicationLocale());
    OpenURLParams params(learn_more_url,
                         Referrer(),
                         CURRENT_TAB,
                         content::PAGE_TRANSITION_LINK,
                         false);
    web_contents_->OpenURL(params);
    return;
  }

  if (command == kShowPrivacyCommand) {
    // User pressed "Safe Browsing privacy policy".
    GURL privacy_url(
        l10n_util::GetStringUTF8(IDS_SAFE_BROWSING_PRIVACY_POLICY_URL));
    privacy_url = google_util::AppendGoogleLocaleParam(
        privacy_url, g_browser_process->GetApplicationLocale());
    OpenURLParams params(privacy_url,
                         Referrer(),
                         CURRENT_TAB,
                         content::PAGE_TRANSITION_LINK,
                         false);
    web_contents_->OpenURL(params);
    return;
  }

  bool proceed_blocked = false;
  if (command == kProceedCommand) {
    if (IsPrefEnabled(prefs::kSafeBrowsingProceedAnywayDisabled)) {
      proceed_blocked = true;
    } else {
      interstitial_page_->Proceed();
      // |this| has been deleted after Proceed() returns.
      return;
    }
  }

  if (command == kTakeMeBackCommand || proceed_blocked) {
    if (is_main_frame_load_blocked_) {
      // If the load is blocked, we want to close the interstitial and discard
      // the pending entry.
      interstitial_page_->DontProceed();
      // |this| has been deleted after DontProceed() returns.
      return;
    }

    // Otherwise the offending entry has committed, and we need to go back or
    // to a safe page.  We will close the interstitial when that page commits.
    if (web_contents_->GetController().CanGoBack()) {
      web_contents_->GetController().GoBack();
    } else {
      web_contents_->GetController().LoadURL(
          GURL(chrome::kChromeUINewTabURL),
          content::Referrer(),
          content::PAGE_TRANSITION_AUTO_TOPLEVEL,
          std::string());
    }
    return;
  }

  // The "report error" and "show diagnostic" commands can have a number
  // appended to them, which is the index of the element they apply to.
  size_t element_index = 0;
  size_t colon_index = command.find(':');
  if (colon_index != std::string::npos) {
    DCHECK(colon_index < command.size() - 1);
    int result_int = 0;
    bool result = base::StringToInt(base::StringPiece(command.begin() +
                                                      colon_index + 1,
                                                      command.end()),
                                    &result_int);
    command = command.substr(0, colon_index);
    if (result)
      element_index = static_cast<size_t>(result_int);
  }

  if (element_index >= unsafe_resources_.size()) {
    NOTREACHED();
    return;
  }

  std::string bad_url_spec = unsafe_resources_[element_index].url.spec();
  if (command == kShowDiagnosticCommand) {
    // We're going to take the user to Google's SafeBrowsing diagnostic page.
    std::string diagnostic =
        base::StringPrintf(kSbDiagnosticUrl,
            net::EscapeQueryParamValue(bad_url_spec, true).c_str());
    GURL diagnostic_url(diagnostic);
    diagnostic_url = google_util::AppendGoogleLocaleParam(
        diagnostic_url, g_browser_process->GetApplicationLocale());
    DCHECK(unsafe_resources_[element_index].threat_type ==
           SB_THREAT_TYPE_URL_MALWARE ||
           unsafe_resources_[element_index].threat_type ==
           SB_THREAT_TYPE_CLIENT_SIDE_MALWARE_URL);
    OpenURLParams params(
        diagnostic_url, Referrer(), CURRENT_TAB, content::PAGE_TRANSITION_LINK,
        false);
    web_contents_->OpenURL(params);
    return;
  }

  if (command == kExpandedSeeMoreCommand) {
    // User expanded the "see more info" section of the page.  We don't actually
    // do any action based on this, it's just so that RecordUserReactionTime can
    // track it.

#if defined(ENABLE_EXTENSIONS)
    // ExperienceSampling: We track that the user expanded the details.
    if (sampling_event_.get())
      sampling_event_->set_has_viewed_details(true);
#endif
    return;
  }

  NOTREACHED() << "Unexpected command: " << command;
}

void SafeBrowsingBlockingPage::OverrideRendererPrefs(
      content::RendererPreferences* prefs) {
  Profile* profile = Profile::FromBrowserContext(
      web_contents_->GetBrowserContext());
  renderer_preferences_util::UpdateFromSystemSettings(prefs, profile);
 }

void SafeBrowsingBlockingPage::SetReportingPreference(bool report) {
  Profile* profile = Profile::FromBrowserContext(
      web_contents_->GetBrowserContext());
  PrefService* pref = profile->GetPrefs();
  pref->SetBoolean(prefs::kSafeBrowsingExtendedReportingEnabled, report);
  UMA_HISTOGRAM_BOOLEAN("SB2.SetExtendedReportingEnabled", report);
}

void SafeBrowsingBlockingPage::OnProceed() {
  proceeded_ = true;
  RecordUserAction(PROCEED);
  // Send the malware details, if we opted to.
  FinishMalwareDetails(malware_details_proceed_delay_ms_);

  NotifySafeBrowsingUIManager(ui_manager_, unsafe_resources_, true);

  // Check to see if some new notifications of unsafe resources have been
  // received while we were showing the interstitial.
  UnsafeResourceMap* unsafe_resource_map = GetUnsafeResourcesMap();
  UnsafeResourceMap::iterator iter = unsafe_resource_map->find(web_contents_);
  SafeBrowsingBlockingPage* blocking_page = NULL;
  if (iter != unsafe_resource_map->end() && !iter->second.empty()) {
    // Build an interstitial for all the unsafe resources notifications.
    // Don't show it now as showing an interstitial while an interstitial is
    // already showing would cause DontProceed() to be invoked.
    blocking_page = factory_->CreateSafeBrowsingPage(ui_manager_, web_contents_,
                                                     iter->second);
    unsafe_resource_map->erase(iter);
  }

#if defined(ENABLE_EXTENSIONS)
  // ExperienceSampling: Notify that user decided to proceed.
  if (sampling_event_.get())
    sampling_event_->CreateUserDecisionEvent(ExperienceSamplingEvent::kProceed);
#endif

  // Now that this interstitial is gone, we can show the new one.
  if (blocking_page)
    blocking_page->Show();
}

void SafeBrowsingBlockingPage::DontCreateViewForTesting() {
  create_view_ = false;
}

void SafeBrowsingBlockingPage::Show() {
  DCHECK(!interstitial_page_);
  interstitial_page_ = InterstitialPage::Create(
      web_contents_, is_main_frame_load_blocked_, url_, this);
  if (!create_view_)
    interstitial_page_->DontCreateViewForTesting();
  interstitial_page_->Show();
}

void SafeBrowsingBlockingPage::OnDontProceed() {
  // Calling this method twice will not double-count.
  RecordUserReactionTime(kNavigatedAwayMetaCommand);
  // We could have already called Proceed(), in which case we must not notify
  // the SafeBrowsingUIManager again, as the client has been deleted.
  if (proceeded_)
    return;

  RecordUserAction(DONT_PROCEED);
  // Send the malware details, if we opted to.
  FinishMalwareDetails(0);  // No delay

  NotifySafeBrowsingUIManager(ui_manager_, unsafe_resources_, false);

  // The user does not want to proceed, clear the queued unsafe resources
  // notifications we received while the interstitial was showing.
  UnsafeResourceMap* unsafe_resource_map = GetUnsafeResourcesMap();
  UnsafeResourceMap::iterator iter = unsafe_resource_map->find(web_contents_);
  if (iter != unsafe_resource_map->end() && !iter->second.empty()) {
    NotifySafeBrowsingUIManager(ui_manager_, iter->second, false);
    unsafe_resource_map->erase(iter);
  }

  // We don't remove the navigation entry if the tab is being destroyed as this
  // would trigger a navigation that would cause trouble as the render view host
  // for the tab has by then already been destroyed.  We also don't delete the
  // current entry if it has been committed again, which is possible on a page
  // that had a subresource warning.
  int last_committed_index =
      web_contents_->GetController().GetLastCommittedEntryIndex();
  if (navigation_entry_index_to_remove_ != -1 &&
      navigation_entry_index_to_remove_ != last_committed_index &&
      !web_contents_->IsBeingDestroyed()) {
    CHECK(web_contents_->GetController().RemoveEntryAtIndex(
        navigation_entry_index_to_remove_));
    navigation_entry_index_to_remove_ = -1;
  }

#if defined(ENABLE_EXTENSIONS)
  // ExperienceSampling: Notify that user decided to go back.
  // This also occurs if the user navigates away or closes the tab.
  if (sampling_event_.get())
    sampling_event_->CreateUserDecisionEvent(ExperienceSamplingEvent::kDeny);
#endif
}

void SafeBrowsingBlockingPage::OnGotHistoryCount(bool success,
                                                 int num_visits,
                                                 base::Time first_visit) {
  if (success)
    num_visits_ = num_visits;
}

void SafeBrowsingBlockingPage::RecordUserAction(BlockingPageEvent event) {
  // This enum is used for a histogram.  Don't reorder, delete, or insert
  // elements.  New elements should be added before MAX_ACTION only.
  enum {
    MALWARE_SHOW = 0,
    MALWARE_DONT_PROCEED,
    MALWARE_FORCED_DONT_PROCEED,
    MALWARE_PROCEED,
    MULTIPLE_SHOW,
    MULTIPLE_DONT_PROCEED,
    MULTIPLE_FORCED_DONT_PROCEED,
    MULTIPLE_PROCEED,
    PHISHING_SHOW,
    PHISHING_DONT_PROCEED,
    PHISHING_FORCED_DONT_PROCEED,
    PHISHING_PROCEED,
    MALWARE_SHOW_ADVANCED,
    MULTIPLE_SHOW_ADVANCED,
    PHISHING_SHOW_ADVANCED,
    MAX_ACTION
  } histogram_action = MAX_ACTION;

  switch (event) {
    case SHOW:
      switch (interstitial_type_) {
        case TYPE_MALWARE_AND_PHISHING:
          histogram_action = MULTIPLE_SHOW;
          break;
        case TYPE_MALWARE:
          histogram_action = MALWARE_SHOW;
          break;
        case TYPE_PHISHING:
          histogram_action = PHISHING_SHOW;
          break;
      }
      break;
    case PROCEED:
      switch (interstitial_type_) {
        case TYPE_MALWARE_AND_PHISHING:
          histogram_action = MULTIPLE_PROCEED;
          break;
        case TYPE_MALWARE:
          histogram_action = MALWARE_PROCEED;
          break;
        case TYPE_PHISHING:
          histogram_action = PHISHING_PROCEED;
          break;
      }
      break;
    case DONT_PROCEED:
      if (IsPrefEnabled(prefs::kSafeBrowsingProceedAnywayDisabled)) {
        switch (interstitial_type_) {
          case TYPE_MALWARE_AND_PHISHING:
            histogram_action = MULTIPLE_FORCED_DONT_PROCEED;
            break;
          case TYPE_MALWARE:
            histogram_action = MALWARE_FORCED_DONT_PROCEED;
            break;
          case TYPE_PHISHING:
            histogram_action = PHISHING_FORCED_DONT_PROCEED;
            break;
        }
      } else {
        switch (interstitial_type_) {
          case TYPE_MALWARE_AND_PHISHING:
            histogram_action = MULTIPLE_DONT_PROCEED;
            break;
          case TYPE_MALWARE:
            histogram_action = MALWARE_DONT_PROCEED;
            break;
          case TYPE_PHISHING:
            histogram_action = PHISHING_DONT_PROCEED;
            break;
        }
      }
      break;
    case SHOW_ADVANCED:
      switch (interstitial_type_) {
        case TYPE_MALWARE_AND_PHISHING:
          histogram_action = MULTIPLE_SHOW_ADVANCED;
          break;
        case TYPE_MALWARE:
          histogram_action = MALWARE_SHOW_ADVANCED;
          break;
        case TYPE_PHISHING:
          histogram_action = PHISHING_SHOW_ADVANCED;
          break;
      }
      break;
    default:
      NOTREACHED() << "Unexpected event: " << event;
  }
  if (histogram_action == MAX_ACTION) {
    NOTREACHED();
  } else {
    UMA_HISTOGRAM_ENUMERATION("SB2.InterstitialAction", histogram_action,
                              MAX_ACTION);
  }

  if (event == PROCEED || event == DONT_PROCEED) {
    if (num_visits_ == 0 && interstitial_type_ != TYPE_MALWARE_AND_PHISHING) {
      RecordDetailedUserAction((interstitial_type_ == TYPE_MALWARE) ?
                               MALWARE_SHOW_NEW_SITE : PHISHING_SHOW_NEW_SITE);
      if (event == PROCEED) {
        RecordDetailedUserAction((interstitial_type_ == TYPE_MALWARE) ?
            MALWARE_PROCEED_NEW_SITE : PHISHING_PROCEED_NEW_SITE);
      }
    }
    if (unsafe_resources_[0].is_subresource &&
        interstitial_type_ != TYPE_MALWARE_AND_PHISHING) {
      RecordDetailedUserAction((interstitial_type_ == TYPE_MALWARE) ?
          MALWARE_SHOW_CROSS_SITE : PHISHING_SHOW_CROSS_SITE);
      if (event == PROCEED) {
        RecordDetailedUserAction((interstitial_type_ == TYPE_MALWARE) ?
            MALWARE_PROCEED_CROSS_SITE : PHISHING_PROCEED_CROSS_SITE);
      }
    }
  }
}

void SafeBrowsingBlockingPage::RecordUserReactionTime(
    const std::string& command) {
  if (interstitial_show_time_.is_null())
    return;  // We already reported the user reaction time.
  base::TimeDelta dt = base::TimeTicks::Now() - interstitial_show_time_;
  DVLOG(1) << "User reaction time for command:" << command
           << " on interstitial_type_:" << interstitial_type_
           << " warning took " << dt.InMilliseconds() << "ms";
  bool recorded = true;
  if (interstitial_type_ == TYPE_MALWARE ||
      interstitial_type_ == TYPE_MALWARE_AND_PHISHING) {
    // There are six ways in which the malware interstitial can go
    // away.  We handle all of them here but we group two together: closing the
    // tag / browser window and clicking on the back button in the browser (not
    // the big green button) are considered the same action.
    if (command == kProceedCommand) {
      UMA_HISTOGRAM_MEDIUM_TIMES("SB2.MalwareInterstitialTimeProceed", dt);
    } else if (command == kTakeMeBackCommand) {
      UMA_HISTOGRAM_MEDIUM_TIMES("SB2.MalwareInterstitialTimeTakeMeBack", dt);
    } else if (command == kShowDiagnosticCommand) {
      UMA_HISTOGRAM_MEDIUM_TIMES("SB2.MalwareInterstitialTimeDiagnostic", dt);
    } else if (command == kShowPrivacyCommand) {
      UMA_HISTOGRAM_MEDIUM_TIMES("SB2.MalwareInterstitialTimePrivacyPolicy",
                                 dt);
    } else if (command == kLearnMoreCommand) {
      UMA_HISTOGRAM_MEDIUM_TIMES("SB2.MalwareInterstitialLearnMore",
                                 dt);
    } else if (command == kNavigatedAwayMetaCommand) {
      UMA_HISTOGRAM_MEDIUM_TIMES("SB2.MalwareInterstitialTimeClosed", dt);
    } else if (command == kExpandedSeeMoreCommand) {
      // Only record the expanded histogram once per display of the
      // interstitial.
      if (has_expanded_see_more_section_)
        return;
      RecordUserAction(SHOW_ADVANCED);
      UMA_HISTOGRAM_MEDIUM_TIMES("SB2.MalwareInterstitialTimeExpandedSeeMore",
                                 dt);
      has_expanded_see_more_section_ = true;
      // Expanding the "See More" section doesn't finish the interstitial, so
      // don't mark the reaction time as recorded.
      recorded = false;
    } else {
      recorded = false;
    }
  } else {
    // Same as above but for phishing warnings.
    if (command == kProceedCommand) {
      UMA_HISTOGRAM_MEDIUM_TIMES("SB2.PhishingInterstitialTimeProceed", dt);
    } else if (command == kTakeMeBackCommand) {
      UMA_HISTOGRAM_MEDIUM_TIMES("SB2.PhishingInterstitialTimeTakeMeBack", dt);
    } else if (command == kShowDiagnosticCommand) {
      UMA_HISTOGRAM_MEDIUM_TIMES("SB2.PhishingInterstitialTimeReportError", dt);
    } else if (command == kLearnMoreCommand) {
      UMA_HISTOGRAM_MEDIUM_TIMES("SB2.PhishingInterstitialTimeLearnMore", dt);
    } else if (command == kNavigatedAwayMetaCommand) {
      UMA_HISTOGRAM_MEDIUM_TIMES("SB2.PhishingInterstitialTimeClosed", dt);
    } else if (command == kExpandedSeeMoreCommand) {
      // Only record the expanded histogram once per display of the
      // interstitial.
      if (has_expanded_see_more_section_)
        return;
      RecordUserAction(SHOW_ADVANCED);
      UMA_HISTOGRAM_MEDIUM_TIMES("SB2.PhishingInterstitialTimeExpandedSeeMore",
                                 dt);
      has_expanded_see_more_section_ = true;
      // Expanding the "See More" section doesn't finish the interstitial, so
      // don't mark the reaction time as recorded.
      recorded = false;
    } else {
      recorded = false;
    }
  }
  if (recorded)  // Making sure we don't double-count reaction times.
    interstitial_show_time_ = base::TimeTicks();  //  Resets the show time.
}

void SafeBrowsingBlockingPage::FinishMalwareDetails(int64 delay_ms) {
  if (malware_details_.get() == NULL)
    return;  // Not all interstitials have malware details (eg phishing).

  const bool enabled =
      IsPrefEnabled(prefs::kSafeBrowsingExtendedReportingEnabled);
  UMA_HISTOGRAM_BOOLEAN("SB2.ExtendedReportingIsEnabled", enabled);
  if (enabled) {
    // Finish the malware details collection, send it over.
    BrowserThread::PostDelayedTask(
        BrowserThread::IO, FROM_HERE,
        base::Bind(&MalwareDetails::FinishCollection, malware_details_.get()),
        base::TimeDelta::FromMilliseconds(delay_ms));
  }
}

bool SafeBrowsingBlockingPage::IsPrefEnabled(const char* pref) {
  Profile* profile =
      Profile::FromBrowserContext(web_contents_->GetBrowserContext());
  return profile->GetPrefs()->GetBoolean(pref);
}

// static
void SafeBrowsingBlockingPage::NotifySafeBrowsingUIManager(
    SafeBrowsingUIManager* ui_manager,
    const UnsafeResourceList& unsafe_resources,
    bool proceed) {
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&SafeBrowsingUIManager::OnBlockingPageDone,
                 ui_manager, unsafe_resources, proceed));
}

// static
SafeBrowsingBlockingPage::UnsafeResourceMap*
    SafeBrowsingBlockingPage::GetUnsafeResourcesMap() {
  return g_unsafe_resource_map.Pointer();
}

// static
SafeBrowsingBlockingPage* SafeBrowsingBlockingPage::CreateBlockingPage(
    SafeBrowsingUIManager* ui_manager,
    WebContents* web_contents,
    const UnsafeResource& unsafe_resource) {
  std::vector<UnsafeResource> resources;
  resources.push_back(unsafe_resource);
  // Set up the factory if this has not been done already (tests do that
  // before this method is called).
  if (!factory_)
    factory_ = g_safe_browsing_blocking_page_factory_impl.Pointer();
  return factory_->CreateSafeBrowsingPage(ui_manager, web_contents, resources);
}

// static
void SafeBrowsingBlockingPage::ShowBlockingPage(
    SafeBrowsingUIManager* ui_manager,
    const UnsafeResource& unsafe_resource) {
  DVLOG(1) << __FUNCTION__ << " " << unsafe_resource.url.spec();
  WebContents* web_contents = tab_util::GetWebContentsByID(
      unsafe_resource.render_process_host_id, unsafe_resource.render_view_id);

  InterstitialPage* interstitial =
      InterstitialPage::GetInterstitialPage(web_contents);
  if (interstitial && !unsafe_resource.is_subresource) {
    // There is already an interstitial showing and we are about to display a
    // new one for the main frame. Just hide the current one, it is now
    // irrelevent
    interstitial->DontProceed();
    interstitial = NULL;
  }

  if (!interstitial) {
    // There are no interstitial currently showing in that tab, go ahead and
    // show this interstitial.
    SafeBrowsingBlockingPage* blocking_page =
        CreateBlockingPage(ui_manager, web_contents, unsafe_resource);
    blocking_page->Show();
    return;
  }

  // This is an interstitial for a page's resource, let's queue it.
  UnsafeResourceMap* unsafe_resource_map = GetUnsafeResourcesMap();
  (*unsafe_resource_map)[web_contents].push_back(unsafe_resource);
}

// static
bool SafeBrowsingBlockingPage::IsMainPageLoadBlocked(
    const UnsafeResourceList& unsafe_resources) {
  // Client-side phishing detection interstitials never block the main frame
  // load, since they happen after the page is finished loading.
  if (unsafe_resources[0].threat_type ==
      SB_THREAT_TYPE_CLIENT_SIDE_PHISHING_URL) {
    return false;
  }

  // Otherwise, check the threat type.
  return unsafe_resources.size() == 1 && !unsafe_resources[0].is_subresource;
}

std::string SafeBrowsingBlockingPage::GetHTMLContents() {
  DCHECK(!unsafe_resources_.empty());

  // Fill in the shared values.
  base::DictionaryValue load_time_data;
  webui::SetFontAndTextDirection(&load_time_data);
  load_time_data.SetBoolean("ssl", false);
  load_time_data.SetString(
      "tabTitle", l10n_util::GetStringUTF16(IDS_SAFEBROWSING_V3_TITLE));
  load_time_data.SetString(
      "openDetails",
      l10n_util::GetStringUTF16(IDS_SAFEBROWSING_V3_OPEN_DETAILS_BUTTON));
  load_time_data.SetString(
      "closeDetails",
      l10n_util::GetStringUTF16(IDS_SAFEBROWSING_V3_CLOSE_DETAILS_BUTTON));
  load_time_data.SetString(
      "primaryButtonText",
      l10n_util::GetStringUTF16(IDS_SAFEBROWSING_OVERRIDABLE_SAFETY_BUTTON));
  load_time_data.SetBoolean(
      "overridable",
      !IsPrefEnabled(prefs::kSafeBrowsingProceedAnywayDisabled));

  if (interstitial_type_ == TYPE_PHISHING)
    PopulatePhishingLoadTimeData(&load_time_data);
  else
    PopulateMalwareLoadTimeData(&load_time_data);

  interstitial_show_time_ = base::TimeTicks::Now();

  base::StringPiece html(
      ResourceBundle::GetSharedInstance().GetRawDataResource(
          IRD_SECURITY_INTERSTITIAL_HTML));
  webui::UseVersion2 version;
  return webui::GetI18nTemplateHtml(html, &load_time_data);
}

void SafeBrowsingBlockingPage::PopulateMalwareLoadTimeData(
    base::DictionaryValue* load_time_data) {
  load_time_data->SetBoolean("phishing", false);
  load_time_data->SetString(
      "heading", l10n_util::GetStringUTF16(IDS_MALWARE_V3_HEADING));
  load_time_data->SetString(
      "primaryParagraph",
      l10n_util::GetStringFUTF16(
          IDS_MALWARE_V3_PRIMARY_PARAGRAPH,
          base::UTF8ToUTF16(url_.host())));
  load_time_data->SetString(
      "explanationParagraph",
      is_main_frame_load_blocked_ ?
          l10n_util::GetStringFUTF16(
              IDS_MALWARE_V3_EXPLANATION_PARAGRAPH,
              base::UTF8ToUTF16(url_.host())) :
          l10n_util::GetStringFUTF16(
              IDS_MALWARE_V3_EXPLANATION_PARAGRAPH_SUBRESOURCE,
              base::UTF8ToUTF16(web_contents_->GetURL().host()),
              base::UTF8ToUTF16(url_.host())));
  load_time_data->SetString(
      "finalParagraph",
      l10n_util::GetStringUTF16(IDS_MALWARE_V3_PROCEED_PARAGRAPH));

  load_time_data->SetBoolean(kDisplayCheckBox, CanShowMalwareDetailsOption());
  if (CanShowMalwareDetailsOption()) {
    std::string privacy_link = base::StringPrintf(
        kPrivacyLinkHtml,
        l10n_util::GetStringUTF8(
            IDS_SAFE_BROWSING_PRIVACY_POLICY_PAGE).c_str());
    load_time_data->SetString(
        "optInLink",
        l10n_util::GetStringFUTF16(IDS_SAFE_BROWSING_MALWARE_REPORTING_AGREE,
                                   base::UTF8ToUTF16(privacy_link)));
    load_time_data->SetBoolean(
        kBoxChecked,
        IsPrefEnabled(prefs::kSafeBrowsingExtendedReportingEnabled));
  }
}

void SafeBrowsingBlockingPage::PopulatePhishingLoadTimeData(
    base::DictionaryValue* load_time_data) {
  load_time_data->SetBoolean("phishing", true);
  load_time_data->SetString(
      "heading",
      l10n_util::GetStringUTF16(IDS_PHISHING_V3_HEADING));
  load_time_data->SetString(
      "primaryParagraph",
      l10n_util::GetStringFUTF16(
          IDS_PHISHING_V3_PRIMARY_PARAGRAPH,
          base::UTF8ToUTF16(url_.host())));
  load_time_data->SetString(
      "explanationParagraph",
      l10n_util::GetStringFUTF16(IDS_PHISHING_V3_EXPLANATION_PARAGRAPH,
                                 base::UTF8ToUTF16(url_.host())));
  load_time_data->SetString(
      "finalParagraph",
      l10n_util::GetStringUTF16(IDS_PHISHING_V3_PROCEED_PARAGRAPH));
}
