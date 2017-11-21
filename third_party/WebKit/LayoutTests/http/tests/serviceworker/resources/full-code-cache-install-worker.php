<?php
header('Content-Type: application/javascript');
// generate_token.py http://127.0.0.1:8000 PWAFullCodeCache --expire-timestamp=2000000000
if (isset($_GET['origin_trial'])) {
  header("Origin-Trial: ApKowNw/Vdnyb1Dxf4WrjnkCJZVqxtFOV/7KnRlvheT0NH+YA+DZ/9NiY5U1ITzABNzxgm6j6Q85qzUMM2SbUgYAAABYeyJvcmlnaW4iOiAiaHR0cDovLzEyNy4wLjAuMTo4MDAwIiwgImZlYXR1cmUiOiAiUFdBRnVsbENvZGVDYWNoZSIsICJleHBpcnkiOiAyMDAwMDAwMDAwfQ==");
}
?>

const script = 'full-code-cache-install-script.js';
const cache_name = 'full-code-cache-install';

self.addEventListener('install', (event) => {
    event.waitUntil(
      caches.open(cache_name)
        .then((cache) => {
          return cache.add(new Request(script));
        })
    );
  });
