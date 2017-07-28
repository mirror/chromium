// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/tools/cert_mapper/write_results.h"

#include <cstdio>
#include <iostream>
#include <map>
#include <memory>
#include <string>

#include "base/base64.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/format_macros.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "net/der/input.h"
#include "net/tools/cert_mapper/entry.h"
#include "net/tools/cert_mapper/metrics.h"

namespace net {
namespace {

// Wraps the lines of text to fit in |width| characters (not including the
// newline)
std::string FormatFixedColumn(const std::string& text, size_t width) {
  std::string result;

  size_t pos = 0;
  while (pos < text.size()) {
    size_t remaining = text.size() - pos;
    size_t chunk_len = remaining < width ? remaining : width;
    result += text.substr(pos, chunk_len) + "\n";
    pos += chunk_len;
  }

  return result;
}

void WriteStringToFile(const std::string& data, const base::FilePath& path) {
  CHECK_EQ(static_cast<int>(data.size()),
           base::WriteFile(path, data.data(), data.size()));
}

std::string ExecCommand(const std::string& cmd) {
  FILE* pipe = popen(cmd.c_str(), "r");

  if (!pipe)
    return "ERROR";

  std::string result = "";
  char buffer[256];
  while (!feof(pipe)) {
    if (fgets(buffer, sizeof(buffer), pipe) != NULL)
      result += buffer;
  }

  pclose(pipe);
  return result;
}

std::string ExecOpenSslCmd(const std::string& cmd, const scoped_refptr<CertBytes>& input) {
  base::FilePath tmp_input_path;
  CHECK(base::CreateTemporaryFile(&tmp_input_path));
  WriteStringToFile(input->AsString(), tmp_input_path);

  std::string result = ExecCommand(base::StringPrintf(
      "%s -in \"%s\"", cmd.c_str(), tmp_input_path.value().c_str()));

  CHECK(base::DeleteFile(tmp_input_path, false));

  return result;
}

// Writes |cert| in PEM format. Which is to say something like:
//
//     -----BEGIN CERTIFICATE-----
//     gobbeldy gook in here here...
//     -----END CERTIFICATE-----
std::string FormatCertificateAsPem(const scoped_refptr<CertBytes>& cert) {
  std::string result;
  result += "-----BEGIN CERTIFICATE-----\n";
  std::string base64_encoded;
  base::Base64Encode(cert->AsStringPiece(), &base64_encoded);
  result += FormatFixedColumn(base64_encoded, 75);
  result += "-----END CERTIFICATE-----\n";
  return result;
}

std::string FormatSampleAsPem(const Sample& sample) {
  // Make a single vector with all the certificates, for simplicity.
  const CertBytesVector& chain = sample.certs;

  std::string data;

  for (size_t i = 0; i < chain.size(); ++i) {
    const auto& cert = chain[i];
    data += "\n\n";
    data +=
        base::StringPrintf("========= Cert%d =========\n", static_cast<int>(i));
    data += ExecOpenSslCmd("openssl x509 -noout -text -inform DER", cert);
  }

  data += "\n";

  data += "=====================================\n";
  data += "Parsed data\n";
  data += "=====================================\n";

  for (size_t i = 0; i < chain.size(); ++i) {
    const auto& cert = chain[i];
    data += "\n\n";
    data +=
        base::StringPrintf("========= Cert%d =========\n", static_cast<int>(i));
    data += ExecOpenSslCmd("openssl asn1parse -i -inform DER", cert);
  }

  data += "\n";
  data += "=====================================\n";
  data += "PEM data\n";
  data += "=====================================\n";

  for (size_t i = 0; i < chain.size(); ++i) {
    const auto& cert = chain[i];
    data += "\n\n";
    data +=
        base::StringPrintf("========= Cert%d =========\n", static_cast<int>(i));
    data += FormatCertificateAsPem(cert);
  }

  return data;
}

base::FilePath GetSrcDir() {
  // Assume that the current program is running in a subdirectory of "src". Keep
  // descending until we find the src directory...

  auto src_path = base::MakeAbsoluteFilePath(
      base::CommandLine::ForCurrentProcess()->GetProgram());

  while (src_path != src_path.DirName()) {
    if (src_path.BaseName().value() == "src") {
      return src_path;
    }
    src_path = src_path.DirName();
  }

  return base::FilePath();
}

void CopyStaticFile(const std::string& src_relative_path,
                    const base::FilePath& dest_dir) {
  auto path = GetSrcDir().AppendASCII(src_relative_path);
  CHECK(base::CopyFile(path, dest_dir.Append(path.BaseName())));
}

std::unique_ptr<base::ListValue> CreateListForSamples(
    const std::vector<Sample>& samples,
    size_t metrics_i,
    size_t bucket_i,
    const base::FilePath& dir_path) {
  std::unique_ptr<base::ListValue> samples_list =
      base::MakeUnique<base::ListValue>();

  if (!samples.empty()) {
    std::string samples_dir_name =
        base::SizeTToString(metrics_i) + "-" + base::SizeTToString(bucket_i);

    // Create a directory to hold all the samples for this bucket
    base::FilePath samples_dir = dir_path.AppendASCII(samples_dir_name);
    CHECK(base::CreateDirectory(samples_dir));

    // Write all the samples into the bucket's directory.
    for (size_t samples_i = 0; samples_i < samples.size(); ++samples_i) {
      const auto& sample = samples[samples_i];
      std::string data = FormatSampleAsPem(sample) + "\n\n";
      std::string samples_relpath =
          samples_dir_name + "/" + base::SizeTToString(samples_i);

      if (sample.certs.size() == 1)
        samples_relpath += ".cert";
      else
        samples_relpath += ".chain";
     
      samples_relpath += ".txt";

      WriteStringToFile(data, dir_path.AppendASCII(samples_relpath));
      samples_list->AppendString(samples_relpath);
    }
  }

  return samples_list;
}

std::unique_ptr<base::DictionaryValue> CreateDictForBucket(
    const BucketValue& bucket,
    const std::string& bucket_name,
    size_t metrics_i,
    size_t bucket_i,
    const base::FilePath& dir_path) {
  std::unique_ptr<base::DictionaryValue> bucket_dict =
      base::MakeUnique<base::DictionaryValue>();

  bucket_dict->SetString("name", bucket_name);
  bucket_dict->SetDouble("total", bucket.total);

  bucket_dict->Set("samples", CreateListForSamples(bucket.samples, metrics_i,
                                                   bucket_i, dir_path));

  return bucket_dict;
}

std::unique_ptr<base::ListValue> CreateListForBuckets(
    const MetricsItem::BucketMap& bucket_map,
    size_t metrics_i,
    const base::FilePath& dir_path) {
  std::unique_ptr<base::ListValue> buckets_value =
      base::MakeUnique<base::ListValue>();

  size_t bucket_i = 0;
  for (const auto& bucket_it : bucket_map) {
    const auto& bucket = bucket_it.second;
    buckets_value->Append(CreateDictForBucket(bucket, bucket_it.first,
                                              metrics_i, bucket_i, dir_path));
    bucket_i++;
  }

  return buckets_value;
}

std::unique_ptr<base::DictionaryValue> CreateDictForMetricsItem(
    const std::string& name,
    const MetricsItem& metrics_item,
    size_t metrics_i,
    const base::FilePath& dir_path) {
  std::unique_ptr<base::DictionaryValue> metrics_dict =
      base::MakeUnique<base::DictionaryValue>();

  metrics_dict->SetString("name", name);
  metrics_dict->SetDouble("total", metrics_item.total());
  metrics_dict->Set("buckets", CreateListForBuckets(metrics_item.buckets(),
                                                    metrics_i, dir_path));
  return metrics_dict;
}

std::unique_ptr<base::ListValue> CreateListForMetrics(
    const Metrics& metrics,
    const base::FilePath& dir_path) {
  std::unique_ptr<base::ListValue> metrics_array_value =
      base::MakeUnique<base::ListValue>();

  size_t metrics_i = 0;
  for (const auto& metrics_item_it : metrics.items()) {
    const std::string& metrics_item_name = metrics_item_it.first;
    const MetricsItem& metrics_item = metrics_item_it.second;

    metrics_array_value->Append(CreateDictForMetricsItem(
        metrics_item_name, metrics_item, metrics_i, dir_path));

    metrics_i++;
  }

  return metrics_array_value;
}

}  // namespace

void WriteResultsToDirectory(const Metrics& metrics,
                             const base::FilePath& dir_path) {
  CopyStaticFile("net/tools/cert_mapper/index.html", dir_path);
  CopyStaticFile("net/tools/cert_mapper/main.js", dir_path);

  base::DictionaryValue results_dict;

  results_dict.Set("metrics", CreateListForMetrics(metrics, dir_path));

  // JSON-ify the overview information.
  std::string json;
  CHECK(base::JSONWriter::WriteWithOptions(
      results_dict,
      base::JSONWriter::OPTIONS_PRETTY_PRINT |
          base::JSONWriter::OPTIONS_OMIT_DOUBLE_TYPE_PRESERVATION,
      &json));

  // Use a .js file rather than a pure data file (since for data file would need
  // to XHR it in... and Chrome doesn't let you XHR on file://. LAME.
  WriteStringToFile("var g_data = " + json + ";",
                    dir_path.AppendASCII("data.js"));
}

}  // namespace net
