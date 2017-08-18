#!/usr/bin/env python
# Copyright (c) 2011 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Makes sure that all EXE and DLL files in the provided directory were built
correctly.

In essense it runs a subset of BinScope tests ensuring that binaries have
/NXCOMPAT, /DYNAMICBASE and /SAFESEH.
"""

import json
import os
import optparse
import sys

# Find /third_party/pefile based on current directory and script path.
sys.path.append(os.path.join(os.path.dirname(__file__), '..', '..',
                             'third_party', 'pefile'))
import pefile

PE_FILE_EXTENSIONS = ['.exe', '.dll']

# Any flags not currently defined in pefile.py:
IMAGE_GUARD_CF_INSTRUMENTED = 0x0100

# Please do not add your file here without confirming that it indeed doesn't
# require /NXCOMPAT and /DYNAMICBASE.  Contact cpu@chromium.org or your local
# Windows guru for advice.
EXCLUDED_FILES = ['mini_installer.exe',
                  'next_version_mini_installer.exe'
                  ]
# Executables that we do not build, or that do not go on client, may need to be
# excluded from CFG checks.
EXCLUDED_CFG_EXES = ['pgosweep.exe']

def IsPEFile(path):
  return (os.path.isfile(path) and
          os.path.splitext(path)[1].lower() in PE_FILE_EXTENSIONS and
          os.path.basename(path) not in EXCLUDED_FILES)

def main(options, args):
  directory = args[0]
  build = args[1]
  pe_total = 0
  pe_passed = 0

  failures = []

  for file in os.listdir(directory):
    path = os.path.abspath(os.path.join(directory, file))
    if not IsPEFile(path):
      continue
    pe = pefile.PE(path, fast_load=True)
    pe.parse_data_directories(directories=[
        pefile.DIRECTORY_ENTRY['IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG']])
    pe_total = pe_total + 1
    success = True

    # Check for /DYNAMICBASE.
    if (pe.OPTIONAL_HEADER.DllCharacteristics &
        pefile.DLL_CHARACTERISTICS['IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE']):
      if options.verbose:
        print "Checking %s for /DYNAMICBASE... PASS" % path
    else:
      success = False
      print "Checking %s for /DYNAMICBASE... FAIL" % path

    # Check for /NXCOMPAT.
    if (pe.OPTIONAL_HEADER.DllCharacteristics &
        pefile.DLL_CHARACTERISTICS['IMAGE_DLLCHARACTERISTICS_NX_COMPAT']):
      if options.verbose:
        print "Checking %s for /NXCOMPAT... PASS" % path
    else:
      success = False
      print "Checking %s for /NXCOMPAT... FAIL" % path

    # Check for /SAFESEH. Binaries should meet one of the following
    # criteria:
    #   1) Have no SEH table as indicated by the DLL characteristics
    #   2) Have a LOAD_CONFIG section containing a valid SEH table
    #   3) Be a 64-bit binary, in which case /SAFESEH isn't required
    #
    # Refer to the following MSDN article for more information:
    # http://msdn.microsoft.com/en-us/library/9a89h429.aspx
    if ((pe.OPTIONAL_HEADER.DllCharacteristics &
         pefile.DLL_CHARACTERISTICS['IMAGE_DLLCHARACTERISTICS_NO_SEH']) or
        (pe.FILE_HEADER.Machine ==
         pefile.MACHINE_TYPE['IMAGE_FILE_MACHINE_AMD64']) or
        (hasattr(pe, "DIRECTORY_ENTRY_LOAD_CONFIG") and
         pe.DIRECTORY_ENTRY_LOAD_CONFIG.struct.SEHandlerCount > 0 and
         pe.DIRECTORY_ENTRY_LOAD_CONFIG.struct.SEHandlerTable != 0)):
      if options.verbose:
        print "Checking %s for /SAFESEH... PASS" % path
    else:
      success = False
      print "Checking %s for /SAFESEH... FAIL" % path

    # ASLR is weakened on Windows 64-bit when the ImageBase is below 4GB
    # (because the loader will never be rebase the image above 4GB).
    if (pe.FILE_HEADER.Machine ==
        pefile.MACHINE_TYPE['IMAGE_FILE_MACHINE_AMD64']):
      if pe.OPTIONAL_HEADER.ImageBase <= 0xFFFFFFFF:
        print("Checking %s ImageBase (0x%X < 4GB)... FAIL" %
              (path, pe.OPTIONAL_HEADER.ImageBase))
        success = False
      elif options.verbose:
        print("Checking %s ImageBase (0x%X > 4GB)... PASS" %
              (path, pe.OPTIONAL_HEADER.ImageBase))

    # Check that CFG is enabled in release executables.
    # Note: This ensures support for any CFG checks compiled into any DLLs
    #       mapped into the process (e.g. Microsoft system DLLs).  The EXE
    #       itself may not have CFG instrumentation compiled in.
    if (build == 'Release' and
        path.lower().endswith('.exe') and
        os.path.basename(path) not in EXCLUDED_CFG_EXES and
        hasattr(pe, "DIRECTORY_ENTRY_LOAD_CONFIG")):
      if not(pe.OPTIONAL_HEADER.DllCharacteristics &
             pefile.DLL_CHARACTERISTICS['IMAGE_DLLCHARACTERISTICS_GUARD_CF']):
        success = False
        print("Checking %s DllCharacteristics for CFG (/GUARD) support... " \
              "FAIL" % path)
      elif not(pe.DIRECTORY_ENTRY_LOAD_CONFIG.struct.GuardFlags &
               IMAGE_GUARD_CF_INSTRUMENTED):
        success = False
        print("Checking %s GuardFlags for CFG (/GUARD) support... FAIL" % path)
      elif pe.DIRECTORY_ENTRY_LOAD_CONFIG.struct.GuardCFFunctionCount == 0:
        success = False
        print("Checking %s GuardCFFunctionCount for CFG (/GUARD) support... " \
              "FAIL" % path)
      else:
        if options.verbose:
          print "Checking %s for CFG (/GUARD) support... PASS" % path

    # Update tally.
    if success:
      pe_passed = pe_passed + 1
    else:
      failures.append(path)

  print "Result: %d files found, %d files passed" % (pe_total, pe_passed)

  if options.json:
    with open(options.json, 'w') as f:
      json.dump(failures, f)

  if pe_passed != pe_total:
    sys.exit(1)

if __name__ == '__main__':
  usage = "Usage: %prog [options] DIRECTORY"
  option_parser = optparse.OptionParser(usage=usage)
  option_parser.add_option("-v", "--verbose", action="store_true",
                           default=False, help="Print debug logging")
  option_parser.add_option("--json", help="Path to JSON output file")
  options, args = option_parser.parse_args()
  if not args:
    option_parser.print_help()
    sys.exit(0)
  main(options, args)
