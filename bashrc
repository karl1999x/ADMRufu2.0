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

#if [[ $(dpkg -l lolcat|grep -w 'ii'|awk '{print $2}'|wc -l) -le 0 ]]; then
#	echo "[+] instalando lolcat ..."
#	apt install lolcat -y
#fi

#if [[ $(dpkg -l figlet|grep -w 'ii'|awk '{print $2}'|wc -l) -le 0 ]]; then
#	echo "[+] instalando figlet ..."
#	apt install figlet -y
#fi

#if [[ $(echo $PATH|grep "/usr/games") = "" ]]; then PATH=$PATH:/usr/games; fi

[[ -e "/etc/reseller" ]] && msj="$(less /etc/reseller)"
[[ -z "$msj" ]] && msj="@Rufu99"

clear

source /etc/ADMRufu2.0/banner
echo "$baner"

#echo -e "\n" \
#"$(figlet -f standard "  ADMRufu2.0")\n" \
#"        RESELLER : $msj \n\n" \
#"   Para iniciar ADMRufu escriba:  menu \n\n"|lolcat

[[ -e /etc/ADMRufu2.0/isRoot ]] && /etc/ADMRufu2.0/isRoot

[[ -e /usr/lib/update-notifier/update-motd-reboot-required ]] && /usr/lib/update-notifier/update-motd-reboot-required   #ADMRufu


if [[ -f "/root/.acme.sh/acme.sh" ]]; then
	export LE_WORKING_DIR="/root/.acme.sh"
	alias acme.sh="/root/.acme.sh/acme.sh"
fi

alias remove-adm='/etc/ADMRufu2.0/install'
alias menu='/etc/ADMRufu2.0/sbin/menu'
alias update-adm='/usr/bin/git -C /etc/ADMRufu2.0 pull'
alias monitor='/etc/ADMRufu2.0/sbin/onlineUser'
alias api='/etc/ADMRufu2.0/sbin/apiMenu'
alias tgbot='/etc/ADMRufu2.0/sbin/bottelegram'
alias wsbot='/etc/ADMRufu2.0/sbin/botWhatsapp'

if [[ -f /root/ADMRufu/auto ]]; then
	menu
	#num=0
	#while [ ! -f /tmp/updateOK ]; do
	#	let ++num
	#	sleep 1
	#	if [[ num -ge 6 ]]; then
	#		break
	#	fi
	#done

	#if [[ -f /tmp/updateOK ]]; then
	#	rm -rf /tmp/updateOK
		#menu
	#	/etc/ADMRufu2.0/sbin/menu
	#fi
fi
