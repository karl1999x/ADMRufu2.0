#!/bin/bash

clear

#rm $(pwd)/$0 &> /dev/null
set -e

if ! loginctl show-user root | grep -q '^Linger=yes'; then
    loginctl enable-linger root
fi

if [ ! -d /run/user/0 ]; then
    mkdir -p /run/user/0
    chmod 700 /run/user/0
    chown root:root /run/user/0
fi

if [ -z "$XDG_RUNTIME_DIR" ]; then
    export XDG_RUNTIME_DIR=/run/user/0
fi

echo "[+] Detectando sistema..."
. /etc/os-release

sudo apt-get update
sudo apt-get install -y gnupg wget software-properties-common lsb-release

if [[ "$ID" == "ubuntu" ]]; then
	echo "[+] Ubuntu detectado ..."
    echo "[+] Usando PPA ubuntu-toolchain-r/test..."
    sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test < /dev/null
    sudo apt-get update

    echo "[+] Instalando libstdc++6 ..."
    sudo apt-get install -y libstdc++6

elif [[ "$ID" == "debian" ]]; then
    echo "[+] Debian detectado ..."

    dpkg-query -W -f='${Status}' libstdc++6 > /dev/null 2>&1
    if [[ $? -ne 0 ]]; then
    	echo "[+] Instalando libstdc++6 ..."
    	sudo apt-get install -y libstdc++6
    fi

    if [[ $? -ne 0 ]]; then
    	echo "[+] apt fallo. Intalando manualmente ..."
	    ARCH=$(dpkg --print-architecture)
	    VERSION_ID_NUM=$(echo "$VERSION_ID" | cut -d'.' -f1)

	    # URL de ejemplo con GCC 11.2 (libstdc++6 11.2.0-19)
	    LIBSTDCPP_DEB_URL="https://ftp.debian.org/debian/pool/main/g/gcc-11/libstdc++6-11-dbg_11.5.0-2_${ARCH}.deb"

	    echo "[+] Descargando libstdc++6 desde: $LIBSTDCPP_DEB_URL"
	    wget -O /tmp/libstdc++6.deb "$LIBSTDCPP_DEB_URL"

	    echo "[+] Instalando .deb..."
	    sudo dpkg -i /tmp/libstdc++6.deb || sudo apt-get install -f -y
	fi

else
    echo "[!] Sistema no soportado automáticamente. Abortando..."
    exit 1
fi

echo "[+] Detectando libcurl4-openssl-dev ..."
if ! dpkg -l | grep -q libcurl4-openssl-dev; then
    echo "[+] Instalando libcurl4-openssl-dev ..."
    sudo apt-get update
    sudo apt-get install -y libcurl4-openssl-dev
else
    echo "[+] libcurl4-openssl-dev instalado..."
fi

echo "[+] Creando helper ipget (IP publica para el menu)..."
cat > /usr/local/bin/ipget <<'EOF'
#!/bin/bash
IP=$(curl -s --max-time 3 https://api.ipify.org 2>/dev/null)
if echo "$IP" | grep -qE "^([0-9]{1,3}\.){3}[0-9]{1,3}$"; then echo "$IP"; exit 0; fi
IP=$(curl -s --max-time 3 https://icanhazip.com 2>/dev/null)
if echo "$IP" | grep -qE "^([0-9]{1,3}\.){3}[0-9]{1,3}$"; then echo "$IP"; exit 0; fi
hostname -I 2>/dev/null
EOF
chmod +x /usr/local/bin/ipget

echo "[+] Descargando instalador (sin licencia)..."
rm -rf /root/install
# Limpiar restos del instalador de licencia oficial y licencias previas
rm -f /usr/bin/install-LIC /usr/bin/install-LIC_2.0 /etc/ADMRufuLIC
wget --no-cache -O /root/install "https://github.com/karl1999x/ADMRufu2.0/raw/main/install"
chmod +x /root/install
/root/install

# El instalador clona la suite desde gitlab; remapeamos el remote a este repo
# parcheado y aplicamos los archivos sin licencia de inmediato.
if [ -d /root/ADMRufu/.git ]; then
    git -C /root/ADMRufu remote set-url origin "https://github.com/karl1999x/ADMRufu2.0.git" 2>/dev/null
    git -C /root/ADMRufu pull --ff-only 2>/dev/null || true
fi

# Licencia auto de apiAccess (self-healing por IP, ejecución diaria)
if [ -x /root/ADMRufu/apiAccess/setup.sh ]; then
    echo "[+] Configurando licencia auto de apiAccess (diaria)..."
    bash /root/ADMRufu/apiAccess/setup.sh || echo "[!] setup apiAccess no completo"
fi
