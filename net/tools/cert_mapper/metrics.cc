// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/tools/cert_mapper/metrics.h"

#include "net/tools/cert_mapper/entry.h"

namespace net {

namespace {

size_t g_max_samples_per_bucket = 10;

}  // namespace

Sample::Sample() = default;
Sample::Sample(const Sample& other) = default;
Sample::Sample(Sample&& other) = default;
Sample& Sample::operator=(const Sample& other) = default;
Sample& Sample::operator=(Sample&& other) = default;
Sample::~Sample() = default;

BucketValue::BucketValue() = default;
BucketValue::BucketValue(const BucketValue&) = default;
BucketValue::BucketValue(BucketValue&&) = default;
BucketValue& BucketValue::operator=(const BucketValue& other) = default;
BucketValue& BucketValue::operator=(BucketValue&& other) = default;
BucketValue::~BucketValue() = default;

void BucketValue::AddSampleChain(const Entry& entry) {
  if (samples.size() >= g_max_samples_per_bucket)
    return;

  Sample sample;
  sample.certs = entry.certs;

  samples.push_back(std::move(sample));
}

void BucketValue::AddSampleCert(const scoped_refptr<CertBytes>& cert) {
  if (samples.size() >= g_max_samples_per_bucket)
    return;

  Sample sample;
  sample.certs.push_back(cert);

  samples.push_back(std::move(sample));
}

void BucketValue::Merge(const BucketValue& other) {
  total += other.total;

  // TODO(eroman): Should enforce the max samples constraint. Moreover for
  // replayability, order the samples by index before truncating.
  samples.insert(samples.end(), other.samples.begin(), other.samples.end());
}

void BucketValue::Finalize() {
  if (samples.size() > g_max_samples_per_bucket)
    samples.resize(g_max_samples_per_bucket);
}

void BucketValue::SetMaxSamples(size_t limit) {
  g_max_samples_per_bucket = limit;
}

MetricsItem::MetricsItem() = default;
MetricsItem::MetricsItem(const MetricsItem&) = default;
MetricsItem::MetricsItem(MetricsItem&&) = default;
MetricsItem& MetricsItem::operator=(const MetricsItem& other) = default;
MetricsItem& MetricsItem::operator=(MetricsItem&& other) = default;
MetricsItem::~MetricsItem() = default;

BucketValue* MetricsItem::GetAndIncrementTotal(const std::string& bucket_name) {
  BucketValue* result = &buckets_[bucket_name];
  result->IncrementTotal();
  return result;
}

void MetricsItem::Merge(const MetricsItem& other) {
  total_ += other.total_;

  for (const auto& it : other.buckets_) {
    const std::string& key = it.first;
    const BucketValue& other_value = it.second;

    BucketValue& value = buckets_[key];
    value.Merge(other_value);
  }
}

void MetricsItem::Finalize() {
  // Yuck. Because merging doesn't strictly adhere to the constraint, apply it
  // at the end.
  for (auto& it : buckets_) {
    it.second.Finalize();
  }
}

Metrics::Metrics() = default;
Metrics::~Metrics() = default;

MetricsItem* Metrics::GetAndIncrementTotal(const std::string& item_name) {
  MetricsItem* result = &items_[item_name];
  result->IncrementTotal();
  return result;
}

void Metrics::Merge(const Metrics& other) {
  for (const auto& it : other.items_) {
    const std::string& key = it.first;
    const MetricsItem& other_value = it.second;

    MetricsItem& value = items_[key];
    value.Merge(other_value);
  }
}

void Metrics::Finalize() {
  for (auto& it : items_) {
    it.second.Finalize();
  }
}

}  // namespace net
