// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_BLUETOOTH_INTERNALS_BLUETOOTH_INTERNALS_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_BLUETOOTH_INTERNALS_BLUETOOTH_INTERNALS_HANDLER_H_

#include "base/macros.h"
#include "chrome/browser/ui/webui/bluetooth_internals/bluetooth_internals.mojom.h"
#include "chrome/browser/ui/webui/mojo_web_ui_handler.h"
#include "mojo/public/cpp/bindings/binding.h"

// Handles API requests from chrome://bluetooth-internals page by implementing
// mojom::BluetoothInternalsHandler.
class BluetoothInternalsHandler : public mojom::BluetoothInternalsHandler,
                                  public MojoWebUIHandler {
 public:
  explicit BluetoothInternalsHandler(
      mojom::BluetoothInternalsHandlerRequest request);
  ~BluetoothInternalsHandler() override;

  // mojom::BluetoothInternalsHandler overrides:
  void GetBluetoothAdapter(GetBluetoothAdapterCallback callback) override;

 private:
  mojo::Binding<mojom::BluetoothInternalsHandler> binding_;

  DISALLOW_COPY_AND_ASSIGN(BluetoothInternalsHandler);
};

#endif  // CHROME_BROWSER_UI_WEBUI_BLUETOOTH_INTERNALS_BLUETOOTH_INTERNALS_HANDLER_H_
