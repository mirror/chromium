// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/traffic_annotation/auditor/traffic_annotation_auditor.h"

#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/process/launch.h"
#include "base/stl_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "third_party/protobuf/src/google/protobuf/io/tokenizer.h"
#include "third_party/protobuf/src/google/protobuf/text_format.h"
#include "tools/traffic_annotation/auditor/traffic_annotation_file_filter.h"

namespace {

// Recursively compute the hash code of the given string as in:
// "net/traffic_annotation/network_traffic_annotation.h"
uint32_t recursive_hash(const char* str, int N) {
  if (N == 1)
    return static_cast<uint32_t>(str[0]);
  else
    return (recursive_hash(str, N - 1) * 31 + str[N - 1]) % 138003713;
}

std::map<int, std::string> kReservedAnnotations = {
    {TRAFFIC_ANNOTATION_FOR_TESTS.unique_id_hash_code, "test"},
    {PARTIAL_TRAFFIC_ANNOTATION_FOR_TESTS.unique_id_hash_code, "test_partial"},
    {NO_TRAFFIC_ANNOTATION_YET.unique_id_hash_code, "undefined"},
    {MISSING_TRAFFIC_ANNOTATION.unique_id_hash_code, "missing"},
};

// This macro merges the content of one string field from two annotations.
// DST->FLD is the destination field, and SRD->FLD is the source field.
#define MERGE_STRING_FIELDS(SRC, DST, FLD)                           \
  if (!SRC.FLD().empty()) {                                          \
    if (!DST->FLD().empty()) {                                       \
      DST->set_##FLD(base::StringPrintf("%s\n%s", SRC.FLD().c_str(), \
                                        DST->FLD().c_str()));        \
    } else {                                                         \
      DST->set_##FLD(SRC.FLD());                                     \
    }                                                                \
  }

// This class receives parsing errors from google::protobuf::TextFormat::Parser
// which is used during protobuf deserialization.
class SimpleErrorCollector : public google::protobuf::io::ErrorCollector {
 public:
  SimpleErrorCollector(int proto_starting_line)
      : google::protobuf::io::ErrorCollector(),
        line_offset_(proto_starting_line) {}

  ~SimpleErrorCollector() override {}

  void AddError(int line,
                google::protobuf::io::ColumnNumber column,
                const std::string& message) override {
    AddMessage(line, column, message);
  }

  void AddWarning(int line,
                  google::protobuf::io::ColumnNumber column,
                  const std::string& message) override {
    AddMessage(line, column, message);
  }

  std::string GetMessage() { return message_; }

 private:
  void AddMessage(int line,
                  google::protobuf::io::ColumnNumber column,
                  const std::string& message) {
    message_ += base::StringPrintf(
        "%sLine %i, column %i, %s", message_.length() ? " " : "",
        line_offset_ + line, static_cast<int>(column), message.c_str());
  }

  std::string message_;
  int line_offset_;
};

}  // namespace

const int AuditorResult::kNoCodeLineSpecified = -1;

AuditorResult::AuditorResult(Type type,
                             const std::string& message,
                             const std::string& file_path,
                             int line)
    : type_(type), file_path_(file_path), line_(line) {
  DCHECK(line != kNoCodeLineSpecified ||
         type == AuditorResult::Type::RESULT_OK ||
         type == AuditorResult::Type::RESULT_IGNORE ||
         type == AuditorResult::Type::ERROR_FATAL ||
         type == AuditorResult::Type::ERROR_DUPLICATE_UNIQUE_ID_HASH_CODE ||
         type == AuditorResult::Type::ERROR_MERGE_FAILED);
  DCHECK(!message.empty() || type == AuditorResult::Type::RESULT_OK ||
         type == AuditorResult::Type::RESULT_IGNORE ||
         type == AuditorResult::Type::ERROR_MISSING ||
         type == AuditorResult::Type::ERROR_NO_ANNOTATION ||
         type == AuditorResult::Type::ERROR_MISSING_EXTRA_ID ||
         type == AuditorResult::Type::ERROR_INCOMPLETED_ANNOTATION);
  if (!message.empty())
    details_.push_back(message);
};

AuditorResult::AuditorResult(Type type, const std::string& message)
    : AuditorResult::AuditorResult(type,
                                   message,
                                   std::string(),
                                   kNoCodeLineSpecified) {}

AuditorResult::AuditorResult(Type type)
    : AuditorResult::AuditorResult(type,
                                   std::string(),
                                   std::string(),
                                   kNoCodeLineSpecified) {}

AuditorResult::AuditorResult(const AuditorResult& other)
    : type_(other.type_),
      details_(other.details_),
      file_path_(other.file_path_),
      line_(other.line_){};

AuditorResult::~AuditorResult() {}

void AuditorResult::AddDetail(const std::string& message) {
  details_.push_back(message);
}

