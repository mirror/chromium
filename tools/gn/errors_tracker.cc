// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/gn/errors_tracker.h"

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/json/json_writer.h"
#include "base/memory/singleton.h"
#include "base/values.h"
#include "tools/gn/input_file.h"
#include "tools/gn/location.h"
#include "tools/gn/source_file.h"
#include "tools/gn/switches.h"

namespace {

base::Value LocationToValue(const Location& location) {
  if (location.is_null())
    return base::Value();

  base::Value location_value(base::Value::Type::DICTIONARY);
  location_value.SetKey("file", base::Value(location.file()->name().value()));
  location_value.SetKey("line", base::Value(location.line_number()));
  location_value.SetKey("column", base::Value(location.column_number()));

  return location_value;
}

base::Value LocationRangeToValue(const LocationRange& range) {
  base::Value range_value(base::Value::Type::DICTIONARY);
  range_value.SetKey("begin", LocationToValue(range.begin()));
  range_value.SetKey("end", LocationToValue(range.end()));
  return range_value;
}

base::Value ErrorToValue(const Err& err) {
  base::Value error_value(base::Value::Type::DICTIONARY);

  error_value.SetKey("location", LocationToValue(err.location()));
  error_value.SetKey("message", base::Value(err.message()));
  error_value.SetKey("help_text", base::Value(err.help_text()));

  base::Value sub_errors_value(base::Value::Type::LIST);
  for (const Err& err : err.sub_errs())
    sub_errors_value.GetList().push_back(ErrorToValue(err));

  error_value.SetKey("sub_errors", std::move(sub_errors_value));

  base::Value ranges_value(base::Value::Type::LIST);
  for (const LocationRange& range : err.ranges())
    ranges_value.GetList().push_back(LocationRangeToValue(range));

  error_value.SetKey("ranges", std::move(ranges_value));

  return error_value;
}

}  // namespace

ErrorsTracker::ErrorsTracker() = default;
ErrorsTracker::~ErrorsTracker() = default;

void ErrorsTracker::AddError(const Err& err) {
  errors_.push_back(err);
}

bool ErrorsTracker::SerializeDiagnostics() const {
  const auto command_line = *base::CommandLine::ForCurrentProcess();

  if (!command_line.HasSwitch(switches::kSerializeDiagnostics))
    return true;

  const auto output_path =
      command_line.GetSwitchValuePath(switches::kSerializeDiagnostics);
  if (output_path.empty()) {
    Err(Location(), "--serialize-diagnostics requires path argument.").Report();
    return false;
  }

  base::Value errors(base::Value::Type::LIST);
  for (const Err& err : errors_)
    errors.GetList().push_back(ErrorToValue(err));

  std::string json;
  if (!base::JSONWriter::WriteWithOptions(
          errors, base::JSONWriter::OPTIONS_PRETTY_PRINT, &json)) {
    Err(Location(), "Failed to serialize diagnostics.").Report();
    return false;
  }

  if (base::WriteFile(output_path, json.data(), json.size()) < 0) {
    Err(Location(), "Failed to write diagnostics.").Report();
    return false;
  }

  return true;
}

// static
ErrorsTracker* ErrorsTracker::GetInstance() {
  return base::Singleton<ErrorsTracker>::get();
}
