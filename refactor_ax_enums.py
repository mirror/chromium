#!/usr/bin/env python

#ax::mojom::Event -> ax::mojom::Event
#ax::mojom::Event::ALERT -> ax::mojom::Event.ALERT
#
#const char* ToString(ax::mojom::Event) {
#}

import os, sys, re, glob

SRC = "ui/accessibility/ax_enums.idl"
SRCGEN = "gn_out/release/gen/ui/accessibility/ax_enums.cc"
DST = "ui/accessibility/ax_enums.mojom"
TOSTRING_H = "ui/accessibility/ax_enum_util.h"
TOSTRING_CC = "ui/accessibility/ax_enum_util.cc"
LICENSE = """// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
"""

def CamelToUpperHacker(str):
  out = ''
  for i in range(len(str)):
    if str[i] >= 'A' and str[i] <= 'Z' and out:
      out += '_'
    out += str[i]
  return out.upper()

def CamelToLowerHacker(str):
  out = ''
  for i in range(len(str)):
    if str[i] >= 'A' and str[i] <= 'Z' and out:
      out += '_'
    out += str[i]
  return out.lower()

def ToCamel(str):
  out = ''
  upper = False
  for i in range(len(str)):
    if str[i] == '_':
      upper = True
    elif upper:
      out += str[i].upper()
      upper = False
    else:
      out += str[i].lower()
  return out

tostring_h = open(TOSTRING_H, "w")
tostring_h.write(LICENSE)
tostring_h.write("\n")
tostring_h.write('#include "ui/accessibility/ax_enums.mojom.h"\n')
tostring_h.write('#include "ui/accessibility/ax_export.h"\n')
tostring_h.write("\n")
tostring_h.write("namespace ui {\n")
tostring_h.write("\n")
tostring_cc = open(TOSTRING_CC, "w")
tostring_cc.write(LICENSE)
tostring_cc.write("\n")
tostring_cc.write('#include "ui/accessibility/ax_enum_util.h"\n')
tostring_cc.write("\n")
tostring_cc.write("namespace ui {\n")
tostring_cc.write("\n")

tostring_fn = ""
parse_fn = ""

dst = open(DST, "w")
old_enum_name = None
new_enum_name = None
last_new_token = None
lookup = {}
for line in open(SRC).readlines():
  line = line[:-1]
  if re.search('namespace ui', line):
    dst.write('module ax.mojom;\n')
    continue
  if line == '};':
    break

  line = line.replace('2014', '2017')

  if len(line) < 2 or line[:2] == '//':
    dst.write(line + '\n')
    continue

  line = line[2:]

  comment_index = line.find('//')
  if comment_index >= 0:
    comment = line[comment_index:]
    line = line[:comment_index]

  m = re.search('enum (\w+) {', line)
  if m:
    old_enum_name = m.group(1)
    new_enum_name = old_enum_name[2:]
    lookup[old_enum_name] = 'ax::mojom::' + new_enum_name
    print '%s -> ax::mojom::%s' % (old_enum_name, new_enum_name)

    std_prefix = 'AX_' + CamelToUpperHacker(new_enum_name)
    m2 = re.search('cpp_enum_prefix_override="(\w+)"', line)
    if m2:
      prefix = m2.group(1).upper()
    else:
      prefix = std_prefix

    lookup['%s_NONE' % std_prefix] = 'ax::mojom::%s::NONE' % new_enum_name
    print '%s_NONE -> ax::mojom::%s::NONE' % (std_prefix, new_enum_name)

    lookup['%s_LAST' % std_prefix] = 'ax::mojom::%s::LAST' % new_enum_name
    print '%s_LAST -> ax::mojom::%s::LAST' % (std_prefix, new_enum_name)

    print new_enum_name + ' prefix=' + prefix
    dst.write('enum %s {\n' % new_enum_name)
    dst.write('  NONE,\n')

    tostring_h.write('// ax::mojom::%s\n' % new_enum_name)
    tostring_h.write('AX_EXPORT const char* ToString(ax::mojom::%s %s);\n' % (
        new_enum_name, CamelToLowerHacker(new_enum_name)))
    tostring_h.write('AX_EXPORT ax::mojom::%s Parse%s(const char *%s);\n' % (
        new_enum_name, new_enum_name, CamelToLowerHacker(new_enum_name)))
    tostring_h.write('\n')
    continue

  if line == '};':
    tostring_cc.write('const char* ToString(ax::mojom::%s %s) {\n' % (
        new_enum_name, CamelToLowerHacker(new_enum_name)))
    tostring_cc.write('  switch (%s) {\n' % CamelToLowerHacker(new_enum_name));
    tostring_cc.write('    case ax::mojom::%s::NONE:\n' % new_enum_name)
    tostring_cc.write('      return "none";\n')
    tostring_cc.write(tostring_fn)
    tostring_cc.write('  }\n')
    tostring_cc.write('}\n')
    tostring_cc.write('\n')

    tostring_cc.write('ax::mojom::%s Parse%s(const char *%s) {\n' % (
        new_enum_name, new_enum_name, CamelToLowerHacker(new_enum_name)))
    tostring_cc.write('  if (0 == strcmp(%s, "none"))\n' %
                      CamelToLowerHacker(new_enum_name))
    tostring_cc.write('    return ax::mojom::%s::NONE;\n' % new_enum_name)
    tostring_cc.write(parse_fn)
    tostring_cc.write('  return ax::mojom::%s::NONE;\n' % new_enum_name)
    tostring_cc.write('}\n')
    tostring_cc.write('\n')
    parse_fn = ""
    tostring_fn = ""

    dst.write('  LAST = %s,\n' % last_new_token)
    old_enum_name = None
    new_enum_name = None

  if new_enum_name:
    line = line.upper()
    m = re.search('(\w+)', line)
    if m:
      if line.find(',') == -1:
        index = len(line) - 1
        while line[index] == ' ':
          index -= 1
        line = line[:index + 1] + ',' + line[index + 1:]
      token = m.group(1)
      new_token = token
      if new_token == 'TRUE':
        new_token = 'TRUE_VALUE'
        line = line.replace('TRUE', 'TRUE_VALUE')
      if new_token == 'FALSE':
        new_token = 'FALSE_VALUE'
        line = line.replace('FALSE', 'FALSE_VALUE')
      if new_token == 'MOUSE_MOVED':
        new_token = 'MOUSE_MOVED_VALUE'
        line = line.replace('MOUSE_MOVED', 'MOUSE_MOVED_VALUE')
      last_new_token = new_token
      old_value = '%s_%s' % (prefix, token)
      new_value = 'ax::mojom::%s::%s' % (new_enum_name, new_token)
      print '%s -> %s' % (old_value, new_value)
      lookup[old_value] = new_value
      tostring_fn += '    case ax::mojom::%s::%s:\n' % (
          new_enum_name, new_token)
      tostring_fn += '      return "%s";\n' % ToCamel(token)
      parse_fn += '  if (0 == strcmp(%s, "%s"))\n' % (
          CamelToLowerHacker(new_enum_name), ToCamel(token))
      parse_fn += '    return ax::mojom::%s::%s;\n' % (
          new_enum_name, new_token)

  if comment_index >= 0:
    line += comment

  dst.write(line + '\n')

