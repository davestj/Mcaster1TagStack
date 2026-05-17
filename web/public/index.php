<?php
// /index.php — public entry point for the TagStack admin web UI.
//
// Phase 1: render a simple landing showing daemon health.
// Phase 2: redirect to /login via Keycloak.

require_once __DIR__ . '/../app/bootstrap.php';

$daemon_url = 'http://127.0.0.1:9890/api/v1/health';
$health = @file_get_contents($daemon_url);
$status = ($health && str_contains($health, '"ok":true')) ? 'online' : 'unknown';

header('Content-Type: text/html; charset=utf-8');
?>
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<title>Mcaster1 TagStack</title>
<style>
  body { font-family: ui-sans-serif, system-ui, sans-serif; background: #0a1e3d; color: #e2e8f0; margin: 0; padding: 60px 40px; }
  h1 { color: #fbbf24; font-size: 32px; margin: 0 0 8px; }
  p  { color: #bbe1fa; }
  .status { display: inline-block; padding: 4px 12px; border-radius: 4px; font-size: 12px; letter-spacing: 1.5px; }
  .online  { background: #22c55e; color: #0a1e3d; }
  .unknown { background: #f59e0b; color: #0a1e3d; }
  code { background: #1a3760; padding: 2px 6px; border-radius: 3px; }
</style>
</head>
<body>
  <h1>Mcaster1 TagStack</h1>
  <p>Metadata operations center · <span class="status <?= htmlspecialchars($status) ?>"><?= strtoupper(htmlspecialchars($status)) ?></span></p>
  <p>Phase 1 skeleton. Real admin UI lands in Phase 2.</p>
  <p>Daemon: <code><?= htmlspecialchars($daemon_url) ?></code></p>
</body>
</html>
