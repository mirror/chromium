// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ntp_snippets/ntp_snippets_service.h"

#include <algorithm>
#include <iterator>
#include <utility>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/location.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/sparse_histogram.h"
#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"
#include "base/task_runner_util.h"
#include "base/time/time.h"
#include "base/values.h"
#include "components/image_fetcher/image_fetcher.h"
#include "components/ntp_snippets/ntp_snippets_constants.h"
#include "components/ntp_snippets/ntp_snippets_database.h"
#include "components/ntp_snippets/pref_names.h"
#include "components/ntp_snippets/switches.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/suggestions/proto/suggestions.pb.h"
#include "components/sync_driver/sync_service.h"
#include "components/variations/variations_associated_data.h"
#include "ui/gfx/image/image.h"

using image_fetcher::ImageFetcher;
using suggestions::ChromeSuggestion;
using suggestions::SuggestionsProfile;
using suggestions::SuggestionsService;

namespace ntp_snippets {

namespace {

// Number of snippets requested to the server. Consider replacing sparse UMA
// histograms with COUNTS() if this number increases beyond 50.
const int kMaxSnippetCount = 10;

// Default values for snippets fetching intervals.
const int kDefaultFetchingIntervalWifiChargingSeconds = 30 * 60;
const int kDefaultFetchingIntervalWifiSeconds = 2 * 60 * 60;
const int kDefaultFetchingIntervalFallbackSeconds = 24 * 60 * 60;

// Variation parameters than can override the default fetching intervals.
const char kFetchingIntervalWifiChargingParamName[] =
    "fetching_interval_wifi_charging_seconds";
const char kFetchingIntervalWifiParamName[] =
    "fetching_interval_wifi_seconds";
const char kFetchingIntervalFallbackParamName[] =
    "fetching_interval_fallback_seconds";

// These define the times of day during which we will fetch via Wifi (without
// charging) - 6 AM to 10 PM.
const int kWifiFetchingHourMin = 6;
const int kWifiFetchingHourMax = 22;

const int kDefaultExpiryTimeMins = 24 * 60;

base::TimeDelta GetFetchingInterval(const char* switch_name,
                                    const char* param_name,
                                    int default_value_seconds) {
  int value_seconds = default_value_seconds;

  // The default value can be overridden by a variation parameter.
  // TODO(treib,jkrcal): Use GetVariationParamValueByFeature and get rid of
  // kStudyName, also in NTPSnippetsFetcher.
  std::string param_value_str = variations::GetVariationParamValue(
        ntp_snippets::kStudyName, param_name);
  if (!param_value_str.empty()) {
    int param_value_seconds = 0;
    if (base::StringToInt(param_value_str, &param_value_seconds))
      value_seconds = param_value_seconds;
    else
      LOG(WARNING) << "Invalid value for variation parameter " << param_name;
  }

  // A value from the command line parameter overrides anything else.
  const base::CommandLine& cmdline = *base::CommandLine::ForCurrentProcess();
  if (cmdline.HasSwitch(switch_name)) {
    std::string str = cmdline.GetSwitchValueASCII(switch_name);
    int switch_value_seconds = 0;
    if (base::StringToInt(str, &switch_value_seconds))
      value_seconds = switch_value_seconds;
    else
      LOG(WARNING) << "Invalid value for switch " << switch_name;
  }
  return base::TimeDelta::FromSeconds(value_seconds);
}

base::TimeDelta GetFetchingIntervalWifiCharging() {
  return GetFetchingInterval(switches::kFetchingIntervalWifiChargingSeconds,
                             kFetchingIntervalWifiChargingParamName,
                             kDefaultFetchingIntervalWifiChargingSeconds);
}

base::TimeDelta GetFetchingIntervalWifi(const base::Time& now) {
  // Only fetch via Wifi (without charging) during the proper times of day.
  base::Time::Exploded exploded;
  now.LocalExplode(&exploded);
  if (kWifiFetchingHourMin <= exploded.hour &&
      exploded.hour < kWifiFetchingHourMax) {
    return GetFetchingInterval(switches::kFetchingIntervalWifiSeconds,
                               kFetchingIntervalWifiParamName,
                               kDefaultFetchingIntervalWifiSeconds);
  }
  return base::TimeDelta();
}

base::TimeDelta GetFetchingIntervalFallback() {
  return GetFetchingInterval(switches::kFetchingIntervalFallbackSeconds,
                             kFetchingIntervalFallbackParamName,
                             kDefaultFetchingIntervalFallbackSeconds);
}

base::Time GetRescheduleTime(const base::Time& now) {
  base::Time::Exploded exploded;
  now.LocalExplode(&exploded);
  // The scheduling changes at both |kWifiFetchingHourMin| and
  // |kWifiFetchingHourMax|. Find the time of the next one that we'll hit.
  bool next_day = false;
  if (exploded.hour < kWifiFetchingHourMin) {
    exploded.hour = kWifiFetchingHourMin;
  } else if (exploded.hour < kWifiFetchingHourMax) {
    exploded.hour = kWifiFetchingHourMax;
  } else {
    next_day = true;
    exploded.hour = kWifiFetchingHourMin;
  }
  // In any case, reschedule at the full hour.
  exploded.minute = 0;
  exploded.second = 0;
  exploded.millisecond = 0;
  base::Time reschedule = base::Time::FromLocalExploded(exploded);
  if (next_day)
    reschedule += base::TimeDelta::FromDays(1);

  return reschedule;
}

// Extracts the hosts from |suggestions| and returns them in a set.
std::set<std::string> GetSuggestionsHostsImpl(
    const SuggestionsProfile& suggestions) {
  std::set<std::string> hosts;
  for (int i = 0; i < suggestions.suggestions_size(); ++i) {
    const ChromeSuggestion& suggestion = suggestions.suggestions(i);
    GURL url(suggestion.url());
    if (url.is_valid())
      hosts.insert(url.host());
  }
  return hosts;
}

void InsertAllIDs(const NTPSnippet::PtrVector& snippets,
                  std::set<std::string>* ids) {
  for (const std::unique_ptr<NTPSnippet>& snippet : snippets) {
    ids->insert(snippet->id());
    for (const SnippetSource& source : snippet->sources())
      ids->insert(source.url.spec());
  }
}

void Compact(NTPSnippet::PtrVector* snippets) {
  snippets->erase(
      std::remove_if(
          snippets->begin(), snippets->end(),
          [](const std::unique_ptr<NTPSnippet>& snippet) { return !snippet; }),
      snippets->end());
}

}  // namespace

NTPSnippetsService::NTPSnippetsService(
    bool enabled,
    PrefService* pref_service,
    sync_driver::SyncService* sync_service,
    SuggestionsService* suggestions_service,
    const std::string& application_language_code,
    NTPSnippetsScheduler* scheduler,
    std::unique_ptr<NTPSnippetsFetcher> snippets_fetcher,
    std::unique_ptr<ImageFetcher> image_fetcher,
    std::unique_ptr<NTPSnippetsDatabase> database)
    : state_(State::NOT_INITED),
      explicitly_disabled_(!enabled),
      pref_service_(pref_service),
      sync_service_(sync_service),
      sync_service_observer_(this),
      suggestions_service_(suggestions_service),
      application_language_code_(application_language_code),
      scheduler_(scheduler),
      snippets_fetcher_(std::move(snippets_fetcher)),
      image_fetcher_(std::move(image_fetcher)),
      database_(std::move(database)),
      fetch_after_load_(false) {
  // TODO(dgn) should be removed after branch point (https:://crbug.com/617585).
  ClearDeprecatedPrefs();

  if (explicitly_disabled_) {
    EnterState(State::DISABLED);
    return;
  }

  // We transition to other states while finalizing the initialization, when the
  // database is done loading.
  database_->Load(base::Bind(&NTPSnippetsService::OnDatabaseLoaded,
                             base::Unretained(this)));
}

NTPSnippetsService::~NTPSnippetsService() {
  DCHECK(state_ == State::SHUT_DOWN);
}

// static
void NTPSnippetsService::RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterListPref(prefs::kDeprecatedSnippets);
  registry->RegisterListPref(prefs::kDeprecatedDiscardedSnippets);
  registry->RegisterListPref(prefs::kSnippetHosts);
}

