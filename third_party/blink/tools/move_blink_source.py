#!/usr/bin/env vpython
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import logging
import os
import re
import sys

# Without abspath(), PathFinder can't find chromium_base correctly.
sys.path.append(os.path.abspath(
    os.path.join(os.path.dirname(__file__), '..', '..', '..',
                 'third_party', 'WebKit', 'Tools', 'Scripts')))
from plan_blink_move import plan_blink_move
from webkitpy.common.path_finder import PathFinder
from webkitpy.common.system.filesystem import FileSystem

_log = logging.getLogger('move_blink_source')


class FileType(object):
    NONE = 0
    BUILD = 1
    BLINK_BUILD = 2
    OWNERS = 3
    DEPS = 4

    def __init__(self):
        return

    @staticmethod
    def detect(path):
        _, basename = os.path.split(path)
        if basename == 'DEPS':
            return FileType.DEPS
        if basename == 'OWNERS':
            return FileType.OWNERS
        if basename.endswith(('.gn', '.gni')):
            path = path.replace('\\', '/')
            if path.find('third_party/WebKit') != -1 or path.find('third_party/blink') != -1:
                return FileType.BLINK_BUILD
            if path.find('third_party') != -1:
                return FileType.NONE
            return FileType.BUILD
        return FileType.NONE


