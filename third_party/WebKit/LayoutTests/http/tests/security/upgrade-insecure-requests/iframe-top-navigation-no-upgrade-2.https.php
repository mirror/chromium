<!DOCTYPE html>
<html>
  <head>
    <title>Upgrade Insecure Requests: top-frame navigation inside iframe (no upgrade expected)</title>
    <script>
      if (window.testRunner) {
        testRunner.dumpAsText();
        testRunner.waitUntilDone();
      }
    </script>

  </head>
  <body>
    <!--
    This is a bit of a hack. UPGRADE doesn't upgrade the port number. So if the
    url is upgraded, the url becomes invalid (https over the 8080 port). The
    expected behavior is that the url is not upgraded and the page loads.
    -->
    <iframe
      sandbox="allow-scripts allow-top-navigation"
      src="https://example.test:8443/security/upgrade-insecure-requests/resources/navigate-top-frame-upgrade.php?url=http://127.0.0.1:8080/misc/resources/success-notify-done.html"
    ></iframe>
  </body>
</html>
