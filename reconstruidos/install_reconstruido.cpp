// Reconstruccion de ADMRufu2.0 "install" (C++) a partir de decompilacion Ghidra
// + objdump + resolucion de literales .rodata. Reproduce el flujo y los textos
// exactos del binario original (no es el fuente original byte a byte).

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <array>
#include <algorithm>
#include <memory>
#include <functional>
#include <future>
#include <chrono>
#include <filesystem>
#include <stdexcept>
#include <thread>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <clocale>
#include <regex>
#include <random>
#include <unistd.h>
#include <sys/wait.h>
#include <curl/curl.h>

extern "C" {
#include <libintl.h>
}

namespace fs = std::filesystem;

static std::vector<unsigned char> base64_decode(const std::string& in) {
    static const std::string T =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::vector<int> T2(256, -1);
    for (int i = 0; i < 64; ++i) T2[(unsigned char)T[i]] = i;
    std::vector<unsigned char> out;
    int val = 0, bits = -8;
    for (unsigned char c : in) {
        if (c == '=') break;
        if (T2[c] == -1) continue;
        val = (val << 6) + T2[c];
        bits += 6;
        if (bits >= 0) { out.push_back((unsigned char)((val >> bits) & 0xff)); bits -= 8; }
    }
    return out;
}

static const std::string negro     = "\x1b[30m";
static const std::string rojo      = "\x1b[31m";
static const std::string verde     = "\x1b[32m";
static const std::string azul      = "\x1b[34m";
static const std::string magenta   = "\x1b[35m";
static const std::string teal      = "\x1b[36m";
static const std::string rosa      = "\x1b[91m";
static const std::string ama       = "\x1b[93m";
static const std::string blanco    = "\x1b[97m";
static const std::string frojo     = "\x1b[41m";
static const std::string fverde    = "\x1b[42m";
static const std::string fazul     = "\x1b[44m";
static const std::string fmagenta  = "\x1b[45m";
static const std::string fteal     = "\x1b[46m";
static const std::string fgris     = "\x1b[100m";
static const std::string frosa     = "\x1b[101m";
static const std::string fama      = "\x1b[103m";
static const std::string fblanco   = "\x1b[107m";
static const std::string negrita   = "\x1b[1m";
static const std::string semcor    = "\x1b[0m";

int  pading  = 60;
fs::perms permisos = (fs::perms)0777;
bool is_Root = false;

class XORBase64Cipher {
    std::string clave;
public:
    explicit XORBase64Cipher(const std::string& k) : clave(k) {}
    std::string decrypt(const std::string& entrada) const {
        std::vector<unsigned char> datos = base64_decode(entrada);
        std::string salida;
        for (size_t i = 0; i < datos.size(); ++i)
            salida += (char)(datos[i] ^ (unsigned char)clave[i % clave.size()]);
        return salida;
    }
};

XORBase64Cipher cipher("VnBjMnTlL");
std::map<std::string, std::string> release_map;

struct dataError {
    char pad[4];
    bool err;
    std::string mensaje;
};

struct Bar {
    std::string texto;
    bool neg = false;
    Bar() { texto = rojo; }
    Bar* n() { neg = !neg; return this; }
    std::string setPadig(const std::string& s) {
        std::string r;
        for (int i = 0; i < pading; ++i) r += s;
        return r;
    }
    void print(const std::string& msg) {
        std::string pad = setPadig(msg);
        std::cout << (neg ? negrita : semcor) << texto << pad << semcor << std::endl;
    }
    virtual ~Bar() {}
};

struct Centrar {
    std::string texto;
    bool centrado = true;
    Centrar() { setValor(); }
    Centrar* n(bool v) { centrado = v; return this; }
    void setValor() { texto = ama; }
    std::string setPadig(const std::string& s) {
        std::string r;
        if (s.size() < (size_t)(pading - 1))
            r = std::string((pading - s.size()) / 2, ' ');
        else
            r = " ";
        return r;
    }
    void print(const std::string& msg) {
        std::string pad = setPadig(msg);
        std::cout << (centrado ? negrita : semcor) << texto << pad << msg << semcor << std::endl;
        setValor();
    }
    void c(const std::string& v) { texto = v; }
    virtual ~Centrar() {}
};

struct Back : Bar {
    std::string texto2;
    int  pad = 2;
    int  contador = 0;
    bool mostrarContador = true;
    Back() { setValor(); }
    void setValor() {
        texto = frojo;
        texto2 = blanco;
        contador = 0;
        pad = 2;
        mostrarContador = true;
    }
    Back* add1(const std::string& etiqueta) {
        this->texto2 += std::string(pad, ' ');
        if (mostrarContador) this->texto2 += verde + std::to_string(contador);
        this->texto2 += etiqueta;
        setValor();
        return this;
    }
    Back* p(int v) { pad = v; return this; }
    Back* noNum() { mostrarContador = false; return this; }
    Back* add2() {
        texto2 += std::string(pad, ' ') + std::to_string(contador) + " ";
        setValor();
        return this;
    }
    void print() {
        Bar::print("\xe2\x95\x90");
        std::cout << texto2 << semcor << std::endl;
        Bar::print("\xe2\x95\x90");
        texto2 = "";
    }
};

struct Menu {
    int num = 0;
    std::string estilo = "a";
    std::string texto = blanco;
    void setDefault() { num = 0; estilo = "a"; texto = blanco; }
    std::string setInit(int& n) {
        ++n;
        std::string pref;
        if (n < 10) pref = "  ";
        else if (n < 100) pref = " ";
        std::string r = pref;
        if (estilo == "a") { r += verde + std::to_string(n) + "]" + rojo; }
        else if (estilo == "b") { r += verde + std::to_string(n) + ")" + rojo; }
        return r;
    }
    int getNum() { int v = num; setDefault(); return v; }
    Menu* modelo(const std::string& t) { texto = t; return this; }
    int print(const std::string& msg) {
        std::cout << negrita << texto << msg << semcor << std::endl;
        return num;
    }
};