class MoveBlinkSource(object):

    def __init__(self, fs, options):
        self._fs = fs
        self._options = options
        _log.debug(options)
        # The following fields are initialized in _create_basename_maps.
        self._basename_map = None
        self._basename_re = None
        self._idl_generated_impl_headers = None

    def _create_basename_maps(self, file_pairs):
        basename_map = {}
        pattern = '\\b('
        idl_headers = set()
        for source, dest in file_pairs:
            _, source_base = self._fs.split(source)
            _, dest_base = self._fs.split(dest)
            if source_base == dest_base:
                continue
            basename_map[source_base] = dest_base
            pattern += re.escape(source_base) + '|'
            # IDL sometimes generates implementation files as well as
            # binding files. We'd like to update #includes for such files.
            if source_base.endswith('.idl'):
                source_header = source_base.replace('.idl', '.h')
                basename_map[source_header] = dest_base.replace('.idl', '.h')
                pattern += re.escape(source_header) + '|'
                idl_headers.add(source_header)
        _log.info('Rename %d files for snake_case', len(basename_map))
        self._basename_map = basename_map
        self._basename_re = re.compile(pattern[0:len(pattern) - 1] + ')"')
        self._idl_generated_impl_headers = idl_headers

    @staticmethod
    def _filter_file(fs, dirname, basename):
        return FileType.detect(fs.join(dirname, basename)) != FileType.NONE

    def _update_build(self, content):
        content = content.replace('//third_party/WebKit/Source', '//third_party/blink/renderer')
        content = content.replace('//third_party/WebKit/common', '//third_party/blink/common')
        content = content.replace('//third_party/WebKit/public', '//third_party/blink/renderer/public')
        return content

    def _update_blink_build(self, content):
        content = content.replace('//third_party/WebKit/Source', '//third_party/blink/renderer')
        content = content.replace('//third_party/WebKit/common', '//third_party/blink/common')
        content = content.replace('//third_party/WebKit/public', '//third_party/blink/renderer/public')
        return self._update_basename(content)

    def _update_owners(self, content):
        content = content.replace('//third_party/WebKit/Source', '//third_party/blink/renderer')
        content = content.replace('//third_party/WebKit/common', '//third_party/blink/common')
        content = content.replace('//third_party/WebKit/public', '//third_party/blink/renderer/public')
        return content

    def _update_deps(self, content):
        original_content = content
        content = content.replace('third_party/WebKit/Source', 'third_party/blink/renderer')
        content = content.replace('third_party/WebKit/common', 'third_party/blink/common')
        content = content.replace('third_party/WebKit/public', 'third_party/blink/renderer/public')
        content = content.replace('third_party/WebKit', 'third_party/blink')
        if original_content == content:
            return content
        return self._update_basename(content)

    def _update_basename(self, content):
        content = self._basename_re.sub(lambda match: self._basename_map[match.group(1)], content)
        return content

    @staticmethod
    def _append_unless_upper_dir_exists(dirs, new_dir):
        for i in range(0, len(dirs)):
            if new_dir.startswith(dirs[i]):
                return
            if dirs[i].startswith(new_dir):
                dirs[i] = new_dir
                return
        dirs.append(new_dir)

    def _update_file_content(self, repo_root):
        _log.info('Find *.gn, DEPS, and OWNERS ...')
        files = self._fs.files_under(
            repo_root, dirs_to_skip=['.git', 'out'], file_filter=self._filter_file)
        _log.info('Scan contents of %d files ...', len(files))
        updated_deps_dirs = []
        for file_path in files:
            file_type = FileType.detect(file_path)
            original_content = self._fs.read_text_file(file_path)
            content = original_content
            if file_type == FileType.BUILD:
                content = self._update_build(content)
            elif file_type == FileType.BLINK_BUILD:
                content = self._update_blink_build(content)
            elif file_type == FileType.OWNERS:
                content = self._update_owners(content)
            elif file_type == FileType.DEPS:
                content = self._update_deps(content)

            if original_content == content:
                continue
            if self._options.run:
                self._fs.write_text_file(file_path, content)
            if file_type == FileType.DEPS and self._fs.dirname(file_path) != repo_root:
                self._append_unless_upper_dir_exists(updated_deps_dirs, self._fs.dirname(file_path))
            _log.info('Updated %s', file_path)
        return updated_deps_dirs

    def _update_cpp_includes_in_directories(self, dirs):
        for dirname in dirs:
            _log.info('Processing #include in %s ...', dirname)
            files = self._fs.files_under(
                dirname, file_filter=lambda fs, _, basename: basename.endswith(('.h', '.cc', '.cpp', '.mm')))
            for file_path in files:
                original_content = self._fs.read_text_file(file_path)
                content = self._update_cpp_includes(original_content)
                if original_content == content:
                    continue
                if self._options.run:
                    self._fs.write_text_file(file_path, content)
                _log.info('Updated %s', file_path)

    def _replace_include_path(self, match):
        path = match.group(1)

        is_generated_file = False
        for basename in self._basename_map.keys():
            if path.endswith('/' + basename):
                path = path[:len(path) - len(basename)] + self._basename_map[basename]
                if basename in self._idl_generated_impl_headers:
                    is_generated_file = True
                break
        if path.startswith('third_party/WebKit'):
            path = path.replace('third_party/WebKit/Source', 'third_party/blink/renderer')
            path = path.replace('third_party/WebKit/common', 'third_party/blink/common')
            path = path.replace('third_party/WebKit/public', 'third_party/blink/renderer/public')
        elif path != match.group(1) and not is_generated_file:
            # Don't add third_party/blink for unknown names. It may be a
            # generated file in gen/blink/.
            path = 'third_party/blink/renderer/' + path

        return '#include "%s"' % path

    def _update_cpp_includes(self, content):
        pattern = re.compile('#include\\s+"((bindings|core|modules|platform|public|' +
                             'third_party/WebKit/(Source|common|public))/[\\w/.]+)"')
        return pattern.sub(self._replace_include_path, content)

    def _move_files(self, file_pairs):
        # TODO(tkent): Implement.
        return file_pairs

    def main(self):
        finder = PathFinder(self._fs)

        _log.info('Planning renaming ...')
        file_pairs = plan_blink_move(self._fs, [])
        _log.info('Will move %d files', len(file_pairs))

        repo_root = finder.chromium_base()
        self._create_basename_maps(file_pairs)
        dirs = self._update_file_content(repo_root)

        # Updates #includes in files in directories with updated DEPS +
        # third_party/WebKit/{Source,common,public}.
        self._append_unless_upper_dir_exists(dirs, self._fs.join(repo_root, 'third_party', 'WebKit', 'Source'))
        self._append_unless_upper_dir_exists(dirs, self._fs.join(repo_root, 'third_party', 'WebKit', 'common'))
        self._append_unless_upper_dir_exists(dirs, self._fs.join(repo_root, 'third_party', 'WebKit', 'public'))
        self._update_cpp_includes_in_directories(dirs)

        self._move_files(file_pairs)


def main():
    logging.basicConfig(level=logging.DEBUG)
    parser = argparse.ArgumentParser(description='Blink source mover')
    parser.add_argument('--run', dest='run', action='store_true')
    options = parser.parse_args()
    MoveBlinkSource(FileSystem(), options).main()


if __name__ == '__main__':
    main()
