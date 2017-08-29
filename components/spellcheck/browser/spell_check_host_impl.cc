// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/spellcheck/browser/spell_check_host_impl.h"

#include "base/memory/ptr_util.h"
#include "components/spellcheck/common/spellcheck_result.h"
#include "content/public/browser/browser_thread.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

#if !defined(OS_ANDROID)
// Android needs different constructor and destructor due to |session_bridge_|.
SpellCheckHostImpl::SpellCheckHostImpl() = default;
SpellCheckHostImpl::~SpellCheckHostImpl() = default;
#endif

// static
void SpellCheckHostImpl::Create(
    spellcheck::mojom::SpellCheckHostRequest request) {
  mojo::MakeStrongBinding(base::MakeUnique<SpellCheckHostImpl>(),
                          std::move(request));
}

void SpellCheckHostImpl::RequestDictionary() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  // This API requires Chrome-only features.
  return;
}

void SpellCheckHostImpl::NotifyChecked(const base::string16& word,
                                       bool misspelled) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  // This API requires Chrome-only features.
  return;
}

void SpellCheckHostImpl::CallSpellingService(
    const base::string16& text,
    CallSpellingServiceCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (text.empty())
    mojo::ReportBadMessage(__FUNCTION__);

  // This API requires Chrome-only features.
  std::move(callback).Run(false, std::vector<SpellCheckResult>());
}

#if !defined(OS_ANDROID)
// Placeholder implementations of Android-only APIs. For real implementations on
// Android, see spell_check_host_impl_android.cc.

// TODO(xiaochengh): Support RequestTextCheck on Mac.
void SpellCheckHostImpl::RequestTextCheck(const base::string16&,
                                          RequestTextCheckCallback callback) {
  std::move(callback).Run(std::vector<SpellCheckResult>());
}

void SpellCheckHostImpl::ToggleSpellCheck(bool, bool) {}
#endif
