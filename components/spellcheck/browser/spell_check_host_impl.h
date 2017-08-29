// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SPELLCHECK_BROWSER_SPELL_CHECK_HOST_IMPL_H_
#define COMPONENTS_SPELLCHECK_BROWSER_SPELL_CHECK_HOST_IMPL_H_

#include "base/macros.h"
#include "build/build_config.h"
#include "components/spellcheck/common/spellcheck.mojom.h"
#include "components/spellcheck/spellcheck_build_features.h"

#if !BUILDFLAG(ENABLE_SPELLCHECK)
#error "Spellcheck should be enabled."
#endif

class SpellCheckerSessionBridge;

// A basic implementation of SpellCheckHost without using any Chrome-only
// feature, so that spellchecking is still supported when those features are not
// available. The full implementation involving Chrome-only features is in
// SpellCheckHostChromeImpl.
class SpellCheckHostImpl : public spellcheck::mojom::SpellCheckHost {
 public:
  explicit SpellCheckHostImpl();
  ~SpellCheckHostImpl() override;

  static void Create(spellcheck::mojom::SpellCheckHostRequest request);

 protected:
  // spellcheck::mojom::SpellCheckHost:
  void RequestDictionary() override;
  void NotifyChecked(const base::string16& word, bool misspelled) override;
  void CallSpellingService(const base::string16& text,
                           CallSpellingServiceCallback callback) override;
  void RequestTextCheck(const base::string16& text,
                        RequestTextCheckCallback callback) override;
  void ToggleSpellCheck(bool enabled, bool checked) override;

 private:
#if defined(OS_ANDROID)
  // Android-specific object used to query the Android spellchecker.
  std::unique_ptr<SpellCheckerSessionBridge> session_bridge_;
#endif

  DISALLOW_COPY_AND_ASSIGN(SpellCheckHostImpl);
};

#endif  // COMPONENTS_SPELLCHECK_BROWSER_SPELL_CHECK_HOST_IMPL_H_
