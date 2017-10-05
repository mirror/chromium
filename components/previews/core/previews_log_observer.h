
// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PREVIEWS_CORE_PREVIEWS_LOG_OBSERVER_H_
#define COMPONENTS_PREVIEWS_CORE_PREVIEWS_LOG_OBSERVER_H_

#include "base/macros.h"
#include "components/previews/core/previews_log.h"

namespace previews {

// An interface for PreviewsLogger observer. This interface is for responding to
// events occur in PreviewsLogger (e.g. LogMessage events)
class MessageLogObserver {
 public:
  MessageLogObserver() {}
  virtual ~MessageLogObserver() {}

  // Perfrom action when |message| is added in PreviewsLogger. Virtualized for
  // implementation in subclasses.
  virtual void OnNewMessageLogAdded(PreviewsLogger::MessageLog message) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(MessageLogObserver);
};

}  // namespace previews

#endif  // COMPONENTS_PREVIEWS_CORE_PREVIEWS_LOG_OBSERVER_H_
