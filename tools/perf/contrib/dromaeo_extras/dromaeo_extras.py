# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


from benchmarks import dromaeo


Dromaeo = dromaeo.Dromaeo


class JSLibAttrJquery(Dromaeo):
  NAME = 'dromaeo.jslibattrjquery'
  URL = 'http://dromaeo.com?jslib-attr-jquery'


class JSLibAttrPrototype(Dromaeo):
  NAME = 'dromaeo.jslibattrprototype'
  URL = 'http://dromaeo.com?jslib-attr-prototype'


class JSLibEventQuery(Dromaeo):
  NAME = 'dromaeo.jslibeventquery'
  URL = 'http://dromaeo.com?jslib-event-query'


class JSLibEventPrototype(Dromaeo):
  NAME = 'dromaeo.jslibeventprototype'
  URL = 'http://dromaeo.com?jslib-event-prototype'


class JSLibModifyJQuery(Dromaeo):
  NAME = 'dromaeo.jslibmodifyjquery'
  URL = 'http://dromaeo.com?jslib-modify-jquery'


class JSLibModifyPrototype(Dromaeo):
  NAME = 'dromaeo.jslibmodifyprototype'
  URL = 'http://dromaeo.com?jslib-modify-prototype'


class JSLibStyleJQuery(Dromaeo):
  NAME = 'dromaeo.jslibstylejquery'
  URL = 'http://dromaeo.com?jslib-style-jquery'


class JSLibStylePrototype(Dromaeo):
  NAME = 'dromaeo.jslibstyleprototype'
  URL = 'http://dromaeo.com?jslib-style-prototype'


class JSLibTraverseJQuery(Dromaeo):
  NAME = 'dromaeo.jslibtraversejquery'
  URL = 'http://dromaeo.com?jslib-traverse-jquery'


class JSLibTraversePrototype(Dromaeo):
  NAME = 'dromaeo.jslibtraverseprototype'
  URL = 'http://dromaeo.com?jslib-traverse-prototype'


class CSSQueryJQuery(Dromaeo):
  NAME = 'dromaeo.cssqueryjquery'
  URL = 'http://dromaeo.com?cssquery-jquery'
