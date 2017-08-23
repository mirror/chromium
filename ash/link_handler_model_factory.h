// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_LINK_HANDLER_MODEL_FACTORY_H_
#define ASH_LINK_HANDLER_MODEL_FACTORY_H_

#include <memory>

#include "ash/ash_export.h"
#include "base/macros.h"

class GURL;

namespace ash {

class LinkHandlerModel;

// A class for creating a LinkHandlerModel object.
class ASH_EXPORT LinkHandlerModelFactory {
 public:
  // Returns a model for the |url|. When such a model for the |url| cannot be
  // created, returns nullptr.
  virtual std::unique_ptr<LinkHandlerModel> CreateModel(const GURL& url) = 0;
};

}  // namespace ash

#endif  // ASH_LINK_HANDLER_MODEL_FACTORY_H_
