#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json5_generator
import template_expander
import make_style_builder

from name_utilities import enum_for_css_keyword, enum_value_name


def _find_continuous_segment(number_list):
    # number_list is type list
    segments = [0]
    sorted(number_list, key=lambda elem: elem[0])
    for i in range(len(number_list) - 1):
        if not (int(number_list[i + 1][0]) - int(number_list[i][0]) == 1
                and int(number_list[i + 1][1]) - int(number_list[i][1]) == 1):
            segments.append(i + 1)
    segments.append(len(number_list))
    return segments


def _find_largest_segment(segments):
    l_segment = -1
    p_segment = 0
    for i in range(len(segments) - 1):
        if segments[i + 1] - segments[i] >= l_segment:
            l_segment = segments[i + 1] - segments[i]
            p_segment = (segments[i], segments[i + 1])
    return (p_segment, l_segment)


class CSSValueIDMappingsWriter(make_style_builder.StyleBuilderWriter):
    def __init__(self, json5_file_path):
        super(CSSValueIDMappingsWriter, self).__init__([json5_file_path[0]])
        self._outputs = {
            'CSSValueIDMappingsGenerated.h': self.generate_css_value_mappings,
        }
        self.css_values_dictionary_file = json5_file_path[1]

    @template_expander.use_jinja('CSSValueIDMappingsGenerated.h.tmpl')
    def generate_css_value_mappings(self):
        mappings = {}
        include_paths = set()
        css_values_dictionary = json5_generator.Json5File.load_from_files(
            [self.css_values_dictionary_file],
            default_parameters=self.json5_file.parameters
        ).name_dictionaries
        name_to_position_dictionary = dict(zip([x['name'] for x in css_values_dictionary], range(len(css_values_dictionary))))
        for property_ in self._properties.values():
            if property_['field_template'] in ('multi_keyword',):
                include_paths.update(property_['include_paths'])

                mappings[property_['type_name']] = {
                    'default_value': enum_value_name(property_['default_value']),
                    'mapping': [(enum_value_name(k), enum_for_css_keyword(k)) for k in property_['keywords']],
                }
            elif property_['field_template'] in ('keyword',):
                include_paths.update(property_['include_paths'])
                # steps: convert, 1 sort
                property_enum_order = range(len(property_['keywords']))
                css_enum_order = [name_to_position_dictionary[x] for x in property_['keywords']]
                enum_pair_list = zip(css_enum_order, property_enum_order)

                # enum_pair_list = sorted([(enum_value_name(k), enum_for_css_keyword(k)) for k in property_['keywords']],
                #         key=lambda elem: (elem[0], elem[1]))
                enum_segment = _find_continuous_segment(enum_pair_list)
                p_segment, _ = _find_largest_segment(enum_segment)

                enum_pair_list = [
                    (enum_value_name(property_['keywords'][x[1]]), x[1],
                     enum_for_css_keyword(property_['keywords'][x[1]]), x[0]) for x in enum_pair_list
                ]
                mappings[property_['type_name']] = {
                    'default_value': enum_value_name(property_['default_value']),
                    'mapping': enum_pair_list,
                    'segment': enum_segment,
                    's_segment': enum_pair_list[p_segment[0]],
                    'e_segment': enum_pair_list[p_segment[1] - 1],
                }

        return {
            'include_paths': list(sorted(include_paths)),
            'mappings': mappings,
        }

if __name__ == '__main__':
    json5_generator.Maker(CSSValueIDMappingsWriter).main()
