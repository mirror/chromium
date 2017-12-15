$core = 'third_party/WebKit/Source/core';

@files = split /\s/, <<EOM;
core/dom/ClassicPendingScript.cpp
core/dom/ClassicPendingScript.h
core/dom/ClassicScript.cpp
core/dom/ClassicScript.h
core/dom/DocumentModulatorImpl.cpp
core/dom/DocumentModulatorImpl.h
core/dom/DocumentWriteIntervention.cpp
core/dom/DocumentWriteIntervention.h
core/dom/DynamicModuleResolver.cpp
core/dom/DynamicModuleResolver.h
core/dom/DynamicModuleResolverTest.cpp
core/dom/IgnoreDestructiveWriteCountIncrementer.h
core/dom/MockScriptElementBase.h
core/dom/Modulator.cpp
core/dom/Modulator.h
core/dom/ModulatorImplBase.cpp
core/dom/ModulatorImplBase.h
core/dom/ModulatorTest.cpp
core/dom/ModuleImportMeta.h
core/dom/ModuleMap.cpp
core/dom/ModuleMap.h
core/dom/ModuleMapTest.cpp
core/dom/ModulePendingScript.cpp
core/dom/ModulePendingScript.h
core/dom/ModuleScript.cpp
core/dom/ModuleScript.h
core/dom/PendingScript.cpp
core/dom/PendingScript.h
core/dom/ScriptElementBase.cpp
core/dom/ScriptElementBase.h
core/dom/Script.h
core/dom/ScriptLoader.cpp
core/dom/ScriptLoader.h
core/dom/ScriptModuleResolver.h
core/dom/ScriptModuleResolverImpl.cpp
core/dom/ScriptModuleResolverImpl.h
core/dom/ScriptModuleResolverImplTest.cpp
core/dom/ScriptRunner.cpp
core/dom/ScriptRunner.h
core/dom/ScriptRunnerTest.cpp
core/dom/WorkletModulatorImpl.cpp
core/dom/WorkletModulatorImpl.h
core/dom/WorkerModulatorImpl.cpp
core/dom/WorkerModulatorImpl.h
core/html/parser/HTMLParserScriptRunner.h
core/html/parser/HTMLParserScriptRunner.cpp
core/html/parser/HTMLParserScriptRunnerHost.h
core/xml/parser/XMLParserScriptRunner.h
core/xml/parser/XMLParserScriptRunner.cpp
core/xml/parser/XMLParserScriptRunnerHost.h
EOM

%newfiles = ();
for $file (@files) {
  $oldfile = $file;
  $newfile = $file;
  $newfile =~ s/^.*\///g;
  $newrelpath{$oldfile} = $newfile;
  $newfile = "core/script/$newfile";
  $newfiles{$oldfile} = $newfile;
}

if ($ARGV[0] eq 'REPLACE') {

# find third_party/WebKit/Source/core/ -type f | xargs perl core_script.pl REPLACE

for $file (@ARGV) {
open(IN, $file);
$matched = 0;
$result = '';
while(<IN>) {

  for $oldfile (@files) {
    if ($_ =~ s/$oldfile/$newfiles{$oldfile}/) {
      $matched = 1;
    }
  }
  $result .= $_;
}
close(IN);

if ($matched) {
  open(IN, ">$file");
  print IN $result;
  close(IN);
  print STDERR "Modified $file\n";
}

}

exit;

} else {

system("rm -r third_party/WebKit/Source/core/script/");
mkdir("$core/script");

  @new_BUILD_sources = ();

  for $oldfile (@files) {
    $newfile = $newfiles{$oldfile};

    # Move actual file.
    system("git mv third_party/WebKit/Source/$oldfile third_party/WebKit/Source/$newfile");

    # Amend BUILD.gn.
    $curdir = '';
    $relpath = $oldfile;
    $newrelpath = $newfile;
    while ($relpath =~ /^([^\/]+)\/(.*)$/) {
      $curdir .= $1 . '/';
      $relpath = $2;
      if ($newrelpath =~ /^$curdir(.*)$/) {
        $newrelpath = $1;
      } else {
        $newrelpath = '';
      }

      open(IN, "third_party/WebKit/Source/${curdir}BUILD.gn");
      $found = 0;
      $out_BUILD_gn = '';
      while(<IN>) {
        if ($newrelpath ne '') {
          if ($_ =~ s/$relpath/$newrelpath/g) {
            $found = 1;
          }
        } else {
          if ($_ =~ /$relpath/) {
            if (!$found) {
              push(@new_BUILD_sources, $newrelpath{$oldfile});
            }
            $found = 1;
            next; # Remove.
          }
        }
        $out_BUILD_gn .= $_;
      }
      close(IN);

      if ($found) {
        open(IN, ">third_party/WebKit/Source/${curdir}BUILD.gn");
        print IN $out_BUILD_gn;
        close(IN);
        last;
      }
    }
  }

open(BUILD, ">$core/script/BUILD.gn");

print BUILD <<EOM;
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import("//third_party/WebKit/Source/core/core.gni")

blink_core_sources("script") {
  split_count = 5

  sources = [
EOM

for $file (@new_BUILD_sources) {
  print BUILD <<EOM;
    "$file",
EOM
}

print BUILD <<EOM;
  ]

  jumbo_excluded_sources = [ "Modulator.cpp" ]  # https://crbug.com/716395

  configs += [
    # TODO(jschuh): crbug.com/167187 fix size_t to int truncations.
    "//build/config/compiler:no_size_t_to_int_warning",
  ]
}
EOM

close(BUILD);
# system("git add $core/script/");
}
