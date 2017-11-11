// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ASH_ACCESSIBILITY_ACCESSIBILITY_CONTROLLER_CLIENT_H_
#define CHROME_BROWSER_UI_ASH_ACCESSIBILITY_ACCESSIBILITY_CONTROLLER_CLIENT_H_

#include "ash/public/interfaces/accessibility_controller.mojom.h"
#include "base/macros.h"
#include "mojo/public/cpp/bindings/binding.h"

class AccessibilityControllerClient
    : public ash::mojom::AccessibilityControllerClient {
 public:
  AccessibilityControllerClient();
  ~AccessibilityControllerClient() override;

  static AccessibilityControllerClient* Get();

  // ash::mojom::AccessibilityControllerClient:
  void TriggerAccessibilityAlert(ash::mojom::AccessibilityAlert alert) override;

 private:
  // Binds to the client interface.
  mojo::Binding<ash::mojom::AccessibilityControllerClient> binding_;

  // AccessibilityController interface in ash. Holding the interface pointer
  // keeps the pipe alive to receive mojo return values.
  ash::mojom::AccessibilityControllerPtr accessibility_controller_;

  DISALLOW_COPY_AND_ASSIGN(AccessibilityControllerClient);
};

#endif  // CHROME_BROWSER_UI_ASH_ACCESSIBILITY_ACCESSIBILITY_CONTROLLER_CLIENT_H_
