import json5_generator


def sort_keyword_css_properties(css_properties, css_value_keywords_file, json5_file_parameters):
    css_values_dictionary = json5_generator.Json5File.load_from_files(
        [css_value_keywords_file],
        default_parameters=json5_file_parameters
    ).name_dictionaries
    css_values_dictionary = [x['name'] for x in css_values_dictionary]
    name_to_position_dictionary = dict(zip(css_values_dictionary, range(len(css_values_dictionary))))
    for css_property in css_properties:
        if css_property['field_template'] == 'keyword':
            css_property['keywords'] = sorted(css_property['keywords'], key=lambda x: name_to_position_dictionary[x])
    return css_properties
