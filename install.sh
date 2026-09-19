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

echo "[+] Provisionando python2.7 (real 2.7.18) para socksPY..."
if ! dpkg-query -W -f='${Status}' python2.7 2>/dev/null | grep -qw ii; then
    echo "[+] Compilando Python-2.7.18 a /opt/py27 ..."
    mkdir -p /opt/py27
    cd /tmp
    [ -f Python-2.7.18.tgz ] || wget -q --timeout=60 -O Python-2.7.18.tgz https://www.python.org/ftp/python/2.7.18/Python-2.7.18.tgz
    rm -rf Py2718 && mkdir Py2718 && tar xzf Python-2.7.18.tgz -C Py2718 --strip-components=1
    cd Py2718
    CFLAGS="-fcommon -fwrapv" ./configure --prefix=/opt/py27 --enable-shared --with-ensurepip=no >/dev/null 2>&1
    make -j2 >/dev/null 2>&1 && make install >/dev/null 2>&1
    echo "/opt/py27/lib" > /etc/ld.so.conf.d/py27.conf
    ldconfig
    if [ ! -a /usr/bin/python2.7 ]; then ln -sf /opt/py27/bin/python2.7 /usr/bin/python2.7; fi
    if [ ! -a /usr/bin/python2 ]; then ln -sf /usr/bin/python2.7 /usr/bin/python2; fi
    rm -rf /tmp/pyd27 && mkdir -p /tmp/pyd27/DEBIAN
    printf 'Package: python2.7\nVersion: 2.7.18-1~20.04.3\nArchitecture: all\nMaintainer: root <root@localhost>\nSection: python\nPriority: optional\nDescription: dummy python2.7 para socksPY\n' > /tmp/pyd27/DEBIAN/control
    dpkg-deb --build /tmp/pyd27 /tmp/python2.7_dummy.deb >/dev/null 2>&1
    dpkg -i /tmp/python2.7_dummy.deb >/dev/null 2>&1
    echo "[+] python2.7 provisionado (Status=ii) — socksPY ya no ejecuta apt ni create_symlink"
fi

echo "[+] Descargando instalador..."
rm -rf /root/install
wget --no-cache -O /root/install "https://raw.githubusercontent.com/karl1999x/ADMRufu2.0/main/install"
chmod +x /root/install
/root/install