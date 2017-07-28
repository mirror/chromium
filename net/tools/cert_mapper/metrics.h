// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_TOOLS_CERT_MAPPER_METRICS_H_
#define NET_TOOLS_CERT_MAPPER_METRICS_H_

#include <map>
#include <string>

#include "base/compiler_specific.h"
#include "net/tools/cert_mapper/entry.h"

namespace net {

struct Sample {
  Sample();
  Sample(const Sample& other);
  Sample(Sample&& other);
  Sample& operator=(const Sample& other);
  Sample& operator=(Sample&& other);
  ~Sample();

  // If just 1 entry, means a single cert. Otherwise means a chain of
  // certificates.
  CertBytesVector certs;
};

struct BucketValue {
  BucketValue();
  BucketValue(const BucketValue&);
  BucketValue(BucketValue&&);
  BucketValue& operator=(const BucketValue& other);
  BucketValue& operator=(BucketValue&& other);
  ~BucketValue();

  void IncrementTotal() { total += 1; }

  void AddSampleChain(const Entry& entry);
  void AddSampleCert(const scoped_refptr<CertBytes>& cert);

  void Merge(const BucketValue& other);
  void Finalize();

  static void SetMaxSamples(size_t limit);

  size_t total = 0;
  std::vector<Sample> samples;
};

class MetricsItem {
 public:
  MetricsItem();
  MetricsItem(const MetricsItem&);
  MetricsItem(MetricsItem&&);
  MetricsItem& operator=(const MetricsItem& other);
  MetricsItem& operator=(MetricsItem&& other);
  ~MetricsItem();

  using BucketMap = std::map<std::string, BucketValue>;

  BucketValue* GetAndIncrementTotal(const std::string& bucket_name);

  void IncrementTotal() { total_ += 1; };

  void Merge(const MetricsItem& other);

  void Finalize();

  const BucketMap& buckets() const { return buckets_; }
  size_t total() const { return total_; }

 private:
  // This is probably going to be equal to the total across buckets_, but
  // doesn't necessarily have to be. For instance with errors, there may be
  // multiple errors emitted per certificate processed. So MetricsItem will have
  // a total that is number of paths processed, whereas buckets total will be
  // all the errors.
  size_t total_ = 0;
  BucketMap buckets_;
};

class Metrics {
 public:
  Metrics();
  ~Metrics();

  using ItemMap = std::map<std::string, MetricsItem>;

  MetricsItem* GetAndIncrementTotal(const std::string& item_name);

  void Merge(const Metrics& metrics);
  void Finalize();

  const ItemMap& items() const { return items_; }

 private:
  ItemMap items_;
};

}  // namespace net

#endif  // NET_TOOLS_CERT_MAPPER_METRICS_H_