// Inherited from KeyedService.
void NTPSnippetsService::Shutdown() {
  EnterState(State::SHUT_DOWN);
}

void NTPSnippetsService::FetchSnippets() {
  if (ready())
    FetchSnippetsFromHosts(GetSuggestionsHosts());
  else
    fetch_after_load_ = true;
}

void NTPSnippetsService::FetchSnippetsFromHosts(
    const std::set<std::string>& hosts) {
  if (!ready())
    return;
  snippets_fetcher_->FetchSnippetsFromHosts(hosts, application_language_code_,
                                            kMaxSnippetCount);
}

void NTPSnippetsService::RescheduleFetching() {
  // The scheduler only exists on Android so far, it's null on other platforms.
  if (!scheduler_)
    return;

  if (ready()) {
    base::Time now = base::Time::Now();
    scheduler_->Schedule(
        GetFetchingIntervalWifiCharging(), GetFetchingIntervalWifi(now),
        GetFetchingIntervalFallback(), GetRescheduleTime(now));
  } else {
    scheduler_->Unschedule();
  }
}

void NTPSnippetsService::FetchSnippetImage(
    const std::string& snippet_id,
    const ImageFetchedCallback& callback) {
  auto it =
      std::find_if(snippets_.begin(), snippets_.end(),
                   [&snippet_id](const std::unique_ptr<NTPSnippet>& snippet) {
                     return snippet->id() == snippet_id;
                   });
  if (it == snippets_.end()) {
    gfx::Image empty_image;
    callback.Run(snippet_id, empty_image);
    return;
  }

  const NTPSnippet& snippet = *it->get();
  image_fetcher_->StartOrQueueNetworkRequest(
      snippet.id(), snippet.salient_image_url(), callback);
  // TODO(treib): Cache/persist the snippet image.
}

