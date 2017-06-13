#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Generate JNI registration entry points

Creates a header file with two static functions: RegisterMainDexNatives() and
RegisterNonMainDexNatives(). Together, these will use manual JNI registration
to register all native methods that exist within an application."""

import os
import sys
import argparse
import string

import jni_generator
from util import build_utils

def GenerateJNIHeader(java_file_paths, output_file, args):
  """Generate a header file including two registration functions.

  Forward declares all JNI registration functions created by jni_generator.py.
  Calls the functions in RegisterMainDexNatives() if they are main dex. And
  calls them in RegisterNonMainDexNatives() if they are non-main dex.

  Args:
      java_file_paths: A list of java file paths.
      output_file: A relative path to output file.
      args: All input arguments.
  """
  dict = {
      'FORWARD_DECLARATIONS': '',
      'REGISTER_MAIN_DEX_NATIVES': '',
      'REGISTER_NON_MAIN_DEX_NATIVES': ''
  }
  for path in java_file_paths:
    if path in args.no_register_java:
      continue
    with open(path) as f:
      contents = f.read()

    # Get the package name and class name.
    fully_qualified_class = jni_generator.ExtractFullyQualifiedJavaClassName(
        path, contents)
    jni_generator.JniParams.ResetStates()
    jni_generator.JniParams.SetFullyQualifiedClass(fully_qualified_class)
    jni_generator.JniParams.ExtractImportsAndInnerClasses(contents)
    jni_namespace = jni_generator.ExtractJNINamespace(contents)
    natives = jni_generator.ExtractNatives(contents, 'long')
    if len(natives) == 0:
      continue
    main_dex = jni_generator.IsMainDexJavaClass(contents)
    header_generator = HeaderGenerator(jni_namespace, fully_qualified_class,
                                       natives, args, dict, main_dex)
    dict = header_generator.GetContent()

  include_file = ''
  if args.includes:
    include_file = '#include "{}"'.format(args.includes)
  dict['INCLUDES'] = include_file
  header_content = CreateFromDict(dict)

  if output_file:
    output_file_path = os.path.dirname(os.path.abspath(output_file))
    if not os.path.exists(output_file_path):
      os.makedirs(output_file_path)
    with open(output_file, 'w') as f:
      f.write(header_content)
  else:
    print header_content

def CreateFromDict(dict):
  """Returns the content of the header file."""

  template = string.Template("""\
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

// Step 2: Main dex and non-main dex registration functions.

bool RegisterMainDexNatives(JNIEnv* env) {
${REGISTER_MAIN_DEX_NATIVES}

  return true;
}

bool RegisterNonMainDexNatives(JNIEnv* env) {
${REGISTER_NON_MAIN_DEX_NATIVES}

  return true;
}

#endif  // HEADER_GUARD
""")
  if len(dict['FORWARD_DECLARATIONS']) == 0:
    return ''
  return jni_generator.WrapOutput(template.substitute(dict))


class HeaderGenerator(object):
  """Generates an inline header file for JNI registration."""

  def __init__(self, namespace, fully_qualified_class, natives,
               args, dict, main_dex):
    self.namespace = namespace
    self.fully_qualified_class = fully_qualified_class
    self.class_name = self.fully_qualified_class.split('/')[-1]
    self.natives = natives
    self.args = args
    self.dict = dict
    self.main_dex = main_dex
    self.inl_header_file_generator = jni_generator.InlHeaderFileGenerator(
        self.namespace, self.fully_qualified_class, self.natives, [], [],
        self.args)

  def AddForwardDeclaration(self):
    """Add the content of the forward declaration to the dictionary."""
    template = string.Template("""\
$OPEN_NAMESPACE
$METHOD_SIGNATURE
$CLOSE_NAMESPACE
""")
    signature = 'JNI_REGISTRATION_EXPORT bool {}(JNIEnv* env);'.format(
        self.inl_header_file_generator.GetRegisterName())
    values = {
        'OPEN_NAMESPACE':
            self.inl_header_file_generator.GetOpenNamespaceString(),
        'METHOD_SIGNATURE': signature,
        'CLOSE_NAMESPACE':
            self.inl_header_file_generator.GetCloseNamespaceString(),
    }
    self.dict['FORWARD_DECLARATIONS'] += template.substitute(values) + '\n\n'

  def AddRegisterNatives(self):
    """Add the body of the RegisterNativesImpl method to the dictionary."""
    register_function = '{}(env)'.format(
        self.inl_header_file_generator.GetRegisterName())
    if self.namespace:
      register_function = self.namespace + '::' + register_function
    register_function_block = """\
  if (!%s)
    return false;

""" % register_function
    if self.main_dex:
      self.dict['REGISTER_MAIN_DEX_NATIVES'] += register_function_block
    else:
      self.dict['REGISTER_NON_MAIN_DEX_NATIVES'] += register_function_block

  def GetContent(self):
    self.AddForwardDeclaration()
    self.AddRegisterNatives()
    return self.dict


def main(argv):
  arg_parser = argparse.ArgumentParser()
  build_utils.AddDepfileOption(arg_parser)

  arg_parser.add_argument('--sources_files',
                           help='A list of .sources files which contain Java'
                                ' file paths. Must be used with --output.')
  arg_parser.add_argument('--output',
                           help='The output file path.')
  arg_parser.add_argument('--includes',
                           help='Helper file for the generated header file')
  arg_parser.add_argument('--no_register_java',
                           help='A list of Java files which should be ignored '
                                'by the parser.')
  args = arg_parser.parse_args(build_utils.ExpandFileArgs(argv[1:]))
  args.sources_files = build_utils.ParseGnList(args.sources_files)

  if args.sources_files:
    java_file_paths = []
    for f in args.sources_files:
      # java_file_paths stores each Java file path as a string.
      java_file_paths += build_utils.ReadSourcesList(f)
  else:
    print '\nError: Must specify --sources_files.'
    return 1
  output_file = args.output
  GenerateJNIHeader(java_file_paths, output_file, args)

  if args.depfile:
    build_utils.WriteDepfile(args.depfile, output_file)

if __name__ == '__main__':
  sys.exit(main(sys.argv))