std::string AuditorResult::ToText() const {
  switch (type_) {
    case AuditorResult::Type::ERROR_FATAL:
      DCHECK(details_.size());
      return details_[0];

    case AuditorResult::Type::ERROR_MISSING:
      return base::StringPrintf("Missing annotation in '%s', line %i.",
                                file_path_.c_str(), line_);

    case AuditorResult::Type::ERROR_NO_ANNOTATION:
      return base::StringPrintf("Empty annotation in '%s', line %i.",
                                file_path_.c_str(), line_);

    case AuditorResult::Type::ERROR_SYNTAX: {
      DCHECK(details_.size());
      std::string flat_message(details_[0]);
      std::replace(flat_message.begin(), flat_message.end(), '\n', ' ');
      return base::StringPrintf("Syntax error in '%s': %s", file_path_.c_str(),
                                flat_message.c_str());
    }

    case AuditorResult::Type::ERROR_RESERVED_UNIQUE_ID_HASH_CODE:
      DCHECK(details_.size());
      return base::StringPrintf(
          "Unique id '%s' in '%s:%i' has a hash code similar to a reserved "
          "word and should be changed.",
          details_[0].c_str(), file_path_.c_str(), line_);

    case AuditorResult::Type::ERROR_DUPLICATE_UNIQUE_ID_HASH_CODE:
      DCHECK(details_.size() == 2);
      return base::StringPrintf(
          "The following annotations have similar unique id "
          "hash codes and should be updated: %s, %s.",
          details_[0].c_str(), details_[1].c_str());

    case AuditorResult::Type::ERROR_UNIQUE_ID_INVALID_CHARACTER:
      DCHECK(details_.size());
      return base::StringPrintf(
          "Unique id '%s' in '%s:%i' contains an invalid character.",
          details_[0].c_str(), file_path_.c_str(), line_);

    case AuditorResult::Type::ERROR_MISSING_ANNOTATION:
      DCHECK(details_.size());
      return base::StringPrintf("Function '%s' in '%s:%i' requires annotation.",
                                details_[0].c_str(), file_path_.c_str(), line_);

    case AuditorResult::Type::ERROR_INCOMPLETE_ANNOTATION:
      DCHECK(details_.size());
      return base::StringPrintf(
          "Annotation at '%s:%i' has the following missing fields: %s",
          file_path_.c_str(), line_, details_[0].c_str());

    case AuditorResult::Type::ERROR_MISSING_EXTRA_ID:
      return base::StringPrintf(
          "Second id of annotation at '%s:%i' should be updated as it has the "
          "same hash code as the first one.",
          file_path_.c_str(), line_);

    case AuditorResult::Type::ERROR_INCONSISTENT_ANNOTATION:
      DCHECK(details_.size());
      return base::StringPrintf(
          "Annotation at '%s:%i' has the following inconsistencies: %s",
          file_path_.c_str(), line_, details_[0].c_str());

    case AuditorResult::Type::ERROR_MERGE_FAILED:
      DCHECK(details_.size() == 3);
      return base::StringPrintf(
          "Annotations '%s' and '%s' cannot be merged due to the following "
          "error(s): %s",
          details_[1].c_str(), details_[2].c_str(), details_[0].c_str());

    case AuditorResult::Type::ERROR_INCOMPLETED_ANNOTATION:
      return base::StringPrintf("Annotation at '%s:%i' is never completed.",
                                file_path_.c_str(), line_);

    default:
      return std::string();
  }
}

std::string AuditorResult::ToShortText() const {
  switch (type_) {
    case AuditorResult::Type::ERROR_INCOMPLETE_ANNOTATION:
      DCHECK(details_.size());
      return base::StringPrintf("the following fields are missing: %s",
                                details_[0].c_str());

    case AuditorResult::Type::ERROR_INCONSISTENT_ANNOTATION:
      DCHECK(details_.size());
      return base::StringPrintf("the following inconsistencies: %s",
                                details_[0].c_str());

    default:
      NOTREACHED();
      return std::string();
  }
}

AnnotationInstance::AnnotationInstance() : type(Type::ANNOTATION_COMPLETE) {}

AnnotationInstance::AnnotationInstance(const AnnotationInstance& other)
    : proto(other.proto),
      type(other.type),
      extra_id(other.extra_id),
      unique_id_hash_code(other.unique_id_hash_code),
      extra_id_hash_code(other.extra_id_hash_code){};

