// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(
      `This tests that ChunkedFileReader properly re-assembles chunks, especially in case these contain multibyte characters.\n`);


  var text = [
    '\u041b\u0430\u0442\u044b\u043d\u044c \u0438\u0437 \u043c\u043e\u0434\u044b \u0432\u044b\u0448\u043b\u0430 \u043d\u044b\u043d\u0435:\n', '\u0422\u0430\u043a, \u0435\u0441\u043b\u0438 \u043f\u0440\u0430\u0432\u0434\u0443 \u0432\u0430\u043c \u0441\u043a\u0430\u0437\u0430\u0442\u044c,\n', '\u041e\u043d \u0437\u043d\u0430\u043b \u0434\u043e\u0432\u043e\u043b\u044c\u043d\u043e \u043f\u043e-\u043b\u0430\u0442\u044b\u043d\u0435,\n',
    '\u0427\u0442\u043e\u0431 \u044d\u043f\u0438\u0433\u0440\u0430\u0444\u044b \u0440\u0430\u0437\u0431\u0438\u0440\u0430\u0442\u044c\n'
  ];

  var blob = new Blob(text);
  // Most of the characters above will be encoded as 2 bytes, so make sure we use odd
  // chunk size to cause chunk boundaries sometimes to happen between chaacter bytes.
  var chunkSize = 5;
  var chunkCount = 0;
  var reader = new Bindings.ChunkedFileReader(blob, chunkSize, () => ++chunkCount);
  var output = new Common.StringOutputStream();

  var error = await reader.read(output);

  TestRunner.addResult('Read result: ' + error);
  TestRunner.addResult('Chunks transferred: ' + chunkCount);
  TestRunner.assertEquals(text.join(''), output.data(), 'Read text is different from written text');
  TestRunner.addResult('DONE');
  TestRunner.completeTest();
})();
