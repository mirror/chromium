// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/declarative_net_request/utils.h"

#include <memory>
#include <set>
#include <utility>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
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

// Helper function to persist the indexed ruleset |data| for |extension|.
bool PersistRuleset(const Extension* extension,
                    const FlatRulesetIndexer::SerializedData& data) {
  const base::FilePath path = GetIndexedRulesetPath(extension);

  // Create the directory corresponding to |path| if it does not exist and then
  // persist the ruleset.
  const int data_size = base::checked_cast<int>(data.second);
  return base::CreateDirectory(path.DirName()) &&
         base::WriteFile(path, reinterpret_cast<const char*>(data.first),
                         data_size) == data_size;
}

// Helper to retrieve the ruleset ExtensionResource for |extension|.
const ExtensionResource* GetRulesetResource(const Extension* extension) {
  return declarative_net_request::DNRManifestData::GetRulesetResource(
      extension);
}

// Helper to retrieve the filename of the JSON ruleset provided by |extension|.
std::string GetJSONRulesetFilename(const Extension* extension) {
  base::ThreadRestrictions::AssertIOAllowed();
  return GetRulesetResource(extension)->GetFilePath().BaseName().AsUTF8Unsafe();
}

// Helper function to index |rules| and persist them to the
// |indexed_ruleset_path|.
ParseInfo IndexAndPersistRulesImpl(const base::Value& rules,
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
  UMA_HISTOGRAM_TIMES(kIndexAndPersistRulesDurationHistogram,
                      base::TimeTicks::Now() - start_index_time);
  UMA_HISTOGRAM_COUNTS_100000(kManifestRulesCountHistogram,
                              indexer.indexed_rules_count());

  return ParseInfo(ParseResult::SUCCESS);
}

}  // namespace

bool IndexAndPersistRules(const base::Value& rules,
                          const Extension* extension,
                          std::string* error,
                          std::vector<InstallWarning>* warnings) {
  DCHECK(IsAPIAvailable());
  DCHECK(extension);
  DCHECK(GetRulesetResource(extension));
  base::ThreadRestrictions::AssertIOAllowed();

  // TODO(crbug.com/696822): While persisting the ruleset, store the ruleset
  // checksum, probably as part of preferences. This would help in verifying the
  // ruleset while loading and help us ascertain quickly whether a ruleset
  // corresponding to an extension exists.

  const ParseInfo info = IndexAndPersistRulesImpl(rules, extension, warnings);
  if (info.result() == ParseResult::SUCCESS)
    return true;

  if (error)
    *error = info.GetErrorDescription(GetJSONRulesetFilename(extension));
  return false;
}

base::FilePath GetIndexedRulesetPath(const Extension* extension) {
  DCHECK(IsAPIAvailable());
  DCHECK(extension);
  DCHECK(GetRulesetResource(extension));

  // COMMENT: This should probably be in some subdirectory. Tried _metadata but
  // it will cause a warning when an unpacked extension is reloaded (it is not
  // whitelisted in CheckIllegalFilenames).
  return extension->path().Append(kIndexedRulesetFilename);
}

}  // namespace declarative_net_request
}  // namespace extensions