AuditorResult AnnotationInstance::Deserialize(
    const std::vector<std::string>& serialized_lines,
    int start_line,
    int end_line) {
  if (end_line - start_line < 7) {
    return AuditorResult(AuditorResult::Type::ERROR_FATAL,
                         "Not enough lines to deserialize annotation.");
  }

  // Extract header lines.
  const std::string& file_path = serialized_lines[start_line++];
  const std::string& function_context = serialized_lines[start_line++];
  int line_number;
  base::StringToInt(serialized_lines[start_line++], &line_number);
  std::string function_type = serialized_lines[start_line++];
  const std::string& unique_id = serialized_lines[start_line++];
  extra_id = serialized_lines[start_line++];

  // Decode function type.
  if (function_type == "Definition") {
    type = Type::ANNOTATION_COMPLETE;
  } else if (function_type == "Partial") {
    type = Type::ANNOTATION_PARTIAL;
  } else if (function_type == "Completing") {
    type = Type::ANNOTATION_COMPLETING;
  } else if (function_type == "BranchedCompleting") {
    type = Type::ANNOTATION_BRANCHED_COMPLETING;
  } else {
    return AuditorResult(AuditorResult::Type::ERROR_FATAL,
                         base::StringPrintf("Unexpected function type: %s",
                                            function_type.c_str()));
  }

  // Process test tags.
  unique_id_hash_code = TrafficAnnotationAuditor::ComputeHashValue(unique_id);
  if (unique_id_hash_code == TRAFFIC_ANNOTATION_FOR_TESTS.unique_id_hash_code ||
      unique_id_hash_code ==
          PARTIAL_TRAFFIC_ANNOTATION_FOR_TESTS.unique_id_hash_code) {
    return AuditorResult(AuditorResult::Type::RESULT_IGNORE);
  }

  // Process undefined tags.
  if (unique_id_hash_code == NO_TRAFFIC_ANNOTATION_YET.unique_id_hash_code ||
      unique_id_hash_code ==
          NO_PARTIAL_TRAFFIC_ANNOTATION_YET.unique_id_hash_code) {
    return AuditorResult(AuditorResult::Type::ERROR_NO_ANNOTATION, "",
                         file_path, line_number);
  }

  // Process missing tag.
  if (unique_id_hash_code == MISSING_TRAFFIC_ANNOTATION.unique_id_hash_code)
    return AuditorResult(AuditorResult::Type::ERROR_MISSING, "", file_path,
                         line_number);

  // Decode serialized proto.
  std::string annotation_text = "";
  while (start_line < end_line) {
    annotation_text += serialized_lines[start_line++] + "\n";
  }

  SimpleErrorCollector error_collector(line_number);
  google::protobuf::TextFormat::Parser parser;
  parser.RecordErrorsTo(&error_collector);
  if (!parser.ParseFromString(annotation_text,
                              (google::protobuf::Message*)&proto)) {
    return AuditorResult(AuditorResult::Type::ERROR_SYNTAX,
                         error_collector.GetMessage().c_str(), file_path,
                         line_number);
  }

  // Add other fields.
  traffic_annotation::NetworkTrafficAnnotation_TrafficSource* src =
      proto.mutable_source();
  src->set_file(file_path);
  src->set_function(function_context);
  src->set_line(line_number);
  proto.set_unique_id(unique_id);
  extra_id_hash_code = TrafficAnnotationAuditor::ComputeHashValue(extra_id);

  return AuditorResult(AuditorResult::Type::RESULT_OK);
}

// Checks if an annotation has all required fields.
AuditorResult AnnotationInstance::IsComplete() const {
  std::vector<std::string> unspecifieds;
  std::string extra_texts;

  const traffic_annotation::NetworkTrafficAnnotation_TrafficSemantics
      semantics = proto.semantics();
  if (semantics.sender().empty())
    unspecifieds.push_back("semantics::sender");
  if (semantics.description().empty())
    unspecifieds.push_back("semantics::description");
  if (semantics.trigger().empty())
    unspecifieds.push_back("semantics::trigger");
  if (semantics.data().empty())
    unspecifieds.push_back("semantics::data");
  if (semantics.destination() ==
      traffic_annotation::
          NetworkTrafficAnnotation_TrafficSemantics_Destination_UNSPECIFIED)
    unspecifieds.push_back("semantics::destination");

  const traffic_annotation::NetworkTrafficAnnotation_TrafficPolicy policy =
      proto.policy();
  if (policy.cookies_allowed() ==
      traffic_annotation::
          NetworkTrafficAnnotation_TrafficPolicy_CookiesAllowed_UNSPECIFIED) {
    unspecifieds.push_back("policy::cookies_allowed");
  } else if (
      policy.cookies_allowed() ==
          traffic_annotation::
              NetworkTrafficAnnotation_TrafficPolicy_CookiesAllowed_YES &&
      policy.cookies_store().empty()) {
    unspecifieds.push_back("policy::cookies_store");
  }

  if (policy.setting().empty())
    unspecifieds.push_back("policy::setting");

  if (!policy.chrome_policy_size() &&
      policy.policy_exception_justification().empty()) {
    unspecifieds.push_back(
        "neither policy::chrome_policy nor "
        "policy::policy_exception_justification");
  }

  if (unspecifieds.size()) {
    std::string error_text;
    for (const std::string& item : unspecifieds)
      error_text += item + ", ";
    error_text = error_text.substr(0, error_text.length() - 2);
    return AuditorResult(AuditorResult::Type::ERROR_INCOMPLETE_ANNOTATION,
                         error_text, proto.source().file(),
                         proto.source().line());
  }

  return AuditorResult(AuditorResult::Type::RESULT_OK);
}

// Checks if annotation fields are consistent.
AuditorResult AnnotationInstance::IsConsistent() const {
  const traffic_annotation::NetworkTrafficAnnotation_TrafficPolicy policy =
      proto.policy();

  if (policy.cookies_allowed() ==
          traffic_annotation::
              NetworkTrafficAnnotation_TrafficPolicy_CookiesAllowed_NO &&
      policy.cookies_store().size()) {
    return AuditorResult(
        AuditorResult::Type::ERROR_INCONSISTENT_ANNOTATION,
        "Cookies store is specified while cookies are not allowed.",
        proto.source().file(), proto.source().line());
  }

  if (policy.chrome_policy_size() &&
      policy.policy_exception_justification().size()) {
    return AuditorResult(
        AuditorResult::Type::ERROR_INCONSISTENT_ANNOTATION,
        "Both chrome policies and policy exception justification are present.",
        proto.source().file(), proto.source().line());
  }

  return AuditorResult(AuditorResult::Type::RESULT_OK);
}

