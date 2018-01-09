<?php
header("Content-Security-Policy: upgrade-insecure-requests");
?>
<!DOCTYPE html>
<html>
  <head>
    <title>Upgrade Insecure Requests: top-frame navigation inside iframe (upgrade expected)</title>
    <script>
      if (window.testRunner) {
        testRunner.dumpAsText();
        testRunner.waitUntilDone();
      }
    </script>

  </head>
  <body>
    <!--
    This is a bit of a hack. UPGRADE doesn't upgrade the port number, so we
    specify this non-existent URL ('http' over port 8443). If UPGRADE doesn't
    work, it won't load. The expected behavior is that the url is upgraded and
    the page loads.
    -->
    <iframe
      sandbox="allow-scripts allow-top-navigation"
      src="https://example.test:8443/security/upgrade-insecure-requests/resources/navigate-top-frame.php?url=http://127.0.0.1:8443/misc/resources/success-notify-done.html"
    ></iframe>
  </body>
</html>
