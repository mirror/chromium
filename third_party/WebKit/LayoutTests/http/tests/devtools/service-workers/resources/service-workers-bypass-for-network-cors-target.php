<?php
header("Access-Control-Allow-Origin: http://127.0.0.1:8000");

$type = $_GET["type"];

if ($type === "img") {
  $image = "../../../resources/square100.png";
  header("Content-Type: image/png");
  header("Content-Length: " . filesize($image));
  header("Access-Control-Allow-Origin: *");
  readfile($image);
} else if ($type === "txt") {
  header("Content-Type: text/plain");
  print("hello");
  exit;
}
?>
