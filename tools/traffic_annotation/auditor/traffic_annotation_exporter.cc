// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/traffic_annotation/auditor/traffic_annotation_exporter.h"

#include "base/files/file_util.h"
#include "base/stl_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "third_party/libxml/chromium/libxml_utils.h"
#include "third_party/protobuf/src/google/protobuf/text_format.h"
#include "tools/traffic_annotation/auditor/traffic_annotation_auditor.h"

namespace {

const std::string kXmlComment =
    "<!--\n"
    "Copyright 2017 The Chromium Authors. All rights reserved.\n"
    "Use of this source code is governed by a BSD-style license that can be\n"
    "found in the LICENSE file.\n"
    "\nRefer to README.md for content description and update process.\n"
    "-->\n\n";
}

TrafficAnnotationExporter::TrafficAnnotationExporter(){};

TrafficAnnotationExporter::~TrafficAnnotationExporter(){};

// Loads annotations from the given XML file.
bool TrafficAnnotationExporter::LoadAnnotationFromXML(
    const base::FilePath& filepath,
    std::vector<ReportItem>* items) {
  XmlReader reader;
  if (!reader.LoadFile(filepath.MaybeAsASCII()))
    return false;

  while (reader.Read()) {
    if (reader.NodeName() != "item")
      continue;

    ReportItem item;
    std::string temp;

    if (!reader.NodeAttribute("id", &item.unique_id))
      return false;

    if (!reader.NodeAttribute("hash_code", &temp) ||
        !base::StringToInt(temp, &item.unique_id_hash_code)) {
      return false;
    }

    if (reader.NodeAttribute("content_hash_code", &temp)) {
      if (!base::StringToInt(temp, &item.content_hash_code))
        return false;
    } else {
      item.content_hash_code = -1;
    }

    item.deprecated = reader.NodeAttribute("deprecated", &temp);

    items->push_back(item);
  }

  return true;
}

bool TrafficAnnotationExporter::UpdateAnnotationsXML(
    const base::FilePath& filepath,
    const std::vector<AnnotationInstance>& annotations,
    const std::map<int, std::string>& reserved_ids) {
  std::vector<ReportItem> items;
  std::set<int> used_hash_codes;

  // Add annotations.
  for (const AnnotationInstance& item : annotations) {
    std::string content;
    google::protobuf::TextFormat::PrintToString(item.proto, &content);

    items.push_back(
        ReportItem({item.proto.unique_id(), item.unique_id_hash_code, false,
                    TrafficAnnotationAuditor::ComputeHashValue(content)}));
    used_hash_codes.insert(item.unique_id_hash_code);
  }

  // Add reserved ids.
  for (const auto& item : reserved_ids) {
    items.push_back(ReportItem({item.second, item.first, false, -1}));
    used_hash_codes.insert(item.first);
  }

  // Add depricated items
  std::vector<ReportItem> former_items;
  if (!LoadAnnotationFromXML(filepath, &former_items))
    LOG(WARNING) << "Could not load former annotations list.";

  for (ReportItem& item : former_items) {
    if (!base::ContainsKey(used_hash_codes, item.unique_id_hash_code)) {
      item.deprecated = true;
      items.push_back(item);
    }
  }

  // Sort and write.
  std::sort(items.begin(), items.end(), ReportItem());

  XmlWriter writer;
  writer.StartWriting();
  writer.StartElement("annotations");

  for (const ReportItem& item : items) {
    writer.StartElement("item");
    writer.AddAttribute("id", item.unique_id);
    writer.AddAttribute("hash_code",
                        base::StringPrintf("%i", item.unique_id_hash_code));
    if (item.deprecated)
      writer.AddAttribute("deprecated", "1");
    if (item.content_hash_code == -1)
      writer.AddAttribute("reserved", "1");
    else
      writer.AddAttribute("content_hash_code",
                          base::StringPrintf("%i", item.content_hash_code));
    writer.EndElement();
  }
  writer.EndElement();

  writer.StopWriting();
  std::string xml_content = writer.GetWrittenString();
  // Add comment before annotation tag (and after xml version).
  xml_content.insert(xml_content.find("<annotations>"), kXmlComment);

  return base::WriteFile(filepath, xml_content.c_str(), xml_content.length()) !=
         -1;
}