# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
try:
  import chrome_device_policy_pb2 as dp
except ImportError:
  dp = None
# env PYTHONPATH=/usr/local/buildtools/current/sitecustomize:out_veyron_minnie/Release/pyproto:third_party/protobuf/python:out_veyron_minnie/Release/pyproto/chrome/browser/chromeos/policy/proto python components/policy/tools/off_hours_generator.py
# Example of generated code
# #include "chrome/browser/chromeos/policy/device_off_hours_controller.h"
# namespace em = enterprise_management;
#
# em::ChromeDeviceSettingsProto clear_policies(
#     em::ChromeDeviceSettingsProto* input_policies,
#         std::set<std::string> off_policies) {
#   em::ChromeDeviceSettingsProto policies;
#   policies.CopyFrom(*input_policies);
#   if (off_policies.find("DeviceAllowNewUsers")) {
#   policies.clear_allow_new_users();
#   }
#   ...
#   return policices;
# }
if __name__ == '__main__':
  file = open("chrome/browser/chromeos/policy/device_off_hours_cleaner.cc",
              'w')
  settings = dp.ChromeDeviceSettingsProto()
  file.write('// Copyright (c) 2012 The Chromium Authors. '
             'All rights reserved.\n')
  file.write('// Use of this source code is governed by a BSD-style '
             'license that can be\n')
  file.write('// found in the LICENSE file.\n\n')
  file.write('#include "chrome/browser/chromeos/settings/' \
             'device_off_hours_controller.h"\n\n')
  file.write('namespace policy {\n\n')
  file.write('namespace em = enterprise_management;\n\n')
  file.write('em::ChromeDeviceSettingsProto DeviceOffHoursController::' \
             'clear_policies(\n')
  file.write('     em::ChromeDeviceSettingsProto* input_policies,\n')
  file.write('         std::set<std::string> off_policies) {\n')
  file.write('  em::ChromeDeviceSettingsProto policies;\n')
  file.write('  policies.CopyFrom(*input_policies);\n')
  for group in settings.DESCRIPTOR.fields:
    # group_message = eval('dp.' + group.message_type.name + '()')
    file.write('  if (off_policies.find("' + group.name + '") != ' \
                                                         'off_policies.end()) {\n')
    file.write('    policies.clear_' + group.name + '();\n')
    file.write('  }\n')
  file.write('  return policies;\n')
  file.write('}\n')
  file.write('}\n')
  file.close()
