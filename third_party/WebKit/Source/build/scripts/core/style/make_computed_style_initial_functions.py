#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import sys
sys.path.append(os.path.join(os.path.dirname(__file__), '../..'))

import json5_generator
import template_expander

from core.css import css_properties


class ComputedStyleInitialFunctionsWriter(css_properties.CSSProperties):
    def __init__(self, json5_file_paths):
        super(ComputedStyleInitialFunctionsWriter, self).__init__(
            json5_file_paths)

        self._includes = set()

        for property_ in self.properties().values():
            self._includes.update(property_['include_paths'])

        self._outputs = {
            'ComputedStyleInitialFunctions.h': self.generate_header,
        }

    @template_expander.use_jinja(
        'core/style/templates/ComputedStyleInitialFunctions.h.tmpl')
    def generate_header(self):
        return {
            'properties': self.properties(),
            'includes': self._includes
        }


if __name__ == '__main__':
    json5_generator.Maker(ComputedStyleInitialFunctionsWriter).main()
