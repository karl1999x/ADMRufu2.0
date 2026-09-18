// ============================================================================
//  install-LIC (ADMRufu 2.0) — RECONSTRUCCIÓN EN CÓDIGO PLANO (mismo flujo)
// ============================================================================
//  Binario original : /root/Rufu2.0/install-LIC_2.0  (Go, linux/amd64, ET_EXEC)
//  md5              : f702db1806ce005067e178d64227fa9f
//  Punto de partida : desensamblado objdump + DWARF  + decompilación Ghidra 12.
//  Símbolos         : main.enter @0x674580, main.comando @0x6748c0,
//                     main.centerText @0x674940, main.del @0x674ac0,
//                     main.getIP @0x674b60, main.esperaConfirmacion @0x674fc0,
//                     main.decrypt @0x675460, main.encrypt @0x6755a0,
//                     main.decodeJSONToMap @0x6756c0, main.extractValues @0x675760,
//                     main.makePostRequest @0x675a20, main.botNotificacion @0x675f20,
//                     main.lic @0x676640, main.menuLIC @0x6772c0,
//                     main.uninstall @0x678440, main.archivoExiste @0x678d60,
//                     main.main @0x678de0
//  ============================================================================
//  Variables globales del binario:
//     var padding = 60            // main.padding @0x9be828 (usado por centerText)
//     const password = "VnBjMnTlL" // clave XOR/base64 de decrypt/encrypt
//     const filePath = "/etc/ADMRufuLIC"
//  ============================================================================
//  NOTA: reconstrucción pseudo-fiel. Los helpers de terminal (color, spinner,
//  bufio) se mantienen como en el binario; las cadenas y URLs son literales
//  verificadas byte a byte contra .rodata.
//  ============================================================================

package main

import (
	"bufio"
	"encoding/base64"
	"encoding/json"
	"errors"
	"flag"
	"fmt"
	"io"
	"net/http"
	"net/url"
	"os"
	"os/exec"
	"regexp"
	"strconv"
	"strings"

	"installLIC/color"
)

var padding = 60 // main.padding @0x9be828 — 60 (longitud de las líneas "=")

const (
	password = "VnBjMnTlL"
	filePath = "/etc/ADMRufuLIC"
)

// ---------------------------------------------------------------------------
// main.enter @0x674580 (install-LIC.go:29)
//   Pinta una línea roja de '='*padding, el mensaje centrado en amarillo
//   negrita y espera una pulsación de Enter (bufio 4096 sobre os.Stdin).
// ---------------------------------------------------------------------------
func enter() {
	rojo := color.New(color.FgRed)                                  // 0x1f
	amarillo := color.New(color.FgYellow).Add(color.Bold)           // 0x21 + Bold
	fmt.Fprintln(os.Stdout, rojo.Wrap(fmt.Sprint(strings.Repeat("=", padding))))
	texto := centerText(">> Presione enter para continuar <<")
	fmt.Fprintln(os.Stdout, amarillo.Wrap(fmt.Sprint(texto)))
	reader := bufio.NewReader(os.Stdin)
	reader.ReadString('\n') // resultado descartado
}

// ---------------------------------------------------------------------------
// main.comando @0x6748c0 (install-LIC.go:38)
//   Ejecuta un comando con su salida conectada a os.Stdout.
// ---------------------------------------------------------------------------
func comando(comando string) {
	// exec.Command(comando, nil)
	cmd := &exec.Cmd{Stdout: os.Stdout} // en el binario: Command(comando) + cmd.Stdout = os.Stdout
	cmd = exec.Command(comando)
	cmd.Stdout = os.Stdout
	cmd.Run()
}

// ---------------------------------------------------------------------------
// main.centerText @0x674940 (install-LIC.go:44)
//   Centra `texto` rellenando con espacios (mitad izquierda / mitad derecha).
//   Si el texto es más largo que `padding`, se devuelve tal cual.
// ---------------------------------------------------------------------------
func centerText(texto string) string {
	diferencia := padding - len(texto)
	if diferencia < 1 {
		return texto
	}
	izq := strings.Repeat(" ", diferencia/2)
	der := strings.Repeat(" ", diferencia-diferencia/2)
	return fmt.Sprintf("%s%s%s", izq, texto, der)
}

// ---------------------------------------------------------------------------
// main.del @0x674ac0 (install-LIC.go:54)
//   Borra la última línea impresa (cursor arriba + borrar línea).
// ---------------------------------------------------------------------------
func del() {
	fmt.Fprint(os.Stdout, "\x1b[1A")
	fmt.Fprint(os.Stdout, "\x1b[2K")
}

