<?php
$acaorigin = $_GET['ACAOrigin'];
if ($acaorigin === 'true') {
  header('Access-Control-Allow-Origin: http://127.0.0.1:8000');
} else if ($acaorigin !== NULL && $acaorigin !== 'false') {
  header('Access-Control-Allow-Origin: ' . $acaorigin);
}

if (isset($_GET['ACASuborigin'])) {
  header('Access-Control-Allow-Suborigin: ' . $_GET['ACASuborigin']); 
}

if (isset($_GET['ACACredentials'])) {
  header('Access-Control-Allow-Credentials: ' . $_GET['ACACredentials']);
}

if (isset($_GET['ACAHeaders'])) {
  header('Access-Control-Allow-Headers: ' . $_GET['ACAHeaders']);
}

if ($_SERVER['HTTP_SUBORIGIN'] === 'foobar') {
  header('Access-Control-Allow-Suborigin: foobar');
}

header('Content-Type: application/javascript');

$delay = $_GET['delay'];
if ($delay)
  usleep(1000 * $delay);

if ($_GET['throw'] === 'true') {
  echo "throw({toString: function(){ return 'SomeError' }});";
} else {
  $cookie = $_GET['cookie'];
  if (isset($cookie)) {
    $value = $_COOKIE[$cookie];
  } else if ($_SERVER['HTTP_ORIGIN'] && isset($_GET['value_cors'])) {
    $value = $_GET['value_cors'];
  } else {
    $value = $_GET['value'];
  }

  if (isset($value)) {
    $result_var = 'result';
    if (!empty($_GET['resultvar'])) {
      $result_var = $_GET['resultvar'];
    }

    echo $result_var . " = \"" . $value . "\";";
  } else {
    echo "alert('script ran.');";
  }
}
?>
