# This class is to parse fields from CSSProperties.json5 and ComputedStyleExtraFields.json5
# Every field related operation should be placed in this class
import types

import json5_generator
import keyword_utils
import bisect
import css_properties_utils
from core.css import css_properties
from name_utilities import upper_camel_case


def _get_properties_ranking(properties_ranking_file, partition_rule):
    """Read the properties ranking file and produce a dictionary of css
    properties with their group number based on the partition_rule

    Args:
        properties_ranking_file: file path to the ranking file
        partition_rule: cumulative distribution over properties_ranking

    Returns:
        dictionary with keys are css properties' name values are the group
        that each css properties belong to. Smaller group number is higher
        popularity in the ranking.
    """
    properties_ranking = [x["name"] for x in
                          json5_generator.Json5File.load_from_files([properties_ranking_file]).name_dictionaries]
    return dict(zip(properties_ranking,
                    [bisect.bisect_left(partition_rule, float(i) / len(properties_ranking)) + 1
                     for i in range(len(properties_ranking))]))


def _evaluate_rare_non_inherited_group(all_properties, properties_ranking_file,
                                       number_of_layer, partition_rule=None):
    """Re-evaluate the grouping of RareNonInherited groups based on each property's
    popularity.

    Args:
        all_properties: list of all css properties
        properties_ranking_file: file path to the ranking file
        number_of_layer: the number of group to split
        partition_rule: cumulative distribution over properties_ranking
                        Ex: [0.3, 0.6, 1]
    """
    if partition_rule is None:
        partition_rule = [1.0 * (i + 1) / number_of_layer for i in range(number_of_layer)]

    assert number_of_layer == len(partition_rule), "Length of rule and number_of_layer mismatch"

    layers_name = ["rare-non-inherited-usage-less-than-" + str(int(round(partition_rule[i] * 100))) + "-percent"
                   for i in range(number_of_layer)]
    properties_ranking = _get_properties_ranking(properties_ranking_file, partition_rule)

    for property_ in all_properties:
        if property_["field_group"] is not None and "*" in property_["field_group"] \
           and not property_["inherited"] and property_["name"] in properties_ranking:

            assert property_["field_group"] == "*", "The property " + property_["name"] \
                + " will be automatically assigned a group, please put '*' as the field_group"

            property_["field_group"] = "->".join(layers_name[0:properties_ranking[property_["name"]]])
        elif property_["field_group"] is not None and "*" in property_["field_group"] \
                and not property_["inherited"] and property_["name"] not in properties_ranking:
            group_tree = property_["field_group"].split("->")[1:]
            group_tree = [layers_name[0], layers_name[0] + "-sub"] + group_tree
            property_["field_group"] = "->".join(group_tree)


def _evaluate_rare_inherit_group(all_properties, properties_ranking_file,
                                 number_of_layer, partition_rule=None):
    """Re-evaluate the grouping of RareInherited groups based on each property's
    popularity.

    Args:
        all_properties: list of all css properties
        properties_ranking_file: file path to the ranking file
        number_of_layer: the number of group to split
        partition_rule: cumulative distribution over properties_ranking
                        Ex: [0.4, 1]
    """
    if partition_rule is None:
        partition_rule = [1.0 * (i + 1) / number_of_layer for i in range(number_of_layer)]

    assert number_of_layer == len(partition_rule), "Length of rule and number_of_layer mismatch"

    layers_name = ["rare-inherited-usage-less-than-" + str(int(round(partition_rule[i] * 100))) + "-percent"
                   for i in range(number_of_layer)]

    properties_ranking = _get_properties_ranking(properties_ranking_file, partition_rule)

    for property_ in all_properties:
        if property_["field_group"] is not None and "*" in property_["field_group"] \
           and property_["inherited"] and property_["name"] in properties_ranking:
            property_["field_group"] = "->".join(layers_name[0:properties_ranking[property_["name"]]])
        elif property_["field_group"] is not None and "*" in property_["field_group"] \
                and property_["inherited"] and property_["name"] not in properties_ranking:
            group_tree = property_["field_group"].split("->")[1:]
            group_tree = [layers_name[0], layers_name[0] + "-sub"] + group_tree
            property_["field_group"] = "->".join(group_tree)


def set_if_none(property_, key, value):
    if property_[key] is None:
        property_[key] = value


def apply_field_naming_defaults(field):
    upper_camel = upper_camel_case(field['name'])
    set_if_none(field, 'name_for_methods', upper_camel.replace('Webkit', ''))
    name = field['name_for_methods']
    simple_type_name = str(field['type_name']).split('::')[-1]
    set_if_none(field, 'type_name', 'E' + name)
    set_if_none(field, 'getter', name if simple_type_name != name else 'Get' + name)
    set_if_none(field, 'setter', 'Set' + name)
    set_if_none(field, 'inherited', False)
    set_if_none(field, 'initial', 'Initial' + name)


