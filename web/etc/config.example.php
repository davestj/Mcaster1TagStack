<?php
// config.example.php — committed template. Copy to config.php and fill in.
// The real config.php is gitignored.

return [
    'daemon_url'       => 'http://127.0.0.1:9890',
    'keycloak_realm'   => 'https://auth.mcaster1.com/realms/mcaster1',
    'keycloak_client'  => 'tagstack-web',
    'keycloak_secret'  => 'your_keycloak_client_secret_here',
    'session_lifetime' => 3600,
];
