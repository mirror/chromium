#!/usr/bin/env bash
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import array
import os
import sys

kStreamsSerializationFormatVersion = 20090218
kEnsembleVersion = 20110216
kMagic = sum(ord(ch) << (i * 8) for i, ch in enumerate('Cou\x00'))

def ReadFile(fname):
  size = os.path.getsize(fname)
  ret = array.array('B')
  with open(fname, 'rb') as f:
    ret.fromfile(f, size)
  return ret

class ReadStream:
  def __init__(self, data = None, pos = 0):
    self.data = data if data != None else array.array('B')
    self.pos = pos
  def size(self):
    return len(self.data)
  def nextBytes(self, delta):
    self.pos += delta
    if self.pos > len(self.data):
      raise ValueError('Exhausted data.')
    return self.data[self.pos - delta:self.pos]
  def nextUInt8(self):
    if self.pos >= len(self.data):
      raise ValueError('Exhausted data.')
    ret = self.data[self.pos]
    self.pos += 1
    return ret
  def nextVarInt32(self):
    ret = 0
    sh = 0
    while True:
      if self.pos >= len(self.data):
        raise ValueError('Exhausted data.')
      v = self.data[self.pos]
      self.pos += 1
      ret |= (v & 0x7F) << sh
      sh += 7
      if v < 128:
        break
    return ret
  def nextVarInt32Signed(self):
    ret = self.nextVarInt32()
    sign = ret & 1
    ret >>= 1
    return ~ret if sign else ret
  def empty(self):
    return self.pos >= len(self.data)

def SingleToMulti(rs):
  ver = rs.nextVarInt32()
  if ver != kStreamsSerializationFormatVersion:
    raise ValueError('Invalid packed stream version %d.' % ver)
  n = rs.nextVarInt32()
  sizes = [rs.nextVarInt32() for _ in xrange(n)]
  return [ReadStream(rs.nextBytes(size)) for size in sizes]


class ExecutableType:
  EXE_UNKNOWN = 0
  EXE_WIN_32_X86 = 1
  EXE_ELF_32_X86 = 2
  EXE_ELF_32_ARM = 3
  EXE_WIN_32_X64 = 4


class MBSHeader:
  def __init__(self):
    self.slen = None
    self.scrc32 = None
    self.dlen = None

  def Read(self, rs):
    tag = rs.nextBytes(8)
    if tag.tostring() != 'GBSDIF42':
      raise ValueError('Invalid MBS header.')
    self.slen = rs.nextVarInt32()
    self.scrc32 = rs.nextVarInt32()
    self.dlen = rs.nextVarInt32()


class PatcherX86_32:
  def __init__(self, kind):
    self.base_offset = -1
    self.base_length = 0
    self.kind = kind

  def ReadBasicInfo(self, params):
    self.base_offset = params.nextVarInt32()
    self.base_length = params.nextVarInt32()

  def __str__(self):
    return '(kind: %d, offset: %d, length: %d)' % \
        (self.kind, self.base_offset, self.base_length)


def Patcher(kind):
  if kind == ExecutableType.EXE_WIN_32_X86:
    return PatcherX86_32(kind)
  if kind == ExecutableType.EXE_ELF_32_X86:
    return PatcherX86_32(kind)
  elif kind == ExecutableType.EXE_WIN_32_X64:
    return PatcherX86_32(kind)
  raise ValueError('Unimplemented kind %d' % kind)


