# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


def SymbolizeLogcat(logcat, dest, symbolizer, abi):
  """Symbolize stack trace in the logcat.

  Symbolize the logcat and write the symbolized logcat to a new file.

  Args:
    logcat: Path to logcat file.
    dest: Path to where to write the symbolized logcat.
    symbolizer: The stack symbolizer to symbolize stack trace in logcat.
    abi: The device's product_cpu_abi. Symbolizer needs it to symbolize.
  """

  with open(logcat, 'r') as logcat_file:
    with open(dest, 'w') as dest_file:
      look_for_start_of_crash_log = True
      have_seen_back_trace = False
      data_to_symbolize = []
      for line in logcat_file:
        if look_for_start_of_crash_log:
          # Check whether it is the start of crash log.
          if 'Build fingerprint: ' in line:
            look_for_start_of_crash_log = False
            # Only include necessary information for symbolization.
            data_to_symbolize.append(line[line.find(' :')+2:])
            continue
          else:
            dest_file.write(line)
        else:
          # Check whether it is the end of stack trace.
          if have_seen_back_trace and not '     #' in line:
            look_for_start_of_crash_log = True
            dest_file.write(
                '\n'.join(
                    symbolizer.ExtractAndResolveNativeStackTraces(
                         data_to_symbolize, abi)) +
                '\n' + line)
            data_to_symbolize = []
          else:
            if "backtrace:" in line:
              have_seen_back_trace = True
            data_to_symbolize.append(line[line.find(' :')+2:])