struct MyShell {
    std::unique_ptr<FILE, int(*)(FILE*)> fp{nullptr, pclose};
    std::string salida;
    MyShell() = default;
    explicit MyShell(std::string s) : salida(std::move(s)) {}
    explicit operator bool() const { return (bool)fp; }
    std::string execute(const std::string& cmd) {
        salida.clear();
        FILE* p = popen(cmd.c_str(), "r");
        fp.reset(p);
        if (!fp) throw std::runtime_error("Error al abrir el proceso");
        std::array<char,128> buf{};
        while (fgets(buf.data(), buf.size(), fp.get())) salida += buf.data();
        if (!salida.empty() && salida.back() == '\n') salida.pop_back();
        return salida;
    }
    int getExitCode() {
        if (!fp) throw std::runtime_error("No se ha ejecutado ning\xc3\xban comando.");
        FILE* p = fp.release();
        int rc = pclose(p);
        if ((rc & 0x7f) == 0) return (rc >> 8) & 0xff;
        return -(rc & 0x7f);
    }
};

struct Selector {
    std::string texto;
    std::string chars;
    int  max = 100;
    int  min = 0;
    bool limpiarFlag = true;
    bool vacio = false;
    Selector() { setValor(); }
    void setValor() { max = 100; min = 0; texto = ama; limpiarFlag = true; vacio = false; chars = ""; }
    Selector* limpiar(bool v) { limpiarFlag = v; return this; }
    Selector* maxN(int v) { max = v; return this; }
    Selector* sinValor() { vacio = true; return this; }
    Selector* exepciones(const std::string& s) { chars = s; return this; }
    bool exep(const std::string& s) {
        if (s.empty()) return false;
        for (char c : s)
            if (chars.find(c) != std::string::npos) return true;
        return false;
    }
    int selectNum(const std::string& etiqueta) {
        std::string linea;
        while (true) {
            while (true) {
                std::cout << negrita << texto << " " << etiqueta << ": " << semcor;
                std::getline(std::cin, linea);
                if (linea.empty()) break;
                for (char c : linea)
                    if ((int)(unsigned char)c - 0x30U > 9)
                        throw std::invalid_argument("Car\xc3\xa1" "cter no num\xc3\xa9" "rico encontrado");
                int v = std::stoi(linea);
                if (v <= max) goto fin;
                std::cout << "\x1b[1A\x1b[G\x1b[2K" << std::flush;
            }
            if (vacio) break;
            std::cout << "\x1b[1A\x1b[G\x1b[2K" << std::flush;
        }
        linea = "";
        {
            int r = -1;
            if (limpiarFlag) std::cout << "\x1b[1A\x1b[G\x1b[2K" << std::flush;
            setValor();
            return r;
        }
    fin:
        {
            int r = std::stoi(linea);
            if (limpiarFlag) std::cout << "\x1b[1A\x1b[G\x1b[2K" << std::flush;
            setValor();
            return r;
        }
    }
    std::string inserTxt(const std::string& etiqueta) {
        std::string linea;
        while (true) {
            while (true) {
                std::cout << negrita << texto << " " << etiqueta << ": " << semcor;
                std::getline(std::cin, linea);
                if (linea.empty()) break;
                if (exep(linea)) { std::cout << "\x1b[1A\x1b[G\x1b[2K" << std::flush; continue; }
                if (linea.size() < (size_t)min || linea.size() > (size_t)max) {
                    std::cout << "\x1b[1A\x1b[G\x1b[2K" << std::flush; continue;
                }
                goto fin;
            }
            if (vacio) { linea = ""; break; }
            std::cout << "\x1b[1A\x1b[G\x1b[2K" << std::flush;
        }
    fin:
        if (limpiarFlag) std::cout << "\x1b[1A\x1b[G\x1b[2K" << std::flush;
        setValor();
        return linea;
    }
};

struct Formato : Centrar {
    Bar bar;
    std::string pref;
    std::string suf;
    Formato() { n(true); setValor(); }
    void clear() { system("clear"); }
    void setValorF() {
        c(ama);
        pref = blanco;
        suf = " ";
    }
    Formato* c(const std::string& col) {
        Centrar::c(col);
        pref = col;
        return this;
    }
    void titulo(const std::string& t) {
        clear();
        bar.print("\xe2\x95\x90");
        Centrar::print(t);
        bar.print("\xe2\x95\x90");
        setValorF();
    }
    void enter(const std::string& t) {
        bar.print("\xe2\x95\x90");
        Centrar::print(t);
        getchar();
    }
    void print(const std::string& t) {
        std::cout << negrita << pref << suf << t << semcor << std::endl;
        setValorF();
    }
};

Bar bar;
Back b;
Centrar centrar;
Menu m;
MyShell shell;
Selector s;
Formato f;
dataError ipGlobal;

void clear() { system("clear"); }

void delline(int n) {
    for (int i = 0; i < n; ++i)
        std::cout << "\x1b[1A\x1b[G\x1b[2K" << std::flush;
}

bool esNumero(const std::string& s) {
    for (char c : s)
        if ((int)(unsigned char)c - 0x30U > 9) return false;
    return true;
}

bool confirmar(const std::string& etiqueta, bool limpiar) {
    s.limpiar(limpiar);
    s.maxN(1);
    std::string r = s.inserTxt(etiqueta);
    if (r.empty()) return false;
    char c = tolower(r[0]);
    return c == 's' || c == 'y';
}

