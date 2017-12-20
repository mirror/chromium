import os
import sys
import shutil
import subprocess

def main():
  src_dir  = os.path.abspath(os.path.join(os.path.dirname(__file__), '../../..'))
  dest_dir = os.path.abspath(os.path.join(os.getcwd(), sys.argv[1]))
  os.chdir(src_dir)
  gn_files = [] 
  gn_files.extend([
    'build/build_config.h',
    'build/buildflag.h',
    'build/write_buildflag_header.py',
    'build/write_build_date_header.py',
    'testing/gtest/include/gtest/gtest_prod.h',
    'third_party/apple_apsl/malloc.h',
    'third_party/googletest/src/googletest/include/gtest/gtest_prod.h',
  ])
  
  for f in gn_files:
    write_if_changed(f, os.path.join(dest_dir, f))



def write_if_changed(src, dest):
  dest_dir = os.path.dirname(dest)
  if not os.path.exists(dest_dir):
    os.makedirs(dest_dir)
  with open(src) as src_fp:
    src_contents = src_fp.read()
  if os.path.exists(dest):
    with open(dest) as dest_fp:
      dest_contents = dest_fp.read()
    if dest_contents != src_contents:
      with open(dest, 'w') as dest_fp:
        dest_fp.write(src_contents)
  else:
    with open(dest, 'w') as dest_fp:
      dest_fp.write(src_contents)


if __name__ == '__main__':
  main()
