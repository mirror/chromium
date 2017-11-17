// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests SHA-1 hashes.\n`);
  await TestRunner.loadModule('product_registry_impl');

  TestRunner.addResult('foobar : ' + Sha1.sha1('foobar'));
  TestRunner.addResult('hello : ' + Sha1.sha1('hello'));
  TestRunner.addResult('abcdefghijklmnopqrstuvwxyz : ' + Sha1.sha1('abcdefghijklmnopqrstuvwxyz'));
  TestRunner.addResult('ABCDEFGHIJKLMNOPQRSTUVWXYZ : ' + Sha1.sha1('ABCDEFGHIJKLMNOPQRSTUVWXYZ'));
  TestRunner.addResult('a : ' + Sha1.sha1('a'));
  TestRunner.addResult('A : ' + Sha1.sha1('A'));
  TestRunner.addResult('A1 : ' + Sha1.sha1('A1'));
  TestRunner.completeTest();
})();
