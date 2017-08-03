// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "mash/public/interfaces/launchable.mojom.h"

namespace chromeos {

class Launchable : public mash::mojom::Launchable {
 public:
  Launchable() = default;
  ~Launchable() override = default;

  void Bind(mash::mojom::LaunchableRequest request);

 private:
  // mash::mojom::Launchable:
  void Launch(uint32_t what, mash::mojom::LaunchMode how) override;

  void CreateNewWindowImpl(bool is_incognito);

  mojo::BindingSet<mash::mojom::Launchable> bindings_;

  DISALLOW_COPY_AND_ASSIGN(Launchable);
};

}  // namespace chromeos
