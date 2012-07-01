#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import buildbot_common
import make_rules
import optparse
import os
import sys

from make_rules import BuildToolDict, BUILD_RULES

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SDK_SRC_DIR = os.path.dirname(SCRIPT_DIR)
SDK_EXAMPLE_DIR = os.path.join(SDK_SRC_DIR, 'examples')
SDK_DIR = os.path.dirname(SDK_SRC_DIR)
SRC_DIR = os.path.dirname(SDK_DIR)
OUT_DIR = os.path.join(SRC_DIR, 'out')
PPAPI_DIR = os.path.join(SRC_DIR, 'ppapi')


def ErrorExit(text):
  sys.stderr.write(text + '\n')
  sys.exit(1)


def Replace(text, replacements):
  for key in replacements:
    text = text.replace(key, replacements[key])
  return text


def WriteReplaced(srcpath, dstpath, replacements):
  text = open(srcpath, 'rb').read()
  text = Replace(text, replacements)
  open(dstpath, 'wb').write(text)


def SetVar(varname, values):
  if not values:
    return varname + ':=\n'

  line = varname + ':='
  out = ''
  for value in values:
    if len(line) + len(value) > 78:
      out += line[:-1] + '\n'
      line = '%s+=%s ' % (varname, value)
    else:
      line += value + ' '

  if line:
    out += line[:-1] + '\n'
  return out


def GenerateCopyList(desc):
  sources = []

  # Add sources for each target
  for target in desc['TARGETS']:
    sources.extend(target['SOURCES'])

  # And HTML and data files
  sources.extend(desc.get('DATA', []))

  if desc['DEST'] == 'examples':
    sources.append('common.js')

  return sources


def GetSourcesDict(sources):
  source_map = {}
  for key in ['.c', '.cc']:
    source_list = [fname for fname in sources if fname.endswith(key)]
    if source_list:
      source_map[key] = source_list
    else:
      source_map[key] = []
  return source_map
    

def GetPlatforms(plat_list, plat_filter):
  platforms = []
  for plat in plat_list:
    if plat in plat_filter:
      platforms.append(plat)
  return platforms


def GenerateToolDefaults(desc, platforms):
  defaults = ''
  for platform in platforms:
    defaults += BUILD_RULES[platform]['DEFS']
  return defaults  


def GenerateSettings(desc, tools):
  settings = SetVar('VALID_TOOLCHAINS', tools)
  settings+= 'TOOLCHAIN?=%s\n\n' % tools[0]
  for target in desc['TARGETS']:
    name = target['NAME']
    macro = name.upper()
    srcs = GetSourcesDict(target['SOURCES'])

    if srcs['.c']:
      flags = target.get('CCFLAGS', ['$(NACL_CCFLAGS)'])
      settings += SetVar(macro + '_CC', srcs['.c'])
      settings += SetVar(macro + '_CCFLAGS', flags)

    if srcs['.cc']:
      flags = target.get('CXXFLAGS', ['$(NACL_CXXFLAGS)'])
      settings += SetVar(macro + '_CXX', srcs['.cc'])
      settings += SetVar(macro + '_CXXFLAGS', flags)

    flags = target.get('LDFLAGS', ['$(NACL_LDFLAGS)'])
    settings += SetVar(macro + '_LDFLAGS', flags)
  return settings


def GenerateTargets(desc, tools):
  prereqs = desc.get('PREREQ', [])
  target_def = ''
 
  if prereqs:
    target_def = '.PHONY : PREREQS\nPREREQS:\n'
    for prereq in prereqs:
      target_def += '\t+$(MAKE) -C %s all\n' % prereq
    target_def+= '\nall : PREREQS ' + (' '.join(targets))
  else:
    target_def += 'all : ALL_TARGETS' 


