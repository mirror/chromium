// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/spellchecker/spell_check_host_impl.h"

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "components/spellcheck/browser/spellchecker_session_bridge_android.h"
#include "components/spellcheck/common/spellcheck_result.h"
#include "content/public/browser/browser_thread.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

#if !defined(OS_ANDROID)
#error "This source file is Android only."
#endif

SpellCheckHostImpl::SpellCheckHostImpl(int render_process_id)
    : render_process_id_(render_process_id),
      session_bridge_(new SpellCheckerSessionBridge()) {}

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
