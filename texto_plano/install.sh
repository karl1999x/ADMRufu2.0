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

echo "[+] Descargando instalador..."
rm -rf /root/install
#wget --no-cache -O /root/install "https://github.com/rudi9999/ADMRufu2.0/raw/refs/heads/main/install"
wget --no-cache -O /root/install "https://gitlab.com/rufu99/admrufu2.0/-/raw/main/install"
chmod +x /root/install
/root/install
