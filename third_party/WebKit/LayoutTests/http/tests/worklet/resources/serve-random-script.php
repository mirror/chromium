<?php
header("Cache-Control: no-store, must-revalidate");
header("Pragma: no-cache");
header('Content-Type:application/javascript');

echo 'console.log("', microtime(), '");';
?>
