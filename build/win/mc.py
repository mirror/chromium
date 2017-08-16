# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import argparse
import os
import xml.etree.ElementTree as etree

parser = argparse.ArgumentParser(add_help=False)
parser.add_argument('-um', action='store_true')
parser.add_argument('-h')
parser.add_argument('-r')
parser.add_argument('-u', action='store_true')
parser.add_argument('input')
args = parser.parse_args()

assert args.input.endswith('.man')  # FIXME: handle .mc inputs too
header_out = os.path.join(args.h,
  os.path.splitext(os.path.basename(args.input))[0] + '.h')
rc_out = os.path.join(args.r,
  os.path.splitext(os.path.basename(args.input))[0] + '.rc')
open(rc_out, 'w').write('# FIXME: implement rc generation')
out = open(header_out, 'w')
tree = etree.parse(args.input)

root = tree.getroot()
assert root.tag == '{http://schemas.microsoft.com/win/2004/08/events}instrumentationManifest'
assert len(root) == 2

instr = root.find('{http://schemas.microsoft.com/win/2004/08/events}instrumentation')
assert instr is not None
assert len(instr) == 1

events = instr[0]
assert events.tag == '{http://schemas.microsoft.com/win/2004/08/events}events'

# FIXME: most stuff should only be emitted if -um is passed.
# FIXME: build AST:
# - [provider]
#   .name, symbol, guid
#   - [channels]
#   - [templates]
#     .tid
#     - [data]
#       .inType
#       .name
#   - [events]

# Emit shared runtime functions.
print >> out, '''#pragma once
#include <winnt.h>
#include <Evntprov.h>
#include <Evntrace.h>
#if !defined(ETW_INLINE)
#define ETW_INLINE DECLSPEC_NOINLINE __inline
#endif
#if defined(__cplusplus)
extern "C" {
#endif
DECLSPEC_NOINLINE __inline ULONG __stdcall
McGenEventRegister(LPCGUID ProviderId, PENABLECALLBACK EnableCallback,
                   PVOID CallbackContext, PREGHANDLE RegHandle) {
  if(*RegHandle)
    return ERROR_SUCCESS;
  return EventRegister(ProviderId, EnableCallback, CallbackContext, RegHandle);
}
DECLSPEC_NOINLINE __inline ULONG __stdcall
McGenEventUnregister(PREGHANDLE RegHandle) {
  if(!*RegHandle)
    return ERROR_SUCCESS;
  ULONG Error = EventUnregister(*RegHandle);
  *RegHandle = (REGHANDLE)0;
  return Error;
}
typedef struct _MCGEN_TRACE_CONTEXT {
    TRACEHANDLE RegistrationHandle;
    TRACEHANDLE Logger;
    ULONGLONG MatchAnyKeyword;
    ULONGLONG MatchAllKeyword;
    ULONG Flags;
    ULONG IsEnabled;
    UCHAR Level;
    UCHAR Reserve;
    USHORT EnableBitsCount;
    PULONG EnableBitMask;
    const ULONGLONG* EnableKeyWords;
    const UCHAR* EnableLevel;
} MCGEN_TRACE_CONTEXT, *PMCGEN_TRACE_CONTEXT;
DECLSPEC_NOINLINE __inline VOID __stdcall
McGenControlCallbackV2(LPCGUID SourceId, ULONG ControlCode, UCHAR Level,
                       ULONGLONG MatchAnyKeyword, ULONGLONG MatchAllKeyword,
                       PEVENT_FILTER_DESCRIPTOR FilterData,
                       PVOID CallbackContext) {
  if (!CallbackContext)
    return;
  MCGEN_TRACE_CONTEXT* Ctx = (MCGEN_TRACE_CONTEXT*)CallbackContext;
  switch (ControlCode) {
    case EVENT_CONTROL_CODE_ENABLE_PROVIDER:
      Ctx->Level = Level;
      Ctx->MatchAnyKeyword = MatchAnyKeyword;
      Ctx->MatchAllKeyword = MatchAllKeyword;
      Ctx->IsEnabled = 1;
      // FIXME: Ctx->EnableBitMask
      break;
    case EVENT_CONTROL_CODE_DISABLE_PROVIDER:
      Ctx->Level = 0;
      Ctx->MatchAnyKeyword = 0;
      Ctx->MatchAllKeyword = 0;
      Ctx->IsEnabled = 0;
      // FIXME: Ctx->EnableBitMask
      break;
    default:
      break;
  }
}
'''

