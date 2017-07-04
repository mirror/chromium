<?php
if ($_GET['type'] == 'fallback') {
  header('HTTP/1.0 404 Not Found');
  exit;
}
header("Content-type: text/javascript");
?>
info = 'Set by the <?= $_GET['type'] ?> script';
