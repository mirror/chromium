#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A script to merge multiple source xml files into a single histograms.xml."""

import argparse
import xml.dom.minidom

def GetElementsByTagName(trees, tag):
  return [e for t in trees for e in t.getElementsByTagName(tag)]

def MakeNodeWithChildren(doc, tag, children):
  node = doc.createElement(tag)
  for child in children:
    node.appendChild(child)
  return node

def MergeTrees(trees):
  doc = xml.dom.minidom.Document()
  doc.appendChild(MakeNodeWithChildren(doc, 'histogram-configuration',
    # This can result in the merged document having multiple <enums> and
    # similar sections, but scripts ignore these anyway.
    GetElementsByTagName(trees, 'enums') +
    GetElementsByTagName(trees, 'histograms') +
    GetElementsByTagName(trees, 'histogram_suffixes_list')))
  return doc

def MergeFiles(filenames):
  trees = [xml.dom.minidom.parse(open(f)) for f in filenames]
  return MergeTrees(trees)

def main():
  parser = argparse.ArgumentParser()
  parser.add_argument('inputs', nargs="+")
  parser.add_argument('--output', required=True)
  args = parser.parse_args()
  MergeFiles(args.inputs).writexml(open(args.output, 'w'))

if __name__ == '__main__':
  main()