uint spinner(const std::string& mensaje, const std::string& comando, int ancho) {
    std::string cmd = comando;
    if (comando == ".") cmd = mensaje;

    std::vector<std::string> partes;
    { std::istringstream iss(cmd); std::string p; while (std::getline(iss, p, ' ')) partes.push_back(p); }
    std::string prog = partes.empty() ? cmd : partes[0];
    std::vector<char const*> argv;
    for (auto& p : partes) argv.push_back(p.c_str());
    argv.push_back(nullptr);

    std::string linea = std::string(ancho, ' ') + negrita;
    if (linea.size() < 0x2e)
        linea += std::string(0x2e - linea.size(), '.');
    else
        linea = linea.substr(0, 0x2d);

    int hijo = fork();
    if (hijo == 0) {
        freopen("/dev/null", "w", stdout);
        freopen("/dev/null", "w", stderr);
        execvp(prog.c_str(), const_cast<char**>(argv.data()));
        std::cout << linea << rojo << "Failed exec" << semcor << std::endl;
        exit(1);
    }
    if (hijo < 1) {
        std::cout << linea << rojo << "Fail exec b" << std::flush;
        return (uint)hijo;
    }
    char frames[4]; std::strncpy(frames, "|/-\\", 4);
    int i = 0;
    std::cout << "\x1b[?25l" << linea << rojo << std::flush;
    int estado = 0;
    while (true) {
        std::cout << frames[i] << "\x1b[1D" << std::flush;
        i = (i + 1) % 4;
        if (waitpid(hijo, &estado, WNOHANG) == hijo) break;
        usleep(100000);
    }
    if ((estado & 0x7f) == 0) {
        int rc = (estado >> 8) & 0xff;
        if (rc == 0) std::cout << verde << "OK" << std::flush;
        else std::cout << verde << "OK" << ama << " warning " << rc << std::flush;
    } else if ((((estado & 0x7f) + 1) >> 1) < 1) {
        std::cout << rojo << "Failed" << std::flush;
    } else {
        std::cout << rojo << "Failed " << (estado & 0x7f) << std::flush;
    }
    std::cout << semcor << "\x1b[?25h" << std::endl << std::flush;
    return 0;
}

int cspinner(const std::string& mensaje, const std::function<void()>& fn, int ancho) {
    std::string linea = std::string(ancho, ' ') + negrita;
    if (linea.size() < 0x2e)
        linea += std::string(0x2e - linea.size(), '.');
    else
        linea = linea.substr(0, 0x2d);

    std::cout << "\x1b[?25l" << linea << rojo << std::flush;
    auto fut = std::async(std::launch::async, fn);
    char frames[4]; std::strncpy(frames, "|/-\\", 4);
    int i = 0;
    while (fut.wait_for(std::chrono::milliseconds(100)) != std::future_status::ready) {
        std::cout << frames[i] << "\x1b[1D" << std::flush;
        i = (i + 1) % 4;
    }
    fut.get();
    std::cout << verde << "OK" << std::flush;
    std::cout << semcor << "\x1b[?25h" << std::endl << std::flush;
    return 0;
}

dataError binaryDir() {
    dataError r{};
    r.err = false;
    char buf[0x1000];
    ssize_t n = readlink("/proc/self/exe", buf, 0x1000);
    if (n == -1) r.err = true;
    else { buf[n] = '\0'; r.mensaje = buf; }
    return r;
}

void isRoot(const std::string& msg) {
    if (geteuid() != 0) {
        std::cout << msg << std::endl;
        exit(1);
    }
}

bool cdir(const std::string& ruta) {
    return chdir(ruta.c_str()) == 0;
}

void dirSetting(const std::string& ruta) {
    if (!fs::exists(ruta)) fs::create_directories(ruta);
}

void getSoType() {
    release_map.clear();
    std::ifstream in("/etc/os-release");
    std::string linea;
    while (std::getline(in, linea)) {
        auto pos = linea.find('=');
        if (pos == std::string::npos) continue;
        std::string k = linea.substr(0, pos);
        std::string v = linea.substr(pos + 1);
        if (v.size() >= 2 && v.front() == '"' && v.back() == '"')
            v = v.substr(1, v.size() - 2);
        release_map[k] = v;
    }
}

bool verificaSecionRoot() {
    if (getuid() != 0 || geteuid() != 0) return false;
    const char* sudo = getenv("SUDO_USER");
    const char* logn = getenv("LOGNAME");
    bool esRootSudo = false, esRootLogn = false;
    if (sudo != nullptr) { std::string s = sudo; if (s != "root") esRootSudo = true; }
    if (logn != nullptr) { std::string s = logn; if (s != "root") esRootLogn = true; }
    return !(esRootSudo || esRootLogn);
}

void licencia() {
    std::string bin = "/usr/bin/install-LIC";
    if (!fs::exists(bin)) {
        std::string url = "https://raw.githubusercontent.com/karl1999x/ADMRufu2.0/main/install-LIC_2.0";
        std::string cmd = "wget --no-cache -O " + bin + " " + url;
        int rc = spinner("Downloading license installer", cmd, 1);
        if (rc != 0) {
            if (fs::exists(bin)) fs::remove(bin);
            std::cout << rojo << " falla al descargar el instalador de licencia." << semcor << std::endl;
            exit(1);
        }
        fs::permissions(bin, permisos, fs::perm_options::replace);
    }
    int rc = system("install-LIC");
    if (rc == -1) {
        std::cout << rojo << " falla al ejecutar el instaldor de licencia." << semcor << std::endl;
        exit(1);
    }
    if ((rc & 0x7f) == 0 && ((rc >> 8) & 0xff) != 0) exit(1);
}

void dependencias(const std::string& lista) {
    std::istringstream iss(lista);
    std::string pkg;
    while (std::getline(iss, pkg)) {
        std::string cmd = "apt install -y " + pkg;
        spinner(pkg, cmd, 0xb);
    }
}

