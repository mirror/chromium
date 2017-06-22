// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HEADLESS_PUBLIC_HEADLESS_TAB_SOCKET_H_
#define HEADLESS_PUBLIC_HEADLESS_TAB_SOCKET_H_

#include <string>

#include "base/macros.h"
#include "headless/public/headless_export.h"

namespace headless {

// A bidirectional communications channel between C++ and JS.
class HEADLESS_EXPORT HeadlessTabSocket {
 public:
  class HEADLESS_EXPORT Listener {
   public:
    Listener() {}
    virtual ~Listener() {}

    // The |message| may be potentially sent by untrusted web content so it
    // should be validated carefully.
    virtual void OnMessageFromContext(const std::string& message,
                                      int v8_execution_context_id) = 0;

   private:
    DISALLOW_COPY_AND_ASSIGN(Listener);
  };

  // Installs a headless tab socket bindings in the specified execution context.
  // If the bindings are successfully installed then the |callback| is run with
  // |success| = true, otherwise with |success| = false.
  // TODO(alexclarke): In theory we shouldn't have to specify
  // |devtools_frame_id|, maybe it's OK to send the IPC to all renderers.
  // Alternatively replace |devtools_frame_id| once there is a solution for
  // stable frame IDs. See http://crbug.com/715541
  virtual void InstallHeadlessTabSocketBindings(
      std::string devtools_frame_id,
      int v8_execution_context_id,
      base::Callback<void(bool success)> callback) = 0;

  // Note this will fail unless the bindings have been installed.
  virtual void SendMessageToContext(const std::string& message,
                                    int v8_execution_context_id) = 0;

  virtual void SetListener(Listener* listener) = 0;

 protected:
  HeadlessTabSocket() {}
  virtual ~HeadlessTabSocket() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(HeadlessTabSocket);
};

}  // namespace headless

#endif  // HEADLESS_PUBLIC_HEADLESS_TAB_SOCKET_H_
