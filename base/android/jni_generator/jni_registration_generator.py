#!/usr/bin/env python
# Copyright (c) 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Get Java files from sources files and extract native methods,Generate
registration functions."""

import os
import sys
import optparse
from string import Template

import jni_generator
from jni_generator import InlHeaderFileGenerator, JniParams, ParseError
from util import build_utils


def GenerateJNIHeader(java_file_paths, output_file, options):
  dict = {'FORWARD_DECLARATIONS': '', 'REGISTER_MAINDEX_NATIVES': '',
          'REGISTER_NONMAINDEX_NATIVES': ''}
  try:
    for path in java_file_paths:
      if path in options.no_register_java:
        continue
      contents = file(path).read()

      # Get the package name and class name.
      fully_qualified_class = jni_generator.ExtractFullyQualifiedJavaClassName(
        path, contents)
      JniParams.ResetStates()
      JniParams.SetFullyQualifiedClass(fully_qualified_class)
      JniParams.ExtractImportsAndInnerClasses(contents)
      jni_namespace = jni_generator.ExtractJNINamespace(contents)
      natives = jni_generator.ExtractNatives(contents, options.ptr_type)
      if len(natives) == 0:
        continue
      maindex = jni_generator.IsMainDexJavaClass(contents)
      header_generator = HeaderGenerator(jni_namespace, fully_qualified_class,
                                         natives, options, dict, maindex)
      dict = header_generator.GetContent()
    include_file = ''
    if options.includes:
      include_file = '#include "' + options.includes + '"'
    dict['INCLUDES'] = include_file
    header_content = CreateFromDict(dict)
  except ParseError, e:
    print e
    sys.exit(1)
  if output_file:
    if not os.path.exists(os.path.dirname(os.path.abspath(output_file))):
      os.makedirs(os.path.dirname(os.path.abspath(output_file)))
    if os.path.exists(output_file):
      with file(output_file, 'r') as f:
        existing_content = f.read()
        if existing_content == header_content:
          return
    with file(output_file, 'w') as f:
      f.write(header_content)
  else:
    print header_content

def CreateFromDict(dict):
  """Returns the content of the header file."""
  template = Template("""\
// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef HEADER_GUARD
#define HEADER_GUARD

#include <jni.h>

${INCLUDES}

#include "base/android/jni_int_wrapper.h"

// Step 1: Forward declaration.
${FORWARD_DECLARATIONS}

// Step 2: Maindex and nonmaindex registration functions.

bool RegisterMaindexNatives(JNIEnv* env) {
${REGISTER_MAINDEX_NATIVES}

  return true;
}

bool RegisterNonmaindexNatives(JNIEnv* env) {
${REGISTER_NONMAINDEX_NATIVES}

  return true;
}

#endif  // HEADER_GUARD
""")
  return '' if len(dict['FORWARD_DECLARATIONS']) == 0 \
    else jni_generator.WrapOutput(template.substitute(dict))


class HeaderGenerator(object):
  """Generates an inline header file for JNI registration."""

  def __init__(self, namespace, fully_qualified_class, natives,
               options, dict, maindex):
    self.namespace = namespace
    self.fully_qualified_class = fully_qualified_class
    self.class_name = self.fully_qualified_class.split('/')[-1]
    self.natives = natives
    self.options = options
    self.dict = dict
    self.maindex = maindex
    self.inl_header_file_generator = InlHeaderFileGenerator(self.namespace,
                self.fully_qualified_class, self.natives, [], [], self.options)

  def AddForwardDeclaration(self):
    """Add the content of the forward declaration to the dictionary."""
    template = Template("""\
$OPEN_NAMESPACE
$METHOD_SIGNATURE
$CLOSE_NAMESPACE
""")

    values = {
      'OPEN_NAMESPACE': self.inl_header_file_generator.GetOpenNamespaceString(),
      'METHOD_SIGNATURE': 'JNI_REGISTRATION_EXPORT bool ' + self.GetMethodName()
                          + '(JNIEnv* env);',
      'CLOSE_NAMESPACE':
    self.inl_header_file_generator.GetCloseNamespaceString(),
    }
    self.dict['FORWARD_DECLARATIONS'] += template.substitute(values) + '\n\n'

  def AddRegisterNatives(self):
    """Add the body of the RegisterNativesImpl method to the dictionary."""
    register_function = self.GetMethodName() + '(env)'
    if self.namespace:
      register_function = self.namespace + '::' + register_function
    register_function_block = """\
  if (!%s)
    return false;

""" % register_function
    if self.maindex == 'true':
      self.dict['REGISTER_MAINDEX_NATIVES'] += register_function_block
    else:
      self.dict['REGISTER_NONMAINDEX_NATIVES'] += register_function_block

  def GetMethodName(self):
    return 'RegisterNative' + self.fully_qualified_class.replace('_',
                                                '_1').title().replace('/', '')

  def GetContent(self):
    self.AddForwardDeclaration()
    self.AddRegisterNatives()
    return self.dict


def main(argv):
  usage = """usage: %prog [OPTIONS]
This script will get java files from the given sources files, extract native
method declarations, create registration functions and print the header file
to stdout (or a file).
  """
  option_parser = optparse.OptionParser(usage=usage)
  build_utils.AddDepfileOption(option_parser)

  option_parser.add_option('--sources_files',
                           help='A list of .sources files which contain Java'
                                ' file paths. Must be used with --output.')
  option_parser.add_option('--output',
                           help='The output file path.')
  option_parser.add_option('--includes',
                           help='Helper file included in the generated header '
                                'file')
  option_parser.add_option('--ptr_type', default='int',
                           type='choice', choices=['int', 'long'],
                           help='The type used to represent native pointers in '
                                'Java code. For 32-bit, use int; '
                                'for 64-bit, use long.')
  option_parser.add_option('--no_register_java',
                           help='A list of Java files which should be ignored '
                                'by the parser.')
  options, args = option_parser.parse_args(build_utils.ExpandFileArgs(argv))

  options.sources_files = build_utils.ParseGnList(options.sources_files)
  if options.sources_files:
    java_file_paths = []
    for f in options.sources_files:
      # java_file_paths stores each Java file path as a string.
      java_file_paths += build_utils.ReadSourcesList(f)
  else:
    option_parser.print_help()
    print '\nError: Must specify --sources_files.'
    return 1
  output_file = options.output
  GenerateJNIHeader(java_file_paths, output_file, options)

if __name__ == '__main__':
  sys.exit(main(sys.argv))