void NTPSnippetsService::ClearSnippets() {
  if (!initialized())
    return;

  if (snippets_.empty())
    return;

  database_->Delete(snippets_);
  snippets_.clear();

  FOR_EACH_OBSERVER(NTPSnippetsServiceObserver, observers_,
                    NTPSnippetsServiceLoaded());
}

std::set<std::string> NTPSnippetsService::GetSuggestionsHosts() const {
  // |suggestions_service_| can be null in tests.
  if (!suggestions_service_)
    return std::set<std::string>();

  // TODO(treib): This should just call GetSnippetHostsFromPrefs.
  return GetSuggestionsHostsImpl(
      suggestions_service_->GetSuggestionsDataFromCache());
}

bool NTPSnippetsService::DiscardSnippet(const std::string& snippet_id) {
  if (!ready())
    return false;

  auto it =
      std::find_if(snippets_.begin(), snippets_.end(),
                   [&snippet_id](const std::unique_ptr<NTPSnippet>& snippet) {
                     return snippet->id() == snippet_id;
                   });
  if (it == snippets_.end())
    return false;

  (*it)->set_discarded(true);

  database_->Save(**it);

  discarded_snippets_.push_back(std::move(*it));
  snippets_.erase(it);

  FOR_EACH_OBSERVER(NTPSnippetsServiceObserver, observers_,
                    NTPSnippetsServiceLoaded());
  return true;
}

void NTPSnippetsService::ClearDiscardedSnippets() {
  if (!initialized())
    return;

  database_->Delete(discarded_snippets_);
  discarded_snippets_.clear();
}

void NTPSnippetsService::AddObserver(NTPSnippetsServiceObserver* observer) {
  observers_.AddObserver(observer);
}

void NTPSnippetsService::RemoveObserver(NTPSnippetsServiceObserver* observer) {
  observers_.RemoveObserver(observer);
}

DisabledReason NTPSnippetsService::GetDisabledReason() const {
  if (explicitly_disabled_)
    return DisabledReason::EXPLICITLY_DISABLED;

  if (!sync_service_ || !sync_service_->CanSyncStart()) {
    DVLOG(1) << "[GetDisabledReason] Sync disabled";
    return DisabledReason::HISTORY_SYNC_DISABLED;
  }

  // !IsSyncActive in cases where CanSyncStart is true hints at the backend not
  // being initialized.
  // ConfigurationDone() verifies that the sync service has properly loaded its
  // configuration and is aware of the different data types to sync.
  if (!sync_service_->IsSyncActive() || !sync_service_->ConfigurationDone()) {
    DVLOG(1) << "[GetDisabledReason] Sync initialization is not complete.";
    return DisabledReason::HISTORY_SYNC_STATE_UNKNOWN;
  }

  if (!sync_service_->GetActiveDataTypes().Has(
          syncer::HISTORY_DELETE_DIRECTIVES)) {
    DVLOG(1) << "[GetDisabledReason] History sync disabled";
    return DisabledReason::HISTORY_SYNC_DISABLED;
  }

  DVLOG(1) << "[GetDisabledReason] Enabled";
  return DisabledReason::NONE;
}

// static
int NTPSnippetsService::GetMaxSnippetCountForTesting() {
  return kMaxSnippetCount;
}

