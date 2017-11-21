// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/browsing_instance.h"

#include <string>

#include "base/command_line.h"
#include "base/logging.h"
#include "content/browser/site_instance_impl.h"
#include "content/common/site_isolation_policy.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/url_constants.h"

namespace content {

BrowsingInstance::BrowsingInstance(BrowserContext* browser_context)
    : browser_context_(browser_context) {}

bool BrowsingInstance::HasSiteInstance(const GURL& url) {
  return !!GetSiteInstanceForURL(url);
}

scoped_refptr<SiteInstanceImpl> BrowsingInstance::GetSiteInstanceForURL(
    const GURL& url) {
  std::string site = SiteInstanceImpl::GetSiteForURL(browser_context_, url)
                         .possibly_invalid_spec();

  // Try to reuse a SiteInstance matching the site of |url|.
  SiteInstanceMap::iterator i = site_instance_map_.find(site);
  if (i != site_instance_map_.end())
    return i->second;

  // Try to reuse a SiteInstance for |url| that doesn't require isolation.
  bool url_requires_dedicated_site =
      SiteInstanceImpl::DoesSiteRequireDedicatedProcess(browser_context_, url);
  if (!url_requires_dedicated_site && undedicated_site_instance_)
    return undedicated_site_instance_;

  // Give up...
  return nullptr;
}

scoped_refptr<SiteInstanceImpl> BrowsingInstance::GetOrCreateSiteInstanceForURL(
    const GURL& url) {
  // Try to reuse an existing SiteInstance.
  scoped_refptr<SiteInstanceImpl> instance = GetSiteInstanceForURL(url);
  if (instance)
    return instance;

  // No current SiteInstance for this site, so let's create one.
  instance = new SiteInstanceImpl(this);

  // Set the site of this new SiteInstance, which will register it with us.
  instance->SetSite(url);
  return instance;
}

scoped_refptr<SiteInstanceImpl>
BrowsingInstance::GetDefaultSubframeSiteInstance() {
  // This should only be used for --top-document-isolation mode.
  CHECK(SiteIsolationPolicy::IsTopDocumentIsolationEnabled());
  if (!default_subframe_site_instance_) {
    SiteInstanceImpl* instance = new SiteInstanceImpl(this);
    instance->set_process_reuse_policy(
        SiteInstanceImpl::ProcessReusePolicy::USE_DEFAULT_SUBFRAME_PROCESS);

    // TODO(nick): This is a hack for now.
    instance->SetSite(GURL("http://web-subframes.invalid"));

    default_subframe_site_instance_ = instance;
  }

  return base::WrapRefCounted(default_subframe_site_instance_);
}

void BrowsingInstance::RegisterSiteInstance(SiteInstanceImpl* site_instance) {
  DCHECK(site_instance->browsing_instance_.get() == this);
  DCHECK(site_instance->HasSite());

  // Don't register the default subframe SiteInstance, to prevent it from being
  // returned by GetSiteInstanceForURL.
  if (default_subframe_site_instance_ == site_instance)
    return;

  std::string site = site_instance->GetSiteURL().possibly_invalid_spec();

  // Only register if we don't have a SiteInstance for this site already.
  // It's possible to have two SiteInstances point to the same site if two
  // tabs are navigated there at the same time.  (We don't call SetSite or
  // register them until DidNavigate.)  If there is a previously existing
  // SiteInstance for this site, we just won't register the new one.
  SiteInstanceMap::iterator i = site_instance_map_.find(site);
  if (i == site_instance_map_.end()) {
    // Not previously registered, so register it.
    site_instance_map_[site] = site_instance;
  }

  if (!undedicated_site_instance_ && !site_instance->RequiresDedicatedProcess())
    undedicated_site_instance_ = site_instance;
}

void BrowsingInstance::UnregisterSiteInstance(SiteInstanceImpl* site_instance) {
  DCHECK(site_instance->browsing_instance_.get() == this);
  DCHECK(site_instance->HasSite());
  std::string site = site_instance->GetSiteURL().possibly_invalid_spec();

  // Only unregister the SiteInstance if it is the same one that is registered
  // for the site.  (It might have been an unregistered SiteInstance.  See the
  // comments in RegisterSiteInstance.)
  SiteInstanceMap::iterator i = site_instance_map_.find(site);
  if (i != site_instance_map_.end() && i->second == site_instance) {
    // Matches, so erase it.
    site_instance_map_.erase(i);
  }
  if (default_subframe_site_instance_ == site_instance)
    default_subframe_site_instance_ = nullptr;
  if (undedicated_site_instance_ == site_instance)
    undedicated_site_instance_ = nullptr;
}

BrowsingInstance::~BrowsingInstance() {
  // We should only be deleted when all of the SiteInstances that refer to
  // us are gone.
  DCHECK(site_instance_map_.empty());
  DCHECK_EQ(0u, active_contents_count_);
}

}  // namespace content
