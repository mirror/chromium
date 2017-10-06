#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Sort a JSON list of IPv4 and IPv6 addresses.

This script acts as a unix filter, reading a JSON object containing a
list of IP addresses as strings, and sorts them in numerical order,
with IPv4 addresses showing up before IPv6 addresses, and then writes
the list to stdout.

The sorting algorithm is to split into numeric and non-numeric
fields, and then sort each "natural", i.e., this is called a "natural sort".
Since these are just '.' or ':'-separated numeric fields, this is actually
even easier than a normal natural sort.
"""

from __future__ import print_function

import argparse
import json
import sys


def compare_ip_addresses_naturally(a, b):
    a_is_ipv6 = ':' in a
    b_is_ipv6 = ':' in b

    if not a_is_ipv6 and not b_is_ipv6:
      base=10
      sep='.'
    elif not a_is_ipv6 and b_is_ipv6:
      return -1
    elif a_is_ipv6 and not b_is_ipv6:
      return 1
    else:
      base=16
      sep=':'

    a_flds = [int(fld, base=base) for fld in a.split(sep)]
    b_flds = [int(fld, base=base) for fld in b.split(sep)]
    if a_flds < b_flds:
      return -1
    elif a_flds == b_flds:
      return 0
    else:
      return 1


def main():
    parser = argparse.ArgumentParser(usage=__doc__)
    parser.add_argument('file', nargs='?', default='-')
    args = parser.parse_args()

    if args.file == '-':
      l = json.load(sys.stdin)
    else:
      with open(args.file) as fp:
        l = json.load(fp)

    l.sort(cmp=compare_ip_addresses_naturally)
    print(json.dumps(l, indent=2))


if __name__ == '__main__':
    main()
