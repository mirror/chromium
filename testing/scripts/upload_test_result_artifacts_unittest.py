# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Tests for upload_test_result_artifacts."""

import json
import mock
import os
import random
import string
import tempfile
import unittest
import upload_test_result_artifacts as UTRA


class UploadTestResultArtifactsTest(unittest.TestCase):
  def setUp(self):
    # Used for load tests
    self._temp_files = []

  def tearDown(self):
    # Used for load tests
    for fname in self._temp_files:
      os.unlink(fname)

  ### These are load tests useful for seeing how long it takes to upload
  ### different kinds of test results files. They won't be run as part of
  ### presubmit testing, since they take a while and talk to the network,
  ### but the code will stay here in case anyone wants to edit the code
  ### and wants to check performance. Change the test names from "loadTestBlah"
  ### to "testBlah" to get them to run.

  def makeTemp(self, size):
    _, fname = tempfile.mkstemp()
    with open(fname, 'w') as f:
      f.write(random.choice(string.ascii_letters) * size)
    self._temp_files.append(fname)

    return fname

  def makeTestJson(self, num_tests, artifact_size):
    return {
      "tests": {
        "suite": {
          "test%d" % i: {
            "artifacts": [
              [
                {
                  "name": "artifact",
                  "location": self.makeTemp(artifact_size),
                }
              ]
            ],
            "expected": "PASS",
            "actual": "PASS",
          } for i in range(num_tests)
        }
      },
      "artifact_type_info": {
        "artifact": "text/plain"
      }
    }

  def _test(self, json_data, upload):
    _, fname = tempfile.mkstemp()
    with open(fname, 'w') as f:
      json.dump(json_data, f)

    return UTRA.process_file(
        fname, '/tmp', False, upload)


  def loadTestSimple(self):
    test_data = self.makeTestJson(1, 10)
    self._test(test_data, False)

  def loadTestManySmall(self):
    test_data = self.makeTestJson(1000, 10)
    self._test(test_data, False)

  def loadTestSomeBig(self):
    test_data = self.makeTestJson(100, 10000000)
    self._test(test_data, False)

  def loadTestVeryBig(self):
    test_data = self.makeTestJson(2, 1000000000)
    self._test(test_data, False)

  ### End load test section.

  def testGetTestsSimple(self):
    self.assertEqual(UTRA.get_tests({
      "foo": {
        "expected": "PASS",
        "actual": "PASS",
      },
    }), {
      ('foo',): {
          "actual": "PASS",
          "expected": "PASS",
      }
    })

  def testGetTestsNested(self):
    self.assertEqual(UTRA.get_tests({
      "foo": {
        "bar": {
          "baz": {
            "actual": "PASS",
            "expected": "PASS",
          },
          "bam": {
            "actual": "PASS",
            "expected": "PASS",
          },
        },
      },
    }), {
      ('foo', 'bar', 'baz'): {
          "actual": "PASS",
          "expected": "PASS",
      },
      ('foo', 'bar', 'bam'): {
          "actual": "PASS",
          "expected": "PASS",
      }
    })

  def testGetTestsError(self):
    with self.assertRaises(ValueError):
      UTRA.get_tests([])

  @mock.patch('upload_test_result_artifacts.get_file_digest')
  @mock.patch('upload_test_result_artifacts.tempfile.mkdtemp')
  @mock.patch('upload_test_result_artifacts.shutil.rmtree')
  @mock.patch('upload_test_result_artifacts.shutil.copyfile')
  def testUploadArtifactsSimple(
      self, copy_patch, rmtree_patch, mkd_patch, digest_patch):
    """Simple test; no artifacts, so data shouldn't change."""
    mkd_patch.return_value = 'foo_dir'
    data = {
        'artifact_type_info': {
            'log': 'text/plain'
        },
        'tests': {
          'foo': {
            "actual": "PASS",
            "expected": "PASS",
          }
        }
    }
    self.assertEqual(UTRA.upload_artifacts(
        data, '/tmp', True), data)
    mkd_patch.assert_called_once_with(prefix='upload_test_artifacts')
    digest_patch.assert_not_called()
    copy_patch.assert_not_called()
    rmtree_patch.assert_called_once_with('foo_dir')

  def testFileDigest(self):
    _, path = tempfile.mkstemp(prefix='file_digest_test')
    with open(path, 'w') as f:
      f.write('a')

    self.assertEqual(
        UTRA.get_file_digest(path),
        'ca978112ca1bbdcafac231b39a23dc4da786eff8147c4e72b9807785afee48bb')

if __name__ == '__main__':
  unittest.main()