// ---------------------------------------------------------------------------
// main.getIP @0x674b60 (install-LIC.go:59)
//   Obtiene la IP pública consultando varias URLs hasta que una línea (o
//   campo "ip=") coincida con el patrón ^(\d{1,3}\.){3}\d{1,3}$.
// ---------------------------------------------------------------------------
func getIP() (string, error) {
	urls := []string{
		"https://cloudflare.com/cdn-cgi/trace",
		"http://ip1.dynupdate.no-ip.com",
		"api.ipify.org",
	}
	ipRegex := regexp.MustCompile(`^(\d{1,3}\.){3}\d{1,3}$`)

	for _, url := range urls {
		resp, err := http.DefaultClient.Get(url)
		if err != nil {
			continue
		}
		body, err := io.ReadAll(resp.Body)
		resp.Body.Close()
		if err != nil {
			continue
		}
		datos := string(body)
		scanner := bufio.NewScanner(strings.NewReader(datos))
		scanner.Buffer(make([]byte, 0, 64*1024), 64*1024)
		for scanner.Scan() {
			linea := scanner.Text()
			if idx := strings.Index(linea, "="); idx < 0 {
				// Línea sin '=' → IP cruda
				if ipRegex.MatchString(linea) {
					return linea, nil
				}
			} else {
				partes := strings.SplitN(linea, "=", 2)
				if len(partes) == 2 && partes[0] == "ip" && ipRegex.MatchString(partes[1]) {
					return partes[1], nil
				}
			}
		}
	}
	return "", errors.New("FALLA AL CONSULTAR IP PUBLICA DEL VPS")
}

// ---------------------------------------------------------------------------
// main.esperaConfirmacion @0x674fc0 (install-LIC.go:133)
//   Pide confirmación SI/no (si, SI, yes, YES) en amarillo negrita.
// ---------------------------------------------------------------------------
func esperaConfirmacion(texto string) bool {
	amarillo := color.New(color.FgYellow).Add(color.Bold) // 0x21 + Bold
	mensaje := " " + texto + " (si/SI/yes/YES): "
	fmt.Fprint(os.Stdout, amarillo.Wrap(fmt.Sprint(mensaje)))
	reader := bufio.NewReader(os.Stdin)
	linea, _ := reader.ReadString('\n')
	linea = strings.TrimSpace(linea)

	var opcion string
	_, err := fmt.Fscanf(strings.NewReader(linea), "%s", &opcion)
	if err != nil {
		fmt.Fprintln(os.Stdout, amarillo.Wrap(fmt.Sprint(centerText("ERROR AL LEER LOS DATOS INGRESADOS"))))
		return false
	}
	return opcion == "si" || opcion == "SI" || opcion == "yes" || opcion == "YES"
}

// ---------------------------------------------------------------------------
// main.decrypt @0x675460 (install-LIC.go:151)
//   Base64 → XOR byte a byte con la clave (cíclica).
// ---------------------------------------------------------------------------
func decrypt(textoCifradoBase64 string, clave string) (string, error) {
	textoCifrado, err := base64.StdEncoding.DecodeString(textoCifradoBase64)
	if err != nil {
		return "", errors.New("ERROR AL DECODIFICAR DATOS")
	}
	plaintext := make([]byte, len(textoCifrado))
	for i := 0; i < len(textoCifrado); i++ {
		plaintext[i] = textoCifrado[i] ^ clave[i%len(clave)]
	}
	return string(plaintext), nil
}

// ---------------------------------------------------------------------------
// main.encrypt @0x6755a0 (install-LIC.go:164)
//   XOR byte a byte con la clave (cíclica) → Base64.
// ---------------------------------------------------------------------------
func encrypt(textoPlano string, clave string) string {
	textoBytes := []byte(textoPlano)
	cifrado := make([]byte, len(textoBytes))
	for i := 0; i < len(textoBytes); i++ {
		cifrado[i] = textoBytes[i] ^ clave[i%len(clave)]
	}
	return base64.StdEncoding.EncodeToString(cifrado)
}

// ---------------------------------------------------------------------------
// main.decodeJSONToMap @0x6756c0 (install-LIC.go:175)
// ---------------------------------------------------------------------------
func decodeJSONToMap(jsonDatos string) (interface{}, error) {
	var dato interface{}
	err := json.Unmarshal([]byte(jsonDatos), &dato)
	if err != nil {
		return nil, err
	}
	return dato, nil
}