class PatchAnalyzerApp(object):
  def __init__(self, patch_data):
    self.rs = ReadStream(patch_data)

  def ReadHeader(self):
    print('******** Header ********')
    magic = self.rs.nextVarInt32()
    if magic != kMagic:
      raise ValueError('Bad patch magic.')
    version = self.rs.nextVarInt32()
    if version != kEnsembleVersion:
      raise ValueError('Bad patch version.')
    self.source_checksum = self.rs.nextVarInt32()
    self.target_checksum = self.rs.nextVarInt32()
    self.final_patch_input_size_prediction = self.rs.nextVarInt32()
    print('                 source_crc: 0x%08X' % self.source_checksum)
    print('                 target_crc: 0x%08X' % self.target_checksum)
    print('final_input_size_prediction: %0d' % \
          self.final_patch_input_size_prediction)
    print('')

  def InitPatchStreams(self):
    sub_data_list = SingleToMulti(self.rs)
    self.transformation_descriptions = sub_data_list[0]
    self.parameter_correction = sub_data_list[1]
    self.transformed_elements_correction = sub_data_list[2]
    self.ensemble_correction = sub_data_list[3]
    if sum(s.size() for s in sub_data_list[4:]) != 0:
      raise ValueError('Trailing stream in patch.')


  def ReadInitialParameters(self):
    number_of_transformations = self.transformation_descriptions.nextVarInt32()
    self.patcher_list = []
    print('******** Patchers ********')
    for _ in xrange(number_of_transformations):
      kind = self.transformation_descriptions.nextVarInt32()
      self.patcher_list.append(Patcher(kind))
    for patcher in self.patcher_list:
      patcher.ReadBasicInfo(self.transformation_descriptions)
      print(str(patcher))
    if not self.transformation_descriptions.empty():
      raise ValueError('Trailing data in transformation_descriptions.')
    print('')

  def AnalyzeDelta(self, delta_rs, title):
    header = MBSHeader()
    header.Read(delta_rs)
    print('******** %s ********' % title)
    print('                       slen: %d' % header.slen)
    print('                     scrc32: 0x%08X' % header.scrc32)
    print('                       dlen: %d' % header.dlen)

    delta_list = SingleToMulti(delta_rs)
    control_stream_copy_counts = delta_list[0]
    control_stream_extra_counts = delta_list[1]
    control_stream_seeks = delta_list[2]
    diff_skips = delta_list[3]
    diff_bytes = delta_list[4]
    extra_bytes = delta_list[5]

    print(' control_stream_copy_counts: %d' % control_stream_copy_counts.size())
    print('control_stream_extra_counts: %d' %
              control_stream_extra_counts.size())
    print('       control_stream_seeks: %d' % control_stream_seeks.size())
    print('                 diff_skips: %d' % diff_skips.size())
    print('                 diff_bytes: %d' % diff_bytes.size())
    print('                extra_bytes: %d' % extra_bytes.size())

    old_pos = 0
    pending_diff_zeros = diff_skips.nextVarInt32()
    num_instructions = 0
    total_copy = 0

    full_dump = False
    instr_use_sum = 0
    use3_sum = 0

    out_offset = 0
    dist = 0
    intervs = []

    while not control_stream_extra_counts.empty():
      use1 = -control_stream_copy_counts.pos
      use2 = -control_stream_extra_counts.pos
      use3 = -control_stream_seeks.pos
      copy_count = control_stream_copy_counts.nextVarInt32()
      extra_count = control_stream_extra_counts.nextVarInt32()
      seek_stream_pos = control_stream_seeks.pos
      seek_adjustment = control_stream_seeks.nextVarInt32Signed()
      use1 += control_stream_copy_counts.pos
      use2 += control_stream_extra_counts.pos
      use3 += control_stream_seeks.pos
      use3_sum += use3
      instr_use_sum += use1 + use2 + use3
      intervs.append((old_pos, copy_count))
      dist += abs(seek_adjustment)

      num_instructions += 1
      total_copy += copy_count

      if full_dump:
        sys.stdout.write('%08X @%08X: %12X%12X%+12X' % (out_offset, old_pos,
            copy_count, extra_count, seek_adjustment))
      if copy_count > header.slen:
        raise ValueError('Exhausted data.')
      num_diff = 0

      out_offset += copy_count + extra_count
      while copy_count > 0:
        num_copy = min(copy_count, pending_diff_zeros)
        if num_copy > 0:
          #ret.append(old_bytes[old_pos:old_pos + num_copy])
          old_pos += num_copy
          pending_diff_zeros -= num_copy
          copy_count -= num_copy
          if copy_count <= 0:
            break
        pending_diff_zeros = diff_skips.nextVarInt32()
        diff_byte = diff_bytes.nextUInt8()
        num_diff += 1
        #ret.appendCh((old_bytes[old_pos] + diff_byte) & 0xFF)
        old_pos += 1
        copy_count -= 1
      if full_dump:
        sys.stdout.write('%12X\n' % num_diff)
      extra = extra_bytes.nextBytes(extra_count)
      if full_dump and extra:
        print(' '.join('%02X' % t for t in extra))
      #ret.append(extra)
      old_pos += seek_adjustment
    if full_dump:
      print('%08X @%08X' % (out_offset, old_pos))

    print('               instructions: %d' % num_instructions)
    print('               average copy: %f' %
          (float(total_copy) / num_instructions))
    print('  average instruction bytes: %f' %
          (float(instr_use_sum) / num_instructions))
    print('         average seek bytes: %f' %
          (float(use3_sum) / num_instructions))
    print('      average seek distance: %f' % (float(dist) / num_instructions))

    # Dedup interval overlaps to count source bytes used.
    intervs.sort()
    prev = 0
    reuse = 0
    for a, s in intervs:
      b = a + s
      d = max(0, b - max(prev, a))
      reuse += d
      prev = max(prev, b)
    print('Reuse: %d => %.2f%%' % (reuse, reuse * 100.0 / header.slen))

    if not control_stream_copy_counts.empty() or \
        not control_stream_extra_counts.empty() or \
        not control_stream_seeks.empty() or \
        not diff_skips.empty() or \
        not diff_bytes.empty() or \
        not extra_bytes.empty():
      raise ValueError('Found leftover data.')
    print('')

  def Run(self):
    self.ReadHeader()
    self.InitPatchStreams()
    self.ReadInitialParameters()
    self.AnalyzeDelta(self.parameter_correction, 'Parameter Correction')
    self.AnalyzeDelta(self.transformed_elements_correction, \
                      'Transformed elements Correction')
    self.AnalyzeDelta(self.ensemble_correction, 'Ensemble Correction')


def main():
  patch_file = None
  params = []
  for tok in sys.argv[1:]:
    if patch_file is None:
      patch_file = tok

  if patch_file is None:
    print('python %s <patch_file>' % sys.argv[0])
    sys.exit(0)

  patch_data = ReadFile(patch_file)
  app = PatchAnalyzerApp(patch_data)
  app.Run()

if __name__ == '__main__':
  main()
