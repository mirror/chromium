// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_TEST_SYSTEM_APP_TEST_SYSTEM_APP_H
#define CHROME_BROWSER_CHROMEOS_TEST_SYSTEM_APP_TEST_SYSTEM_APP_H

#include "chrome/browser/chromeos/test_system_app/test_system_app.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"

class TestSystemAppImpl : public mojom::TestSystemApp {
 public:
  TestSystemAppImpl(mojo::InterfaceRequest<mojom::TestSystemApp> request);
  ~TestSystemAppImpl() override;

  // mojom::TestSystemApp overrides:
  void GetNumber(GetNumberCallback callback) override;

 private:
  mojo::Binding<mojom::TestSystemApp> binding_;

  DISALLOW_COPY_AND_ASSIGN(TestSystemAppImpl);
};

#endif  // CHROME_BROWSER_CHROMEOS_TEST_SYSTEM_APP_TEST_SYSTEM_APP_H
