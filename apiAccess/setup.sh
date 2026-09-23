#!/bin/bash
# Mantenimiento de apiAccess en cualquier VPS (idempotente y self-healing):
#  - despliega binario + autoLic desde SRC (repo) si estan disponibles
#  - garantiza apiAccess.service con ExecStartPre=autoLic + RestartSec=5
#  - garantiza timer diario apiAccess-lic.timer (boot + cada 1 dia)
#  - regenera /etc/ADMRufuLIC si la IP publica cambio o falta
#  - solo reinicia servicios cuando algo cambio
# Se ejecuta automatico: desde install.sh, al arranque (OnBootSec) y cada dia.

set -e
SUITE=/root/ADMRufu
DIR=$SUITE/apiAccess
SRC=$(dirname "$(readlink -f "$0")")
CHG=0

echo "[+] apiAccess: mantenimiento (unit + licencia)"

mkdir -p "$DIR"

# 1) despliegue de binario/script desde el repo (si SRC los trae)
if [ -f "$SRC/apiAccess" ] && [ -f "$SRC/autoLic" ]; then
    cp -f "$SRC/apiAccess" "$DIR/apiAccess" 2>/dev/null || :
    cp -f "$SRC/autoLic" "$DIR/autoLic" 2>/dev/null || :
    chmod 755 "$DIR/apiAccess" "$DIR/autoLic" 2>/dev/null || :
fi

# 2) units: solo se reescriben si difieren (evita restarts inutiles)
cat > "$DIR/.apiAccess.service" <<'UNIT'
[Unit]
Description=apiAccess Service by @Rufu99
After=network.target nss-lookup.target
StartLimitIntervalSec=0

[Service]
Type=simple
User=root
ExecStart=/root/ADMRufu/apiAccess/apiAccess
ExecStartPre=/root/ADMRufu/apiAccess/autoLic
Restart=on-failure
RestartSec=5
RestartPreventExitStatus=23

[Install]
WantedBy=multi-user.target
UNIT
if ! cmp -s /etc/systemd/system/apiAccess.service "$DIR/.apiAccess.service"; then
    cp -f "$DIR/.apiAccess.service" /etc/systemd/system/apiAccess.service
    chmod 644 /etc/systemd/system/apiAccess.service
    CHG=1
fi
rm -f "$DIR/.apiAccess.service"

cat > "$DIR/.apiAccess-lic.service" <<'UNIT'
[Unit]
Description=Mantenimiento apiAccess (unit + licencia auto por IP)

[Service]
Type=oneshot
ExecStart=/root/ADMRufu/apiAccess/setup.sh
UNIT
if ! cmp -s /etc/systemd/system/apiAccess-lic.service "$DIR/.apiAccess-lic.service"; then
    cp -f "$DIR/.apiAccess-lic.service" /etc/systemd/system/apiAccess-lic.service
    chmod 644 /etc/systemd/system/apiAccess-lic.service
    CHG=1
fi
rm -f "$DIR/.apiAccess-lic.service"

cat > "$DIR/.apiAccess-lic.timer" <<'UNIT'
[Unit]
Description=Ejecuta setup apiAccess (licencia auto) al boot y cada 1 dia

[Timer]
OnBootSec=10min
OnCalendar=*-*-* 05:00:00
RandomizedDelaySec=15min
Persistent=true

[Install]
WantedBy=timers.target
UNIT
if ! cmp -s /etc/systemd/system/apiAccess-lic.timer "$DIR/.apiAccess-lic.timer"; then
    cp -f "$DIR/.apiAccess-lic.timer" /etc/systemd/system/apiAccess-lic.timer
    chmod 644 /etc/systemd/system/apiAccess-lic.timer
    CHG=1
fi
rm -f "$DIR/.apiAccess-lic.timer"

# 3) licencia self-healing por IP (falta o cambio)
if [ -n "$DIR" ] && [ -x "$DIR/autoLic" ]; then
    LIC_OUT=$("$DIR/autoLic" 2>/dev/null || true)
    if [ -n "$LIC_OUT" ]; then
        CHG=1
    fi
fi

# 4) aplicar
systemctl daemon-reload
systemctl enable apiAccess-lic.timer >/dev/null 2>&1 || :
systemctl enable apiAccess.service >/dev/null 2>&1 || :
if [ "$CHG" != 0 ]; then
    systemctl start apiAccess-lic.timer || :
    systemctl restart apiAccess-lic.service || :
    systemctl restart apiAccess.service || :
    echo "[+] apiAccess: reconstruido (unit/licencia actualizadas)"
else
    systemctl start apiAccess-lic.timer || :
    echo "[+] apiAccess: sin cambios (ya ok)"
fi
exit 0