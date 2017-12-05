// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_PRESENTATION_RECEIVER_WINDOW_H_
#define CHROME_BROWSER_MEDIA_ROUTER_PRESENTATION_RECEIVER_WINDOW_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/macros.h"

class GURL;

namespace content {
class WebContents;
}

namespace views {
class Widget;
}

namespace media_router {

// This interface describes a class which shows a Presentation API receiver
// page in its own window.  After Start() has been called, the receiver page
// will be loaded and controlling pages may connect to it with
// |presentation_id|.  It should not be destroyed until Terminate has been
// called and it has called |callback|.
class PresentationReceiverWindow {
 public:
  virtual ~PresentationReceiverWindow() = default;

  // Starts the presentation by loading |start_url| in the window and connecting
  // presentation API communication channels for |presentation_id|.
  virtual void Start(const std::string& presentation_id,
                     const GURL& start_url) = 0;

  // Terminate the presentation, ensuring that the window is closed.
  // |callback| will be called when this is done.  It may not be safe to destroy
  // this object until |callback| has been called.
  virtual void Terminate(base::OnceClosure callback) = 0;

  virtual content::WebContents* presentation_web_contents() const = 0;
  virtual views::Widget* window() const = 0;

 protected:
  PresentationReceiverWindow() = default;

  DISALLOW_COPY_AND_ASSIGN(PresentationReceiverWindow);
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_PRESENTATION_RECEIVER_WINDOW_H_
