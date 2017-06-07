// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_CRASHER_H_
#define CONTENT_RENDERER_CRASHER_H_

#include <vector>

#include "content/common/crasher.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace service_manager {
struct BindSourceInfo;
}

namespace content {

class CrasherImpl : public mojom::Crasher {
 public:
  explicit CrasherImpl(mojom::CrasherRequest request);
  ~CrasherImpl() override;

  static void Create(const service_manager::BindSourceInfo&,
                     mojom::CrasherRequest request);
  void Crash(mojom::CrashKeysPtr crash_keys) override;

 private:
  mojo::Binding<mojom::Crasher> binding_;

  DISALLOW_COPY_AND_ASSIGN(CrasherImpl);
};

}  // namespace content

#endif  // CONTENT_RENDERER_CRASHER_H_
