<?php
if ($_GET['type'] == 'fallback') {
  header('HTTP/1.0 404 Not Found');
  exit;
}
print $_GET['type'];
?>