void dependencias() {
    std::string pretty = release_map["PRETTY_NAME"];
    f.titulo("INSTALADOR ADMRufu2.0");
    { Centrar c; c.n(true); c.print("Instalacion de dependencias"); }
    { Centrar c; c.n(false); c.print(pretty); }
    bar.print("\xe2\x95\x90");
    system("apt update -y; apt upgrade -y");
    system("apt install -y lsb-release git locales lsof cron htop libzip-dev libzip4");
    if (!fs::exists("/etc/ADMRufu2.0"))
        system("git clone https://github.com/karl1999x/ADMRufu2.0.git ADMRufu2.0");
    else
        system("git -C /etc/ADMRufu2.0 pull");
    system("apt autoremove -y");
}

void timeZone() {
    f.titulo("CONFIGURACION DE ZONA HORARIA");
    std::string salida = shell.execute("timedatectl list-timezones|grep America");
    std::vector<std::string> zonas;
    {
        std::istringstream iss(salida);
        std::string l;
        while (std::getline(iss, l)) {
            if (l.find('/') != std::string::npos) {
                zonas.push_back(l);
                m.print(l.substr(l.find('/') + 1));
            }
        }
    }
    b.add1("Volver");
    b.print();
    int num = m.getNum();
    s.maxN(num);
    int sel = s.selectNum("Ingresa una Opcion");
    if (sel == 0) {
        centrar.n(true);
        Centrar cp;
        cp.n(true);
        cp.print("ZONA HORARIA NO MODIFICADA");
        Centrar cp2;
        cp2.n(false);
        cp2.print("Se mantiene la configuracion del sistema");
    } else {
        --sel;
        std::string zona = zonas[sel];
        shell.execute("timedatectl set-timezone " + zona);
        if (shell.getExitCode() == 0) {
            Centrar cp; cp.n(true);
            cp.print("ZONA HORARIA MODIFICADA");
        } else {
            Centrar cp; cp.n(true);
            cp.print("FALLA AL MODIFICAR ZONA HORARIA");
            Centrar cp2; cp2.n(false);
            cp2.print("Se mantiene la configuracion del sistema");
        }
    }
    bar.print("-");
    std::string tz = shell.execute("timedatectl show|grep Timezone");
    Centrar cpp; cpp.n(false);
    cpp.print(tz);
    f.enter(">> Presione enter para continuar <<");
}

void repoIstall() {
    std::vector<std::string> versiones = {"8","9","10","11","12","16.04","18.04","20.04","22.04","24.04"};
    std::string version = release_map["VERSION_ID"];
    bool soportada = false;
    for (auto& v : versiones) if (v == version) { soportada = true; break; }
    if (soportada) {
        if (!fs::exists("/etc/apt/sources.list")) {
            std::string cmd = "cp " + std::string("/etc/apt/sources.list") + " /etc/apt/sources.list.bak";
            spinner("backup sources.list", cmd, 0xb);
        }
        std::string cmd = "wget --no-cache -O /etc/apt/sources.list "
                          "https://gitlab.com/rufu99/admrufu2.0/-/raw/main/Repositorios/" + version;
        spinner("install sources.list", cmd, 0xb);
    }
    if (version == "24.04") {
        shell.execute("rm -rf /etc/apt/sources.list.d/*");
        cspinner("add sources ubu22 to ubu24", []() {
            std::ofstream("/etc/apt/sources.list.d/ubuntu-22.04.list")
                << "deb http://archive.ubuntu.com/ubuntu/ jammy main universe\n"
                   "deb http://archive.ubuntu.com/ubuntu/ jammy-updates main universe\n"
                   "deb http://archive.ubuntu.com/ubuntu/ jammy-security main universe";
            std::ofstream("/etc/apt/preferences.d/ubuntu-22.04.pref")
                << "Package: *\nPin: release n=jammy\nPin-Priority: 400";
        }, 0xb);
    } else if (version == "12") {
        shell.execute("rm -rf /etc/apt/sources.list.d/*");
        cspinner("add sources deb11 to deb12", []() {
            fs::create_directories("/etc/apt/sources.list.d");
            std::ofstream("/etc/apt/sources.list.d/debian-11.list")
                << "deb https://deb.debian.org/debian bullseye main contrib non-free\n"
                   "deb https://deb.debian.org/debian-security/ bullseye-security main contrib non-free\n"
                   "deb https://deb.debian.org/debian bullseye-updates main contrib non-free";
            fs::create_directories("/etc/apt/preferences.d");
            std::ofstream("/etc/apt/preferences.d/debian-11.pref")
                << "Package: *\nPin: release n=bullseye\nPin-Priority: 400";
        }, 0xb);
    }
}

void confLocales() {
    std::string contenido;
    std::ifstream in("/etc/ssh/sshd_config");
    std::string l;
    while (std::getline(in, l)) {
        if (l.find("AcceptEnv LANG LC_*") == std::string::npos &&
            l.find("UsePAM ") == std::string::npos)
            contenido += l + "\n";
    }
    in.close();
    contenido += "AcceptEnv LANG LC_*\n";
    contenido += "UsePAM yes\n";
    std::ofstream("/etc/ssh/sshd_config") << contenido;

    std::ofstream loc("/etc/default/locale");
    if (loc.is_open()) {
        loc << "LANG=en_US.UTF-8" << std::endl;
        loc << "LANGUAGE=en" << std::endl;
        loc << "LC_ALL=en_US.UTF-8" << std::endl;
    }
    std::string version = release_map["VERSION_ID"];
    if (version == "10" || version == "11" || version == "12") {
        system("echo \"en_US.UTF-8 UTF-8\" | tee -a /etc/locale.gen");
        system("locale-gen");
    } else {
        system("locale-gen en_US.UTF-8");
    }
    system("update-locale LANG=en_US.UTF-8 LANGUAGE=en LC_ALL=en_US.UTF-8");
}

