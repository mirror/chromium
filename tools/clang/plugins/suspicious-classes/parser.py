#!/usr/bin/env python

import argparse
import re
import sys


class TypeName(object):
  def __init__(self):
    self._string = None

  @property
  def string(self):
    return self._string

  @property
  def is_pointer(self):
    return self._string.endswith('*')

  def __str__(self):
    return self._string

  __repr__ = __str__

  @classmethod
  def FromString(self, string):
    type_name = self._TYPE_NAME_BY_STRING.get(string)
    if type_name is None:
      type_name = TypeName()
      type_name._string = string
      self._TYPE_NAME_BY_STRING[string] = type_name
    return type_name

  _TYPE_NAME_BY_STRING = {}


class Field(object):
  def __init__(self):
    self._record_type_name = None
    self._type_name = None
    self._name = None
    self._total_offset = None
    self._total_offset_to = None

  @property
  def record_type_name(self):
    return self._record_type_name

  @property
  def type_name(self):
    return self._type_name

  @property
  def name(self):
    return self._name

  @property
  def total_offset(self):
    return self._total_offset

  @property
  def total_offset_to(self):
    return self._total_offset_to

  @classmethod
  def ParsePath(self, string):
    path = []
    for field_string in string.split('#'):
      field = self._Parse(field_string)
      assert field is not None, 'not a field: {}'.format(field_string)
      path.append(field)

    return path

  @classmethod
  def _Parse(self, string):
    match = self._PATTERN.match(string)
    if not match:
      return None

    field = Field()
    field._record_type_name = TypeName.FromString(match.group('record_type'))
    field._type_name = TypeName.FromString(match.group('type'))
    field._name = match.group('name') or '<unnamed>'
    field._total_offset = int(match.group('offset'))
    field._total_offset_to = int(match.group('offset_to'))

    return field

  _PATTERN = re.compile(
      r'^'
      + r'(?P<record_type>[^\$]+)\$'
      + r'(?P<type>[^\$]+)\$'
      + r'(?P<name>[^:]*):'
      + r'(?P<offset>\d+)-'
      + r'(?P<offset_to>\d+)'
      + r'$')


class Location(object):
  def __init__(self):
    self._file_path = None
    self._line = None

  @property
  def file_path(self):
    return self._file_path

  @property
  def line(self):
    return self._line

  def __str__(self):
    return '{}:{}'.format(self.file_path, self.line)

  __repr__ = __str__

  @classmethod
  def Parse(self, string):
    match = self._PATTERN.match(string)

    location = Location()
    if not match:
      location._file_path = string
      location._line = 0
    else:
      file_path = match.group('file_path')
      file_path = re.sub(r'^(../)+', '', file_path)
      location._file_path = file_path
      location._line = int(match.group('line'))

    return location

  _PATTERN = re.compile(
      r'^'
      + r'(?P<file_path>.+):'
      + r'(?P<line>\d+):'
      + r'(?P<column>\d+)'
      + r'$')


class SuspiciousClass(object):
  def __init__(self):
    self._arch = None
    self._location = None
    self._type_name = None
    self._size = None
    self._suspicious_field_path = None

  @property
  def arch(self):
    return self._arch

  @property
  def location(self):
    return self._location

  @property
  def type_name(self):
    return self._type_name

  @property
  def size(self):
    return self._size

  @property
  def suspicious_field(self):
    return self.suspicious_field_path[-1]

  @property
  def suspicious_field_path(self):
    return self._suspicious_field_path

  @classmethod
  def Parse(self, string):
    match = self._PATTERN.match(string)
    if not match:
      return None

    sclass = SuspiciousClass()
    sclass._arch = match.group('arch')
    sclass._location = Location.Parse(match.group('location'))
    sclass._type_name = TypeName.FromString(match.group('type'))
    sclass._size = int(match.group('size'))
    sclass._suspicious_field_path = Field.ParsePath(match.group('field_path'))

    return sclass

  _PATTERN = re.compile(
      r'^'
      + r'(?P<arch>.+?)\|'
      + r'(?P<location>.+?)\|'
      + r'(?P<type>.+?)\|'
      + r'(?P<size>\d+)\|'
      + r'(?P<field_path>.*)'
      + r'$')


def ParseSuspiciousClasses(input_file, output_file, print_lines=False):
  sclass_by_line = {}

  for line in input_file:
    line = line.rstrip()
    if line not in sclass_by_line:
      sclass = SuspiciousClass.Parse(line)
      if sclass:
        if print_lines:
          print line
        if output_file:
          output_file.write(line)
          output_file.write('\n')
        sclass_by_line[line] = sclass

  return list(sclass_by_line.itervalues())


def main():
  parser = argparse.ArgumentParser()
  parser.add_argument('file', help='File path or "stdin".')
  parser.add_argument('--output', help='Output file.')
  parser.add_argument('--print-parsed-lines', default=False,
                      action='store_true')
  parser.add_argument('--print-human', default=False, action='store_true')
  parser.add_argument('--print-table', default=False, action='store_true')

  options = parser.parse_args()

  output_file = None
  if options.output:
    output_file = open(options.output, 'w')

  def _Parse(input_file):
    return ParseSuspiciousClasses(
        input_file, output_file, options.print_parsed_lines)

  if options.file == 'stdin':
    sclasses = _Parse(sys.stdin)
  else:
    with open(options.file) as input_file:
      sclasses = _Parse(input_file)

  if output_file:
    output_file.close()

  not_interesting_class_names = [
      'class tracked_objects::Location',
      'class scoped_refptr',
  ]

  not_interesting_file_paths = [
      'third_party/WebKit',
      'third_party/webrtc',
  ]

  def _IsInteresting(sclass):
    for file_path in not_interesting_file_paths:
      if sclass.location.file_path.startswith(file_path):
        return False

    sfield = sclass.suspicious_field
    if not (sfield.total_offset == 20 and sfield.total_offset_to == 24):
      return False
    if sfield.name.startswith('__') and sfield.name.endswith('_'):
      return False
    for class_name in not_interesting_class_names:
      if sfield.record_type_name.string.startswith(class_name):
        return False

    return True

  interesting_sclasses = [c for c in sclasses if _IsInteresting(c)]
  #interesting_sclasses.sort(key=lambda c: str(c.location))
  interesting_sclasses.sort(key=lambda c: c.suspicious_field.record_type_name)

  if options.print_human:
    print '{} suspicious classes'.format(len(interesting_sclasses))
    print
    for sclass in interesting_sclasses:
      sfield = sclass.suspicious_field
      print '{}'.format(sclass.location)
      print '{} {}'.format(sclass.size, sclass.type_name)
      if sfield.record_type_name is not sclass.type_name:
        print '  ({})'.format(sfield.record_type_name)
      print '  {}-{} {} {}'.format(
          sfield.total_offset,
          sfield.total_offset_to,
          sfield.name,
          sfield.type_name)
      print '  ({})'.format(
          '.'.join(f.name for f in sclass.suspicious_field_path))
      print

  if options.print_table:
    for sclass in interesting_sclasses:
      sfield = sclass.suspicious_field
      row = [sclass.location, sclass.size, sclass.type_name]
      if sfield.record_type_name is not sclass.type_name:
        row.append(sfield.record_type_name)
      else:
        row.append('')
      row.extend((
          sfield.total_offset,
          sfield.total_offset_to,
          sfield.name,
          sfield.type_name,
          '.'.join(f.name for f in sclass.suspicious_field_path)))
      print '\t'.join(str(i) for i in row)


if __name__ == '__main__':
  main()
