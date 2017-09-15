# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=relative-import

import argument


class Exposure(object):
    """An Exposure holds one Exposed or RuntimeEnabled condition.
    Each exposure has two properties: exposed and runtime_enabled.
    Exposure(e, r) corresponds to [Exposed(e r)]. Exposure(e) corresponds to
    [Exposed=e].
    """
    def __init__(self, exposed, runtime_enabled=None):
        self.exposed = exposed
        self.runtime_enabled = runtime_enabled


# Helper functions

def expand_extended_attributes(node):
    """
    Returns:
      Dictionary of {ExtAttributeName: ExtAttributeValue}.
      Value is usually a string, with these exceptions:
      Constructors: value is a list of Arguments nodes, corresponding to
        possible signatures of the constructor.
      CustomConstructors: value is a list of Arguments nodes, corresponding to
        possible signatures of the custom constructor.
      NamedConstructor: value is a Call node, corresponding to the single
        signature of the named constructor.
    """
    assert node.GetClass() == 'ExtAttributes'

    # Primarily just make a dictionary from the children.
    # The only complexity is handling various types of constructors:
    # Constructors and Custom Constructors can have duplicate entries due to
    # overloading, and thus are stored in temporary lists.
    # However, Named Constructors cannot be overloaded, and thus do not have
    # a list.
    # FIXME: move Constructor logic into separate function, instead of modifying
    #        extended attributes in-place.
    constructors = []
    custom_constructors = []
    extended_attributes = {}

    def child_node(extended_attribute_node):
        children = extended_attribute_node.GetChildren()
        if not children:
            return None
        if len(children) > 1:
            raise ValueError('ExtAttributes node with %s children, expected at most 1' % len(children))
        return children[0]

    extended_attribute_node_list = node.GetChildren()
    for extended_attribute_node in extended_attribute_node_list:
        name = extended_attribute_node.GetName()
        child = child_node(extended_attribute_node)
        child_class = child and child.GetClass()
        if name == 'Constructor':
            if child_class and child_class != 'Arguments':
                raise ValueError('Constructor only supports Arguments as child, but has child of class: %s' % child_class)
            constructors.append(child)
        elif name == 'CustomConstructor':
            if child_class and child_class != 'Arguments':
                raise ValueError('[CustomConstructor] only supports Arguments as child, but has child of class: %s' % child_class)
            custom_constructors.append(child)
        elif name == 'NamedConstructor':
            if child_class and child_class != 'Call':
                raise ValueError('[NamedConstructor] only supports Call as child, but has child of class: %s' % child_class)
            extended_attributes[name] = child
        elif name == 'Exposed':
            if child_class and child_class != 'Arguments':
                raise ValueError('[Exposed] only supports Arguments as child, but has child of class: %s' % child_class)
            exposures = []
            if child_class == 'Arguments':
                exposures = [Exposure(exposed=str(arg.type),
                                      runtime_enabled=arg.identifier)
                             for arg in argument.arguments_node_to_arguments(child)]
            else:
                value = extended_attribute_node.GetProperty('VALUE')
                if type(value) is str:
                    exposures = [Exposure(exposed=value)]
                else:
                    exposures = [Exposure(exposed=v) for v in value]
            extended_attributes[name] = exposures
        elif child:
            raise ValueError('ExtAttributes node with unexpected children: %s' % name)
        else:
            value = extended_attribute_node.GetProperty('VALUE')
            extended_attributes[name] = value

    # Store constructors and custom constructors in special list attributes,
    # which are deleted later. Note plural in key.
    if constructors:
        extended_attributes['Constructors'] = constructors
    if custom_constructors:
        extended_attributes['CustomConstructors'] = custom_constructors

    return extended_attributes
