#!/usr/bin/env python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Update the list of languages in the network speech component extension.
"""

import os, sys, urllib, json, re

manifest_path = 'chrome/browser/resources/network_speech_synthesis/manifest.json'
if not os.access(manifest_path, os.F_OK):
  print 'Could not find:'
  print manifest_path
  print 'Please run this inside the Chromium src directory'
  sys.exit(-1)
manifest_json = open(manifest_path).read()
manifest = json.loads(manifest_json)

chrome_accept_lang_path = 'ui/base/l10n/l10n_util.cc'
chrome_codes = set()
for line in open(chrome_accept_lang_path).readlines():
  m = re.search('    "(\w+)".*//', line)
  if m:
    chrome_code = m.group(1)
    chrome_codes.add(chrome_code)
print 'Loaded %d Chrome accept languages' % len(chrome_codes)

# Note: run prodaccess
localeutil_path = '/home/build/google3/javascript/apps/docs/ui/localeutil.js'
code_to_const = {}
const_to_localname = {}
for line in open(localeutil_path).readlines():
  m = re.search("'(\w+)': docs.ui.LocaleUtil.(LOCALE_\w+),", line)
  if m:
    code = m.group(1).lower()
    const = m.group(2)
    code_to_const[code] = const
  m = re.search("docs.ui.LocaleUtil.(LOCALE_\w+) = '([^']+)'", line)
  if m:
    const = m.group(1)
    localname = m.group(2)
    const_to_localname[const] = localname
print 'Loaded %d localized language names from localeutil.js' % len(const_to_localname)

capabilities_url = 'http://www.google.com/speech-api/v1/capabilities?synthesis_voices=1&locale=en-us'
capabilities_json = urllib.urlopen(capabilities_url).read()
capabilities = json.loads(capabilities_json)
print 'Loaded %d supported voices from the speech-api server' % len(capabilities['voices'])

manifest_voices = []
voice_name_map = []
for voice in capabilities['voices']:
  if voice['name'].find('vocoded') >= 0 or voice['name'].find('#') >= 0:
    continue
  if voice['engine'] != 'Phonetic Arts':
    continue
  if voice['quality'] < 2:
    continue
  code = voice['language']
  if code == 'nb-no':
    code = 'no'
  code_components = code.split('-')
  if len(code_components) < 1 or len(code_components) > 2:
    continue
  code_components[0] = code_components[0].lower()
  if code_components[0] == 'yue' or code_components[0] == 'cmn':
    code_components[0] = 'zh'
  if len(code_components) == 2:
    code_components[1] = code_components[1].upper()
  chrome_code = '-'.join(code_components)
  localeutil_code = '_'.join(code_components).lower()

  if (chrome_code not in chrome_codes and
      chrome_code.split('-')[0] not in chrome_codes):
    print 'Unrecognized language: ' + chrome_code
    sys.exit()

  if localeutil_code in code_to_const:
    const = code_to_const[localeutil_code]
  else:
    localeutil_code = code.lower().split('-')[0]
    if localeutil_code in code_to_const:
      const = code_to_const[localeutil_code]
    else:
      print 'LANGUAGE CODE NOT HANDLED: ' + code
      sys.exit()

  localname = const_to_localname[const]

  manifest_voice = {}
  manifest_voice['event_types'] = [ 'start', 'end', 'error' ]
  manifest_voice['language'] = chrome_code
  manifest_voice['gender'] = voice['gender']
  manifest_voice['voice_name'] = 'Google ' + localname
  manifest_voice['remote'] = True
  manifest_voices.append(manifest_voice)

  lang_and_gender = chrome_code.lower() + '-' + voice['gender']
  voice_name_map.append([lang_and_gender, voice['name']])

manifest['tts_engine']['voices'] = sorted(manifest_voices, key=lambda k: k['language'])
manifest_json = json.dumps(manifest, indent=2, sort_keys=True)
manifest_json = manifest_json.replace(' \n', '\n')
outfp = open(manifest_path, 'w')
outfp.write(manifest_json)
outfp.close()
print 'Updated %s in-place with %d languages' % (manifest_path, len(manifest_voices))

tts_extension_path = 'chrome/browser/resources/network_speech_synthesis/tts_extension.js'
tts_extension_lines = open(tts_extension_path).readlines()
start_index = tts_extension_lines.index('  LANG_AND_GENDER_TO_VOICE_NAME_: {\n')
end_index = start_index + tts_extension_lines[start_index:].index('  },\n')
new_lines = []
for voice_name_map_entry in sorted(voice_name_map):
  new_lines.append("    '%s': '%s',\n" % (voice_name_map_entry[0], voice_name_map_entry[1]))
tts_extension_lines = tts_extension_lines[:start_index + 1] + new_lines + tts_extension_lines[end_index:]
outfp = open(tts_extension_path, 'w')
outfp.write(''.join(tts_extension_lines))
outfp.close()
print 'Updated %s in-place with %d languages' % (tts_extension_path, len(new_lines))
