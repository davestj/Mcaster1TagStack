# packaging/ — platform packaging

| Subdir         | Target                                                |
|----------------|-------------------------------------------------------|
| `macos/`       | DMG signed + notarized for distribution               |
| `debian/`      | `.deb` for the daemon on ovh-us-east                  |
| `ansible/`     | Ansible playbooks coordinated with the existing fleet |

Capistrano (`config/deploy.rb`) is the primary daemon deploy
mechanism; Ansible is reserved for fleet-level setup (firewall, ufw,
cert renewal, systemd unit install).
