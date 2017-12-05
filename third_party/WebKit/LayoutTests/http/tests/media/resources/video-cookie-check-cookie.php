<?php
    if($_COOKIE["TEST"])
    {
        $filePath = $_COOKIE["TEST"];
        $extension = end(explode(".", $filePath));

        if ($extension == "webm") {
            header("Content-Type: video/webm");
        } else {
            die;
        }

        header("Cache-Control: no-store");
        header("Connection: close");

        $fn = fopen($filePath, "r");
        fpassthru($fn);
        fclose($fn);
        exit;
    }
?>
