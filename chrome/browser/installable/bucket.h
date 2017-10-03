// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_INSTALLABLE_BUCKET_H_
#define CHROME_BROWSER_INSTALLABLE_BUCKET_H_

#include <map>
#include <memory>
#include <vector>

#include "base/memory/ptr_util.h"

namespace installable {

enum PwaStatus {
  PWA,
  NON_PWA,
  UNKNOWN,
};

// A single UMA histogram bucket.
class Bucket {
 public:
  virtual ~Bucket() {}
  virtual void Increment() = 0;
};

// An interface for objects which take a delayed action which is dependent on
// the PwaStatus.
class Resolvable {
 public:
  virtual ~Resolvable() {}

  // Performs the action associated with |status|.
  virtual void Resolve(PwaStatus status) = 0;

  // Returns whether there are outstanding actions to be resolved.
  virtual bool HasOutstanding() = 0;
};

// An accumulation of counts which are destined for a single UMA histogram
// bucket. The counts may be directed to a specific bucket by calling Resolve.
// After Resolve is called, any further increments will immediately increment
// the specific bucket.
class ResolvableBucket : public Bucket, public Resolvable {
 public:
  // Constructs a ResolvableBucket which may later be resolved to one of the
  // supplied buckets.
  ResolvableBucket(std::unique_ptr<Bucket> pwa_bucket,
                   std::unique_ptr<Bucket> non_pwa_bucket,
                   std::unique_ptr<Bucket> pwa_unknown_bucket);

  ~ResolvableBucket() override;

  void Increment() override;

  void Resolve(PwaStatus status) override;
  bool HasOutstanding() override;

 private:
  Bucket* resolved_bucket_;
  int count_;
  std::map<PwaStatus, std::unique_ptr<Bucket>> buckets_;
};

// BucketManager owns Buckets, and provides a mechansim to resolve all of its
// ResolvableBuckets.
class BucketManager {
 public:
  BucketManager();
  ~BucketManager();

  // Takes ownership of |bucket|.
  void Add(std::unique_ptr<Bucket> bucket);

  // Takes ownership of |resolvable_bucket|, and adds it to the list of buckets
  // to resolve when Resolve() is called.
  void AddResolvable(std::unique_ptr<ResolvableBucket> resolvable_bucket);

  // Resolves the PWA status of buckets added via AddResolvable.
  void Resolve(PwaStatus pwa_status);

  // Deletes all buckets, restoring BucketManager to its initial state.
  // Resolve should be called before Reset.
  void Reset();

 private:
  bool HasOutstanding();

  std::vector<std::unique_ptr<Bucket>> buckets_;
  std::vector<Resolvable*> resolvables_;
};

}  // namespace installable

#endif  // CHROME_BROWSER_INSTALLABLE_BUCKET_H_