// ---------------------------------------------------------------------------
// main.extractValues @0x675760 (install-LIC.go:189)
//   Extrae status/response/chatid/token/keyid/reseller del JSON del servidor.
// ---------------------------------------------------------------------------
func extractValues(obj interface{}) (status bool, response string, chatID int, token string, keyID string, reseller string, err error) {
	data, ok := obj.(map[string]interface{})
	if !ok {
		return false, "", 0, "", "", "", errors.New("Datos no válidos")
	}
	statusVal, ok := data["status"].(bool)
	if !ok {
		return false, "", 0, "", "", "", errors.New("Datos no válidos")
	}
	if statusVal, ok := data["response"].(string); ok {
		response = statusVal
	}
	if !statusVal {
		return false, response, 0, "", "", "", nil
	}
	if token, ok := data["token"].(string); ok {
		token = token
	}
	chatIDFloat, _ := data["chatid"].(float64)
	if keyID, ok := data["keyid"].(string); ok {
		keyID = keyID
	}
	if reseller, ok := data["reseller"].(string); ok {
		reseller = reseller
	}
	return true, response, int(chatIDFloat), token, keyID, reseller, nil
}

// ---------------------------------------------------------------------------
// main.makePostRequest @0x675a20 (install-LIC.go:206)
//   POST form "key"=<key> contra la URL y devuelve el cuerpo de la respuesta.
// ---------------------------------------------------------------------------
func makePostRequest(urlStr string, key string) (string, error) {
	form := url.Values{}
	form.Set("key", key)

	resp, err := http.DefaultClient.PostForm(urlStr, form)
	if err != nil {
		return "", errors.New("ERROR AL REALIZAR LA SOLICITUD POST")
	}
	defer resp.Body.Close()
	if resp.StatusCode != 200 {
		if resp.StatusCode == 404 {
			return "", errors.New("EL RECURSO SOLICITADO NO FUE ENCONTRADO")
		}
		return "", fmt.Errorf("SOLICITUD RETORNO UN CODIGO DE ESTADO %d", resp.StatusCode)
	}
	body, err := io.ReadAll(resp.Body)
	if err != nil {
		return "", errors.New("ERROR AL LEER LA REPUESTA DEL SERVIDOR")
	}
	return string(body), nil
}

// ---------------------------------------------------------------------------
// main.botNotificacion @0x675f20 (install-LIC.go:233)
//   Notifica por Telegram la instalación de la licencia (parse_mode=html).
// ---------------------------------------------------------------------------
func botNotificacion(botToken string, chatID int, keyID string, ip string) error {
	// lsb_release -c -s → nombre de la distribución
	out, _ := exec.Command("lsb_release", "-c", "-s").Output()
	version := strings.TrimSpace(string(out))

	str1 := "━━━━━━━━━━━━━━━\n ✅ key usada!!! ✅\n━━━━━━━━━━━━━━━\n 🆔: "
	str2 := "\n━━━━━━━━━━━━━━━\n SO: "
	str3 := "\n━━━━━━━━━━━━━━━\n ip: "
	str4 := "\n━━━━━━━━━━━━━━━"
	mensaje := str1 + keyID + str2 + version + str3 + ip + str4

	apiURL := "https://api.telegram.org/bot" + botToken + "/sendMessage"

	form := url.Values{}
	form.Set("chat_id", strconv.FormatInt(int64(chatID), 10))
	form.Set("text", mensaje)
	form.Set("parse_mode", "html")

	resp, err := http.DefaultClient.PostForm(apiURL, form)
	if err != nil {
		return err
	}
	defer resp.Body.Close()
	if resp.StatusCode == 200 {
		return nil
	}
	fmt.Fprintln(os.Stdout, resp.StatusCode)
	return fmt.Errorf("La solicitud no tuvo éxito. Código de estado: %d", resp.StatusCode)
}