for provider in events:
  assert provider.tag == '{http://schemas.microsoft.com/win/2004/08/events}provider'
  assert 'guid' in provider.attrib
  guid = provider.attrib['guid']
  assert 'name' in provider.attrib
  name = provider.attrib['name']
  provider_name = name
  assert 'symbol' in provider.attrib
  symbol = provider.attrib['symbol']

  print >> out, '// Provider %s.' % name

  # Write GUID.
  assert guid.count('-') == 4
  assert guid[0] == '{'
  assert guid[-1] == '}'
  g1, g2, g3, g4, g5 = guid[1:-1].split('-')
  g5 = g4 + g5
  g5 = ', '.join(['0x' + g5[i:i+2].lower() for i in range(0, len(g5), 2)])
  guid = ', '.join(['0x' + s.lower() for s in [g1, g2, g3]] + ['{' + g5  + '}'])
  print >> out, 'EXTERN_C __declspec(selectany) const GUID %s =\n{%s};' % (symbol, guid)

  # Write context.
  print >> out, 'EXTERN_C __declspec(selectany) DECLSPEC_CACHEALIGN ULONG %sEnableBits[1];' % name
  # See <keywords> in winmeta.xml in the SDK, and
  # https://msdn.microsoft.com/en-us/library/windows/desktop/aa382786(v=vs.85).aspx
  # 0x8000000000000000 is marked as "used to pass channel information".
  keywords = '0x8000000000000000'
  print >> out, 'EXTERN_C __declspec(selectany) const ULONGLONG %sKeywords[1] = {%s};' % (name, keywords)
  level = '4'  # FIXME: is this just win:Informational below?
  # Looks like if there are multiple events with different levels, then
  # ChromeEnableBits keeps size 1 but ChromeKeywords and ChromeLevels have then
  # multiple entries (one per used level)
  print >> out, 'EXTERN_C __declspec(selectany) const UCHAR %sLevels[1] = {%s};' % (name, level)
  print >> out, 'EXTERN_C __declspec(selectany) MCGEN_TRACE_CONTEXT %s_Context =' % symbol
  # FIXME: the 1 here should be the size of the keywords / levels arrays
  # then the bitmask has one bit per level.
  # For system channel, can only have levels 0, 1, 2, 3. with custom levels, can
  # have more.
  print >> out,'{0, 0, 0, 0, 0, 0, 0, 0, 1, %sEnableBits, %sKeywords, %sLevels};' % (name, name, name)
  print >> out, 'EXTERN_C __declspec(selectany) REGHANDLE %sHandle = (REGHANDLE)0;' % name

  # Write functions.
  print >> out, '''#ifndef EventRegister%(name)s
#define EventRegister%(name)s() McGenEventRegister(&%(symbol)s, McGenControlCallbackV2, &%(symbol)s_Context, &%(name)sHandle)
#endif
#ifndef EventUnregister%(name)s
#define EventUnregister%(name)s() McGenEventUnregister(&%(name)sHandle)
#endif''' % { 'name': name, 'symbol': symbol }

  # Write templates.
  templates = provider.find('{http://schemas.microsoft.com/win/2004/08/events}templates')
  assert templates is not None
  template_map = {}
  for template in templates:
    assert template.tag == '{http://schemas.microsoft.com/win/2004/08/events}template'
    assert 'tid' in template.attrib
    tid = template.attrib['tid']
    names = []
    for data in template:
      assert data.tag == '{http://schemas.microsoft.com/win/2004/08/events}data'
      assert 'inType' in data.attrib
      inType = data.attrib['inType']
      assert 'name' in data.attrib
      name = data.attrib['name']
      names.append(name)

      assert inType == 'win:AnsiString'  # FIXME: more?
    template_map[tid] = (names, )
    arg_count = len(template)
    pattern = arg_count * 's'
    args = ',\n    '.join(['LPCSTR _Arg%d' % i for i in range(arg_count)])
    print >> out, '''#if !defined(Template_%s_def)
#define Template_%s_def
ETW_INLINE ULONG Template_%s(
    REGHANDLE RegHandle,
    PCEVENT_DESCRIPTOR Descriptor,
    %s) {
  EVENT_DATA_DESCRIPTOR EventData[%d];''' % (pattern, pattern, pattern, args, arg_count)

    for i in range(arg_count):
      print >> out, '  EventDataDescCreate(&EventData[%d],' % i
      print >> out, '                      _Arg%d ? _Arg%d : "NULL",' % (i, i)
      print >> out, '                      _Arg%d ? (ULONG)(strlen(_Arg%d) + 1) : (ULONG)sizeof("NULL"));' % (i, i)

    print >> out, '''  return EventWrite(RegHandle, Descriptor, %d, EventData);
}
#endif  // Template_%s_def''' % (arg_count, pattern)

  # Write events.
  events = provider.find('{http://schemas.microsoft.com/win/2004/08/events}events')
  assert events is not None
  for event in events:
    assert event.tag == '{http://schemas.microsoft.com/win/2004/08/events}event'
    assert 'channel' in event.attrib
    channel = event.attrib['channel']
    assert 'opcode' in event.attrib
    opcode = event.attrib['opcode']
    assert 'symbol' in event.attrib
    symbol = event.attrib['symbol']
    assert 'template' in event.attrib
    template = event.attrib['template']
    assert 'value' in event.attrib
    value = event.attrib['value']
    assert 'task' not in event.attrib
    task = '0'

    assert template in template_map
    assert channel == 'SYSTEM'; channel = '8'
    assert opcode == 'win:Info'; opcode = 0

    level = '0'
    if 'level' in event.attrib:
      # https://msdn.microsoft.com/en-us/library/windows/desktop/aa382793(v=vs.85).aspx
      level = {
        'win:Critical': '1',
        'win:Error': '2',
        'win:Warning': '3',
        'win:Informational': '4',
        'win:Verbose': '5',
      }[event.attrib['level']]

    version = event.attrib.get('version', '0')

    # FIXME: check that all events have different ids + versions
    # FIXME: check that IDs withd different IDs have different message strings

    # https://msdn.microsoft.com/en-us/library/windows/desktop/aa363754(v=vs.85).aspx
    print >> out, 'EXTERN_C __declspec(selectany) const EVENT_DESCRIPTOR %s = {%s, %s, %s, %s, %s, %s, %s};' % (symbol, value, version, channel, level, opcode, task, keywords)
    # FIXME: hardcoding 0x00000001 here likely isn't right
    # Looks like this is 1 bit per event with a different level.
    print >> out, '#define EventEnabled%s() ((%sEnableBits[0] & 0x00000001) != 0)' % (symbol, provider_name)
    arg_names, = template_map[template]
    arg_names = [a.replace(' ', '_') for a in arg_names]
    print >> out, '#define EventWrite%s(%s) \\' % (symbol, ', '.join(arg_names))
    print >> out, '    EventEnabled%s() \\' % symbol
    print >> out, '        ? Template_%s(%sHandle, &%s, %s) \\' % ('s' * len(arg_names), provider_name, symbol, ', '.join(arg_names))
    print >> out, '        : ERROR_SUCCESS'

print >> out, '''
#if defined(__cplusplus)
}  // extern "C"
#endif'''
