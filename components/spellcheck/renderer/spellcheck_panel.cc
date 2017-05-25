// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/spellcheck/renderer/spellcheck_panel.h"

#include "base/bind.h"
#include "base/metrics/histogram_macros.h"
#include "components/spellcheck/common/spellcheck.mojom.h"
#include "components/spellcheck/common/spellcheck_messages.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_thread.h"
#include "content/public/renderer/render_view.h"
#include "services/service_manager/public/cpp/bind_source_info.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"

SpellCheckPanel::SpellCheckPanel(content::RenderFrame* render_frame)
    : content::RenderFrameObserver(render_frame),
      content::RenderFrameObserverTracker<SpellCheckPanel>(render_frame),
      spelling_panel_visible_(false) {
  if (!render_frame || !content::RenderThread::Get())
    return;  // Can be NULL in tests.

  render_frame->GetInterfaceRegistry()->AddInterface(base::Bind(
      &SpellCheckPanel::SpellCheckPanelRequest, base::Unretained(this)));

  render_frame->GetWebFrame()->SetSpellCheckPanelClient(this);
}

SpellCheckPanel::~SpellCheckPanel() = default;

void SpellCheckPanel::OnDestruct() {
  bindings_.CloseAllBindings();
  delete this;
}

bool SpellCheckPanel::IsShowingSpellingUI() {
  return spelling_panel_visible_;
}

void SpellCheckPanel::ShowSpellingUI(bool show) {
  // TODO(crbug.com/714480): convert this IPC to mojo.
  UMA_HISTOGRAM_BOOLEAN("SpellCheck.api.showUI", show);
  Send(new SpellCheckHostMsg_ShowSpellingPanel(routing_id(), show));
}

void SpellCheckPanel::UpdateSpellingUIWithMisspelledWord(
    const blink::WebString& word) {
  // TODO(crbug.com/714480): convert this IPC to mojo.
  Send(new SpellCheckHostMsg_UpdateSpellingPanelWithMisspelledWord(
      routing_id(), word.Utf16()));
}

void SpellCheckPanel::SpellCheckPanelRequest(
    const service_manager::BindSourceInfo& source_info,
    spellcheck::mojom::SpellCheckPanelRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void SpellCheckPanel::AdvanceToNextMisspelling() {
  auto* render_frame = content::RenderFrameObserver::render_frame();
  if (!render_frame || !render_frame->GetWebFrame())
    return;

  render_frame->GetWebFrame()->ExecuteCommand(
      blink::WebString::FromUTF8("AdvanceToNextMisspelling"));
}

void SpellCheckPanel::ToggleSpellPanel(bool visible) {
  auto* render_frame = content::RenderFrameObserver::render_frame();
  if (!render_frame || !render_frame->GetWebFrame())
    return;

  // Tell our frame whether the spelling panel is visible or not so
  // that it won't need to make ipc calls later.
  spelling_panel_visible_ = visible;

  render_frame->GetWebFrame()->ExecuteCommand(
      blink::WebString::FromUTF8("ToggleSpellPanel"));
}
