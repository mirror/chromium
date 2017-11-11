"""Tests for upload_test_result_artifacts."""

import json
import os
import random
import string
import tempfile
import unittest
import upload_test_result_artifacts


class UploadTestResultArtifactsTest(unittest.TestCase):
  def setUp(self):
    self._temp_files = []

  def tearDown(self):
    for fname in self._temp_files:
      os.unlink(fname)

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

    return upload_test_result_artifacts.process_file(
        fname, '/tmp', False, upload)


  def testSimple(self):
    test_data = self.makeTestJson(1, 10)
    self._test(test_data, False)

  def testManySmall(self):
    test_data = self.makeTestJson(1000, 10)
    self._test(test_data, False)

  def testSomeBig(self):
    test_data = self.makeTestJson(100, 10000000)
    self._test(test_data, False)

  def testVeryBig(self):
    test_data = self.makeTestJson(2, 1000000000)
    self._test(test_data, False)


if __name__ == '__main__':
  unittest.main()
