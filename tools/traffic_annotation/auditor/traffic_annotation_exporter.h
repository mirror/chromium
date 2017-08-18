// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_TRAFFIC_ANNOTATION_AUDITOR_TRAFFIC_ANNOTATION_EXPORTER_H_
#define TOOLS_TRAFFIC_ANNOTATION_AUDITOR_TRAFFIC_ANNOTATION_EXPORTER_H_

#include <map>
#include <vector>

#include "base/files/file_path.h"
#include "base/strings/string_util.h"
#include "tools/traffic_annotation/auditor/instance.h"

class TrafficAnnotationExporter {
 public:
  TrafficAnnotationExporter();
  ~TrafficAnnotationExporter();

  struct ReportItem {
    std::string unique_id;
    int unique_id_hash_code = -1;
    std::string deprecation_date;
    int content_hash_code = -1;

    bool operator()(ReportItem a, ReportItem b) const {
      return base::CompareCaseInsensitiveASCII(a.unique_id, b.unique_id) < 0;
    }
  };

  // Updates the xml file including annotations unique id, hash code, content
  // hash code, and a flag specifying that annotation is depricated.
  bool UpdateAnnotationsXML(const base::FilePath& filepath,
                            const std::vector<AnnotationInstance>& annotations,
                            const std::map<int, std::string>& reserved_ids);

  // Loads annotations from the given XML file.
  bool LoadAnnotationFromXML(const base::FilePath& filepath,
                             std::vector<ReportItem>* items);
};

#endif  // TOOLS_TRAFFIC_ANNOTATION_AUDITOR_TRAFFIC_ANNOTATION_EXPORTER_H_