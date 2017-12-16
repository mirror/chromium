// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef _EXTENSIONS_BROWSER_CONTENT_VERIFIER_CANCELLATION_STATUS_H
#define _EXTENSIONS_BROWSER_CONTENT_VERIFIER_CANCELLATION_STATUS_H

namespace extensions {

// Typically for jobs, a thread-safe interface to cancel the job or query
// whether the job has been cancelled.
class CancellationStatus
    : public base::RefCountedThreadSafe<CancellationStatus> {
 public:
  CancellationStatus() = default;

  // Whether or not |this| has been cancelled.
  virtual bool IsCancelled() = 0;

 protected:
  virtual ~CancellationStatus() = default;

  // Sets the status to cancelled.
  virtual void Cancel() = 0;

 private:
  friend class base::RefCountedThreadSafe<CancellationStatus>;

  DISALLOW_COPY_AND_ASSIGN(CancellationStatus);
};

}  // namespace extensions

#endif  // _EXTENSIONS_BROWSER_CONTENT_VERIFIER_CANCELLATION_STATUS_H
