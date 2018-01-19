<?php
// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This test ensures that when unsized-media is disabled, videos are laid out by
// default dimensions.
Header("Feature-Policy: unsized-media 'none'");
?>

<!DOCTYPE html>
<html>
<body onload="init();">
<script>
function init() {
  var total_count = document.getElementsByTagName('video').length;
  var count = total_count;
  document.addEventListener("canplaythrough", function() {
    if (!--count) {
      if (window.testRunner)
        testRunner.notifyDone();
    }
  }, true);
}
if (window.testRunner)
  testRunner.waitUntilDone();
</script>
  <video src="resources/test.ogv"></video>
  <video width="500" src="resources/test.ogv"></video>
  <video style="width:500px;" src="resources/test.ogv"></video>
  <video height="800" src="resources/test.ogv"></video>
  <video style="height:800px;" src="resources/test.ogv"></video>
  <video width="500" height="800" src="resources/test.ogv"></video>
  <video style="width:500px;height:800px;" src="resources/test.ogv"></video>
</body>
</html>
