# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Framework for stripping unnecessary elements from resource files"""

from os import path
import re
import subprocess
import sys

__js_minifier = None

def SetJsMinifier(minifier):
  global __js_minifier
  __js_minifier = minifier.split()

def MinifyJS_JSON(lines, filename):
  # Try to search for Copyright comments for including them again.
  # If we have few copyright info, after minification all of them
  # will be included in the beginning.
  copyright = re.findall(re.compile('[\s\n\(](/\*.+?\*/)', re.DOTALL),
    ' ' + lines)
  # It doesn't handle comments with text just after //
  # (match \s to avoid matching such strings like "chrome://url")
  copyright2 = re.findall(r'//\s.*?(?=Copyright|license|LICENSE).+?\n',
    ' ' + lines)

  file_type = path.splitext(filename)[1]
  if file_type == '.json':
    # Convert JSON into JS
    lines = "x=" + lines

  # Remove /* */ comments without Copyright text inside
  # because minifier doesn't remove them inside strings.
  lines = re.sub(re.compile(r'([\s\n\(])/\*((?!Copyright).)+?\*/', re.DOTALL),
    r'\1', ' ' + lines)

  p = subprocess.Popen(
      __js_minifier,
      stdin=subprocess.PIPE,
      stdout=subprocess.PIPE,
      stderr=subprocess.PIPE)
  (stdout, stderr) = p.communicate(lines)
  if p.returncode != 0:
    print 'Minification failed for %s' % filename
    print stderr
    sys.exit(p.returncode)

  if file_type == '.json':
    # Convert JS into JSON by removing prefix "x=" and suffix added by minifier
    lines = stdout[2:][:-3]
  else:
    lines = stdout

  # Remove new lines
  lines = re.sub(r'\n', '', lines)

  copyrightlist = ''
  for d in copyright:
    if 'Copyright' in d and not '//' in d:
      copyrightlist = copyrightlist + d
  copyright = copyrightlist

  copyright2 = ''.join(copyright2)
  # Replace repeating Chromium copyrights with one without date
  copyright2 = re.sub(r'\n// // Copyright', '\n// Copyright', copyright2)
  copyright2 = re.sub(r'// Copyright.+? The Chromium Authors',
    '// Copyright The Chromium Authors', copyright2)
  copyright2 = re.sub(r'(^// Copyright The Chromium Authors. ' +
    'All rights reserved.\n' +
    '// Use of this source code is governed by a BSD.style license ' +
    'that can be\n// found in the LICENSE file.\n)+',
    r'\1', copyright2, flags=re.MULTILINE)

  return copyright + copyright2 + lines

def MinifyCSS(lines):
  # Remove /* */ comments without Copyright text inside
  lines = re.sub(re.compile(r'([\s\n\(])/\*((?!Copyright).)+?\*/', re.DOTALL),
    r'\1', ' ' + lines)
  # Remove initial whitespace
  lines = re.sub(r'^\s+', '', lines, flags=re.MULTILINE)
  # Remove trailing whitespace
  lines = re.sub(r'\s+$', '', lines, flags=re.MULTILINE)
  # Remove spaces around { } : , ;
  lines = re.sub(r'\s*([{}:,;])\s*', r'\1', lines)
  # Remove new lines
  lines = re.sub(r'\n', '', lines)

  return lines

def MinifyCSSBlockInsideHTML_SVG(match):
  # group(1) is opening <style> tag with parameters
  # group(2) is content inside
  # group(3) is closing </style> tag
  return match.group(1) + MinifyCSS(match.group(2)) + match.group(3)

def MinifyJSBlockInsideHTML(match):
  # group(1) is opening <script> tag with parameters
  # group(2) is content inside
  # group(3) is closing </script> tag
  return match.group(1) + MinifyJS_JSON(match.group(2),
    'JS inside HTML') + match.group(3)

def MinifyHTML_SVG(lines, file_type):
  # Remove initial whitespace
  lines = re.sub(r'^\s+', '', lines, flags=re.MULTILINE)

  if file_type == '.htm' or file_type == '.html':
    # Format included JS scripts
    lines = re.sub(re.compile(r'(<script[^>]*>)((?!<).+?)(</script>)',
      re.DOTALL), MinifyJSBlockInsideHTML, lines)

  # Format included CSS styles
  lines = re.sub(re.compile(r'(<style[^>]*>)((?!<).+?)(</style>)', re.DOTALL),
    MinifyCSSBlockInsideHTML_SVG, lines)
  # Remove <!-- --> comments without Copyright text inside
  lines = re.sub(re.compile(r'<\!--((?!Copyright).)+?-->', re.DOTALL), '',
    lines)
  # Save some space inside <!-- --> comments
  lines = re.sub(re.compile(r'(<\!--)[\n\s](.*)[\n\s](-->)', re.DOTALL),
    r'\1\2\3', lines)
  # Remove trailing whitespace
  lines = re.sub(r'\s+$', '', lines, flags=re.MULTILINE)
  # Remove new lines before and after tags
  lines = re.sub(r'>\n*', '>', lines)
  lines = re.sub(r'\n*<', '<', lines)
  # Remove empty lines
  lines = re.sub(r'\n+', '\n', lines)

  if file_type == '.htm' or file_type == '.html':
    # Remove unnecessary html sequences
    lines = re.sub(r'<style type="text/css">', '<style>', lines,
      flags=re.IGNORECASE)
    lines = re.sub(r'<!doctype html>', '', lines, flags=re.IGNORECASE)
    lines = re.sub(r'</style><style>', '', lines)
    lines = re.sub(r'</script><script>', '', lines)

  return lines

def Minify(source, filename):
  file_type = path.splitext(filename)[1]

  if file_type == '.json':
    return MinifyJS_JSON(source, filename)
  if file_type == '.css':
    return MinifyCSS(source)
  if file_type == '.htm' or file_type == '.html' or file_type == '.svg':
    return MinifyHTML_SVG(source, file_type)
  if not file_type == '.js' or not __js_minifier:
    return source
  return MinifyJS_JSON(source, filename)
