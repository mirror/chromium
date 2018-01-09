<?php
header("Content-Security-Policy: upgrade-insecure-requests");
?>
<!DOCTYPE html>
<html>
  <body>
    <script>
      window.top.location.href = <?php echo json_encode($_GET['url']) ?>;
    </script>
  </body>
</html>