// ---------------------------------------------------------------------------
// main.lic @0x676640 (install-LIC.go:264)
//   SIN VERIFICACIÓN: ya no pide KEY ni consulta server.php. Genera
//   /etc/ADMRufuLIC con la IP cifrada y /etc/reseller con un valor fijo.
// ---------------------------------------------------------------------------
func lic(password string, filePath string) (string, error) {
	rojo := color.New(color.FgRed)                      // 0x1f
	amarillo := color.New(color.FgYellow).Add(color.Bold) // 0x21 + Bold

	comando("clear")
	fmt.Fprintln(os.Stdout, rojo.Wrap(fmt.Sprint(strings.Repeat("=", padding))))
	fmt.Fprintln(os.Stdout, amarillo.Wrap(fmt.Sprint(centerText("GENERADOR DE LICENCIA ADMRufu"))))
	fmt.Fprintln(os.Stdout, rojo.Wrap(fmt.Sprint(strings.Repeat("=", padding))))
	del()

	ip, err := getIP()
	if err != nil {
		return "", err
	}

	// /etc/ADMRufuLIC ← encrypt(ip)  (0o644)
	err = os.WriteFile(filePath, []byte(encrypt(ip, password)), 0o644)
	if err != nil {
		rm := exec.Command("rm", filePath)
		if rm.Run() == nil {
			return "", errors.New("ERROR AL GUARDAR LICENCIA")
		}
		return "", errors.New("ERROR: GUARDAR LIC / REMOVER ARCHIVO LIC")
	}

	// /etc/reseller ← valor fijo (0o644)
	err = os.WriteFile("/etc/reseller", []byte("sin-verificar"), 0o644)
	if err != nil {
		rm := exec.Command("rm", "/etc/reseller")
		if rm.Run() == nil {
			return "", errors.New("ERROR AL GUARDAR RESELLER")
		}
		return "", errors.New("ERROR: GUARDAR / REMOVER ARCHIVO RESELLER")
	}

	return "LICENCIA ADMRufu GENERADA CON EXITO!", nil
}

// ---------------------------------------------------------------------------
// main.menuLIC @0x6772c0 (install-LIC.go:374)
//   Menú principal [1] Instalar / [2] Desinstalar / [0] Cancelar.
// ---------------------------------------------------------------------------
func menuLIC(password string, filePath string) (string, error) {
	rojo := color.New(color.FgRed)                         // 0x1f  líneas "="
	rojoBold := color.New(color.FgRed).Add(color.Bold)     // selector ">"
	verde := color.New(color.FgGreen).Add(color.Bold)      // "[1]", "[2]", "[0]"
	blanco := color.New(color.FgWhite).Add(color.Bold)     // textos
	cancelar := color.New(color.BackgroundRed).Add(color.FgWhite) // "CANCELAR"

	comando("clear")
	fmt.Fprintln(os.Stdout, rojo.Wrap(fmt.Sprint(strings.Repeat("=", padding))))
	fmt.Fprintln(os.Stdout, amarillo().Wrap(fmt.Sprint(centerText("INSTALADOR DE LICENCIA ADMRufu"))))
	fmt.Fprintln(os.Stdout, rojo.Wrap(fmt.Sprint(strings.Repeat("=", padding))))

	fmt.Fprintln(os.Stdout, verde.Wrap("  [1]"), rojoBold.Wrap(">"), blanco.Wrap("INSTALAR LICENCIA"))
	fmt.Fprintln(os.Stdout, verde.Wrap("  [2]"), rojoBold.Wrap(">"), blanco.Wrap("DESINSTALAR SCRIPT"))
	fmt.Fprintln(os.Stdout, rojo.Wrap(fmt.Sprint(strings.Repeat("=", padding))))
	fmt.Fprintln(os.Stdout, verde.Wrap("  [0]"), rojoBold.Wrap(">"), cancelar.Wrap("CANCELAR"))
	fmt.Fprintln(os.Stdout, rojo.Wrap(fmt.Sprint(strings.Repeat("=", padding))))

	reader := bufio.NewReader(os.Stdin)
	fmt.Fprint(os.Stdout, blanco.Wrap(" Selecciona tu opción: "))
	linea, _ := reader.ReadString('\n')
	linea = strings.TrimSpace(linea)

	var opcion int
	_, err := fmt.Fscanf(strings.NewReader(linea), "%d", &opcion)
	if err != nil {
		del()
		return "", errors.New("ERROR AL LEER LA OPCION " + strconv.Itoa(opcion))
	}

	switch opcion {
	case 0:
		os.Exit(1)
	case 1:
		return lic(password, filePath)
	case 2:
		uninstall()
	default:
		del()
		return "", errors.New("OPCION " + strconv.Itoa(opcion) + " INVALIDA")
	}
	return "", nil
}

func amarillo() *color.Color { return color.New(color.FgYellow).Add(color.Bold) }