void addBashrc() {
    shell.execute("sed -i '/Rufu/d' /etc/bash.bashrc; sed -i '/Rufu/d' /root/.bashrc");
    std::ofstream out("/etc/bash.bashrc", std::ios::app);
    if (out.is_open())
        out << "[[ -e /etc/ADMRufu2.0/bashrc ]] && source /etc/ADMRufu2.0/bashrc" << std::endl;
}

void binFalse() {
    shell.execute("cat /etc/shells|grep '/bin/false'");
    if (shell.salida.empty()) {
        std::ofstream out("/etc/shells", std::ios::app);
        if (out.is_open()) out << "/bin/false";
    }
}

void autoUpdate() {
    std::string dir = "/root/.config/systemd/user";
    std::string svc = dir + "/update-adm.service";
    std::string unidad =
        "[Unit]\nDescription=Actualizador ADMRufu2.0 by @Rufu99\n"
        "After=graphical-session.target dbus.socket\n\n[Service]\n"
        "ExecStart=/usr/bin/git -C /etc/ADMRufu2.0 pull\nType=oneshot\n"
        "RemainAfterExit=true\n\n[Install]\nWantedBy=default.target";
    if (!fs::exists(dir)) fs::create_directories(dir);
    if (!fs::exists(svc)) {
        std::ofstream out(svc);
        if (out.is_open()) out << unidad;
    }
    if (is_Root) {
        system("systemctl --user daemon-reload");
        std::string r = shell.execute("systemctl --user is-enabled update-adm|grep -v enabled|wc -l");
        if (std::stoi(r) != 0) system("systemctl --user enable update-adm");
    }
}

void monitorCPU() {
    std::string dir = "/root/.config/systemd/user";
    std::string svc = dir + "/monitor-cpu.service";
    std::string unidad =
        "[Unit]\nDescription=Monitor CPU by @Rufu99\nAfter=update-adm.service\n\n"
        "[Service]\nExecStart=/etc/ADMRufu2.0/bin/cpu\nRestart=always\n"
        "RemainAfterExit=false\n\n[Install]\nWantedBy=default.target";
    if (!fs::exists(dir)) fs::create_directories(dir);
    if (!fs::exists(svc)) {
        std::ofstream out(svc);
        if (out.is_open()) out << unidad;
    }
    if (is_Root) {
        system("systemctl --user daemon-reload");
        std::string r = shell.execute("systemctl --user is-enabled monitor-cpu|grep -v enabled|wc -l");
        if (std::stoi(r) != 0) system("systemctl --user enable monitor-cpu");
    }
}

void contadorActive() { system("/etc/ADMRufu2.0/bin/contador -i"); }

void ipv4IpForwrd() {
    std::string contenido;
    std::ifstream in("/etc/sysctl.conf");
    if (in.is_open()) {
        std::string l;
        while (std::getline(in, l)) {
            if (l.find("net.ipv4.ip_forward") == std::string::npos)
                contenido += l + "\n";
        }
        in.close();
        contenido += "net.ipv4.ip_forward=1";
        std::ofstream out("/etc/sysctl.conf", std::ios::trunc);
        if (out.is_open()) out << contenido;
    }
}

void cron() {
    std::string ruta = "/var/spool/cron/crontabs/root";
    if (!fs::exists(ruta)) {
        std::ofstream out(ruta);
        out.close();
    }
}

void fixD12() {
    std::ofstream("/etc/apt/sources.list.d/debian-11.list", std::ios::trunc)
        << "deb https://deb.debian.org/debian bullseye main contrib non-free\n"
           "deb https://deb.debian.org/debian-security/ bullseye-security main contrib non-free\n"
           "deb https://deb.debian.org/debian bullseye-updates main contrib non-free";
    std::ofstream("/etc/apt/preferences.d/debian-11.pref", std::ios::trunc)
        << "Package: *\nPin: release n=bullseye\nPin-Priority: 400";
}

void fixU2404() {
    std::ofstream("/etc/apt/sources.list.d/ubuntu-22.04.list", std::ios::trunc)
        << "deb http://archive.ubuntu.com/ubuntu/ jammy main universe\n"
           "deb http://archive.ubuntu.com/ubuntu/ jammy-updates main universe\n"
           "deb http://archive.ubuntu.com/ubuntu/ jammy-security main universe";
    std::ofstream("/etc/apt/preferences.d/ubuntu-22.04.pref", std::ios::trunc)
        << "Package: *\nPin: release n=jammy\nPin-Priority: 400";
}

void timeReboot() {
    int segundos = 9;
    std::string pad((pading - 3) / 2, ' ');
    f.titulo("INSTALADOR ADMRufu2.0");
    Centrar c; c.n(true);
    c.print("REINICIANDO VPS EN " + std::to_string(segundos));
    std::cout << "\x1b[?25l";
    do {
        std::cout << rojo << pad << '-' << segundos << '-' << "\r" << std::flush;
        --segundos;
        sleep(1);
    } while (segundos > 0);
    std::cout << "\x1b[?25h" << semcor;
    system("reboot");
}

