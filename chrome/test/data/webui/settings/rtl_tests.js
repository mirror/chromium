// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

suite('settings drawer panel RTL tests', function() {
  setup(function() {
    PolymerTest.clearBody();
  });

  test('test i18n processing flips drawer panel', function() {
    var loadTimeData = new LoadTimeData;
    loadTimeData.data_ = {'textdirection': 'ltr'};

    var ui = document.createElement('settings-ui');
    document.body.appendChild(ui);
    Polymer.dom.flush();
    assertEquals('left', ui.$.drawer.align);
    ui.remove();

    loadTimeData.overrideValues({'textdirection': 'rtl'});
    ui = document.createElement('settings-ui');
    document.body.appendChild(ui);
    Polymer.dom.flush();
    assertEquals('right', ui.$.drawer.align);
    ui.remove();
  });
});
