// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/common/api/declarative_net_request/utils.h"

#include <memory>
#include <set>
#include <utility>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/json/json_file_value_serializer.h"
#include "base/metrics/histogram_macros.h"
#include "base/optional.h"
#include "base/stl_util.h"
#include "base/threading/thread_restrictions.h"
#include "components/version_info/channel.h"
#include "extensions/common/api/declarative_net_request.h"
#include "extensions/common/api/declarative_net_request/constants.h"
#include "extensions/common/api/declarative_net_request/indexed_rule.h"
#include "extensions/common/api/declarative_net_request/ruleset_indexer.h"
#include "extensions/common/error_utils.h"
#include "extensions/common/features/feature_channel.h"
#include "extensions/common/manifest_constants.h"

namespace extensions {
namespace errors = manifest_errors;
namespace declarative_net_request {
namespace {

constexpr version_info::Channel kAPIChannel = version_info::Channel::UNKNOWN;

// Holds the |parse_result| together with the rule index at which the error, if
// any, occurred.
struct ParseInfo {
  ParseInfo(ParseResult result, size_t rule_index)
      : result(result), rule_index(rule_index) {}
  explicit ParseInfo(ParseResult result) : result(result) {}

  ParseResult result;
  base::Optional<size_t> rule_index;
};

// Helper function to persist the indexed ruleset |data| at location provided by
// |path|.
bool PersistRuleset(const base::FilePath& path,
                    const RulesetIndexer::SerializedData& data) {
  // Create the directory corresponding to |path| if it does not exist and then
  // persist the ruleset.
  const int data_size = base::checked_cast<int>(data.second);
  return base::CreateDirectory(path.DirName()) &&
         base::WriteFile(path, reinterpret_cast<const char*>(data.first),
                         data_size) == data_size;
}

// Helper function to generate an error description script from |info|.
std::string GenerateErrorDescription(const ParseInfo& info,
                                     const std::string& file_name) {
  std::string error;
  switch (info.result) {
    case ParseResult::SUCCESS:
      NOTREACHED();
      break;
    case ParseResult::ERROR_LIST_NOT_PASSED:
      DCHECK(!info.rule_index);
      error = ErrorUtils::FormatErrorMessage(errors::kRulesFileIsNotList,
                                             file_name);
      break;
    case ParseResult::ERROR_DUPLICATE_IDS:
      DCHECK(info.rule_index);
      error = ErrorUtils::FormatErrorMessage(errors::kRulesFileDuplicateID,
                                             file_name,
                                             std::to_string(*info.rule_index));
      break;
    case ParseResult::ERROR_RESOURCE_TYPE_DUPLICATED:
      DCHECK(info.rule_index);
      error = ErrorUtils::FormatErrorMessage(
          errors::kRulesFileDuplicateResourceType, file_name,
          std::to_string(*info.rule_index));
      break;
    case ParseResult::ERROR_PERSISTING_RULESET:
      DCHECK(!info.rule_index);
      error = ErrorUtils::FormatErrorMessage(errors::kRulesFileErrorPersisting,
                                             file_name);
      break;
    case ParseResult::ERROR_EMPTY_REDIRECT_RULE_PRIORITY:
      DCHECK(info.rule_index);
      error = ErrorUtils::FormatErrorMessage(
          errors::kRulesFileRedirectRuleKeyEmpty, file_name,
          std::to_string(*info.rule_index), kPriorityKey);
      break;
    case ParseResult::ERROR_EMPTY_REDIRECT_URL:
      DCHECK(info.rule_index);
      error = ErrorUtils::FormatErrorMessage(
          errors::kRulesFileRedirectRuleKeyEmpty, file_name,
          std::to_string(*info.rule_index), kRedirectUrlKey);
      break;
    case ParseResult::ERROR_INVALID_RULE_ID:
      DCHECK(info.rule_index);
      error = ErrorUtils::FormatErrorMessage(
          errors::kRulesFileInvalidKey, file_name,
          std::to_string(*info.rule_index), kIDKey,
          std::to_string(kMinValidID));
      break;
    case ParseResult::ERROR_INVALID_REDIRECT_RULE_PRIORITY:
      DCHECK(info.rule_index);
      error = ErrorUtils::FormatErrorMessage(
          errors::kRulesFileInvalidKey, file_name,
          std::to_string(*info.rule_index), kPriorityKey,
          std::to_string(kMinValidPriority));
      break;
    case ParseResult::ERROR_JSON_PARSE:
      DCHECK(info.rule_index);
      error = ErrorUtils::FormatErrorMessage(errors::kRulesFileJSONParseError,
                                             file_name,
                                             std::to_string(*info.rule_index));
      break;
    case ParseResult::ERROR_RULE_NOT_MEANINGFUL:
      DCHECK(info.rule_index);
      error = ErrorUtils::FormatErrorMessage(
          errors::kRulesFileRuleNotMeaningful, file_name,
          std::to_string(*info.rule_index));
      break;
  }
  return error;
}

// Helper function to index |rules| and persist them to the
// |indexed_ruleset_path|.
ParseInfo IndexAndPersistRuleset(const base::Value& rules,
                                 const base::FilePath& indexed_ruleset_path) {
  base::ThreadRestrictions::AssertIOAllowed();

  const base::ListValue* rules_list = nullptr;
  if (!rules.GetAsList(&rules_list))
    return ParseInfo(ParseResult::ERROR_LIST_NOT_PASSED);

  RulesetIndexer indexer;
  {
    using ParsedRule = extensions::api::declarative_net_request::Rule;

    const base::TimeTicks start_index_time = base::TimeTicks::Now();

    std::set<int> id_set;  // Ensure all ids are distinct.
    std::unique_ptr<ParsedRule> parsed_rule;

    for (auto iter = rules_list->begin(); iter != rules_list->end(); iter++) {
      const size_t index = iter - rules_list->begin();

      // COMMENT: use generate_error_messages to get obtain parse errors? It is
      // used only in manifest_types.json.
      parsed_rule = ParsedRule::FromValue(*iter);
      if (!parsed_rule)
        return ParseInfo(ParseResult::ERROR_JSON_PARSE, index);

      if (base::ContainsKey(id_set, parsed_rule->id))
        return ParseInfo(ParseResult::ERROR_DUPLICATE_IDS, index);
      id_set.insert(parsed_rule->id);

      IndexedRule indexed_rule;
      ParseResult parse_result =
          CreateIndexedRule(std::move(parsed_rule), &indexed_rule);
      if (parse_result != ParseResult::SUCCESS)
        return ParseInfo(parse_result, index);

      indexer.AddUrlRule(indexed_rule);
    }
    UMA_HISTOGRAM_TIMES("Extensions.DeclarativeNetRequest.IndexRulesDuration",
                        base::TimeTicks::Now() - start_index_time);
  }

  // The actual data buffer is still owned by |indexer|.
  const RulesetIndexer::SerializedData data = indexer.FinishAndGetData();
  if (!PersistRuleset(indexed_ruleset_path, data))
    return ParseInfo(ParseResult::ERROR_PERSISTING_RULESET);

  return ParseInfo(ParseResult::SUCCESS);
}

}  // namespace

bool IndexAndPersistRuleset(const base::FilePath& json_rules_path,
                            const base::FilePath& indexed_ruleset_path,
                            std::string* error) {
  base::ThreadRestrictions::AssertIOAllowed();
  SCOPED_UMA_HISTOGRAM_TIMER(
      "Extensions.DeclarativeNetRequest.IndexAndPersistRulesetDuration");

  const base::TimeTicks start_deserialization_time = base::TimeTicks::Now();
  JSONFileValueDeserializer deserializer(json_rules_path);
  std::unique_ptr<base::Value> rules =
      deserializer.Deserialize(nullptr /*error_code*/, error);
  if (!rules)
    return false;  // |error| will be populated by the call to Deserialize.
  UMA_HISTOGRAM_TIMES(
      "Extensions.DeclarativeNetRequest.JSONDeserializeDuration",
      base::TimeTicks::Now() - start_deserialization_time);

  const ParseInfo info = IndexAndPersistRuleset(*rules, indexed_ruleset_path);

  if (info.result != ParseResult::SUCCESS) {
    if (error) {
      *error = GenerateErrorDescription(
          info, json_rules_path.BaseName().AsUTF8Unsafe());
    }
    return false;
  }
  return true;
}

bool IsAPIAvailable() {
  // TODO(karandeepb): Use Feature::IsAvailableToEnvironment() once it behaves
  // correctly.
  return kAPIChannel >= GetCurrentChannel();
}

}  // namespace declarative_net_request
}  // namespace extensions