bool AnnotationInstance::IsCompletableWith(
    const AnnotationInstance& other) const {
  if (type != AnnotationInstance::Type::ANNOTATION_PARTIAL || extra_id.empty())
    return false;
  if (other.type == AnnotationInstance::Type::ANNOTATION_COMPLETING) {
    return extra_id_hash_code == other.unique_id_hash_code;
  } else if (other.type ==
             AnnotationInstance::Type::ANNOTATION_BRANCHED_COMPLETING) {
    return extra_id_hash_code == other.extra_id_hash_code;
  } else {
    return false;
  }
}

AuditorResult AnnotationInstance::CreateCompleteAnnotation(
    AnnotationInstance& completing_annotation,
    AnnotationInstance* combination) const {
  DCHECK(IsCompletableWith(completing_annotation));

  // To keep the source information meta data, if completing annotation is of
  // type COMPLETING, keep |this| as the main and the other as completing.
  // But if compliting annotation is of type BRANCHED_COMPLETING, reverse
  // the order.
  const AnnotationInstance* other;
  if (completing_annotation.type ==
      AnnotationInstance::Type::ANNOTATION_COMPLETING) {
    *combination = *this;
    other = &completing_annotation;
  } else {
    *combination = completing_annotation;
    other = this;
  }

  combination->type = AnnotationInstance::Type::ANNOTATION_COMPLETE;
  combination->extra_id.clear();
  combination->extra_id_hash_code = 0;
  combination->comments = base::StringPrintf(
      "This annotation is a merge of the following two annotations:\n"
      "'%s' in '%s:%i' and '%s' in '%s:%i'.",
      proto.unique_id().c_str(), proto.source().file().c_str(),
      proto.source().line(), completing_annotation.proto.unique_id().c_str(),
      completing_annotation.proto.source().file().c_str(),
      completing_annotation.proto.source().line());

  // Copy TrafficSemantics
  const traffic_annotation::NetworkTrafficAnnotation_TrafficSemantics
      src_semantics = other->proto.semantics();
  traffic_annotation::NetworkTrafficAnnotation_TrafficSemantics* dst_semantics =
      combination->proto.mutable_semantics();

  MERGE_STRING_FIELDS(src_semantics, dst_semantics, empty_policy_justification);
  MERGE_STRING_FIELDS(src_semantics, dst_semantics, sender);
  MERGE_STRING_FIELDS(src_semantics, dst_semantics, description);
  MERGE_STRING_FIELDS(src_semantics, dst_semantics, trigger);
  MERGE_STRING_FIELDS(src_semantics, dst_semantics, data);
  MERGE_STRING_FIELDS(src_semantics, dst_semantics, destination_other);

  // If destination is not specified in dst_semantics, get it from
  // src_semantics. If both are specified and they differ, issue error.
  if (dst_semantics->destination() ==
      traffic_annotation::
          NetworkTrafficAnnotation_TrafficSemantics_Destination_UNSPECIFIED) {
    dst_semantics->set_destination(src_semantics.destination());
  } else if (
      src_semantics.destination() !=
          traffic_annotation::
              NetworkTrafficAnnotation_TrafficSemantics_Destination_UNSPECIFIED &&
      src_semantics.destination() != dst_semantics->destination()) {
    AuditorResult error(
        AuditorResult::Type::ERROR_MERGE_FAILED,
        "Annotations contain different semantics::destination values.");
    error.AddDetail(proto.unique_id());
    error.AddDetail(completing_annotation.proto.unique_id());
    return error;
  }

  // Copy TrafficPolicy
  const traffic_annotation::NetworkTrafficAnnotation_TrafficPolicy src_policy =
      other->proto.policy();
  traffic_annotation::NetworkTrafficAnnotation_TrafficPolicy* dst_policy =
      combination->proto.mutable_policy();

  MERGE_STRING_FIELDS(src_policy, dst_policy, cookies_store);
  MERGE_STRING_FIELDS(src_policy, dst_policy, setting);

  // Set cookies_allowed to the superseding value of both.
  dst_policy->set_cookies_allowed(
      std::max(dst_policy->cookies_allowed(), src_policy.cookies_allowed()));
  DCHECK_GT(traffic_annotation::
                NetworkTrafficAnnotation_TrafficPolicy_CookiesAllowed_YES,
            traffic_annotation::
                NetworkTrafficAnnotation_TrafficPolicy_CookiesAllowed_NO);
  DCHECK_GT(
      traffic_annotation::
          NetworkTrafficAnnotation_TrafficPolicy_CookiesAllowed_NO,
      traffic_annotation::
          NetworkTrafficAnnotation_TrafficPolicy_CookiesAllowed_UNSPECIFIED);

  for (int i = 0; i < src_policy.chrome_policy_size(); i++)
    *dst_policy->add_chrome_policy() = src_policy.chrome_policy(i);

  if (!src_policy.policy_exception_justification().empty()) {
    if (!dst_policy->policy_exception_justification().empty()) {
      dst_policy->set_policy_exception_justification(
          dst_policy->policy_exception_justification() + "\n");
    }
    dst_policy->set_policy_exception_justification(
        dst_policy->policy_exception_justification() +
        src_policy.policy_exception_justification());
  }

  return AuditorResult::Type::RESULT_OK;
}

