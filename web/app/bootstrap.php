<?php
// app/bootstrap.php — shared init.
// Phase 2 will load config + Keycloak SDK + audit logger here.

declare(strict_types=1);

if (!defined('TAGSTACK_ROOT')) {
    define('TAGSTACK_ROOT', dirname(__DIR__, 2));
}

// Load the gitignored real config if present, else the example template.
$config_file = TAGSTACK_ROOT . '/web/etc/config.php';
if (!is_readable($config_file)) {
    $config_file = TAGSTACK_ROOT . '/web/etc/config.example.php';
}
require $config_file;