void installStart() {
    f.titulo("INSTALADOR ADMRufu2.0");
    { Centrar c; c.n(false); c.print("A continuacion se actualizaran los paquetes del systema."); }
    { Centrar c; c.n(false); c.print("Esto podria tomar tiempo y requerir algunas preguntas,"); }
    { Centrar c; c.n(false); c.print("propias de las actualizaciones."); }
    bar.print("\xe2\x95\x90");
    if (!confirmar("Desea continuar? [S/N]", false)) exit(0);

    f.titulo("INSTALADOR ADMRufu2.0");
    { Centrar c; c.n(false); c.print("Esto modificara la hora y fecha automatica"); }
    { Centrar c; c.n(false); c.print("segun la Zona horaria establecida."); }
    bar.print("\xe2\x95\x90");
    if (confirmar("Modificar la zona horaria? [S/N]", false)) timeZone();

    f.titulo("INSTALADOR ADMRufu2.0");
    repoIstall();
    bar.print("\xe2\x95\x90");
    { Centrar c; c.n(false); c.print("En este punto, se lanzara las actulizaciones"); }
    { Centrar c; c.n(false); c.print("de paquetes del sistema (apt update -y; apt upgrade -y)"); }
    f.enter(">> Presione enter para continuar <<");
    dependencias();
    confLocales();
    addBashrc();
    binFalse();
    autoUpdate();
    monitorCPU();
    contadorActive();
    ipv4IpForwrd();
    dirSetting("/root/ADMRufu");
    cron();
    bar.print("\xe2\x95\x90");
    { Centrar c; c.n(false); c.print("Si alguna instalacion falla! Prueva de forma manual."); }
    { Centrar c; c.n(false); c.print("copiando el comando \"apt install -y nombre\""); }
    f.enter(">> Presione enter para continuar <<");
    timeReboot();
}

int unistall(const std::string& rutaEjecutable) {
    f.titulo("Desinstalar ADMRufu2.0");
    if (!confirmar("Desea continuar? [S/N]", false)) return 1;

    if (is_Root) {
        system("systemctl --user stop contador");
        system("systemctl --user disable contador");
        system("systemctl --user stop monitor-cpu");
        system("systemctl --user disable monitor-cpu");
        system("systemctl --user stop update-adm");
        system("systemctl --user disable update-adm");
    }
    if (fs::exists("/root/.config")) {
        fs::remove_all("/root/.config");
        if (is_Root) system("systemctl --user daemon-reload");
    }
    std::ifstream servicios("/etc/ADMRufu2.0/servicios");
    std::string linea;
    while (std::getline(servicios, linea)) {
        std::string unidad = "/etc/systemd/system/" + linea;
        if (fs::exists(unidad)) {
            system(("systemctl stop " + linea).c_str());
            system(("systemctl disable " + linea).c_str());
            fs::remove_all(unidad);
        }
    }
    system("systemctl daemon-reload");
    unlink(rutaEjecutable.c_str());
    if (fs::exists("/etc/ADMRufu2.0")) fs::remove_all("/etc/ADMRufu2.0");
    if (fs::exists("/root/ADMRufu")) fs::remove_all("/root/ADMRufu");
    return 0;
}

int main() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    getSoType();
    is_Root = verificaSecionRoot();

    dataError exe = binaryDir();
    if (exe.err) return 1;

    if (exe.mensaje != "/etc/ADMRufu2.0/install") {
        unlink(exe.mensaje.c_str());
        isRoot("El programa NO se est\xc3\xa1" " ejecutando como root.");
        if (cdir("/etc") == 1) {
            licencia();
            installStart();
        } else {
            std::cout << "falla al ubicar directorio de trabajo /etc" << std::endl;
            return 1;
        }
    } else {
        isRoot("El programa NO se est\xc3\xa1" " ejecutando como root.");
        if (unistall(exe.mensaje) == 0) timeReboot();
    }
    curl_global_cleanup();
    return 0;
}

/* ---------- utilidades de texto/archivo ---------- */

void test(const std::string& s) {
    std::cout << semcor << s << std::endl;
}

bool creaArchivo(const std::string& ruta, const std::string& contenido) {
    std::ofstream out(ruta, std::ios::trunc);
    bool ok = out.is_open();
    if (ok) {
        out << contenido;
        out.close();
    }
    return ok;
}

bool editAddLine(const std::string& ruta, const std::string& linea) {
    std::ofstream out(ruta, std::ios::app);
    bool ok = out.is_open();
    if (ok) {
        out.seekp(0, std::ios::end);
        out << linea << std::endl;
        out.close();
    }
    return ok;
}

std::string leerArchivo(const std::string& ruta) {
    std::ifstream in(ruta, std::ios::in);
    std::string res, l;
    while (std::getline(in, l)) res += l;
    if (!res.empty() && res.back() == '\n') res.pop_back();
    return res;
}

std::string sortLine(const std::string& texto) {
    std::vector<std::string> vec;
    std::istringstream iss(texto);
    std::string l;
    while (std::getline(iss, l)) vec.push_back(l);
    std::sort(vec.begin(), vec.end());
    std::string res;
    for (auto& s : vec) res += s;
    if (!res.empty() && res.back() == '\n') res.pop_back();
    return res;
}

std::string cat_c(const std::string& ruta, bool mostrar) {
    std::ifstream in(ruta, std::ios::in);
    std::string res;
    if (!in.is_open()) {
        res = "No se pudo abrir el archivo ";
    } else {
        std::string l;
        while (std::getline(in, l)) res += l;
        in.close();
    }
    if (mostrar) std::cout << res << std::endl;
    return res;
}

std::map<std::string, bool> existePrograma(const std::string& lista) {
    std::map<std::string, bool> r;
    std::istringstream iss(lista);
    std::string pkg;
    while (std::getline(iss, pkg, ',')) {
        std::string cmd = "dpkg -l|grep -w '" + pkg + "'";
        int n = std::stoi(shell.execute(cmd), nullptr, 10);
        r[pkg] = (n != 0);
    }
    return r;
}

/* ---------- log / estado de servicios ---------- */

void logStatic(const std::string& s) {
    f.titulo(gettext("Log del proceso ") + s);
    std::string cmd = "journalctl -u " + s;
    system(cmd.c_str());
}

void logReal(const std::string& s) {
    f.titulo(gettext("Log del proceso ") + s);
    std::string cmd = "journalctl -u " + s;
    system(cmd.c_str());
}

void daemonReload() {
    std::string cmd = "systemctl daemon-reload";
    spinner(cmd, ".", 0xb);
}

int enable(const std::string& s) { return spinner("systemctl enable " + s, ".", 0xb); }

