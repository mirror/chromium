import json5_generator


def get_fields_from_css_properties(css_properties, extra_field_path, css_property_parameters):
    fields = []
    extra_fields_file = json5_generator.Json5File.load_from_files([extra_field_path])
    field_parameters = extra_fields_file.parameters
    common_parameters = set(field_parameters.keys()).intersection(set(css_property_parameters))
    field_only_parameters = set(field_parameters.keys()).difference(common_parameters)
    for property_ in css_properties:
        if len(property_['fields']) > 0:
            for field in property_['fields']:
                # VALIDATION: prevent multi field declaration from declaring "inherited" attribute. This attribute
                # will be copy from property_
                assert 'inherited' not in field, "Field's inherited attribute will be the same as its property, \
                    do not specify field's inherited attribute"

                if 'name' not in field:
                    field['name'] = property_['name']

                for param in common_parameters:
                    if param not in field:
                        field[param] = property_[param]

                for param in field_only_parameters:
                    if param not in field:
                        if 'default' in field_parameters[param]:
                            field[param] = field_parameters[param]['default']
                        elif 'default' not in field_parameters[param]:
                            field[param] = None
                extra_fields_file.validate_dictionary(field)
                field['css_property'] = property_
                field['independent'] = property_['independent']
                fields.append(field)

    return {field['name']: field for field in fields}
