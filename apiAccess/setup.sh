#!/bin/bash
# Configura apiAccess con licencia self-healing por IP en cualquier VPS:
#   - despliega binario + autoLic (desde el repo)
#   - unit con ExecStartPre que regenera /etc/ADMRufuLIC si la IP cambia
#   - timer diario que ejecuta autoLic
#   - regenera la licencia al instalarse
# Idempotente: re-ejecutable en cualquier momento.

set -e
SUITE=/root/ADMRufu
DIR=$SUITE/apiAccess
SRC=$(dirname "$(readlink -f "$0")")

echo "[+] Configurando apiAccess con licencia auto diaria"

mkdir -p "$DIR"

if [ -f "$SRC/apiAccess" ] && [ -f "$SRC/autoLic" ]; then
    cp -f "$SRC/apiAccess" "$DIR/apiAccess" 2>/dev/null || :
    cp -f "$SRC/autoLic" "$DIR/autoLic" 2>/dev/null || :
    chmod 755 "$DIR/apiAccess" "$DIR/autoLic" 2>/dev/null || :
fi

cat > /etc/systemd/system/apiAccess.service <<'UNIT'
[Unit]
Description=apiAccess Service by @Rufu99
After=network.target nss-lookup.target

[Service]
Type=simple
User=root
ExecStart=/root/ADMRufu/apiAccess/apiAccess
ExecStartPre=/root/ADMRufu/apiAccess/autoLic
Restart=on-failure
RestartPreventExitStatus=23

[Install]
WantedBy=multi-user.target
UNIT

cat > /etc/systemd/system/apiAccess-lic.service <<'UNIT'
[Unit]
Description=Refresh diario de licencia apiAccess (self-healing por IP)

[Service]
Type=oneshot
ExecStart=/root/ADMRufu/apiAccess/autoLic
UNIT

cat > /etc/systemd/system/apiAccess-lic.timer <<'UNIT'
[Unit]
Description=Ejecuta autoLic (licencia apiAccess) cada 1 dia

[Timer]
OnBootSec=10min
OnCalendar=*-*-* 05:00:00
RandomizedDelaySec=15min
Persistent=true

[Install]
WantedBy=timers.target
UNIT

if [ -x "$DIR/autoLic" ]; then
    "$DIR/autoLic"
fi

systemctl daemon-reload
systemctl enable apiAccess-lic.timer >/dev/null 2>&1
systemctl start apiAccess-lic.timer || :
systemctl enable apiAccess.service >/dev/null 2>&1
systemctl restart apiAccess-lic.service || :
systemctl restart apiAccess.service || :

echo "[+] apiAccess: licencia auto activa (diaria)"
exit 0