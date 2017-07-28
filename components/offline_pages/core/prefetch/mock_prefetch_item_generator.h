// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_MOCK_PREFETCH_ITEM_GENERATOR_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_MOCK_PREFETCH_ITEM_GENERATOR_H_

#include <string>

#include "components/offline_pages/core/prefetch/prefetch_item.h"
#include "components/offline_pages/core/prefetch/prefetch_types.h"

namespace offline_pages {

static const char kMockClientNamespace[] = "test_prefetch_namespace";
static const char kMockClientIdPrefix[] = "test_client_id_";
static const char kMockUrlPrefix[] = "http://www.requested.com/";
static const char kMockFinalUrlPrefix[] = "http://www.final.com/";
static const char kMockOperationNamePrefix[] = "test_operation_name_";
static const char kMockArchiveBodyNamePrefix[] = "test_archive_body_name_";

// Generator of PrefetchItem instances with all fields automatically
// pre-populated with values that are reasonable and unique (within the
// instance). To further customize returned items one can set custom prefixes or
// just change the actual values of returned instances.
class MockPrefetchItemGenerator {
 public:
  MockPrefetchItemGenerator();
  ~MockPrefetchItemGenerator();

  // Creates a new item using all set prefixes and an internal counter to set
  // reasonable and unique values to all fields.
  PrefetchItem CreateItem(PrefetchItemState state);

  // Setters for all prefixes.
  void set_client_namespace(std::string client_namespace) {
    client_namespace_ = client_namespace;
  }
  void set_client_id_prefix(std::string client_id_prefix) {
    client_id_prefix_ = client_id_prefix;
  }
  void set_url_prefix(std::string url_prefix) { url_prefix_ = url_prefix; }
  void set_final_url_prefix(std::string final_url_prefix) {
    final_url_prefix_ = final_url_prefix;
  }
  void set_operation_name_prefix(std::string operation_name_prefix) {
    operation_name_prefix_ = operation_name_prefix;
  }
  void set_archive_body_name_prefix(std::string archive_body_name_prefix) {
    archive_body_name_prefix_ = archive_body_name_prefix;
  }

 private:
  // These namespace name and prefixes must always be set.
  std::string client_namespace_ = kMockClientNamespace;
  std::string client_id_prefix_ = kMockClientIdPrefix;
  std::string url_prefix_ = kMockUrlPrefix;

  // These prefixes, if custom set to the empty string, will cause related
  // values not to be set.
  std::string final_url_prefix_ = kMockFinalUrlPrefix;
  std::string operation_name_prefix_ = kMockOperationNamePrefix;
  std::string archive_body_name_prefix_ = kMockArchiveBodyNamePrefix;
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_MOCK_PREFETCH_ITEM_GENERATOR_H_
