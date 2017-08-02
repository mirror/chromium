// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_TOOLS_CERT_MAPPER_MY_VISITOR_H_
#define NET_TOOLS_CERT_MAPPER_MY_VISITOR_H_

#include <memory>
#include "net/tools/cert_mapper/visitor.h"

namespace net {

std::unique_ptr<VisitorFactory> CreateMyVisitorFactory();

}  // namespace net

#endif  // NET_TOOLS_CERT_MAPPER_MY_VISITOR_H_