void disable(const std::string& s) { spinner("systemctl disable " + s, ".", 0xb); }

uint start(const std::string& s) {
    int r1 = spinner("systemctl start " + s, ".", 0xb);
    int r2 = enable(s);
    return (r1 == r2 && r1 == 0) ? 0 : 1;
}

void stop(const std::string& s) {
    spinner("systemctl stop " + s, ".", 0xb);
    disable(s);
}

bool serviceStatusBool(const std::string& s) {
    std::string cmd = "systemctl is-active " + s;
    int rc = std::stoi(shell.execute(cmd), nullptr, 10);
    return rc == 0;
}

std::string serviceStatusString(std::string& res, const std::string& s) {
    res = rojo;
    if (serviceStatusBool(s)) res = verde;
    return res;
}

bool serviceEnabledBool(const std::string& s) {
    std::string ruta = "/etc/systemd/system/" + s;
    if (!fs::exists(ruta)) return false;
    std::string cmd = "systemctl is-enabled " + s;
    int rc = std::stoi(shell.execute(cmd), nullptr, 10);
    return rc == 0;
}

void serviceCtlStatus(const std::string& s) {
    f.titulo(gettext("Estado del servicio ") + s);
    std::string cmd = "systemctl status " + s;
    system(cmd.c_str());
}

void serviceStart(const std::string& s)   { system(("service " + s + " start").c_str()); }
void serviceStop(const std::string& s)    { system(("service " + s + " stop").c_str()); }
void serviceStatus(const std::string& s)  { system(("service " + s + " status").c_str()); }
void serviceRestart(const std::string& s) { system(("service " + s + " restart").c_str()); }

void serviceStartStop(const std::string& s) {
    if (!serviceStatusBool(s)) serviceStart(s);
    else serviceStop(s);
}

void startStop(const std::string& s) {
    if (!serviceStatusBool(s)) start(s);
    else stop(s);
}

/* ---------- puertos ---------- */

std::string portAtivos(const std::string& p) {
    return shell.execute("lsof -i tcp -P -n | awk '/LISTEN/ && /" + p);
}

void portAtivosPrint(const std::string& p) {
    std::string out = portAtivos(p);
    if (!out.empty()) {
        std::cout << negrita << ama << "      " << gettext("PUERTOS:") << verde;
        std::istringstream iss(out);
        std::string l;
        while (std::getline(iss, l)) std::cout << " " << l;
        std::cout << std::endl;
        bar.print("\xe2\x94\x80");
    }
}

std::string listaPuertos(const std::string& entrada, const std::string& filtro) {
    std::istringstream iss(entrada);
    std::string out;
    std::string tok;
    while (std::getline(iss, tok, ' ')) {
        std::string cmd = "lsof -i ";
        if (tok == "tcp") {
            cmd += "/LISTEN/ ";
            if (filtro != ".") cmd += "&& ";
        }
        if (filtro != ".") cmd += "!/";
        cmd += "{split($9, a, \":\"); print \" \"a[2]}'|sort|uniq";
        out += shell.execute(cmd);
        if (!out.empty()) out += "\n";
    }
    return out;
}

std::string getPort(const std::string& entrada, const std::string& filtro, const std::string& etiqueta) {
    std::string puertos;
    if (filtro != ".")
        puertos = listaPuertos(entrada, filtro);
    else
        puertos = listaPuertos(entrada, ".");
    std::string r;
    while (true) {
        int maxc = (int)puertos.size() + 3;
        s.limpiar(false);
        s.maxN(0xffff);
        int v = s.selectNum(etiqueta);
        r = std::to_string(v);
        std::string n = " " + r;
        if (puertos.find(n) == std::string::npos) break;
        maxc += (int)r.size() + 1;
        std::cout << "\x1b[1A\x1b[" << maxc << "C";
        std::cout << negrita << rojo << gettext("EN USO!") << semcor << std::flush;
        sleep(2);
        std::cout << "\x1b[G\x1b[2K" << std::flush;
    }
    return r;
}

/* ---------- directorios ---------- */

dataError lsDir(const std::string& ruta) {
    dataError r;
    std::string res;
    for (auto& e : fs::directory_iterator(ruta)) {
        std::string n = e.path().filename().string();
        std::string tmp = n;
        res += tmp;
    }
    if (!res.empty() && res.back() == '\n') res.pop_back();
    r.mensaje = res;
    return r;
}

dataError ls_c(const std::string& dir, const std::string& tipo, std::string ext) {
    dataError r;
    std::string res;
    if (ext != "all" && !ext.empty() && ext[0] != '.') ext = "." + ext;
    for (auto& e : fs::directory_iterator(dir)) {
        if ((tipo == "dir") || (tipo == "all")) {
            if (fs::is_directory(e.path())) {
                res += e.path().filename().string();
                continue;
            }
        }
        if ((tipo == "file") || (tipo == "all")) {
            if (fs::is_regular_file(e.path())) {
                if (ext == "all" || e.path().extension() == ext)
                    res += e.path().filename().string();
            }
        }
    }
    if (res.empty()) {
        r.mensaje = "No se encontraron archivos o directorios.";
        r.err = true;
    } else {
        if (res.back() == '\n') res.pop_back();
        r.mensaje = res;
    }
    return r;
}

void cdWorkDir(const std::string& dir) {
    dirSetting("/root/ADMRufu");
    cdir(dir);
}

/* ---------- señales / procesos ---------- */

void closeVentana(int) { exit(0); }

void closeCTRLC(int) {
    clear();
    closeVentana(0);
}

void senales() {
    signal(2, closeCTRLC);
    signal(1, closeVentana);
}

bool enlaceSimbolico(const char* destino, const char* enlace) {
    return symlink(destino, enlace) == 0;
}

/* ---------- red: getDataUrl / getDownload / ip ---------- */

