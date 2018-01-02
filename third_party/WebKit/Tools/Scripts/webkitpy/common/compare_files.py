# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import filecmp
import os
from webkitpy.common.system.executive import Executive


def compare_files(
        reference_directory, output_directory, verbose, suppress_diff):
    executive = Executive()

    def list_files(directory):
        files = []
        for filename in os.listdir(directory):
            files.append(os.path.join(directory, filename))
        return files

    def diff(filename1, filename2):
        # Python's difflib module is too slow, especially on long output, so
        # run external diff(1) command
        cmd = ['diff',
               '-u',  # unified format
               '-N',  # treat absent files as empty
               filename1,
               filename2]
        # Return output and don't raise exception, even though diff(1) has
        # non-zero exit if files differ.
        return executive.run_command(cmd, error_handler=lambda x: None)

    def identical_file(reference_filename, output_filename):
        reference_basename = os.path.basename(reference_filename)

        if not os.path.isfile(reference_filename):
            print 'Missing reference file!'
            print '(if adding new test, update reference files)'
            print reference_basename
            print
            return False

        if not filecmp.cmp(reference_filename, output_filename):
            # cmp is much faster than diff, and usual case is "no difference",
            # so only run diff if cmp detects a difference
            print 'FAIL: %s' % reference_basename
            if not suppress_diff:
                print diff(reference_filename, output_filename)
            return False

        if verbose:
            print 'PASS: %s' % reference_basename
        return True

    def identical_output_files(output_files):
        reference_files = [os.path.join(reference_directory,
                                        os.path.relpath(path, output_directory))
                           for path in output_files]
        return all(
            [identical_file(reference_filename, output_filename)
             for (reference_filename, output_filename) in zip(
                    reference_files, output_files)])

    def no_excess_files(output_files):
        generated_files = set([os.path.relpath(path, output_directory)
                               for path in output_files])
        excess_files = []
        for path in list_files(reference_directory):
            relpath = os.path.relpath(path, reference_directory)
            if relpath not in generated_files:
                excess_files.append(relpath)
        if excess_files:
            print ('Excess reference files! '
                   '(probably cruft from renaming or deleting):\n' +
                   '\n'.join(excess_files))
            return False
        return True

    # Detect all changes
    output_files = list_files(output_directory)
    passed = identical_output_files(output_files)
    passed &= no_excess_files(output_files)
    return passed