def apply_property_naming_defaults(property_):
    # TODO(meade): Delete this once all methods are moved to CSSPropertyAPIs.
    # TODO(shend): Use name_utilities for manipulating names.
    # TODO(shend): Rearrange the code below to separate assignment and set_if_none
    upper_camel = upper_camel_case(property_['name'])
    set_if_none(
        property_, 'name_for_methods', upper_camel.replace('Webkit', ''))
    name = property_['name_for_methods']
    simple_type_name = str(property_['type_name']).split('::')[-1]
    set_if_none(property_, 'type_name', 'E' + name)
    set_if_none(
        property_, 'getter', name if simple_type_name != name else 'Get' + name)
    set_if_none(property_, 'setter', 'Set' + name)
    set_if_none(property_, 'inherited', False)
    set_if_none(property_, 'initial', 'Initial' + name)

    if property_['custom_all']:
        property_['custom_initial'] = True
        property_['custom_inherit'] = True
        property_['custom_value'] = True
    if property_['inherited']:
        property_['is_inherited_setter'] = 'Set' + name + 'IsInherited'
    property_['should_declare_functions'] = \
        not property_['use_handlers_for'] \
        and not property_['longhands'] \
        and not property_['direction_aware'] \
        and not property_['builder_skip'] \
        and property_['is_property']
    # Functions should only be used in StyleBuilder if the CSSPropertyAPI
    # class is shared or not implemented yet (shared classes are denoted by
    # api_class = "some string").
    property_['use_api_in_stylebuilder'] = \
        property_['should_declare_functions'] \
        and not (property_['custom_initial'] or
                 property_['custom_inherit'] or
                 property_['custom_value']) \
        and property_['api_class'] \
        and isinstance(property_['api_class'], types.BooleanType)


def apply_primary_field_function_name(property_):
    if len(property_['fields']) > 0:
        primary_field = property_['fields'][0]
        set_if_none(property_, 'getter', primary_field['getter'])
        set_if_none(property_, 'setter', primary_field['setter'])
        set_if_none(property_, 'initial', primary_field['initial'])
        set_if_none(property_, 'name_for_methods', primary_field['name_for_methods'])
        set_if_none(property_, 'type_name', primary_field['type_name'])
        if len(primary_field['keywords']) > 0:
            property_['keywords'] = primary_field['keywords']


class CSSPropertyFieldWriter(css_properties.CSSProperties):
    def __init__(self, css_properties_path, extra_field_path,
                 css_value_id_path=None, grouping_parameter_path=None, css_ranking_path=None):
        super(CSSPropertyFieldWriter, self).__init__([css_properties_path])

        # Ignore shorthand properties
        for property_ in self._properties.values():
            if len(property_["fields"]) > 0:
                assert not property_['longhands'], \
                    "Shorthand '{}' cannot have a field_template.".format(property_['name'])

        css_properties_ = [value for value in self._properties.values() if not value['longhands']]

        self._fields = css_properties_utils.get_fields_from_css_properties(css_properties_,
                                                                           extra_field_path,
                                                                           self.json5_file.parameters)

        self._extra_fields = json5_generator.Json5File.load_from_files(
            [extra_field_path],
            default_parameters=self.json5_file.parameters
        ).name_dictionaries

        self._all_fields = self._fields.values() + self._extra_fields

        for field_ in self._all_fields:
            if field_['mutable']:
                assert field_['field_template'] == 'monotonic_flag', \
                    'mutable keyword only implemented for monotonic_flag'
            apply_field_naming_defaults(field_)

        for property_ in self._properties.values():
            apply_primary_field_function_name(property_)
            apply_property_naming_defaults(property_)

        if css_value_id_path is not None:
            # We sort the enum values based on each value's position in
            # the keywords as listed in CSSProperties.json5. This will ensure that if there is a continuous
            # segment in CSSProperties.json5 matching the segment in this enum then
            # the generated enum will have the same order and continuity as
            # CSSProperties.json5 and we can get the longest continuous segment.
            # Thereby reduce the switch case statement to the minimum.
            keyword_utils.sort_keyword_properties_by_canonical_order(self._fields.values(),
                                                                     css_value_id_path,
                                                                     self.json5_file.parameters)

        if grouping_parameter_path is not None and css_ranking_path is not None:
            group_parameters = dict([(conf["name"], conf["cumulative_distribution"]) for conf in
                                     json5_generator.Json5File.load_from_files([grouping_parameter_path]).name_dictionaries])
            _evaluate_rare_non_inherited_group(self._all_fields, css_ranking_path,
                                               len(group_parameters["rare_non_inherited_properties_rule"]),
                                               group_parameters["rare_non_inherited_properties_rule"])
            _evaluate_rare_inherit_group(self._all_fields, css_ranking_path,
                                         len(group_parameters["rare_inherited_properties_rule"]),
                                         group_parameters["rare_inherited_properties_rule"])
