# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from telemetry import benchmark

from benchmarks import dromaeo


_BaseDromaeoBenchmark = dromaeo.Dromaeo


@benchmark.Owner(emails=['yukishiino@chromium.org',
                         'bashi@chromium.org',
                         'haraken@chromium.org'])
class DromaeoExtras(_BaseDromaeoBenchmark):
  """Dromaeo JSLib and CSS JavaScript benchmark."""
  stories = [('dromaeo.jslibattrjquery', 'http://dromaeo.com?jslib-attr-jquery'),
             ('dromaeo.jslibattrprototype', 'http://dromaeo.com?jslib-attr-prototype'),
             ('dromaeo.jslibeventquery', 'http://dromaeo.com?jslib-event-query'),
             ('dromaeo.jslibeventprototype', 'http://dromaeo.com?jslib-event-prototype'),
             ('dromaeo.jslibmodifyjquery', 'http://dromaeo.com?jslib-modify-jquery'),
             ('dromaeo.jslibmodifyprototype', 'http://dromaeo.com?jslib-modify-prototype'),
             ('dromaeo.jslibstylejquery', 'http://dromaeo.com?jslib-style-jquery'),
             ('dromaeo.jslibstyleprototype', 'http://dromaeo.com?jslib-style-prototype'),
             ('dromaeo.jslibtraversejquery', 'http://dromaeo.com?jslib-traverse-jquery'),
             ('dromaeo.jslibtraverseprototype', 'http://dromaeo.com?jslib-traverse-prototype'),
             ('dromaeo.cssqueryjquery', 'http://dromaeo.com?cssquery-jquery'),]
  name = 'dromaeo.extras'
