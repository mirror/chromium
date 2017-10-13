# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Utilities for checking for disallowed usage of non-Blink declarations.

For convenience, the script can be run in standalone mode to check for existing
violations. Sample command:

$ git ls-files third_party/WebKit \
    | python third_party/WebKit/non_blink_scanner.py
"""

import os
import re
import sys

_CONFIG = [
    {
        'paths': ['third_party/WebKit/Source/', ],
        'allowed': [
            # TODO(dcheng): Check that the Oilpan plugin warns on this.
            'base::Optional',
            'base::make_span',
            'base::span',
            # TODO(dcheng): Should these be in a more specific config?
            'gfx::ColorSpace',
            'gfx::CubicBezier',
            'gfx::ICCProfile',
            'gfx::ScrollOffset',
            # TODO(dcheng): Remove this once Connector isn't needed in Blink
            # anymore.
            'service_manager::Connector',
            'service_manager::InterfaceProvider',
            # Nested namespace under the blink namespace for CSSValue classes.
            'cssvalue::.+',
            # Third-party libraries that don't depend on non-Blink Chrome code
            # are OK.
            'icu::.+',
            'testing::.+',  # googlemock / googletest
            'v8::.+',
            'v8_inspector::.+',
            # Inspector instrumentation and protocol
            'probe::.+',
            'protocol::.+',
            # Blink code shouldn't need to be qualified with the Blink namespace,
            # but there are exceptions.
            'blink::.+',
            # Assume that identifiers where the first qualifier is internal are
            # nested in the blink namespace.
            'internal::.+',
            # Blink uses Mojo, so it needs mojo::Binding, mojo::InterfacePtr, et
            # cetera, as well as generated Mojo bindings.
            'mojo::.+',
            '(?:.+::)?mojom::.+',
            # STL containers such as std::string and std::vector are discouraged
            # but still needed for interop with WebKit/common. Note that other
            # STL types such as std::unique_ptr are encouraged.
            'std::.+',
        ],
        'disallowed': ['.+', ],
    },
    {
        'paths': ['third_party/WebKit/Source/bindings/', ],
        'allowed': ['gin::.+', ],
    },
    {
        'paths': ['third_party/WebKit/Source/core/css', ],
        'allowed': [
            # Internal implementation details for CSS.
            'detail::.+',
        ],
    },
    {
        'paths': [
            'third_party/WebKit/Source/modules/device_orientation/',
            'third_party/WebKit/Source/modules/gamepad/',
            'third_party/WebKit/Source/modules/sensor/',
        ],
        'allowed': ['device::.+', ],
    },
    {
        'paths': ['third_party/WebKit/Source/modules/webgl/', ],
        'allowed': ['gpu::gles2::GLES2Interface', ],
    },
    # Suppress checks on controller and platform since code in these directories
    # is meant to be a bridge between Blink and non-Blink code..
    {
        'paths': [
            'third_party/WebKit/Source/controller/',
            'third_party/WebKit/Source/platform/',
        ],
        'allowed': ['.+', ],
    },
]


def _precompile_config_regexp():
    for entry in _CONFIG:
        # Note: if a particular key for an entry doesn't exist, assume that section
        # of the config shouldn't match anything.
        if 'allowed' in entry:
            entry['allowed'] = re.compile('(?:%s)$' % '|'.join(entry['allowed']))
        else:
            entry['allowed'] = re.compile('.^')
        if 'disallowed' in entry:
            entry['disallowed'] = re.compile('(?:%s)$' % '|'.join(entry['disallowed']))
        else:
            entry['disallowed'] = re.compile('.^')
_precompile_config_regexp()

_IDENTIFIER_WITH_NAMESPACE_RE = re.compile(
    r'\b(?:[a-z_][a-z0-9_]*::)+[A-Za-z_][A-Za-z0-9_]*?\b')


def _find_matching_configs(filename):
    """Finds configs that should be used for filename.

    Returns:
      A list of configs, sorted in order of relevance.
    """
    configs = []
    for entry in _CONFIG:
        for path in entry['paths']:
            if filename.startswith(path):
                configs.append({'path': path, 'config': entry})
    # Longer path implies more relevant, since that config is a more exact match.
    configs = reversed(sorted(configs, key=lambda x: len(x['path'])))
    configs = [entry['config'] for entry in configs]
    return configs


def _check_configs_for_identifier(configs, identifier):
    for config in configs:
        if config['allowed'].match(identifier):
            return True
        if config['disallowed'].match(identifier):
            return False
    # Disallow by default.
    return False


def check(filename, contents):
    """Checks for disallowed usage of non-Blink classes, functions, et cetera.

    Args:
      contents: The contents of the file to check.
      filename: The filename of the file to determine which set of rules to use.

    Returns:
      A list of disallowed identifiers.
    """
    results = []
    configs = _find_matching_configs(filename)
    if not configs:
        return
    for line in contents.splitlines():
        idx = line.find('//')
        if idx >= 0:
            line = line[:idx]
        match = _IDENTIFIER_WITH_NAMESPACE_RE.search(line)
        if match:
            if not _check_configs_for_identifier(configs, match.group(0)):
                results.append(match.group(0))
    return results


def main():
    for filename in sys.stdin.read().splitlines():
        basename, ext = os.path.splitext(filename)
        if ext not in ('.cc', '.cpp', '.h', '.mm'):
            continue
        # Ignore test files.
        if basename.endswith('Test'):
            continue
        with open(filename, 'r') as f:
            contents = f.read()
            disallowed_identifiers = check(filename, contents)
            if disallowed_identifiers:
                print '%s uses disallowed identifiers:' % filename
                for i in disallowed_identifiers:
                    print i


if __name__ == '__main__':
    sys.exit(main())