////////////////////////////////////////////////////////////////////////////////
// Private methods

void NTPSnippetsService::OnStateChanged() {
  if (state_ == State::SHUT_DOWN)
    return;

  DVLOG(1) << "[OnStateChanged]";
  EnterState(GetStateForDependenciesStatus());
}

void NTPSnippetsService::OnDatabaseLoaded(NTPSnippet::PtrVector snippets) {
  DCHECK(state_ == State::NOT_INITED || state_ == State::SHUT_DOWN);
  if (state_ == State::SHUT_DOWN)
    return;

  DCHECK(snippets_.empty());
  DCHECK(discarded_snippets_.empty());
  for (std::unique_ptr<NTPSnippet>& snippet : snippets) {
    if (snippet->is_discarded())
      discarded_snippets_.emplace_back(std::move(snippet));
    else
      snippets_.emplace_back(std::move(snippet));
  }
  std::sort(snippets_.begin(), snippets_.end(),
            [](const std::unique_ptr<NTPSnippet>& lhs,
               const std::unique_ptr<NTPSnippet>& rhs) {
              return lhs->score() > rhs->score();
            });

  ClearExpiredSnippets();
  FinishInitialization();
}

void NTPSnippetsService::OnSuggestionsChanged(
    const SuggestionsProfile& suggestions) {
  DCHECK(initialized());

  std::set<std::string> hosts = GetSuggestionsHostsImpl(suggestions);
  if (hosts == GetSnippetHostsFromPrefs())
    return;

  // Remove existing snippets that aren't in the suggestions anymore.
  // TODO(treib,maybelle): If there is another source with an allowed host,
  // then we should fall back to that.
  // First, move them over into |to_delete|.
  NTPSnippet::PtrVector to_delete;
  for (std::unique_ptr<NTPSnippet>& snippet : snippets_) {
    if (!hosts.count(snippet->best_source().url.host()))
      to_delete.emplace_back(std::move(snippet));
  }
  Compact(&snippets_);
  // Then delete the removed snippets from the database.
  database_->Delete(to_delete);

  StoreSnippetHostsToPrefs(hosts);

  FOR_EACH_OBSERVER(NTPSnippetsServiceObserver, observers_,
                    NTPSnippetsServiceLoaded());

  FetchSnippetsFromHosts(hosts);
}

void NTPSnippetsService::OnFetchFinished(
    NTPSnippetsFetcher::OptionalSnippets snippets) {
  if (!ready())
    return;

  if (snippets) {
    // Sparse histogram used because the number of snippets is small (bound by
    // kMaxSnippetCount).
    DCHECK_LE(snippets->size(), static_cast<size_t>(kMaxSnippetCount));
    UMA_HISTOGRAM_SPARSE_SLOWLY("NewTabPage.Snippets.NumArticlesFetched",
                                snippets->size());
    MergeSnippets(std::move(*snippets));
  }

  ClearExpiredSnippets();

  // If there are still more snippets than we want to show, move the extra ones
  // over into |to_delete|.
  NTPSnippet::PtrVector to_delete;
  if (snippets_.size() > kMaxSnippetCount) {
    to_delete.insert(
        to_delete.end(),
        std::make_move_iterator(snippets_.begin() + kMaxSnippetCount),
                     std::make_move_iterator(snippets_.end()));
    snippets_.resize(kMaxSnippetCount);
  }
  database_->Delete(to_delete);

  UMA_HISTOGRAM_SPARSE_SLOWLY("NewTabPage.Snippets.NumArticles",
                              snippets_.size());
  if (snippets_.empty() && !discarded_snippets_.empty()) {
    UMA_HISTOGRAM_COUNTS("NewTabPage.Snippets.NumArticlesZeroDueToDiscarded",
                         discarded_snippets_.size());
  }

  FOR_EACH_OBSERVER(NTPSnippetsServiceObserver, observers_,
                    NTPSnippetsServiceLoaded());
}