def GenerateNEXE(target, tool):
  rules = ''
  name = target['NAME']
  srcs = GetSourcesDict(target['SOURCES'])
  for arch in BUILD_RULES[tool]['ARCHES']:
    object_sets = []
    if srcs['.c']:
      replace = BuildToolDict(tool, name, arch, 'c')
      compile_rule = BUILD_RULES[tool]['CC']
      rules += Replace(compile_rule, replace)
      object_sets.append('$(%s)' % replace['<OBJS>'])

    if srcs['.cc']:
      replace = BuildToolDict(tool, name, arch, 'cc')
      compile_rule = BUILD_RULES[tool]['CXX']
      rules += Replace(compile_rule, replace)
      object_sets.append('$(%s)' % replace['<OBJS>'])

    objs = ' '.join(object_sets)
    link_rule = BUILD_RULES[tool][ target['TYPE'] ]
    replace = BuildToolDict(tool, name, arch, 'nexe', OBJS=objs)
    rules += Replace(link_rule, replace) 
  return rules


def GenerateRules(desc, tools):
  rules = ''
  for tc in tools:
    rules += '\n#\n# Rules for %s toolchain\n#\n%s:\n\t$(MKDIR) %s\n' % (
        tc, tc, tc)
    nexe = None
    for target in desc['TARGETS']:
      name = target['NAME']
      srcs = target['SOURCES']
      rules += GenerateNEXE(target, tc)
      if target['TYPE'] == 'main':
    	main = target

    if main:
      rules += GenerateNMF(main, tc)
  return rules


def GenerateNMF(target, tool):
  nmf_rule = BUILD_RULES[tool]['nmf']
  replace = BuildToolDict(tool, target['NAME'])
  return Replace(nmf_rule, replace)


def GenerateReplacements(desc, tools):
  # Generate target settings
  plats = GetPlatforms(desc['TOOLS'], tools)
  
  settings = GenerateSettings(desc, tools)
  tool_def = GenerateToolDefaults(desc, tools)
  rules = GenerateRules(desc, tools)
  prelaunch = desc.get('LAUNCH', '')
  prerun = desc.get('PRE', '')
  postlaunch = desc.get('POST', '')

  targets = []
  target_def = ''
  for tool in tools:
    for target in desc['TARGETS']:
      if target['TYPE'] == 'main':
	targets.append('%s/%s.nmf' % (tool, target['NAME']))  
  target_def = 'all: ' + ' '.join(targets)        

  return {
      '__PROJECT_SETTINGS__' : settings,
      '__PROJECT_TARGETS__' : target_def,
      '__PROJECT_TOOLS__' : tool_def,
      '__PROJECT_RULES__' : rules,
      '__PROJECT_PRELAUNCH__' : prelaunch,
      '__PROJECT_PRERUN__' : prerun,
      '__PROJECT_POSTLAUNCH__' : postlaunch
  }



# 'KEY' : ( <TYPE>, [Accepted Values], <Required?>)
DSC_FORMAT = {
    'TOOLS' : (list, ['newlib', 'glibc', 'pnacl'], True),
    'PREREQ' : (list, '', False),
    'TARGETS' : (list, {
        'NAME': (str, '', True),
        'TYPE': (str, ['main', 'nexe', 'lib', 'so'], True),
        'SOURCES': (list, '', True),
        'CCFLAGS': (list, '', False),
        'CXXFLAGS': (list, '', False),
        'LDFLAGS': (list, '', False),
        'LIBS' : (list, '', False)
    }, True),
    'SEARCH': (list, '', False),
    'POST': (str, '', False),
    'PRE': (str, '', False),
    'DEST': (str, ['examples', 'src'], True),
    'NAME': (str, '', False),
    'DATA': (list, '', False),
    'TITLE': (str, '', False),
    'DESC': (str, '', False),
    'INFO': (str, '', False)
}


def ErrorMsgFunc(text):
  sys.stderr.write(text + '\n')