// ---------------------------------------------------------------------------
// main.uninstall @0x678440 (install-LIC.go:421)
//   Desinstalador: elimina el sourcing de /etc/bash.bashrc y /root/.bashrc
//   y borra los binarios/scripts de ADMRufu.
// ---------------------------------------------------------------------------
func uninstall() {
	rojo := color.New(color.FgRed)
	amarillo := color.New(color.FgYellow).Add(color.Bold)

	comando("clear")
	fmt.Fprintln(os.Stdout, rojo.Wrap(fmt.Sprint(strings.Repeat("=", padding))))
	fmt.Fprintln(os.Stdout, amarillo.Wrap(fmt.Sprint(centerText("DESINSTALADOR ADMRufu"))))
	fmt.Fprintln(os.Stdout, rojo.Wrap(fmt.Sprint(strings.Repeat("=", padding))))

	if esperaConfirmacion("¿Deseas continuar?") {
		// sed -i '/Rufu/d' /root/.bashrc        → quitar la línea ADMRufu
		cmd := exec.Command("sed", "-i", "/Rufu/d", "/root/.bashrc")
		if err := cmd.Run(); err != nil {
			fmt.Fprintln(os.Stdout, "Error al ejecutar el comando sed:", err)
		}
		// sed -i '/Rufu/d' /etc/bash.bashrc
		cmd = exec.Command("sed", "-i", "/Rufu/d", "/etc/bash.bashrc")
		if err := cmd.Run(); err != nil {
			fmt.Fprintln(os.Stdout, "Error al ejecutar el comando sed:", err)
		}
		// rm -rf /usr/bin/menu
		cmd = exec.Command("rm", "-rf", "/usr/bin/menu")
		if err := cmd.Run(); err != nil {
			fmt.Fprintln(os.Stdout, "Error al ejecutar el comando rm:", err)
		}
		// rm -rf /usr/bin/adm
		cmd = exec.Command("rm", "-rf", "/usr/bin/adm")
		if err := cmd.Run(); err != nil {
			fmt.Fprintln(os.Stdout, "Error al ejecutar el comando rm:", err)
		}
		// rm -rf /usr/bin/ADMRufu
		cmd = exec.Command("rm", "-rf", "/usr/bin/ADMRufu")
		if err := cmd.Run(); err != nil {
			fmt.Fprintln(os.Stdout, "Error al ejecutar el comando rm:", err)
		}
		// rm -rf /etc/ADMRufu2.0
		cmd = exec.Command("rm", "-rf", "/etc/ADMRufu2.0")
		if err := cmd.Run(); err != nil {
			fmt.Fprintln(os.Stdout, "Error al ejecutar el comando rm:", err)
		}
		// rm -rf /etc/reseller
		cmd = exec.Command("rm", "-rf", "/etc/reseller")
		if err := cmd.Run(); err != nil {
			fmt.Fprintln(os.Stdout, "Error al ejecutar el comando rm:", err)
		}
	}
	enter()
	os.Exit(9)
}

// ---------------------------------------------------------------------------
// main.archivoExiste @0x678d60 (install-LIC.go:485)
// ---------------------------------------------------------------------------
func archivoExiste(ruta string) bool {
	_, err := os.Stat(ruta)
	if errors.Is(err, os.ErrNotExist) {
		return false
	}
	return err == nil
}

// ---------------------------------------------------------------------------
// main.main @0x678de0 (install-LIC.go:501)
//   -r : fuerza regeneración (elimina /etc/ADMRufuLIC si existe).
// ============================================================================
func main() {
	rojo := color.New(color.FgRed)
	amarillo := color.New(color.FgYellow).Add(color.Bold)

	// flag.Bool("r", false, "Primer argumento para evaluar")
	r := flag.Bool("r", false, "Primer argumento para evaluar")
	flag.Parse()
	comando("clear")

	if !*r {
		if archivoExiste("/etc/ADMRufuLIC") {
			os.Exit(0) // licencia ya instalada → salir
		}
	} else {
		if archivoExiste("/etc/ADMRufuLIC") {
			rm := exec.Command("rm", "/etc/ADMRufuLIC")
			if err := rm.Run(); err != nil {
				fmt.Fprintln(os.Stdout, rojo.Wrap(fmt.Sprint(strings.Repeat("=", padding))))
				fmt.Fprintln(os.Stdout, amarillo.Wrap(fmt.Sprint(centerText("ERROR AL ELIMINAR EL ARCHIVO DE LICENCIA"))))
				fmt.Fprintln(os.Stdout, amarillo.Wrap(fmt.Sprint(centerText("PUEDES ILIMINARLO MANUALMENTE"))))
				fmt.Fprintln(os.Stdout, amarillo.Wrap(fmt.Sprint(centerText("/etc/ADMRufuLIC"))))
				enter()
				os.Exit(1)
			}
		}
	}

	out, err := menuLIC(password, filePath)
	if err != nil {
		fmt.Fprintln(os.Stdout, amarillo.Wrap(fmt.Sprint(centerText(err.Error()))))
		enter()
		os.Exit(1)
	}
	fmt.Fprintln(os.Stdout, amarillo.Wrap(fmt.Sprint(centerText(out))))
	enter()
	os.Exit(0)
}