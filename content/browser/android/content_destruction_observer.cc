// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/android/content_destruction_observer.h"

namespace content {
// static
void ContentDestructionObserver::Create(WebContents* web_contents,
                                        RenderWidgetHostViewAndroid* rwhva) {
  // Owns itself, and gets destroyed together with WebContents.
  new ContentDestructionObserver(web_contents, rwhva);
}

ContentDestructionObserver::ContentDestructionObserver(
    WebContents* web_contents,
    RenderWidgetHostViewAndroid* rwhva)
    : WebContentsObserver(web_contents), rwhva_(rwhva) {
  DCHECK(rwhva);
  rwhva_->AddDestructionObserver(this);
}

ContentDestructionObserver::~ContentDestructionObserver() {
  rwhva_->RemoveDestructionObserver(this);
}

void ContentDestructionObserver::RenderViewHostChanged(
    RenderViewHost* old_host,
    RenderViewHost* new_host) {
  auto* new_view = new_host ? static_cast<RenderWidgetHostViewAndroid*>(
                                  new_host->GetWidget()->GetView())
                            : nullptr;
  if (new_view != rwhva_) {
    // The WebContents got paired with a new RenderWidgetHostView (or lost
    // connection). No need to signal the destruction of the current WebContents
    // any more. Delete itself.
    delete this;
  }
}

void ContentDestructionObserver::WebContentsDestroyed() {
  rwhva_->OnContentDestroyed();
  delete this;
}

void ContentDestructionObserver::RenderWidgetHostViewDestroyed(
    RenderWidgetHostViewAndroid* rwhva) {
  // RWHVA is going away. No need to signal the destruction of the current
  // WebContents any more. Delete itself.
  delete this;
}

}  // namespace content
