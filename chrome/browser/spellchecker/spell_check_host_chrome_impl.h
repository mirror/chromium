// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SPELLCHECKER_SPELL_CHECK_HOST_CHROME_IMPL_H_
#define CHROME_BROWSER_SPELLCHECKER_SPELL_CHECK_HOST_CHROME_IMPL_H_

#include "build/build_config.h"
#include "components/spellcheck/browser/spell_check_host_impl.h"
#include "components/spellcheck/browser/spelling_service_client.h"
#include "services/service_manager/public/cpp/bind_source_info.h"

class SpellcheckCustomDictionary;
class SpellcheckService;

struct SpellCheckResult;

// Implementation of SpellCheckHost involving Chrome-only features.
class SpellCheckHostChromeImpl : public SpellCheckHostImpl {
 public:
  explicit SpellCheckHostChromeImpl(
      const service_manager::Identity& renderer_identity);
  ~SpellCheckHostChromeImpl() override;

  static void Create(spellcheck::mojom::SpellCheckHostRequest request,
                     const service_manager::BindSourceInfo& source_info);

#if defined(OS_MACOSX)
  // Adjusts remote_results by examining local_results. Any result that's both
  // local and remote stays type SPELLING, all others are flagged GRAMMAR.
  // (This is needed to force gray underline for remote-only results.)
  static void CombineResults(
      std::vector<SpellCheckResult>* remote_results,
      const std::vector<SpellCheckResult>& local_results);
#endif

 private:
  friend class TestSpellCheckHostChromeImpl;
  friend class SpellCheckHostChromeImplMacTest;

  // SpellCheckHostImpl:
  void RequestDictionary() override;
  void NotifyChecked(const base::string16& word, bool misspelled) override;

#if !BUILDFLAG(USE_BROWSER_SPELLCHECKER)
  void CallSpellingService(const base::string16& text,
                           CallSpellingServiceCallback callback) override;

  // Invoked when the remote Spelling service has finished checking the
  // text of a CallSpellingService request.
  void CallSpellingServiceDone(
      CallSpellingServiceCallback callback,
      bool success,
      const base::string16& text,
      const std::vector<SpellCheckResult>& service_results) const;

  // Filter out spelling corrections of custom dictionary words from the
  // Spelling service results.
  static std::vector<SpellCheckResult> FilterCustomWordResults(
      const std::string& text,
      const SpellcheckCustomDictionary& custom_dictionary,
      const std::vector<SpellCheckResult>& service_results);
#endif

#if defined(OS_MACOSX)
  void CheckSpelling(const base::string16& word,
                     int route_id,
                     CheckSpellingCallback callback) override;
  void FillSuggestionList(const base::string16& word,
                          FillSuggestionListCallback callback) override;
  void RequestTextCheck(const base::string16& text,
                        int route_id,
                        RequestTextCheckCallback callback) override;

  int ToDocumentTag(int route_id);
  void RetireDocumentTag(int route_id);
  std::map<int, int> tag_map_;
#endif  // defined(OS_MACOSX)

  // Returns the SpellcheckService of our |render_process_id_|. The return
  // is null if the render process is being shut down.
  virtual SpellcheckService* GetSpellcheckService() const;

  // The identity of the renderer service.
  const service_manager::Identity renderer_identity_;

  // A JSON-RPC client that calls the remote Spelling service.
  SpellingServiceClient client_;

  DISALLOW_COPY_AND_ASSIGN(SpellCheckHostChromeImpl);
};

#endif  // CHROME_BROWSER_SPELLCHECKER_SPELL_CHECK_HOST_CHROME_IMPL_H_
