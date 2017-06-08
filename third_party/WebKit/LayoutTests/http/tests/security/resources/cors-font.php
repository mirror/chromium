<?php
$font = '../../resources/Ahem.ttf';

header('Cache-Control: public, max-age=86400');
header('Last-Modified: ' . gmdate('D, d M Y H:i:s', filemtime($font)) . ' GMT');
header('Content-Type: font/truetype');
header('Content-Length: ' . filesize($font));

$acaorigin = $_GET['ACAOrigin'];
if ($acaorigin === 'true') {
  header('Access-Control-Allow-Origin: http://127.0.0.1:8000');
} else if ($acaorigin !== NULL || $acaorigin !== 'false') {
  header('Access-Control-Allow-Origin: ' . $acaorigin);
}

if (isset($_GET['ACACredentials'])) {
  header('Access-Control-Allow-Credentials: ' . $_GET['ACACredentials']);
}

header('Timing-Allow-Origin: *');
ob_clean();
flush();
readfile($font);
?>