CallInstance::CallInstance() : line_number(0), is_annotated(false) {}

CallInstance::CallInstance(const CallInstance& other)
    : file_path(other.file_path),
      line_number(other.line_number),
      function_context(other.function_context),
      function_name(other.function_name),
      is_annotated(other.is_annotated){};

AuditorResult CallInstance::Deserialize(
    const std::vector<std::string>& serialized_lines,
    int start_line,
    int end_line) {
  if (end_line - start_line != 5) {
    return AuditorResult(AuditorResult::Type::ERROR_FATAL,
                         "Not enough lines to deserialize call.");
  }

  file_path = serialized_lines[start_line++];
  function_context = serialized_lines[start_line++];
  int line_number_int;
  base::StringToInt(serialized_lines[start_line++], &line_number_int);
  line_number = static_cast<uint32_t>(line_number_int);
  function_name = serialized_lines[start_line++];
  int is_annotated_int;
  base::StringToInt(serialized_lines[start_line++], &is_annotated_int);
  is_annotated = is_annotated_int != 0;
  return AuditorResult(AuditorResult::Type::RESULT_OK);
}

int TrafficAnnotationAuditor::ComputeHashValue(const std::string& unique_id) {
  return unique_id.length() ? static_cast<int>(recursive_hash(
                                  unique_id.c_str(), unique_id.length()))
                            : -1;
}

TrafficAnnotationAuditor::TrafficAnnotationAuditor(
    const base::FilePath& source_path,
    const base::FilePath& build_path)
    : source_path_(source_path), build_path_(build_path) {
  LoadWhiteList();
};

TrafficAnnotationAuditor::~TrafficAnnotationAuditor(){};

bool TrafficAnnotationAuditor::RunClangTool(
    const std::vector<std::string>& path_filters,
    const bool full_run) {
  base::FilePath options_filepath;
  if (!base::CreateTemporaryFile(&options_filepath)) {
    LOG(ERROR) << "Could not create temporary options file.";
    return false;
  }
  FILE* options_file = base::OpenFile(options_filepath, "wt");
  if (!options_file) {
    LOG(ERROR) << "Could not create temporary options file.";
    return false;
  }
  fprintf(options_file,
          "--generate-compdb --tool=traffic_annotation_extractor -p=%s ",
          build_path_.MaybeAsASCII().c_str());

  // |ignore_list_[ALL]| is not passed when |full_run| is happening as there is
  // no way to pass it to run_tools.py except enumerating all alternatives.
  // The paths in |ignore_list_[ALL]| are removed later from the results.
  if (full_run) {
    for (const std::string& file_path : path_filters)
      fprintf(options_file, "%s ", file_path.c_str());
  } else {
    TrafficAnnotationFileFilter filter;
    std::vector<std::string> file_paths;

    if (path_filters.size()) {
      for (const auto& path_filter : path_filters) {
        filter.GetRelevantFiles(source_path_,
                                ignore_list_[static_cast<int>(
                                    AuditorException::ExceptionType::ALL)],
                                path_filter, &file_paths);
      }
    } else {
      filter.GetRelevantFiles(
          source_path_,
          ignore_list_[static_cast<int>(AuditorException::ExceptionType::ALL)],
          "", &file_paths);
    }

    if (!file_paths.size()) {
      base::CloseFile(options_file);
      base::DeleteFile(options_filepath, false);
      return false;
    }
    for (const auto& file_path : file_paths)
      fprintf(options_file, "%s ", file_path.c_str());
  }
  base::CloseFile(options_file);

  base::CommandLine cmdline(source_path_.Append(FILE_PATH_LITERAL("tools"))
                                .Append(FILE_PATH_LITERAL("clang"))
                                .Append(FILE_PATH_LITERAL("scripts"))
                                .Append(FILE_PATH_LITERAL("run_tool.py")));

#if defined(OS_WIN)
  cmdline.PrependWrapper(L"python");
#endif

  cmdline.AppendArg(base::StringPrintf(
      "--options-file=%s", options_filepath.MaybeAsASCII().c_str()));

  bool result = base::GetAppOutput(cmdline, &clang_tool_raw_output_);

  base::DeleteFile(options_filepath, false);

  return result;
}

bool TrafficAnnotationAuditor::IsWhitelisted(
    const std::string& file_path,
    AuditorException::ExceptionType whitelist_type) {
  const std::vector<std::string>& whitelist =
      ignore_list_[static_cast<int>(whitelist_type)];

  for (const std::string& ignore_path : whitelist) {
    if (!strncmp(file_path.c_str(), ignore_path.c_str(), ignore_path.length()))
      return true;
  }

  // If the given filepath did not match the rules with the specified type,
  // check it with rules of type 'ALL' as well.
  if (whitelist_type != AuditorException::ExceptionType::ALL)
    return IsWhitelisted(file_path, AuditorException::ExceptionType::ALL);
  return false;
}

