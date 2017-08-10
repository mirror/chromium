// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/client_hints/client_hints_observer.h"

#include "base/metrics/histogram_macros.h"
#include "base/values.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/origin_util.h"
#include "url/gurl.h"

namespace {

// kNumClientHints should match the kWebClientHintsTypeLast + 1.
// TODO(tbansal): crbug.com/735518. Expose the kWebClientHintsTypeLast from
// Blink to here.
static const int kNumClientHints = 4;

}  // namespace

ClientHintsObserver::ClientHintsObserver(content::WebContents* tab)
    : binding_(tab, this) {}

ClientHintsObserver::~ClientHintsObserver() {}

void ClientHintsObserver::PersistClientHints(
    const GURL& primary_origin,
    std::unique_ptr<base::DictionaryValue> expiration_times) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!content::IsOriginSecure(primary_origin)) {
    // Allow only https:// origins.
    return;
  }

  if (!expiration_times || expiration_times->size() != 1)
    return;

  const base::ListValue* list_value = nullptr;
  if (!expiration_times->GetList("expiration_times", &list_value))
    return;

  if (list_value->GetSize() != kNumClientHints) {
    // Return early if the list has right number of values before persisting.
    // Persisting wrong number of values to the disk may cause errors when
    // reading them back in the future.
    return;
  }

  content::RenderProcessHost* rph =
      binding_.GetCurrentTargetFrame()->GetProcess();
  content::BrowserContext* browser_context = rph->GetBrowserContext();
  Profile* profile = Profile::FromBrowserContext(browser_context);

  HostContentSettingsMap* map =
      HostContentSettingsMapFactory::GetForProfile(profile);

  // TODO (tbansal): crbug.com/735518. Disable updates to client hints settings
  // when cookies are disabled for |primary_origin|.
  map->SetWebsiteSettingDefaultScope(
      primary_origin, GURL(), CONTENT_SETTINGS_TYPE_CLIENT_HINTS, std::string(),
      std::move(expiration_times));

  UMA_HISTOGRAM_EXACT_LINEAR("ClientHints.UpdateEventCount", 1, 2);
}

DEFINE_WEB_CONTENTS_USER_DATA_KEY(ClientHintsObserver);
