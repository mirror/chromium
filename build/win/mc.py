import xml.etree.ElementTree as etree

tree = etree.parse('base/trace_event/etw_manifest/chrome_events_win.man')

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
print '''#pragma once
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

  print '// Provider %s.' % name

  # Write GUID.
  assert guid.count('-') == 4
  assert guid[0] == '{'
  assert guid[-1] == '}'
  g1, g2, g3, g4, g5 = guid[1:-1].split('-')
  g5 = g4 + g5
  g5 = ', '.join(['0x' + g5[i:i+2].lower() for i in range(0, len(g5), 2)])
  guid = ', '.join(['0x' + s.lower() for s in [g1, g2, g3]] + ['{' + g5  + '}'])
  print 'EXTERN_C __declspec(selectany) const GUID %s =\n{%s};' % (symbol, guid)

  # Write context.
  print 'EXTERN_C __declspec(selectany) DECLSPEC_CACHEALIGN ULONG %sEnableBits[1];' % name
  keywords = '0x8000000000000000'  # FIXME
  print 'EXTERN_C __declspec(selectany) const ULONGLONG %sKeywords[1] = {%s};' % (name, keywords)
  level = '4'  # FIXME
  print 'EXTERN_C __declspec(selectany) const UCHAR %sLevels[1] = {%s};' % (name, level)
  print 'EXTERN_C __declspec(selectany) MCGEN_TRACE_CONTEXT %s_Context =' % symbol
  print'{0, 0, 0, 0, 0, 0, 0, 0, 1, %sEnableBits, %sKeywords, %sLevels};' % (name, name, name)
  print 'EXTERN_C __declspec(selectany) REGHANDLE %sHandle = (REGHANDLE)0;' % name

  # Write functions.
  # FIXME: hardcoding 0x00000001 here likely isn't right
  print '''#ifndef EventRegister%(name)s
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

      assert inType == 'win:AnsiString'  # TODO: more?
    template_map[tid] = (names, )
    arg_count = len(template)
    pattern = arg_count * 's'
    args = ',\n    '.join(['LPCSTR _Arg%d' % i for i in range(arg_count)])
    print '''#if !defined(Template_%s_def)
#define Template_%s_def
ETW_INLINE ULONG Template_%s(
    REGHANDLE RegHandle,
    PCEVENT_DESCRIPTOR Descriptor,
    %s) {
  EVENT_DATA_DESCRIPTOR EventData[%d];''' % (pattern, pattern, pattern, args, arg_count)

    for i in range(arg_count):
      print '  EventDataDescCreate(&EventData[%d],' % i
      print '                      _Arg%d ? _Arg%d : "NULL",' % (i, i)
      print '                      _Arg%d ? (ULONG)(strlen(_Arg%d) + 1) : (ULONG)sizeof("NULL"));' % (i, i)

    print '''  return EventWrite(RegHandle, Descriptor, %d, EventData);
}
#endif  // Template_%s_def''' % (arg_count, pattern)

  # Write events.
  events = provider.find('{http://schemas.microsoft.com/win/2004/08/events}events')
  assert events is not None
  for event in events:
    assert event.tag == '{http://schemas.microsoft.com/win/2004/08/events}event'
    assert 'symbol' in event.attrib
    symbol = event.attrib['symbol']
    assert 'template' in event.attrib
    template = event.attrib['template']
    assert template in template_map

    # XXX channel?
    print '#define EventEnabled%s() ((%sEnableBits[0] & 0x00000001) != 0)' % (symbol, provider_name)
    arg_names, = template_map[template]
    arg_names = [a.replace(' ', '_') for a in arg_names]
    print '#define EventWrite%s(%s) \\' % (symbol, ', '.join(arg_names))
    print '    EventEnabled%s() \\' % symbol
    print '        ? Template_%s(%s) \\' % ('s' * len(arg_names), ', '.join(arg_names))
    print '        : ERROR_SUCCESS'

print '''
#if defined(__cplusplus)
}  // extern "C"
#endif'''
