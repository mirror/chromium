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

  A sample logcat that needs to be symbolized would be:
  Build fingerprint: 'google/shamu/shamu:7.1.1/NMF20B/3370:userdebug/dev-keys'
  Revision: '0'
  ABI: 'arm'
  pid: 28936, tid: 28936, name: chromium.chrome  >>> org.chromium.chrome <<<
  signal 6 (SIGABRT), code -6 (SI_TKILL), fault addr --------
  Abort message: '[FATAL:debug_urls.cc(151)] Check failed: false.
  #00 0x63e16c41 /data/app/org.chromium.chrome-1/lib/arm/libchrome.so+0x0006cc4
  #01 0x63f19be3 /data/app/org.chromium.chrome-1/lib/arm/libchrome.so+0x0016fbe
  #02 0x63f19737 /data/app/org.chromium.chrome-1/lib/arm/libchrome.so+0x0016f73
  #03 0x63f18ddf /data/app/org.chromium.chrome-1/lib/arm/libchrome.so+0x0016edd
  #04 0x63f18b79 /data/app/org.chromium.chrome-1/lib/arm/libchrome.so+0x0016eb7
  #05 0xab53f319 /system/lib/libart.so+0x000a3319
  #06
     r0 00000000  r1 00007108  r2 00000006  r3 00000008
     r4 ae60258c  r5 00000006  r6 ae602534  r7 0000010c
     r8 bede5cd0  r9 00000030  sl 00000000  fp 9265a800
     ip 0000000b  sp bede5c38  lr ac8e5537  pc ac8e7da0  cpsr 600f0010

  backtrace:
     #00 pc 00049da0  /system/lib/libc.so (tgkill+12)
     #01 pc 00047533  /system/lib/libc.so (pthread_kill+34)
     #02 pc 0001d635  /system/lib/libc.so (raise+10)
     #03 pc 00019181  /system/lib/libc.so (__libc_android_abort+34)
     #04 pc 00017048  /system/lib/libc.so (abort+4)
     #05 pc 00948605  /data/app/org.chromium.chrome-1/lib/arm/libchrome.so
     #06 pc 002c9f73  /data/app/org.chromium.chrome-1/lib/arm/libchrome.so
     #07 pc 003ccbe1  /data/app/org.chromium.chrome-1/lib/arm/libchrome.so
     #08 pc 003cc735  /data/app/org.chromium.chrome-1/lib/arm/libchrome.so
     #09 pc 003cbddf  /data/app/org.chromium.chrome-1/lib/arm/libchrome.so
     #10 pc 003cbb77  /data/app/org.chromium.chrome-1/lib/arm/libchrome.so
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
            continue
        else:
          # Check whether it is the end of stack trace.
          if have_seen_back_trace and not '     #' in line:
            look_for_start_of_crash_log = True
            have_seen_back_trace = False
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
