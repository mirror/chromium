// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NTP_TILES_POPULAR_SITES_H_
#define COMPONENTS_NTP_TILES_POPULAR_SITES_H_

#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string16.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "url/gurl.h"

namespace base {
class Value;
}

namespace net {
class URLRequestContextGetter;
}

namespace user_prefs {
class PrefRegistrySyncable;
}

namespace variations {
class VariationsService;
}

class PrefService;
class TemplateURLService;

namespace ntp_tiles {

// Downloads and provides a list of suggested popular sites, for display on
// the NTP when there are not enough personalized suggestions. Caches the
// downloaded file on disk to avoid re-downloading on every startup.
class PopularSites : public net::URLFetcherDelegate {
 public:
  struct Site {
    Site(const base::string16& title,
         const GURL& url,
         const GURL& favicon_url,
         const GURL& large_icon_url,
         const GURL& thumbnail_url);
    Site(const Site& other);
    ~Site();

    base::string16 title;
    GURL url;
    GURL favicon_url;
    GURL large_icon_url;
    GURL thumbnail_url;
  };

  using FinishedCallback = base::Callback<void(bool /* success */)>;

  // When the suggestions have been fetched (from cache or URL) and parsed,
  // invokes |callback|, on the same thread as the caller.
  //
  // Set |force_download| to enforce re-downloading the suggestions file, even
  // if it already exists on disk.
  //
  // TODO(treib): PopularSites should query the variation params itself instead
  // of having them passed in.
  PopularSites(const scoped_refptr<base::SequencedWorkerPool>& blocking_pool,
               PrefService* prefs,
               const TemplateURLService* template_url_service,
               variations::VariationsService* variations_service,
               net::URLRequestContextGetter* download_context,
               const base::FilePath& directory,
               const std::string& variation_param_country,
               const std::string& variation_param_version,
               bool force_download,
               const FinishedCallback& callback);

  // This fetches the popular sites from a given url and is only used for
  // debugging through the popular-sites-internals page.
  PopularSites(const scoped_refptr<base::SequencedWorkerPool>& blocking_pool,
               PrefService* prefs,
               net::URLRequestContextGetter* download_context,
               const base::FilePath& directory,
               const GURL& url,
               const FinishedCallback& callback);

  ~PopularSites() override;

  const std::vector<Site>& sites() const { return sites_; }

  // The country/version of the file that was last downloaded.
  std::string GetCountry() const;
  std::string GetVersion() const;

  const base::FilePath& local_path() const { return local_path_; }

  // Register preferences used by this class.
  static void RegisterProfilePrefs(
      user_prefs::PrefRegistrySyncable* user_prefs);

 private:
  PopularSites(const scoped_refptr<base::SequencedWorkerPool>& blocking_pool,
               PrefService* prefs,
               net::URLRequestContextGetter* download_context,
               const base::FilePath& directory,
               const std::string& country,
               const std::string& version,
               const GURL& override_url,
               bool force_download,
               const FinishedCallback& callback);

  GURL GetPopularSitesURL() const;

  void OnReadFileDone(const GURL& url,
                      std::unique_ptr<std::string> data,
                      bool success);

  // Fetch the popular sites at the given URL, overwriting any file that already
  // exists.
  void FetchPopularSites(const GURL& url);

  // net::URLFetcherDelegate implementation.
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  void OnJsonParsed(std::unique_ptr<base::Value> json);
  void OnJsonParseFailed(const std::string& error_message);
  void OnFileWriteDone(std::unique_ptr<base::Value> json, bool success);
  void ParseSiteList(std::unique_ptr<base::Value> json);
  void OnDownloadFailed();

  FinishedCallback callback_;
  std::unique_ptr<net::URLFetcher> fetcher_;
  bool is_fallback_;
  std::vector<Site> sites_;
  std::string pending_country_;
  std::string pending_version_;

  base::FilePath local_path_;

  PrefService* prefs_;
  net::URLRequestContextGetter* download_context_;

  scoped_refptr<base::TaskRunner> blocking_runner_;

  base::WeakPtrFactory<PopularSites> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(PopularSites);
};

}  // namespace ntp_tiles

#endif  // COMPONENTS_NTP_TILES_POPULAR_SITES_H_
