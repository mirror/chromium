// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/declarative_net_request/utils.h"

#include <memory>
#include <set>
#include <utility>

#include <base/debug/stack_trace.h>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/json/json_file_value_serializer.h"
#include "base/metrics/histogram_macros.h"
#include "base/stl_util.h"
#include "base/threading/thread_restrictions.h"
#include "base/values.h"
#include "extensions/browser/api/declarative_net_request/constants.h"
#include "extensions/browser/api/declarative_net_request/flat_ruleset_indexer.h"
#include "extensions/browser/api/declarative_net_request/indexed_rule.h"
#include "extensions/browser/api/declarative_net_request/parse_info.h"
#include "extensions/common/api/declarative_net_request.h"
#include "extensions/common/api/declarative_net_request/dnr_manifest_data.h"
#include "extensions/common/api/declarative_net_request/utils.h"
#include "extensions/common/constants.h"
#include "extensions/common/install_warning.h"
#include "extensions/common/manifest_constants.h"

namespace extensions {
namespace declarative_net_request {
namespace {

namespace dnr_api = extensions::api::declarative_net_request;

// Helper function to persist the indexed ruleset |data| at location provided by
// |path|.
bool PersistRuleset(const Extension* extension,
                    const FlatRulesetIndexer::SerializedData& data) {
  base::FilePath path = GetIndexedRulesetPath(extension);

  // Create the directory corresponding to |path| if it does not exist and then
  // persist the ruleset.
  const int data_size = base::checked_cast<int>(data.second);
  return base::CreateDirectory(path.DirName()) &&
         base::WriteFile(path, reinterpret_cast<const char*>(data.first),
                         data_size) == data_size;
}

std::string GetJSONRulesetFilename(const Extension* extension) {
  return declarative_net_request::DNRManifestData::GetRulesetResource(extension)
      ->GetFilePath()
      .BaseName()
      .AsUTF8Unsafe();
}

// Helper function to index |rules| and persist them to the
// |indexed_ruleset_path|.
ParseInfo IndexAndPersistRulesetImpl(const base::Value& rules,
                                     const Extension* extension,
                                     std::vector<InstallWarning>* warnings) {
  base::ThreadRestrictions::AssertIOAllowed();
  const base::ListValue* rules_list = nullptr;
  if (!rules.GetAsList(&rules_list))
    return ParseInfo(ParseResult::ERROR_LIST_NOT_PASSED);

  FlatRulesetIndexer indexer;
  bool all_rules_parsed = true;
  const base::TimeTicks start_index_time = base::TimeTicks::Now();
  {
    std::set<int> id_set;  // Ensure all ids are distinct.
    std::unique_ptr<dnr_api::Rule> parsed_rule;

    size_t index = 0;
    for (auto iter = rules_list->begin(); iter != rules_list->end();
         iter++, index++) {
      parsed_rule = dnr_api::Rule::FromValue(*iter);

      // Ignore rules which can't be successfully parsed and show an install
      // warning for them.
      if (!parsed_rule) {
        all_rules_parsed = false;
        continue;
      }

      if (base::ContainsKey(id_set, parsed_rule->id))
        return ParseInfo(ParseResult::ERROR_DUPLICATE_IDS, index);
      id_set.insert(parsed_rule->id);

      IndexedRule indexed_rule;
      ParseResult parse_result =
          IndexedRule::CreateIndexedRule(std::move(parsed_rule), &indexed_rule);
      if (parse_result != ParseResult::SUCCESS)
        return ParseInfo(parse_result, index);

      indexer.AddUrlRule(indexed_rule);
    }
  }
  const base::TimeTicks end_index_time = base::TimeTicks::Now();

  // The actual data buffer is still owned by |indexer|.
  indexer.Finish();
  const FlatRulesetIndexer::SerializedData data = indexer.GetData();
  if (!PersistRuleset(extension, data))
    return ParseInfo(ParseResult::ERROR_PERSISTING_RULESET);

  if (!all_rules_parsed && warnings) {
    warnings->push_back(InstallWarning(
        kRulesNotPassedWarning, manifest_keys::kDeclarativeNetRequestKey,
        manifest_keys::kDeclarativeRuleResourcesKey));
  }
  UMA_HISTOGRAM_TIMES(kIndexRulesDurationHistogram,
                      end_index_time - start_index_time);
  UMA_HISTOGRAM_COUNTS_100000(kManifestRulesCountHistogram,
                              indexer.indexed_rules_count());

  return ParseInfo(ParseResult::SUCCESS);
}

}  // namespace

bool IndexAndPersistRuleset(const base::Value& rules,
                            const Extension* extension,
                            std::string* error,
                            std::vector<InstallWarning>* warnings) {
  DCHECK(IsAPIAvailable());
  DCHECK(extension);
  base::ThreadRestrictions::AssertIOAllowed();

  const base::TimeTicks start_time = base::TimeTicks::Now();

  const ParseInfo info = IndexAndPersistRulesetImpl(rules, extension, warnings);

  if (info.result() != ParseResult::SUCCESS) {
    if (error)
      *error = info.GetErrorDescription(GetJSONRulesetFilename(extension));
    return false;
  }

  UMA_HISTOGRAM_TIMES(kIndexAndPersistRulesDurationHistogram,
                      base::TimeTicks::Now() - start_time);

  return true;
}

base::FilePath GetIndexedRulesetPath(const Extension* extension) {
  // This should probably be in some sub directory.
  DCHECK(extension);
  return extension->path().Append(kIndexedRulesetFilename);
}

}  // namespace declarative_net_request
}  // namespace extensions
