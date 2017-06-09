#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import sys
import argparse
from importlib import import_module


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--module', help='The module path to the script to import and run')

    args, remaining_args = parser.parse_known_args()

    # Import and run the specified module, forwarding any command line arguments that remain.
    module = import_module(args.module)
    sys.argv = sys.argv[:1] + remaining_args
    module.run()

if __name__ == "__main__":
    main()
