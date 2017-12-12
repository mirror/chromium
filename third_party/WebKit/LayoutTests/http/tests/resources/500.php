<?
$mime_type = isset($_GET['mime_type']) ? $_GET['mime_type'] : 'text/html';
header("HTTP/1.1 500 Internal Server Error");
header("Content-Type: $mime_type");
header("Last-Modified: Thu, 01 Jun 2017 06:09:43 GMT");
echo(file_get_contents("dummy.html"));
?>