size_t WriteCallback(void* ptr, size_t size, size_t nmemb, void* userdata) {
    ((std::string*)userdata)->append((char*)ptr, size * nmemb);
    return size * nmemb;
}

dataError getDataUrl(const std::string& url) {
    dataError r;
    std::string body;
    curl_global_init(3);
    CURL* c = curl_easy_init();
    if (c) {
        curl_easy_setopt(c, CURLOPT_URL, url.c_str());
        curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, &WriteCallback);
        curl_easy_setopt(c, CURLOPT_WRITEDATA, &body);
        curl_easy_setopt(c, CURLOPT_FOLLOWLOCATION, 1);
        CURLcode rc = curl_easy_perform(c);
        if (rc != 0) {
            body = curl_easy_strerror(rc);
            r.err = true;
        }
        curl_easy_cleanup(c);
    }
    curl_global_cleanup();
    if (!body.empty() && body.back() == '\n') body.pop_back();
    r.mensaje = body;
    return r;
}

size_t WriteCallbackDown(void* ptr, size_t size, size_t nmemb, void* userdata) {
    std::ofstream* out = (std::ofstream*)userdata;
    if (out->is_open()) out->write((char*)ptr, size * nmemb);
    return size * nmemb;
}

dataError getDownload(const std::string& url, const std::string& ruta) {
    dataError r;
    std::ofstream out(ruta, std::ios::binary);
    if (out.is_open()) {
        CURL* c = curl_easy_init();
        if (!c) {
            r.err = true;
            r.mensaje = "No se pudo inicializar cURL.";
        } else {
            curl_easy_setopt(c, CURLOPT_URL, url.c_str());
            curl_easy_setopt(c, CURLOPT_FOLLOWLOCATION, 1);
            curl_easy_setopt(c, CURLOPT_USERAGENT, "Mozilla/5.0");
            curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, &WriteCallbackDown);
            curl_easy_setopt(c, CURLOPT_WRITEDATA, &out);
CURLcode rc = curl_easy_perform(c);
            if (rc == 0)
                r.mensaje = "Descarga exitosa.";
            else {
                r.err = true;
                r.mensaje = "curl_easy_perform() fall\xc3\xb3: " + std::string(curl_easy_strerror(rc));
            }
            curl_easy_cleanup(c);
        }
        out.close();
    } else {
        r.err = true;
        r.mensaje = "No se pudo abrir el archivo: " + ruta;
    }
    return r;
}

dataError ip() {
    static const std::string urls[] = {
        "https://1.1.1.1/cdn-cgi/trace",
        "https://cloudflare.com/cdn-cgi/trace",
        "http://ip1.dynupdate.no-ip.com",
        "api.ipify.org"
    };
    dataError out;
    std::regex re("(\\d{1,3}\\.){3}\\d{1,3}", std::regex::ECMAScript);
    for (const auto& u : urls) {
        dataError de = getDataUrl(u);
        if (de.err) continue;
        std::istringstream ss(de.mensaje);
        std::string linea;
        while (true) {
            if (!std::getline(ss, linea)) break;
            if (linea.find("error") != std::string::npos) break;
            std::string st = linea;
            auto pos = linea.find('=');
            if (pos != std::string::npos) {
                if (linea.substr(0, pos) == "ip") st = linea.substr(pos + 1);
            }
            if (std::regex_match(st, re)) {
                out.mensaje = st;
                goto exito;
            }
        }
    }
    out.err = true;
exito:
    return out;
}

/* ---------- licencia / configuración ---------- */

dataError validarLIC() {
    dataError r;
    r.err = false;
    return r;
}

class Configuracion {
public:
    std::string idioma;
    std::string rutaLocales;
    std::string diretorio;

    static Configuracion& getInstance(const std::string& curdir, const std::string& archivo) {
        static Configuracion inst(curdir, archivo);
        return inst;
    }

    Configuracion() = default;

    Configuracion(const std::string& curdir, const std::string& archivo) {
        diretorio = curdir.substr(curdir.find_last_of('/'));
        cargarConfiguracion(archivo);
        configurarIdioma();
    }

    void cargarConfiguracion(const std::string& archivo) {
        std::ifstream in(archivo, std::ios::in);
        if (!in.is_open()) {
            idioma = "es_ES.utf8";
            rutaLocales = "/etc/ADMRufu2.0/locale";
        } else {
            std::string l;
            while (std::getline(in, l)) {
                auto pos = l.find('=');
                if (pos != std::string::npos) {
                    std::string k = l.substr(0, pos);
                    std::string v = l.substr(pos + 1);
                    if (k == "Idioma") idioma = v;
                    else if (k == "RutaLocales") rutaLocales = v;
                }
            }
            in.close();
        }
    }

    void configurarIdioma() {
        std::string l = "en_US.utf8";
        setenv("LANGUAGE", l.c_str(), 1);
        setlocale(0, l.c_str());
        setlocale(5, l.c_str());
        bindtextdomain(l.c_str(), l.c_str());
        textdomain(l.c_str());
    }
};

void configMenu(const std::string& dir) {
    isRoot("El programa NO se est\xc3\xa1" " ejecutando como root.");
    ipGlobal = ip();
    std::string dirw = "/etc/ADMRufu2.0/etc";
    cdWorkDir(dirw);
    std::string archivo = "lang.ini";
    Configuracion::getInstance(dir, archivo);
}

std::string generateRandomString(int tam, bool incluirDigitos) {
    std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    if (incluirDigitos) chars += "0123456789";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, (int)chars.size() - 1);
    std::string r;
    for (int i = 0; i < tam; ++i) r += chars[dist(gen)];
    return r;
}

dataError workDir() {
    dataError r;
    char buf[4104];
    if (getcwd(buf, 0x1000) == nullptr)
        r.err = true;
    else
        r.mensaje = buf;
    return r;
}
