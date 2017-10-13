# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json5_generator


class FieldAliasHelper(object):
    def __init__(self, file_path):
        loaded_file = json5_generator.Json5File.load_from_files([file_path])
        self._field_aliases = dict([(alias["name"], alias)
                                    for alias in loaded_file.name_dictionaries])

    def expand_field_alias(self, property_):
        if property_['field_template'] in self._field_aliases:
            alias_template = property_['field_template']
            for field in self._field_aliases[alias_template]:
                if field == 'name':
                    continue
                property_[field] = self._field_aliases[alias_template][field]
