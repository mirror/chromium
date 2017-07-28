// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_TOOLS_CERT_MAPPER_VISITOR_H_
#define NET_TOOLS_CERT_MAPPER_VISITOR_H_

#include <map>
#include <memory>

#include "base/compiler_specific.h"
#include "base/memory/ptr_util.h"

namespace net {

class Entry;
class Metrics;

class Visitor {
 public:
  virtual ~Visitor() {}
  virtual void Start() {}
  virtual void Visit(const Entry& entry, Metrics* metrics) = 0;
  virtual void End() {}
};

class VisitorFactory {
 public:
  virtual ~VisitorFactory() {}
  virtual std::unique_ptr<Visitor> Create() = 0;
};

}  // namespace net

#endif  // NET_TOOLS_CERT_MAPPER_VISITOR_H_