tostring_h.flush()
tostring_cc.flush()

print 'Includes'
for fileline in os.popen('git grep -l ax_enums.h'):
  filename = fileline.strip()
  changes = 0
  ax_enums = 0
  ax_enum_util = 0
  to_string = False
  lines = open(filename, 'r').readlines()
  for index, line in enumerate(lines):
    if line.find('#include "ui/accessibility/ax_enums"') == 0:
      ax_enums = index
    if line.find('#include "ui/accessibility/ax_enum_util"') == 0:
      ax_enum_util = index
    if (line.find('ui::ToString(') > 0 or
        (line.find('ToString(') > 0 and filename.find('ccessib') > 0)):
      to_string = True
    if line.find('#include "ui/accessibility/ax_enums.h"') == 0:
      line = line.replace('ax_enums.h', 'ax_enums.mojom.h')
      lines[index] = line
      changes += 1
  if ax_enums and to_string and not ax_enum_util:
    lines = (lines[:ax_enums+1] +
             ['#include "ui/accessibility/ax_enum_util.h"'] +
             lines[ax_enums+1:])
    changes += 1
  if changes > 0:
    print '%5d %s' % (changes, filename)
    fp = open(filename, 'w')
    for line in lines:
      fp.write(line)
    fp.close()

print 'Enums'
#for fileline in ['ui/accessibility/ax_action_data.h']: #os.popen('git grep -l AX'):
for fileline in os.popen('git grep -l AX'):
  filename = fileline.strip()
  if filename.find('third_party/WebKit') >= 0:
    continue
  if filename[-4:] == '.idl':
    continue
  if filename[-4:] == '.grd':
    continue
  if filename[-4:] == '.txt':
    continue
  if filename[-5:] == '.html':
    continue
  if filename.find('libsupersize') > 0:
    continue
  changes = 0
  lines = open(filename, 'r').readlines()
  for index, line in enumerate(lines):
    off = 0
    for m in re.finditer('((ui::)?AX\w+)', line):
      c = m.start()
      if c > 0:
        prev_char = line[c + off - 1]
        if (prev_char == '"' or
            prev_char == '_' or
            prev_char.isalpha() or
            prev_char == ')'):
          continue
      if c > 5 and line[c + off - 5 : c + off] == 'self ':
        continue
      token = m.group(1)
      key = token
      if token[:4] == 'ui::':
        key = token[4:]
      if key in lookup:
        line = line[:m.start()+off] + lookup[key] + line[m.end()+off:]
        lines[index] = line
        off += len(lookup[key])-len(token)
        changes += 1
  if changes > 0:
    print '%5d %s' % (changes, filename)
    fp = open(filename, 'w')
    for line in lines:
      fp.write(line)
    fp.close()


tostring_h.write("\n")
tostring_h.write("}  // namespace ui\n")
tostring_cc.write("}  // namespace ui\n")
