# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from .utilities import assert_no_extra_args


# https://heycam.github.io/webidl/#idl-extended-attributes
# As a basic rule, we handle an extended attribute list as a dict object, and each value is either of
# None, a string, a list of strings, or an object.
# To support irregular style extended attributes, we define some classes in this file.

# https://heycam.github.io/webidl/#Constructor
class Constructor(object):

    def __init__(self, **kwargs):
        self._arguments = kwargs.pop('arguments', [])
        self._is_custom = kwargs.pop('is_custom', False)
        assert_no_extra_args(kwargs)

    @property
    def arguments(self):
        return self._arguments

    @property
    def is_custom(self):
        return self._is_custom


# https://heycam.github.io/webidl/#NamedConstructor
class NamedConstructor(object):

    def __init__(self, **kwargs):
        self._identifier = kwargs.pop('identifier', None)
        self._arguments = kwargs.pop('arguments', [])
        assert_no_extra_args(kwargs)

    @property
    def identifier(self):
        return self._identifier

    @property
    def arguments(self):
        return self._arguments


# https://heycam.github.io/webidl/#Exposed
class Exposure(object):
    """Exposure holds an exposed target, and can hold a runtime enabled condition.
    "[Exposed=global_interface]" is represented as Exposure(global_interface), and
    "[Exposed(global_interface runtime_enabled_feature)] is represented as Exposure(global_interface, runtime_enabled_feature).
    """
    def __init__(self, **kwargs):
        self._global_interface = kwargs.pop('global_interface')
        self._runtime_enabled_feature = kwargs.pop('runtime_enabled_feature', None)
        assert_no_extra_args(kwargs)

    @property
    def global_interface(self):
        return self._global_interface

    @property
    def runtime_enabled_feature(self):
        return self._runtime_enabled_feature
