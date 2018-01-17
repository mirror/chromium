<?php
header('OriginSignedResponseUrlForDemo: https://example.com/test.html');
?>
<!DOCTYPE html>

<body>
<script>
console.log(document.location.href);
</script>
hello<br>

<?php
/*
  ob_flush();
  flush();
  usleep(500000);
  */
?>
world
</body>