bool TrafficAnnotationAuditor::ParseClangToolRawOutput() {
  // Remove possible carriage return characters before splitting lines.
  base::RemoveChars(clang_tool_raw_output_, "\r", &clang_tool_raw_output_);
  std::vector<std::string> lines =
      base::SplitString(clang_tool_raw_output_, "\n", base::KEEP_WHITESPACE,
                        base::SPLIT_WANT_ALL);

  for (unsigned int current = 0; current < lines.size(); current++) {
    bool annotation_block;
    if (lines[current] == "==== NEW ANNOTATION ====")
      annotation_block = true;
    else if (lines[current] == "==== NEW CALL ====") {
      annotation_block = false;
    } else if (lines[current].empty()) {
      continue;
    } else {
      LOG(ERROR) << "Unexpected token at line: " << current;
      return false;
    }

    // Get the block.
    current++;
    unsigned int end_line = current;
    std::string end_marker =
        annotation_block ? "==== ANNOTATION ENDS ====" : "==== CALL ENDS ====";
    while (end_line < lines.size() && lines[end_line] != end_marker)
      end_line++;
    if (end_line == lines.size()) {
      LOG(ERROR) << "Block starting at line " << current
                 << " is not ended by the appropriate tag.";
      return false;
    }

    // Deserialize and handle errors.
    AnnotationInstance new_annotation;
    CallInstance new_call;
    AuditorResult result(AuditorResult::Type::RESULT_OK);

    result = annotation_block
                 ? new_annotation.Deserialize(lines, current, end_line)
                 : new_call.Deserialize(lines, current, end_line);

    if (!IsWhitelisted(result.file_path(),
                       AuditorException::ExceptionType::ALL) &&
        (result.type() != AuditorResult::Type::ERROR_MISSING ||
         !IsWhitelisted(result.file_path(),
                        AuditorException::ExceptionType::MISSING))) {
      switch (result.type()) {
        case AuditorResult::Type::RESULT_OK: {
          if (annotation_block)
            extracted_annotations_.push_back(new_annotation);
          else
            extracted_calls_.push_back(new_call);
          break;
        }
        case AuditorResult::Type::RESULT_IGNORE:
          break;
        case AuditorResult::Type::ERROR_FATAL: {
          LOG(ERROR) << "Aborting after line " << current
                     << " because: " << result.ToText().c_str();
          return false;
        }
        default:
          errors_.push_back(result);
      }
    }

    current = end_line;
  }  // for

  return true;
}

bool TrafficAnnotationAuditor::LoadWhiteList() {
  base::FilePath white_list_file = base::MakeAbsoluteFilePath(
      source_path_.Append(FILE_PATH_LITERAL("tools"))
          .Append(FILE_PATH_LITERAL("traffic_annotation"))
          .Append(FILE_PATH_LITERAL("auditor"))
          .Append(FILE_PATH_LITERAL("white_list.txt")));
  std::string file_content;
  if (base::ReadFileToString(white_list_file, &file_content)) {
    base::RemoveChars(file_content, "\r", &file_content);
    std::vector<std::string> lines = base::SplitString(
        file_content, "\n", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);
    for (const std::string& line : lines) {
      // Ignore comments.
      if (line.length() && line[0] == '#')
        continue;
      size_t comma = line.find(',');
      if (comma == std::string::npos) {
        LOG(ERROR) << "Unexpected syntax in white_list.txt, line: " << line;
        return false;
      }

      AuditorException::ExceptionType exception_type;
      if (AuditorException::TypeFromString(line.substr(0, comma),
                                           &exception_type)) {
        ignore_list_[static_cast<int>(exception_type)].push_back(
            line.substr(comma + 1, line.length() - comma - 1));
      } else {
        LOG(ERROR) << "Unexpected type in white_list.txt line: " << line;
        return false;
      }
    }
    return true;
  }

  LOG(ERROR)
      << "Could not read tools/traffic_annotation/auditor/white_list.txt";
  return false;
}

// static
const std::map<int, std::string>&
TrafficAnnotationAuditor::GetReservedUniqueIDs() {
  return kReservedAnnotations;
}

void TrafficAnnotationAuditor::PurgeAnnotations(
    const std::set<int>& hash_codes) {
  for (unsigned int i = 0; i < extracted_annotations_.size(); i++) {
    if (base::ContainsKey(hash_codes,
                          extracted_annotations_[i].unique_id_hash_code)) {
      extracted_annotations_.erase(extracted_annotations_.begin() + i);
      i--;
    }
  }
}

