// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebSpellCheckPanelClient_h
#define WebSpellCheckPanelClient_h

#include "public/platform/WebString.h"

namespace blink {

class WebSpellCheckPanelClient {
 public:
  // Show or hide the spelling panel.
  virtual void ShowSpellingUI(bool show) {}

  // Returns true if the spelling panel is showing.
  virtual bool IsShowingSpellingUI() { return false; }

  // Update the spelling panel with the given word.
  virtual void UpdateSpellingUIWithMisspelledWord(const WebString& word) {}

 protected:
  virtual ~WebSpellCheckPanelClient() {}
};

}  // namespace blink

#endif