def ValidateFormat(src, format, ErrorMsg=ErrorMsgFunc):
  failed = False

  # Verify all required keys are there
  for key in format:
    (exp_type, exp_value, required) = format[key]
    if required and key not in src:
      ErrorMsg('Missing required key %s.' % key)
      failed = True

  # For each provided key, verify it's valid
  for key in src:
    # Verify the key is known
    if key not in format:
      ErrorMsg('Unexpected key %s.' % key)
      failed = True
      continue

    exp_type, exp_value, required = format[key]
    value = src[key] 

    # Verify the key is of the expected type
    if exp_type != type(value):
      ErrorMsg('Key %s expects %s not %s.' % (
          key, exp_type.__name__.upper(), type(value).__name__.upper()))
      failed = True
      continue

    # Verify the value is non-empty if required
    if required and not value:
      ErrorMsg('Expected non-empty value for %s.' % key)
      failed = True
      continue

    # If it's a string and there are expected values, make sure it matches
    if exp_type is str:
      if type(exp_value) is list and exp_value:
        if value not in exp_value:
          ErrorMsg('Value %s not expected for %s.' % (value, key))
          failed = True
      continue

    # if it's a list, then we need to validate the values
    if exp_type is list:
      # If we expect a dictionary, then call this recursively
      if type(exp_value) is dict:
        for val in value:
          if not ValidateFormat(val, exp_value, ErrorMsg):
            failed = True
        continue
      # If we expect a list of strings
      if type(exp_value) is str:
        for val in value:
          if type(val) is not str:
            ErrorMsg('Value %s in %s is not a string.' % (val, key))
            failed = True
        continue
      # if we expect a particular string
      if type(exp_value) is list:
        for val in value:
          if val not in exp_value:
            ErrorMsg('Value %s not expected in %s.' % (val, key))
            failed = True
        continue

    # If we got this far, it's an unexpected type
    ErrorMsg('Unexpected type %s for key %s.' % (str(type(src[key])), key))
    continue
  return not failed


def AddMakeBat(pepperdir, makepath):
  """Create a simple batch file to execute Make.

  Creates a simple batch file named make.bat for the Windows platform at the
  given path, pointing to the Make executable in the SDK."""

  makepath = os.path.abspath(makepath)
  if not makepath.startswith(pepperdir):
    buildbot_common.ErrorExit('Make.bat not relative to Pepper directory: ' +
                              makepath)

  makeexe = os.path.abspath(os.path.join(pepperdir, 'tools'))
  relpath = os.path.relpath(makeexe, makepath)

  fp = open(os.path.join(makepath, 'make.bat'), 'wb')
  outpath = os.path.join(relpath, 'make.exe')

  # Since make.bat is only used by Windows, for Windows path style
  outpath = outpath.replace(os.path.sep, '\\')
  fp.write('@%s %%*\n' % outpath)
  fp.close()


def FindFile(name, srcroot, srcdirs):
  checks = []
  for srcdir in srcdirs:
    srcfile = os.path.join(srcroot, srcdir, name)
    srcfile = os.path.abspath(srcfile)
    if os.path.exists(srcfile):
      return srcfile
    else:
      checks.append(srcfile)

  ErrorMsgFunc('%s not found in:\n\t%s' % (name, '\n\t'.join(checks)))
  return None


def IsNexe(desc):
  for target in desc['TARGETS']:
    if target['TYPE'] == 'main':
      return True
  return False


def ProcessHTML(srcroot, dstroot, desc, toolchains):
  name = desc['NAME']
  outdir = os.path.join(dstroot, desc['DEST'], name)
  
  srcfile = os.path.join(srcroot, 'index.html')
  tools = GetPlatforms(toolchains, desc['TOOLS'])
  for tool in tools:
    dstfile = os.path.join(outdir, 'index_%s.html' % tool);
    print 'Writting from %s to %s' % (srcfile, dstfile)
    replace = {
      '<NAME>': name,
      '<TITLE>': desc['TITLE'],
      '<tc>': tool
    }
    WriteReplaced(srcfile, dstfile, replace)

  replace['<tc>'] = tools[0]
  srcfile = os.path.join(SDK_SRC_DIR, 'build_tools', 'redirect.html')
  dstfile = os.path.join(outdir, 'index.html')
  WriteReplaced(srcfile, dstfile, replace)
    

def LoadProject(filename, toolchains):
  """Generate a Master Makefile that builds all examples. 

  Load a project desciption file, verifying it conforms and checking
  if it matches the set of requested toolchains.  Return None if the
  project is filtered out."""

  print '\n\nProcessing %s...' % filename
  # Default src directory is the directory the description was found in 
  desc = open(filename, 'r').read()
  desc = eval(desc, {}, {})

  # Verify the format of this file
  if not ValidateFormat(desc, DSC_FORMAT):
    ErrorExit('Failed to validate: ' + filename)

  # Check if we are actually interested in this example
  match = False
  for toolchain in toolchains:
    if toolchain in desc['TOOLS']:
      match = True
      break
  if not match:
    return None
  return desc


