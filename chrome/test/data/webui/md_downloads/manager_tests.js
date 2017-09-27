// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

suite('manager tests', function() {
  /** @type {!downloads.Manager} */
  let manager;

  setup(function() {
    PolymerTest.clearBody();
    manager = document.createElement('downloads-manager');
    document.body.appendChild(manager);
    assertEquals(manager, downloads.Manager.get());
  });

  test('long URLs ellide', function() {
    downloads.Manager.insertItems(0, [{
                                    file_name: 'file name',
                                    state: downloads.States.COMPLETE,
                                    url: 'a'.repeat(1000),
                                  }]);
    Polymer.dom.flush();
    // console.log(manager.outerHTML);

    const item = manager.$$('downloads-item');
    // console.log(item.outerHTML);
    assertLT(item.$$('#url').offsetWidth, item.offsetWidth);
    assertEquals(300, item.$$('#url').textContent.length);
  });

  test('inserting items at beginning render dates correctly', function() {
    const download1 = {
      by_ext_id: '',
      by_ext_name: '',
      danger_type: '',
      date_string: '',
      file_externally_removed: false,
      file_path: '/some/file/path',
      file_name: 'download 1',
      file_url: 'file:///some/file/path',
      id: '',
      last_reason_text: '',
      otr: false,
      percent: 100,
      progress_status_text: '',
      resume: false,
      return: false,
      since_string: 'Today',
      started: Date.now() - 10000,
      state: downloads.States.COMPLETE,
      total: -1,
      url: 'http://permission.site',
    };
    const download2 = Object.assign({}, download1);

    downloads.Manager.insertItems(0, [download1, download2]);
    assertFalse(download1.hideDate);
    assertTrue(download2.hideDate);

    const dateQuery = '* /deep/ h3[id=date]:not(:empty)';
    const countDates = () => manager.querySelectorAll(dateQuery).length;

    Polymer.dom.flush();
    assertEquals(1, countDates());

    downloads.Manager.removeItem(0);
    assertFalse(download2.hideDate);
    Polymer.dom.flush();
    assertEquals(1, countDates());

    downloads.Manager.insertItems(0, [download1]);
    assertFalse(download1.hideDate);
    assertTrue(download2.hideDate);
    Polymer.dom.flush();
    assertEquals(1, countDates());
  });
});
