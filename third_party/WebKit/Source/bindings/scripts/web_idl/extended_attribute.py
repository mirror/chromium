# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


# https://heycam.github.io/webidl/#idl-extended-attributes
# We handle an extended attribute list as a dict object, and each value is either of
# None, a string, a list of strings, or an object.
# Here we define classes for objects to store as objects.

class Constructor(object):

    def __init__(self, **kwargs):
        self._arguments = kwargs.pop('arguments', [])
        self._is_custom = kwargs.pop('is_custom', False)
        if kwargs:
            raise ValueError('Unknown parameters are passed: %s' % kwargs.keys())

    @property
    def arguments(self):
        return self._arguments

    @property
    def is_custom(self):
        return self._is_custom


class NamedConstructor(object):

    def __init__(self, **kwargs):
        self._identifier = kwargs.pop('identifier', None)
        self._arguments = kwargs.pop('arguments', [])
        if kwargs:
            raise ValueError('Unknown parameters are passed: %s' % kwargs.keys())

    @property
    def identifier(self):
        return self._identifier

    @property
    def arguments(self):
        return self._arguments


class Exposure(object):
    """Exposure holds an exposed target, and can hold a runtime enabled condition.
    "[Exposed=global_interface]" is represented as Exposure(global_interface), and
    "[Exposed(global_interface runtime_enabled_feature)] is represented as Exposure(global_interface, runtime_enabled_feature).
    """
    def __init__(self, **kwargs):
        self._global_interface = kwargs.pop('global_interface')
        self._runtime_enabled_feature = kwargs.pop('runtime_enabled_feature', None)
        if kwargs:
            raise ValueError('Unknown parameters are passed: %s' % kwargs.keys())

    @property
    def global_interface(self):
        return self._global_interface

    @property
    def runtime_enabled_feature(self):
        return self._runtime_enabled_feature