def ProcessProject(srcroot, dstroot, desc, toolchains):
  name = desc['NAME']
  out_dir = os.path.join(dstroot, desc['DEST'], name)
  buildbot_common.MakeDir(out_dir)
  srcdirs = desc.get('SEARCH', ['.', '..'])

  # Copy sources to example directory
  sources = GenerateCopyList(desc)
  for src_name in sources:
    src_file = FindFile(src_name, srcroot, srcdirs)
    if not src_file:
      ErrorMsgFunc('Failed to find: ' + src_name)
      return (None, None)
    dst_file = os.path.join(out_dir, src_name)
    buildbot_common.CopyFile(src_file, dst_file)

  if IsNexe(desc):
    template=os.path.join(SCRIPT_DIR, 'template.mk')
  else:
    template=os.path.join(SCRIPT_DIR, 'library.mk')

  tools = []
  for tool in desc['TOOLS']:
    if tool in toolchains:
      tools.append(tool)

  # Add Makefile and make.bat
  repdict = GenerateReplacements(desc, tools)
  make_path = os.path.join(out_dir, 'Makefile')
  WriteReplaced(template, make_path, repdict)
  outdir = os.path.dirname(os.path.abspath(make_path))
  pepperdir = os.path.dirname(os.path.dirname(outdir))
  AddMakeBat(pepperdir, outdir)
  return (name, desc['DEST'])


def GenerateExamplesMakefile(in_path, out_path, examples):
  """Generate a Master Makefile that builds all examples. """
  replace = {  '__PROJECT_LIST__' : SetVar('PROJECTS', examples) }
  WriteReplaced(in_path, out_path, replace)

  outdir = os.path.dirname(os.path.abspath(out_path))
  pepperdir = os.path.dirname(outdir)
  AddMakeBat(pepperdir, outdir)
  

def main(argv):
  parser = optparse.OptionParser()
  parser.add_option('--dstroot', help='Set root for destination.',
      dest='dstroot', default=OUT_DIR)
  parser.add_option('--master', help='Create master Makefile.',
      action='store_true', dest='master', default=False)
  parser.add_option('--newlib', help='Create newlib examples.',
      action='store_true', dest='newlib', default=False)
  parser.add_option('--glibc', help='Create glibc examples.',
      action='store_true', dest='glibc', default=False)
  parser.add_option('--pnacl', help='Create pnacl examples.',
      action='store_true', dest='pnacl', default=False)
  parser.add_option('--host', help='Create host examples.',
      action='store_true', dest='host', default=False)

  toolchains = []
  options, args = parser.parse_args(argv)
  if options.newlib:
    toolchains.append('newlib')
  if options.glibc:
    toolchains.append('glibc')
  if options.pnacl:
    toolchains.append('pnacl')
  if options.host:
    toolchains.append('host')

  # By default support newlib and glibc
  if not toolchains:
    toolchains = ['newlib', 'glibc']
    print 'Using default toolchains: ' + ' '.join(toolchains)

  examples = []
  for filename in args:
    desc = LoadProject(filename, toolchains)
    if not desc:
      print 'Skipping %s, not in [%s].' % (filename, ', '.join(toolchains))
      continue

    srcroot = os.path.dirname(os.path.abspath(filename))
    if not ProcessProject(srcroot, options.dstroot, desc, toolchains):
      ErrorExit('\n*** Failed to process project: %s ***' % filename)

    # if this is an example, do the HTML files as well
    if desc['DEST'] == 'examples':
      examples.append(desc['NAME'])
      ProcessHTML(srcroot, options.dstroot, desc, toolchains)


  if options.master:
    master_in = os.path.join(SDK_EXAMPLE_DIR, 'Makefile')
    master_out = os.path.join(options.dstroot, 'examples', 'Makefile')
    GenerateExamplesMakefile(master_in, master_out, examples)
  return 0


if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
