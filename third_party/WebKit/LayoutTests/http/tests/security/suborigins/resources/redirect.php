<?php
$expect_cookie = $_GET['expect_cookie'];
$cookie_name = $_GET['cookie_name'];
if ($_SERVER['HTTP_SUBORIGIN'] == 'foobar') {
  if ($expect_cookie == NULL ||
      ($expect_cookie == 'false' && !isset($_COOKIE[$cookie_name])) ||
      ($expect_cookie == 'true' && isset($_COOKIE[$cookie_name]))) {
    header("Location: " . $_GET['url']);

    if (isset($_GET['ACAOrigin'])) {
      header('Access-Control-Allow-Origin: ' . $_GET['ACAOrigin']);
    }

    if (isset($_GET['ACASuborigin'])) {
      header('Access-Control-Allow-Suborigin: ' . $_GET['ACASuborigin']);
    }

    if (isset($_GET['ACACredentials'])) {
      header('Access-Control-Allow-Credentials: ' . $_GET['ACACredentials']);
    }
  }
}
?>
