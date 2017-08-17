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
#include "url/origin.h"

ClientHintsObserver::ClientHintsObserver(content::WebContents* tab)
    : binding_(tab, this) {}

ClientHintsObserver::~ClientHintsObserver() {}

void ClientHintsObserver::PersistClientHints(
    const url::Origin& primary_origin,
    const std::vector<::blink::mojom::WebClientHintsType>& client_hints,
    base::TimeDelta expiration_duration) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  // TODO(tbansal): crbug.com/735518. Consider killing the renderer that sent
  // the malformed IPC.
  if (!content::IsOriginSecure(primary_origin.GetURL()))
    return;

  if (client_hints.empty() ||
      client_hints.size() >
          static_cast<int>(
              blink::mojom::WebClientHintsType::kWebClientHintsTypeLast)) {
    // Return early if the list does not have the right number of values.
    // Persisting wrong number of values to the disk may cause errors when
    // reading them back in the future.
    return;
  }

  if (expiration_duration <= base::TimeDelta::FromSeconds(0))
    return;

  content::RenderProcessHost* rph =
      binding_.GetCurrentTargetFrame()->GetProcess();
  content::BrowserContext* browser_context = rph->GetBrowserContext();
  Profile* profile = Profile::FromBrowserContext(browser_context);

  HostContentSettingsMap* map =
      HostContentSettingsMapFactory::GetForProfile(profile);

  std::unique_ptr<base::ListValue> expiration_times_list =
      base::MakeUnique<base::ListValue>();
  expiration_times_list->Reserve(
      static_cast<int>(
          blink::mojom::WebClientHintsType::kWebClientHintsTypeLast) +
      1);

  // Use wall clock since the expiration time would be persisted across embedder
  // restarts.
  double expiration_time =
      (base::Time::Now() + expiration_duration).ToDoubleT();

  for (const auto& entry : client_hints) {
    int index = static_cast<int>(entry);

    if (index < 0 ||
        index >
            static_cast<int>(
                blink::mojom::WebClientHintsType::kWebClientHintsTypeLast)) {
      // Bad client hint value.
      continue;
    }
    expiration_times_list->Set(static_cast<int>(entry),
                               base::MakeUnique<base::Value>(expiration_time));
  }

  std::unique_ptr<base::DictionaryValue> expiration_times_dictionary(
      new base::DictionaryValue());
  expiration_times_dictionary->SetList("expiration_times",
                                       std::move(expiration_times_list));

  // TODO(tbansal): crbug.com/735518. Disable updates to client hints settings
  // when cookies are disabled for |primary_origin|.
  map->SetWebsiteSettingDefaultScope(
      primary_origin.GetURL(), GURL(), CONTENT_SETTINGS_TYPE_CLIENT_HINTS,
      std::string(), std::move(expiration_times_dictionary));

  UMA_HISTOGRAM_EXACT_LINEAR("ClientHints.UpdateEventCount", 1, 2);
}

DEFINE_WEB_CONTENTS_USER_DATA_KEY(ClientHintsObserver);
