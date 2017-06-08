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
  dict = {'CLASS_PATH_DEFINITIONS': '', 'FORWARD_DECLARATIONS': '',
          'JNI_NATIVE_METHODS': '', 'REGISTER_MAINDEX_NATIVES': '',
          'REGISTER_NONMAINDEX_NATIVES': ''}
  class_counter = 0
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
      natives = jni_generator.ExtractNatives(contents, options.ptr_type)
      if len(natives) == 0:
        continue
      maindex = jni_generator.IsMainDexJavaClass(contents)
      header_generator = HeaderGenerator(fully_qualified_class, natives,
                                         options, dict, class_counter, maindex)
      class_counter, dict = header_generator.GetContent()
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

// Step 1: Array definition and forward declaration.
namespace {
${CLASS_PATH_DEFINITIONS}

}  // namespace

${FORWARD_DECLARATIONS}

// Step 2: List JNI native methods.
${JNI_NATIVE_METHODS}

// Step 3: Maindex and nonmaindex registration functions.

static bool RegisterMaindexNatives(JNIEnv* env) {
${REGISTER_MAINDEX_NATIVES}

  return true;
}

static bool RegisterNonmaindexNatives(JNIEnv* env) {
${REGISTER_NONMAINDEX_NATIVES}

  return true;
}

#endif  // HEADER_GUARD
""")
  return '' if len(dict['FORWARD_DECLARATIONS']) == 0 \
               or len(dict['JNI_NATIVE_METHODS']) == 0 \
    else jni_generator.WrapOutput(template.substitute(dict))

class HeaderGenerator(object):
  """Generates an inline header file for JNI registration."""

  def __init__(self, fully_qualified_class, natives,
               options, dict, class_counter, maindex):
    self.fully_qualified_class = fully_qualified_class
    self.class_name = self.fully_qualified_class.split('/')[-1]
    self.natives = natives
    self.options = options
    self.dict = dict
    self.class_counter = class_counter
    self.maindex = maindex
    self.inl_header_file_generator = \
      InlHeaderFileGenerator('', self.fully_qualified_class, self.natives,
                             [], [], self.options)

  def AddClassPathDefinitions(self):
    """Returns the ClassPath constants."""
    ret = []
    template = Template("""\
const char ${CLASS_COUNTER}ClassPath[] = "${JNI_CLASS_PATH}";""")
    all_classes = self.inl_header_file_generator.GetUniqueClasses(
      self.natives)
    class_counter = self.class_counter
    for clazz in all_classes:
      values = {
        'CLASS_COUNTER': 'k' + str(class_counter),
        'JNI_CLASS_PATH': all_classes[clazz],
      }
      ret += [template.substitute(values)]
      class_counter += 1
    ret += ''

    template = Template("""\
// Leaking this jclass as we cannot use LazyInstance from some threads.
base::subtle::AtomicWord g_${CLASS_COUNTER}_clazz __attribute__((unused)) = 0;
#define ${CLASS_COUNTER}_clazz(env) \
base::android::LazyGetClass(env, ${CLASS_COUNTER}ClassPath, \
&g_${CLASS_COUNTER}_clazz)""")

    class_counter = self.class_counter
    for clazz in all_classes:
      values = {
        'CLASS_COUNTER': 'k' + str(class_counter),
      }
      ret += [template.substitute(values)]
      class_counter += 1
    self.dict['CLASS_PATH_DEFINITIONS'] += '\n\n' + '\n'.join(ret)

  def AddForwardDeclaration(self):
    """Add the content of the forward declaration to the dictionary."""
    for native in self.natives:
      template = Template("""\
extern "C" ${RETURN} ${STUB_NAME}(JNIEnv* env, ${PARAMS_IN_STUB});
""")
      return_type = jni_generator.JavaDataTypeToC(native.return_type)
      values = {
        'RETURN': return_type,
        'STUB_NAME': self.inl_header_file_generator.GetStubName(native),
        'PARAMS_IN_STUB': self.inl_header_file_generator.GetParamsInStub(
          native),
      }
      self.dict['FORWARD_DECLARATIONS'] += template.substitute(values) + '\n\n'

  def AddNativeMethods(self):
    """Add the content of the JNI native methods to the dictionary."""
    template = Template("""\
static const JNINativeMethod kMethods${CLASS_COUNTER}[] = {
${KMETHODS}
};
""")
    self.dict['JNI_NATIVE_METHODS'] += \
      self.SubstituteNativeMethods(template)

  def AddRegisterNatives(self):
    """Add the body of the RegisterNativesImpl method to the dictionary."""
    template = Template("""\
  const int kMethods${CLASS_COUNTER}Size = arraysize(kMethods${CLASS_COUNTER});
  if (env->RegisterNatives(${CLASS_COUNTER}_clazz(env),
                           kMethods${CLASS_COUNTER},
                           kMethods${CLASS_COUNTER}Size) < 0) {
    jni_generator::HandleRegistrationError(
        env, ${CLASS_COUNTER}_clazz(env), __FILE__);
    return false;
  }
""")
    register_function_body = self.SubstituteNativeMethods(template)
    if self.maindex == 'true':
      self.dict['REGISTER_MAINDEX_NATIVES'] += register_function_body
    else:
      self.dict['REGISTER_NONMAINDEX_NATIVES'] += register_function_body

  def SubstituteNativeMethods(self, template):
    """Substitutes JAVA CLASS and KMETHODS in the provided template."""
    ret = []
    all_classes = self.inl_header_file_generator.GetUniqueClasses(self.natives)
    all_classes[self.class_name] = self.fully_qualified_class
    class_counter = self.class_counter
    for clazz in all_classes:
      kmethods = self.inl_header_file_generator.GetKMethodsString(clazz)

      if kmethods:
        values = {'CLASS_COUNTER': 'k' + str(class_counter),
                  'KMETHODS': kmethods}
        ret += [template.substitute(values)]
        class_counter += 1
    if not ret: return ''
    return '\n' + '\n'.join(ret)

  def GetContent(self):
    self.AddClassPathDefinitions()
    self.AddForwardDeclaration()
    self.AddNativeMethods()
    self.AddRegisterNatives()
    self.class_counter += len(self.inl_header_file_generator.
                              GetUniqueClasses(self.natives))
    return self.class_counter, self.dict


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
