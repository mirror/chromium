// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_ARC_VOICE_INTERACTION_ARC_VOICE_INTERACTION_HIGHLIGHTER_CLIENT_H_
#define CHROME_BROWSER_CHROMEOS_ARC_VOICE_INTERACTION_ARC_VOICE_INTERACTION_HIGHLIGHTER_CLIENT_H_

#include "ash/public/interfaces/highlighter_controller.mojom.h"
#include "base/macros.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace gfx {
class Rect;
}  // namespace gfx

namespace base {
class Timer;
}  // namespace base

namespace arc {

class ArcVoiceInteractionFrameworkService;

class ArcVoiceInteractionHighlighterClient
    : public ash::mojom::HighlighterControllerClient {
 public:
  explicit ArcVoiceInteractionHighlighterClient(
      ArcVoiceInteractionFrameworkService* service);
  ~ArcVoiceInteractionHighlighterClient() override;

 private:
  // ash::mojom::HighlighterControllerClient:
  void HandleSelection(int x, int y, int w, int h) override;
  void HandleFailedSelection() override;
  void HandleEnabledStateChange(bool enabled) override;

  void ReportSelection(const gfx::Rect& rect);

  bool start_session_pending() const { return delay_timer_.get(); }

  // Binds to the client interface.
  mojo::Binding<ash::mojom::HighlighterControllerClient> binding_;
  // HighlighterController interface in ash. Holding the interface pointer keeps
  // the pipe alive to receive mojo return values.
  ash::mojom::HighlighterControllerPtr highlighter_controller_;

  ArcVoiceInteractionFrameworkService* service_;

  std::unique_ptr<base::Timer> delay_timer_;

  DISALLOW_COPY_AND_ASSIGN(ArcVoiceInteractionHighlighterClient);
};

}  // namespace arc

#endif  // CHROME_BROWSER_CHROMEOS_ARC_VOICE_INTERACTION_ARC_VOICE_INTERACTION_HIGHLIGHTER_CLIENT_H_
