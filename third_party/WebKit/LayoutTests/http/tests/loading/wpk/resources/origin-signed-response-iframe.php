<?php
header('Content-Type: application/http-exchange+cbor');

$payload  = <<< EOM
<!DOCTYPE html>
<body>
<script>
window.addEventListener('message', (event) => {
  event.data.port.postMessage({location: document.location.href});
}, false);
</script>
hello<br>
world
</body>
EOM;

$response = array(
  'htxg',
  'request',
  array(
    ':method' => 'GET',
    ':url' => 'https://example.com/test.html',
    'accept'=> '*/*'
  ),
  'response',
  array(
    ':status' => '200',
    'content-type'=> 'text/html'
  ),
  'payload',
  $payload
);

// TODO(880527): Use CBOR format rather than JSON format.
echo json_encode($response);
?>

