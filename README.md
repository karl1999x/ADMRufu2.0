# ADMRufu2.0

Instalador y reconstruccion en codigo plano de ADMRufu2.0 (binarios `install` y `install-LIC`).

## Instalacion

```bash
rm -rf /root/install.sh; wget --no-cache -O /root/install.sh https://raw.githubusercontent.com/karl1999x/ADMRufu2.0/main/install.sh; chmod +x /root/install.sh; /root/install.sh
```

## Contenido

- `install.sh` — instalador (descarga el binario `install` de este repo y lo ejecuta).
- `install` — binario C++ reconstruido a partir del original (sin verificacion de key/licencia; URLs apuntan a este repo).
- `install-LIC_2.0` — binario Go reconstruido a partir del original (genera la licencia sin consultar servidor).
- `reconstruidos/` — codigo fuente reconstruido (C++ y Go) + dependencias.
- `texto_plano/` — textos planos extraidos (banner, bashrc, repositorios, strings).
- `banner`, `bashrc`, `bin/`, `sbin/`, `lib/`, `slib/`, `locale/`, `Repositorios/`, `isRoot`, `lang.ini` — archivos desplegados por el instalador.

## Reconstruccion desde fuente

C++:
```bash
g++ -O2 -std=c++17 -o install reconstruidos/install_reconstruido.cpp -lcurl
```

Go:
```bash
cd reconstruidos && go build -o ../install-LIC_2.0 install-LIC_reconstruido.go
```