void NTPSnippetsService::MergeSnippets(NTPSnippet::PtrVector new_snippets) {
  DCHECK(ready());

  // Remove new snippets that we already have, or that have been discarded.
  std::set<std::string> old_snippet_ids;
  InsertAllIDs(discarded_snippets_, &old_snippet_ids);
  InsertAllIDs(snippets_, &old_snippet_ids);
  new_snippets.erase(
      std::remove_if(
          new_snippets.begin(), new_snippets.end(),
          [&old_snippet_ids](const std::unique_ptr<NTPSnippet>& snippet) {
            if (old_snippet_ids.count(snippet->id()))
              return true;
            for (const SnippetSource& source : snippet->sources()) {
              if (old_snippet_ids.count(source.url.spec()))
                return true;
            }
            return false;
          }),
      new_snippets.end());

  // Fill in default publish/expiry dates where required.
  for (std::unique_ptr<NTPSnippet>& snippet : new_snippets) {
    if (snippet->publish_date().is_null())
      snippet->set_publish_date(base::Time::Now());
    if (snippet->expiry_date().is_null()) {
      snippet->set_expiry_date(
          snippet->publish_date() +
          base::TimeDelta::FromMinutes(kDefaultExpiryTimeMins));
    }

    // TODO(treib): Prefetch and cache the snippet image. crbug.com/605870
  }

  if (!base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kAddIncompleteSnippets)) {
    int num_new_snippets = new_snippets.size();
    // Remove snippets that do not have all the info we need to display it to
    // the user.
    new_snippets.erase(
        std::remove_if(new_snippets.begin(), new_snippets.end(),
                       [](const std::unique_ptr<NTPSnippet>& snippet) {
                         return !snippet->is_complete();
                       }),
        new_snippets.end());
    int num_snippets_discarded = num_new_snippets - new_snippets.size();
    UMA_HISTOGRAM_BOOLEAN("NewTabPage.Snippets.IncompleteSnippetsAfterFetch",
                          num_snippets_discarded > 0);
    if (num_snippets_discarded > 0) {
      UMA_HISTOGRAM_SPARSE_SLOWLY("NewTabPage.Snippets.NumIncompleteSnippets",
                                  num_snippets_discarded);
    }
  }

  // Save the new snippets to the DB.
  database_->Save(new_snippets);

  // Insert the new snippets at the front.
  snippets_.insert(snippets_.begin(),
                   std::make_move_iterator(new_snippets.begin()),
                   std::make_move_iterator(new_snippets.end()));
}

std::set<std::string> NTPSnippetsService::GetSnippetHostsFromPrefs() const {
  std::set<std::string> hosts;
  const base::ListValue* list = pref_service_->GetList(prefs::kSnippetHosts);
  for (const auto& value : *list) {
    std::string str;
    bool success = value->GetAsString(&str);
    DCHECK(success) << "Failed to parse snippet host from prefs";
    hosts.insert(std::move(str));
  }
  return hosts;
}

void NTPSnippetsService::StoreSnippetHostsToPrefs(
    const std::set<std::string>& hosts) {
  base::ListValue list;
  for (const std::string& host : hosts)
    list.AppendString(host);
  pref_service_->Set(prefs::kSnippetHosts, list);
}

void NTPSnippetsService::ClearExpiredSnippets() {
  base::Time expiry = base::Time::Now();

  // Move expired snippets over into |to_delete|.
  NTPSnippet::PtrVector to_delete;
  for (std::unique_ptr<NTPSnippet>& snippet : snippets_) {
    if (snippet->expiry_date() <= expiry)
      to_delete.emplace_back(std::move(snippet));
  }
  Compact(&snippets_);

  // Move expired discarded snippets over into |to_delete| as well.
  for (std::unique_ptr<NTPSnippet>& snippet : discarded_snippets_) {
    if (snippet->expiry_date() <= expiry)
      to_delete.emplace_back(std::move(snippet));
  }
  Compact(&discarded_snippets_);

  // Finally, actually delete the removed snippets from the DB.
  database_->Delete(to_delete);

  // If there are any snippets left, schedule a timer for the next expiry.
  if (snippets_.empty() && discarded_snippets_.empty())
    return;

  base::Time next_expiry = base::Time::Max();
  for (const auto& snippet : snippets_) {
    if (snippet->expiry_date() < next_expiry)
      next_expiry = snippet->expiry_date();
  }
  for (const auto& snippet : discarded_snippets_) {
    if (snippet->expiry_date() < next_expiry)
      next_expiry = snippet->expiry_date();
  }
  DCHECK_GT(next_expiry, expiry);
  expiry_timer_.Start(FROM_HERE, next_expiry - expiry,
                      base::Bind(&NTPSnippetsService::ClearExpiredSnippets,
                                 base::Unretained(this)));
}