void TrafficAnnotationAuditor::CheckDuplicateHashes() {
  const std::map<int, std::string> reserved_ids = GetReservedUniqueIDs();

  struct AnnotationID {
    // Two ids can be the same in the following cases:
    // 1- One is extra id of a partial annotation, and the other is either the
    //    unique id of a completing annotation, or extra id of a partial or
    //    branched completing annotation
    // 2- Both are extra ids of branched completing annotations.
    // The following Type value facilitate these checks.
    enum class Type { kPatrialExtra, kCompletingMain, kBranchedExtra, kOther };
    Type type;
    std::string text;
    int hash;
    AnnotationInstance* instance;
  };
  std::map<int, std::vector<AnnotationID>> collisions;
  std::set<int> to_be_purged;

  for (AnnotationInstance& instance : extracted_annotations_) {
    // Check if partial and branched completing annotation have an extra id
    // which is different from their unique id.
    if ((instance.type == AnnotationInstance::Type::ANNOTATION_PARTIAL ||
         instance.type ==
             AnnotationInstance::Type::ANNOTATION_BRANCHED_COMPLETING) &&
        (instance.unique_id_hash_code == instance.extra_id_hash_code)) {
      errors_.push_back(AuditorResult(
          AuditorResult::Type::ERROR_MISSING_EXTRA_ID, std::string(),
          instance.proto.source().file(), instance.proto.source().line()));
      continue;
    }

    AnnotationID current;
    current.instance = &instance;
    // Iterate over unique id and extra id.
    for (int id = 0; id < 2; id++) {
      if (id) {
        // If it's an empty extra id, no further check is required.
        if (instance.extra_id.empty()) {
          continue;
        } else {
          current.text = instance.extra_id;
          current.hash = instance.extra_id_hash_code;
          if (instance.type == AnnotationInstance::Type::ANNOTATION_PARTIAL) {
            current.type = AnnotationID::Type::kPatrialExtra;
          } else if (instance.type ==
                     AnnotationInstance::Type::ANNOTATION_BRANCHED_COMPLETING) {
            current.type = AnnotationID::Type::kBranchedExtra;
          } else {
            current.type = AnnotationID::Type::kOther;
          }
        }
      } else {
        current.text = instance.proto.unique_id();
        current.hash = instance.unique_id_hash_code;
        current.type =
            instance.type == AnnotationInstance::Type::ANNOTATION_COMPLETING
                ? AnnotationID::Type::kCompletingMain
                : AnnotationID::Type::kOther;
      }

      // If the id's hash code is the same as a reserved id, add an error.
      if (base::ContainsKey(reserved_ids, current.hash)) {
        errors_.push_back(AuditorResult(
            AuditorResult::Type::ERROR_RESERVED_UNIQUE_ID_HASH_CODE,
            current.text, instance.proto.source().file(),
            instance.proto.source().line()));
        continue;
      }

      // Check for collisions.
      if (!base::ContainsKey(collisions, current.hash)) {
        collisions[current.hash] = std::vector<AnnotationID>();
      } else {
        // Add error for ids with the same hash codes. If the texts are really
        // different, there is a hash collision and should be corrected in any
        // case. Otherwise, it's an error if it doesn't match the criteria that
        // are previously spcified in definition of AnnotationID struct.
        for (const auto& other : collisions[current.hash]) {
          if (current.text == other.text &&
              ((current.type == AnnotationID::Type::kPatrialExtra &&
                (other.type == AnnotationID::Type::kPatrialExtra ||
                 other.type == AnnotationID::Type::kCompletingMain ||
                 other.type == AnnotationID::Type::kBranchedExtra)) ||
               (other.type == AnnotationID::Type::kPatrialExtra &&
                (current.type == AnnotationID::Type::kCompletingMain ||
                 current.type == AnnotationID::Type::kBranchedExtra)) ||
               (current.type == AnnotationID::Type::kBranchedExtra &&
                other.type == AnnotationID::Type::kBranchedExtra))) {
            continue;
          }

          AuditorResult error(
              AuditorResult::Type::ERROR_DUPLICATE_UNIQUE_ID_HASH_CODE,
              base::StringPrintf(
                  "%s in '%s:%i'", current.text.c_str(),
                  current.instance->proto.source().file().c_str(),
                  current.instance->proto.source().line()));
          error.AddDetail(
              base::StringPrintf("%s in '%s:%i'", other.text.c_str(),
                                 other.instance->proto.source().file().c_str(),
                                 other.instance->proto.source().line()));

          errors_.push_back(error);
          to_be_purged.insert(current.hash);
          to_be_purged.insert(other.hash);
        }
      }
      collisions[current.hash].push_back(current);
    }
  }

  PurgeAnnotations(to_be_purged);
}

void TrafficAnnotationAuditor::CheckUniqueIDsFormat() {
  std::set<int> to_be_purged;
  for (const AnnotationInstance& instance : extracted_annotations_) {
    if (!base::ContainsOnlyChars(base::ToLowerASCII(instance.proto.unique_id()),
                                 "0123456789_abcdefghijklmnopqrstuvwxyz")) {
      errors_.push_back(AuditorResult(
          AuditorResult::Type::ERROR_UNIQUE_ID_INVALID_CHARACTER,
          instance.proto.unique_id(), instance.proto.source().file(),
          instance.proto.source().line()));
      to_be_purged.insert(instance.unique_id_hash_code);
    }
    if (!instance.extra_id.empty() &&
        !base::ContainsOnlyChars(base::ToLowerASCII(instance.extra_id),
                                 "0123456789_abcdefghijklmnopqrstuvwxyz")) {
      errors_.push_back(
          AuditorResult(AuditorResult::Type::ERROR_UNIQUE_ID_INVALID_CHARACTER,
                        instance.extra_id, instance.proto.source().file(),
                        instance.proto.source().line()));
      to_be_purged.insert(instance.unique_id_hash_code);
    }
  }
  PurgeAnnotations(to_be_purged);
}

void TrafficAnnotationAuditor::CheckAllRequiredFunctionsAreAnnotated() {
  for (const CallInstance& call : extracted_calls_) {
    if (!call.is_annotated && !CheckIfCallCanBeUnannotated(call)) {
      errors_.push_back(
          AuditorResult(AuditorResult::Type::ERROR_MISSING_ANNOTATION,
                        call.function_name, call.file_path, call.line_number));
    }
  }
}

