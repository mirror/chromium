// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/traffic_annotation/auditor/traffic_annotation_id_checker.h"

#include "base/stl_util.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"

TrafficAnnotationIDChecker::TrafficAnnotationIDChecker(
    const std::set<int>& reserved_ids,
    const std::set<int>& deprecated_ids)
    : deprecated_ids_(deprecated_ids), reserved_ids_(reserved_ids) {}

TrafficAnnotationIDChecker::~TrafficAnnotationIDChecker() = default;

void TrafficAnnotationIDChecker::Load(
    const std::vector<AnnotationInstance>& extracted_annotations) {
  annotations_.clear();
  for (const AnnotationInstance& instance : extracted_annotations) {
    AnnotationItem item;
    item.type = instance.type;
    item.ids[0].hash_code = instance.unique_id_hash_code;
    item.ids[0].text = instance.proto.unique_id();
    if (instance.NeedsExtraID()) {
      item.ids_count = 2;
      item.ids[1].hash_code = instance.extra_id_hash_code;
      item.ids[1].text = instance.extra_id;
    } else {
      item.ids_count = 1;
    }
    item.file_path = instance.proto.source().file();
    item.line_number = instance.proto.source().line();
    annotations_.push_back(item);
  }
}

void TrafficAnnotationIDChecker::CheckIDs(std::vector<AuditorResult>* errors) {
  CheckForInvalidValues(reserved_ids_,
                        AuditorResult::Type::ERROR_RESERVED_UNIQUE_ID_HASH_CODE,
                        errors);
  CheckForInvalidValues(
      deprecated_ids_,
      AuditorResult::Type::ERROR_DEPRECATED_UNIQUE_ID_HASH_CODE, errors);
  CheckForHashCollisions(errors);
  CheckIDsFormat(errors);
  CheckForInvalidRepeatedIDs(errors);
}

void TrafficAnnotationIDChecker::CheckForInvalidValues(
    const std::set<int>& invalid_set,
    AuditorResult::Type error_type,
    std::vector<AuditorResult>* errors) {
  for (AnnotationItem& item : annotations_) {
    for (int i = 0; i < item.ids_count; i++) {
      if (base::ContainsKey(invalid_set, item.ids[i].hash_code)) {
        errors->push_back(AuditorResult(error_type, item.ids[i].text,
                                        item.file_path, item.line_number));
      }
    }
  }
}

void TrafficAnnotationIDChecker::CheckForExtraIDs(
    std::vector<AuditorResult>* errors) {
  for (AnnotationItem& item : annotations_) {
    if (item.ids_count == 2 &&
        (item.ids[1].text.empty() ||
         item.ids[0].hash_code == item.ids[1].hash_code)) {
      errors->push_back(
          AuditorResult(AuditorResult::Type::ERROR_MISSING_EXTRA_ID,
                        std::string(), item.file_path, item.line_number));
    }
  }
}

void TrafficAnnotationIDChecker::CheckForHashCollisions(
    std::vector<AuditorResult>* errors) {
  std::map<int, std::string> collisions;
  for (AnnotationItem& item : annotations_) {
    for (int i = 0; i < item.ids_count; i++) {
      if (!base::ContainsKey(collisions, item.ids[i].hash_code)) {
        collisions.insert(
            std::make_pair(item.ids[i].hash_code, item.ids[i].text));
      } else {
        if (item.ids[i].text != collisions[item.ids[i].hash_code]) {
          AuditorResult error(AuditorResult::Type::ERROR_HASH_CODE_COLLISION,
                              item.ids[i].text);
          error.AddDetail(collisions[item.ids[i].hash_code]);
          errors->push_back(error);
        }
      }
    }
  }
}

void TrafficAnnotationIDChecker::CheckForInvalidRepeatedIDs(
    std::vector<AuditorResult>* errors) {
  std::map<int, AnnotationItem*> first_ids;
  std::map<int, AnnotationItem*> second_ids;

  // Check if first ids are unique.
  for (AnnotationItem& item : annotations_) {
    if (!base::ContainsKey(first_ids, item.ids[0].hash_code)) {
      first_ids[item.ids[0].hash_code] = &item;
    } else {
      errors->push_back(CreateRepeatedIDError(
          item.ids[0].text, item, *first_ids[item.ids[0].hash_code]));
    }
  }

  // Check if a second id is equal to a first id, owner of the second id is of
  // type PARTIAL and owner of the first id is of type COMPLETING.
  for (AnnotationItem& item : annotations_) {
    if (item.ids_count == 2 &&
        base::ContainsKey(first_ids, item.ids[1].hash_code)) {
      if (item.type != AnnotationInstance::Type::ANNOTATION_PARTIAL ||
          first_ids[item.ids[1].hash_code]->type !=
              AnnotationInstance::Type::ANNOTATION_COMPLETING) {
        errors->push_back(CreateRepeatedIDError(
            item.ids[1].text, item, *first_ids[item.ids[1].hash_code]));
      }
    }
  }

  // Check if two second ids are equal, they are either PARTIAL or
  // BRANCHED_COMPLETING.
  for (AnnotationItem& item : annotations_) {
    if (item.ids_count != 2)
      continue;
    if (!base::ContainsKey(second_ids, item.ids[1].hash_code)) {
      second_ids[item.ids[1].hash_code] = &item;
    } else {
      AnnotationInstance::Type other_type =
          second_ids[item.ids[1].hash_code]->type;
      if ((item.type != AnnotationInstance::Type::ANNOTATION_PARTIAL &&
           item.type !=
               AnnotationInstance::Type::ANNOTATION_BRANCHED_COMPLETING) ||
          (other_type != AnnotationInstance::Type::ANNOTATION_PARTIAL &&
           other_type !=
               AnnotationInstance::Type::ANNOTATION_BRANCHED_COMPLETING)) {
        errors->push_back(CreateRepeatedIDError(
            item.ids[1].text, item, *second_ids[item.ids[1].hash_code]));
      }
    }
  }
}

void TrafficAnnotationIDChecker::CheckIDsFormat(
    std::vector<AuditorResult>* errors) {
  for (AnnotationItem& item : annotations_) {
    for (int i = 0; i < item.ids_count; i++) {
      if (!base::ContainsOnlyChars(base::ToLowerASCII(item.ids[i].text),
                                   "0123456789_abcdefghijklmnopqrstuvwxyz")) {
        errors->push_back(AuditorResult(
            AuditorResult::Type::ERROR_UNIQUE_ID_INVALID_CHARACTER,
            item.ids[i].text, item.file_path, item.line_number));
      }
    }
  }
}

AuditorResult TrafficAnnotationIDChecker::CreateRepeatedIDError(
    const std::string& common_id,
    const AnnotationItem& item1,
    const AnnotationItem& item2) {
  AuditorResult error(
      AuditorResult::Type::ERROR_REPEATED_ID,
      base::StringPrintf("%s in '%s:%i'", common_id.c_str(),
                         item1.file_path.c_str(), item1.line_number));
  error.AddDetail(base::StringPrintf("'%s:%i'", item2.file_path.c_str(),
                                     item2.line_number));
  return error;
}
