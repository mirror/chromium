<?php
$acaorigin = $_GET["ACAOrigin"];
if ($acaorigin === "true") {
  header("Access-Control-Allow-Origin: http://127.0.0.1:8000");
} else if ($acaorigin !== NULL && $acaorigin !== 'false') {
  header("Access-Control-Allow-Origin: " . $acaorigin);
}

if (isset($_GET["ACACredentials"])) {
  header("Access-Control-Allow-Credentials: " . $_GET["ACACredentials"]);
}
?>
<!DOCTYPE html>
<html>
<head></head>
<body><h1>Hello</h1></body>
</html>