void NTPSnippetsService::EnterStateEnabled(bool fetch_snippets) {
  if (fetch_snippets)
    FetchSnippets();

  // If host restrictions are enabled, register for host list updates.
  // |suggestions_service_| can be null in tests.
  if (snippets_fetcher_->UsesHostRestrictions() && suggestions_service_) {
    suggestions_service_subscription_ =
        suggestions_service_->AddCallback(base::Bind(
            &NTPSnippetsService::OnSuggestionsChanged, base::Unretained(this)));
  }

  RescheduleFetching();
}

void NTPSnippetsService::EnterStateDisabled() {
  ClearSnippets();
  ClearDiscardedSnippets();

  suggestions_service_subscription_.reset();
  expiry_timer_.Stop();

  RescheduleFetching();
  FOR_EACH_OBSERVER(NTPSnippetsServiceObserver, observers_,
                    NTPSnippetsServiceDisabled());
}

void NTPSnippetsService::EnterStateShutdown() {
  FOR_EACH_OBSERVER(NTPSnippetsServiceObserver, observers_,
                    NTPSnippetsServiceShutdown());

  expiry_timer_.Stop();
  suggestions_service_subscription_.reset();

  if (sync_service_)
    sync_service_observer_.Remove(sync_service_);
}

void NTPSnippetsService::FinishInitialization() {
  snippets_fetcher_->SetCallback(
      base::Bind(&NTPSnippetsService::OnFetchFinished, base::Unretained(this)));

  // |sync_service_| can be null in tests or if sync is disabled.
  // This is a service we want to keep listening to all the time, independently
  // from the state, since it will allow us to enable or disable the snippets
  // service.
  if (sync_service_)
    sync_service_observer_.Add(sync_service_);

  // Change state after we started loading the snippets. During startup, the
  // Sync service might not be completely loaded when we initialize this
  // service, so we might stay in the NOT_INITED state until the sync state is
  // updated. See |GetStateForDependenciesStatus|.
  EnterState(GetStateForDependenciesStatus());

  FOR_EACH_OBSERVER(NTPSnippetsServiceObserver, observers_,
                    NTPSnippetsServiceLoaded());

  // Start a fetch if we don't have any snippets yet, or a fetch was requested
  // earlier.
  if (ready() && (snippets_.empty() || fetch_after_load_)) {
    fetch_after_load_ = false;
    FetchSnippets();
  }
}

NTPSnippetsService::State NTPSnippetsService::GetStateForDependenciesStatus() {
  switch (GetDisabledReason()) {
    case DisabledReason::NONE:
      return State::READY;

    case DisabledReason::HISTORY_SYNC_STATE_UNKNOWN:
      // HistorySync is not initialized yet, so we don't know what the actual
      // state is and we just return the current one. If things change,
      // |OnStateChanged| will call this function again to update the state.
      DVLOG(1) << "Sync configuration incomplete, continuing based on the "
             "current state.";
      return state_;

    case DisabledReason::EXPLICITLY_DISABLED:
    case DisabledReason::HISTORY_SYNC_DISABLED:
      return State::DISABLED;
  }

  // All cases should be handled by the above switch
  NOTREACHED();
  return State::DISABLED;
}

void NTPSnippetsService::EnterState(State state) {
  if (state == state_)
    return;

  switch (state) {
    case State::NOT_INITED:
      // Initial state, it should not be possible to get back there.
      NOTREACHED();
      return;

    case State::READY: {
      DCHECK(state_ == State::NOT_INITED || state_ == State::DISABLED);

      // If the service was previously disabled, we will need to start a fetch
      // because otherwise there won't be any.
      bool fetch_snippets = state_ == State::DISABLED || fetch_after_load_;
      DVLOG(1) << "Entering state: READY";
      state_ = State::READY;
      fetch_after_load_ = false;
      EnterStateEnabled(fetch_snippets);
      return;
    }

    case State::DISABLED:
      DCHECK(state_ == State::NOT_INITED || state_ == State::READY);

      DVLOG(1) << "Entering state: DISABLED";
      state_ = State::DISABLED;
      EnterStateDisabled();
      return;

    case State::SHUT_DOWN:
      DVLOG(1) << "Entering state: SHUT_DOWN";
      state_ = State::SHUT_DOWN;
      EnterStateShutdown();
      return;
  }
}

void NTPSnippetsService::ClearDeprecatedPrefs() {
  pref_service_->ClearPref(prefs::kDeprecatedSnippets);
  pref_service_->ClearPref(prefs::kDeprecatedDiscardedSnippets);
}

}  // namespace ntp_snippets
