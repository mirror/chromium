// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
 
#ifndef CHROME_BROWSER_UI_OVERLAY_OVERLAY_SURFACE_EMBEDDER_H_
#define CHROME_BROWSER_UI_OVERLAY_OVERLAY_SURFACE_EMBEDDER_H_

#include "cc/surfaces/surface_reference_factory.h"
#include "chrome/browser/ui/pip/pip_window.h"

namespace cc {
class SurfaceInfo;
}

// Embed a surface into the PiP window to show stuff on it.
class PipSurfaceEmbedder {
 public:
  PipSurfaceEmbedder(PipWindow* window);
   ~PipSurfaceEmbedder();

  void SetPrimarySurfaceInfo(const cc::SurfaceInfo& surface_info);
  void SetFallbackSurfaceInfo(const cc::SurfaceInfo& surface_info);

 private:
  // The window which embeds the client.
  PipWindow* window_;

  // Contains the client's content.
  std::unique_ptr<ui::Layer> surface_layer_;

  scoped_refptr<cc::SurfaceReferenceFactory> ref_factory_;

  DISALLOW_COPY_AND_ASSIGN(PipSurfaceEmbedder);
};

#endif  // CHROME_BROWSER_UI_OVERLAY_OVERLAY_SURFACE_EMBEDDER_H_