<?php
header("Content-Security-Policy: upgrade-insecure-requests");
?>
<!DOCTYPE html>
<html>
  <title>Upgrade Insecure Requests: link no upgrade.</title>
  <head>
    <script>
      if (window.testRunner) {
        testRunner.dumpAsText();
        testRunner.waitUntilDone();
      }

      function click() {
        document.getElementById("link").click();
      }
    </script>
  </head>
  <body onload="click()">
    <!--
    This is a bit of a hack. UPGRADE doesn't upgrade the port number. So if
    the url is upgraded, the url becomes invalid (https over the 8080 port).
    The expected behavior is that the url is not upgraded and the page loads.
    -->
    <a id="link" href="http://example.test:8080/resources/notify-done.html">Click me</a>
  </body>
</html>
