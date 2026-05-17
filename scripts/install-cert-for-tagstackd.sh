#!/usr/bin/env bash
# scripts/install-cert-for-tagstackd.sh
#
# Certbot post-renewal deploy hook. Copies the fullchain + privkey to
# a www-data-owned location under shared/etc/certs/ so the daemon can
# read them directly without any group/ACL gymnastics. Also reloads
# tagstackd so the new cert is picked up.
#
# Install location on the server:
#   /etc/letsencrypt/renewal-hooks/deploy/tagstackd-cert.sh
# (symlink or copy of this file)

set -euo pipefail

DOMAIN="tagstack.mcaster1.com"
SRC_LIVE="/etc/letsencrypt/live/${DOMAIN}"
DST_DIR="/var/www/tagstack.mcaster1.com/shared/etc/certs"

mkdir -p "$DST_DIR"

# Resolve the symlinks so we copy the actual content.
install -o www-data -g www-data -m 0644 "${SRC_LIVE}/fullchain.pem" "${DST_DIR}/fullchain.pem"
install -o www-data -g www-data -m 0640 "${SRC_LIVE}/privkey.pem"   "${DST_DIR}/privkey.pem"
install -o www-data -g www-data -m 0644 "${SRC_LIVE}/cert.pem"      "${DST_DIR}/cert.pem"
install -o www-data -g www-data -m 0644 "${SRC_LIVE}/chain.pem"     "${DST_DIR}/chain.pem"

echo "install-cert-for-tagstackd: copied ${DOMAIN} cert to ${DST_DIR}"

# Reload the daemon if it's running so the new cert is picked up
if systemctl is-active --quiet tagstackd; then
    systemctl reload-or-restart tagstackd
    echo "install-cert-for-tagstackd: tagstackd reloaded"
fi
