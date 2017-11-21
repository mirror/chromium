# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import argparse
import json
import yaml


def main():
  parser = argparse.ArgumentParser()
  parser.add_argument("--manifest_in")
  parser.add_argument("--output_dir")
  args = parser.parse_args()

  with open(args.output_dir + "/manifest.json", "w") as file:
    json.dump(yaml.load(open(args.manifest_in)), file)


main()
