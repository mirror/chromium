// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/spellcheck/browser/spell_check_host_impl.h"

#include "build/build_config.h"
#include "components/spellcheck/browser/spellchecker_session_bridge_android.h"
#include "components/spellcheck/common/spellcheck_result.h"
#include "content/public/browser/browser_thread.h"

#if !defined(OS_ANDROID)
#error "This source file is Android only."
#endif

SpellCheckHostImpl::SpellCheckHostImpl()
    : session_bridge_(new SpellCheckerSessionBridge()) {}

SpellCheckHostImpl::~SpellCheckHostImpl() = default;

void SpellCheckHostImpl::RequestTextCheck(const base::string16& text,
                                          RequestTextCheckCallback callback) {
  DCHECK(!text.empty());
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  session_bridge_->RequestTextCheck(text, std::move(callback));
}

void SpellCheckHostImpl::ToggleSpellCheck(bool enabled, bool checked) {
  if (!enabled)
    session_bridge_->DisconnectSession();
}