bool TrafficAnnotationAuditor::CheckIfCallCanBeUnannotated(
    const CallInstance& call) {
  // At this stage we do not enforce annotation on native network requests,
  // hence all calls except those to 'net::URLRequestContext::CreateRequest' and
  // 'net::URLFetcher::Create' are ignored.
  if (call.function_name != "net::URLFetcher::Create" &&
      call.function_name != "net::URLRequestContext::CreateRequest") {
    return true;
  }

  // Is in whitelist?
  if (IsWhitelisted(call.file_path, AuditorException::ExceptionType::MISSING))
    return true;

  // Unittests should be all annotated. Although this can be detected using gn,
  // doing that would be very slow. The alternative solution would be to bypass
  // every file including test or unittest, but in this case there might be some
  // ambiguety in what should be annotated and what not.
  if (call.file_path.find("unittest") != std::string::npos)
    return false;

  // Already checked?
  if (base::ContainsKey(checked_dependencies_, call.file_path))
    return checked_dependencies_[call.file_path];

  std::string gn_output;
  if (gn_file_for_test_.empty()) {
    // Check if the file including this function is part of Chrome build.
    const base::CommandLine::CharType* args[] = {FILE_PATH_LITERAL("gn"),
                                                 FILE_PATH_LITERAL("refs"),
                                                 FILE_PATH_LITERAL("--all")};

    base::CommandLine cmdline(3, args);
    cmdline.AppendArgPath(build_path_);
    cmdline.AppendArg(call.file_path);

    base::FilePath original_path;
    base::GetCurrentDirectory(&original_path);
    base::SetCurrentDirectory(source_path_);

    if (!base::GetAppOutput(cmdline, &gn_output)) {
      LOG(ERROR) << "Could not run gn to get dependencies.";
      gn_output.clear();
    }

    base::SetCurrentDirectory(original_path);
  } else {
    if (!base::ReadFileToString(gn_file_for_test_, &gn_output)) {
      LOG(ERROR) << "Could not load mock gn output file from "
                 << gn_file_for_test_.MaybeAsASCII().c_str();
      gn_output.clear();
    }
  }

  checked_dependencies_[call.file_path] =
      gn_output.length() &&
      gn_output.find("//chrome:chrome") == std::string::npos;

  return checked_dependencies_[call.file_path];
}

void TrafficAnnotationAuditor::CheckAnnotationsContents() {
  std::vector<AnnotationInstance*> partial_annotations;
  std::vector<AnnotationInstance*> completing_annotations;
  std::vector<AnnotationInstance> new_annotations;
  std::set<int> to_be_purged;

  // Process complete annotations and separate the others.
  for (AnnotationInstance& instance : extracted_annotations_) {
    bool keep_annotation = false;
    switch (instance.type) {
      case AnnotationInstance::Type::ANNOTATION_COMPLETE: {
        AuditorResult result = instance.IsComplete();
        if (result.OK())
          result = instance.IsConsistent();
        if (result.OK())
          keep_annotation = true;
        else
          errors_.push_back(result);
        break;
      }
      case AnnotationInstance::Type::ANNOTATION_PARTIAL:
        partial_annotations.push_back(&instance);
        break;
      default:
        completing_annotations.push_back(&instance);
    }
    if (!keep_annotation)
      to_be_purged.insert(instance.unique_id_hash_code);
  }

  std::set<AnnotationInstance*> used_completing_annotations;

  for (AnnotationInstance* partial : partial_annotations) {
    bool found_a_pair = false;
    for (AnnotationInstance* completing : completing_annotations) {
      if (partial->IsCompletableWith(*completing)) {
        found_a_pair = true;
        used_completing_annotations.insert(completing);

        AnnotationInstance completed;
        AuditorResult result =
            partial->CreateCompleteAnnotation(*completing, &completed);
        if (result.OK()) {
          result = completed.IsComplete();
          if (result.OK())
            result = completed.IsConsistent();

          if (!result.OK()) {
            result = AuditorResult(AuditorResult::Type::ERROR_MERGE_FAILED,
                                   result.ToShortText());
            result.AddDetail(partial->proto.unique_id());
            result.AddDetail(completing->proto.unique_id());
          }
        }
        if (result.OK())
          new_annotations.push_back(completed);
        else
          errors_.push_back(result);
      }
    }

    if (!found_a_pair) {
      errors_.push_back(AuditorResult(
          AuditorResult::Type::ERROR_INCOMPLETED_ANNOTATION, std::string(),
          partial->proto.source().file(), partial->proto.source().line()));
    }
  }

  for (AnnotationInstance* instance : completing_annotations) {
    if (!base::ContainsKey(used_completing_annotations, instance)) {
      errors_.push_back(AuditorResult(
          AuditorResult::Type::ERROR_INCOMPLETED_ANNOTATION, std::string(),
          instance->proto.source().file(), instance->proto.source().line()));
    }
  }

  PurgeAnnotations(to_be_purged);
  if (new_annotations.size())
    extracted_annotations_.insert(extracted_annotations_.end(),
                                  new_annotations.begin(),
                                  new_annotations.end());
}

void TrafficAnnotationAuditor::RunAllChecks() {
  CheckDuplicateHashes();
  CheckUniqueIDsFormat();
  CheckAnnotationsContents();

  CheckAllRequiredFunctionsAreAnnotated();
}