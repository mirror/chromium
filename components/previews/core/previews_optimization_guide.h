// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PREVIEWS_CORE_PREVIEWS_OPTIMIZATION_GUIDE_H_
#define COMPONENTS_PREVIEWS_CORE_PREVIEWS_OPTIMIZATION_GUIDE_H_

#include <string>

#include "base/sequence_checker.h"
#include "base/sequenced_task_runner.h"
#include "base/single_thread_task_runner.h"
#include "components/previews/core/previews_experiments.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"

namespace content {
class NotificationDetails;
class NotificationRegistrar;
class NotificationSource;
}  // namespace content

namespace net {
class URLRequest;
}

namespace optimization_guide {
class Configuration;
class OptimizationGuideService;
}  // namespace optimization_guide

namespace previews {

class Hints {
 public:
  Hints();
  Hints(const Hints& other);
  ~Hints();

  void AddHint(const std::string& host_suffix);

 private:
  std::vector<std::string> host_suffixes;
};

class PreviewsOptimizationGuide : public content::NotificationObserver {
 public:
  PreviewsOptimizationGuide(
      const scoped_refptr<base::SequencedTaskRunner>& background_task_runner,
      const scoped_refptr<base::SingleThreadTaskRunner>& io_task_runner,
      optimization_guide::OptimizationGuideService* optimization_guide_service);

  ~PreviewsOptimizationGuide() override;

  // Returns whether |type| is whitelisted for |request|.
  bool IsWhitelisted(const net::URLRequest& request, PreviewsType type);

  // content::NotificationObserver implementation:
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

 private:
  void InitializeOnIOThread(
      optimization_guide::OptimizationGuideService* optimization_guide_service);

  // Must be called in background priority task.
  void ProcessUnindexedHints(const optimization_guide::Configuration& config);

  // Must be called on the IO thread.
  void SwapHints(const Hints& hints);

  std::unique_ptr<content::NotificationRegistrar> registrar_;
  std::unique_ptr<const Hints> hints_;

  scoped_refptr<base::SequencedTaskRunner> background_task_runner_;
  scoped_refptr<base::SingleThreadTaskRunner> io_task_runner_;

  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace previews

#endif  // COMPONENTS_PREVIEWS_CORE_PREVIEWS_OPTIMIZATION_GUIDE_H_
