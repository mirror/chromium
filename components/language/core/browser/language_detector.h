// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_LANGUAGE_CORE_BROWSER_LANGUAGE_DETECTOR_H_
#define COMPONENTS_LANGUAGE_CORE_BROWSER_LANGUAGE_DETECTOR_H_

#include <vector>

#include "base/memory/weak_ptr.h"

// TODO(crbug.com/736929): Move LanguageDetectionDetails to the language
// component.
namespace translate {
struct LanguageDetectionDetails;
}  // namespace translate

namespace language {

// Common functionality for classes offering language detection. Maintains a
// list of observers and updates them when a page's language has been
// determined.
//
// Only handles the most basic "observer" functionality; does not support e.g.
// observer removal or concurrent access (i.e. is not thread safe). This serves
// our purposes while allowing us to handle the indeterminate destruction order
// of various tab helpers.
class LanguageDetector {
 public:
  class Observer {
   public:
    // Called with the results of a successful page language detection.
    virtual void OnLanguageDetected(
        const translate::LanguageDetectionDetails& details) = 0;
  };

  LanguageDetector();
  virtual ~LanguageDetector();

  // Adds an observer of language detection.
  void AddLanguageDetectionObserver(base::WeakPtr<Observer> callback);

  // Notifies all observers that language detection has occured.
  void NotifyLanguageDetectionObservers(
      const translate::LanguageDetectionDetails& details);

 private:
  std::vector<base::WeakPtr<Observer>> language_detection_observers_;
};

}  // namespace language

#endif  // COMPONENTS_LANGUAGE_CORE_BROWSER_LANGUAGE_DETECTOR_H_
