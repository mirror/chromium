<?php
header("Link: <http://127.0.0.1:8000/preload/resources/preload-with-integrity.css>;rel=preload;as=style", false);
header("Link: <http://127.0.0.1:8000/preload/resources/preload-with-integrity.js>;rel=preload;as=script", false);
?>
<html>
<head>
<script>
if (window.testRunner) {
  testRunner.dumpAsText();
  testRunner.waitUntilDone();
  // notifyDone() will be called in preload-with-integrity.js
}

document.write(
  '<link href="resources/preload-with-integrity.css" rel="stylesheet" type="text/css" integrity="sha256-GgaKaN8yWqrczPOqvVFnrRM9yVvKJL8e4GSwEqDDc6M=">');
document.write(
'<scr' + 'ipt src="resources/preload-with-integrity.js" type="text/javascript" integrity="sha256-DsUqlkq+0Z2JeRo68QT//XgamzIIZ8qC6tTOKlskHMQ="> </scr' + 'ipt>');
</script>
</head>
<body>
<p class="red">Regression test for <a href="crbug.com/677022">677022</a>.</p>
<p>The test is successful if it completes without console warnings about unused preloads.</p>
</body>
</html>

