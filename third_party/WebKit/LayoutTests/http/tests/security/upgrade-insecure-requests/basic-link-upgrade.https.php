<?php
 header("Content-Security-Policy: upgrade-insecure-requests");
?>
<!DOCTYPE html>
<html>
  <title>Upgrade Insecure Requests: link upgrade.</title>
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
    This is a bit of a hack. UPGRADE doesn't upgrade the port number, so we
    specify this non-existent URL ('http' over port 8443). If UPGRADE doesn't
    work, it won't load. The expected behavior is that the url is upgraded and
    the page loads.
    -->
    <a id="link" href="http://127.0.0.1:8443/resources/notify-done.html">Click me</a>
  </body>
</html>
