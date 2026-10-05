# CLAUDE.md — Maquete do ENIAC

Contexto e conhecimento para trabalhar neste repositório. Destilado de um projeto
anterior (maquete de horta vertical robótica) que usava a mesma fita WS2812B e o
mesmo motor NEMA 17 + A4988 — aqui reaproveitamos só o que importa.

## 🎯 O projeto
Maquete do **ENIAC** (1945, primeiro computador eletrônico de grande porte, ~17.468
válvulas, programado por painéis/cartões/fita perfurada). **Uma única ESP32** controla:
- **Fita de LED WS2812B = as VÁLVULAS** — brilho/piscar simula "processando".
- **Motor de passo NEMA 17 + A4988 = a FITA DE PAPEL PERFURADO** — avança e "cospe" o programa.
- **App no navegador** (Wi-Fi + WebSocket) comanda os dois.

## 🧱 Hardware
- **ESP32 DevKit** (WROOM, USB).
- **Fita WS2812B** (endereçável) — biblioteca **FastLED**.
- **NEMA 17 + driver A4988** — biblioteca **AccelStepper**.

### Pinos (arquivo `firmware/eniac_valvulas_fita/eniac_valvulas_fita.ino`)
| Função | GPIO |
|---|---|
| Dados da fita (DIN) | 13 |
| A4988 STEP | 26 |
| A4988 DIR | 27 |
| A4988 /ENABLE (LOW = ligado) | 14 |

### ⚡ Energia (regra de ouro, herdada do projeto anterior)
- **+5 V da fita** e **+12 V do motor** são **SEPARADOS**; **só o GND é comum**
  (ESP32 + A4988 + fita + as fontes). Essa foi a causa #1 de bugs no outro projeto.
- **A4988:** capacitor **470 µF** (ou 2×) entre **VMOT** e **GND**, perto do driver.
  Ajuste o **Vref (limite de corrente)** para o NEMA 17 **antes** de ligar o motor —
  para papel, deixe a corrente **baixa** (motor frio/silencioso; torque sobra).
- **WS2812B:** data de 3,3 V costuma funcionar; para robustez use resistor **~330 Ω**
  na linha de dados, capacitor **~1000 µF** no 5 V e, em fitas longas, um **level
  shifter** (74AHCT125). Dá pra sacrificar o 1º LED como "shifter".

## ⏱️ FastLED + AccelStepper na mesma placa (importante)
`FastLED.show()` **desliga interrupções por ~1 ms** → pode "roubar" passos do motor.
Mitigação já aplicada no firmware: o LED só atualiza **a cada ~25 ms** e **quando há
mudança/animação**; o motor roda em velocidade moderada. Se precisar de passos muito
precisos **e** animação pesada ao mesmo tempo, intercale (anime entre movimentos).

## 📡 Wi-Fi + app (padrão reaproveitado, já no firmware)
- **WiFiMulti**: rede **principal + secundária** (`WIFI_SSID2`/`WIFI_SENHA2` opcionais).
- **WiFiManager**: se não achar rede conhecida, abre o portal **"ENIAC-Setup"** (hotspot);
  você escolhe a rede do lugar pelo celular e fica salvo na **flash (NVS)**. Bom pra levar.
- **WebServer :80** serve a página; **WebSocketsServer :81** recebe comandos ao vivo.
- **"Segurar para mover"** com **watchdog** (a página reenvia a cada 150 ms; se parar de
  chegar, o motor para sozinho — segurança).
- ⚠️ **Sempre** `<meta charset="utf-8">` nas páginas HTML — sem isso os acentos/emoji quebram
  (lição aprendida: fora do host da Claude, o navegador assume Latin-1).

### Protocolo WebSocket (comandos "cmd:valor")
- Fita (motor): `mA` avançar, `mR` recuar (ambos `:1`/`:0` = segurar/soltar), `vel:<n>`,
  `rodar:1|0` (avança contínuo + válvulas em "processando").
- Válvulas (LED): `on:1|0`, `bri:<0-255>`, `modo:ligado|processando|onda|standby`.

## 🛠️ Mecânica
Como montar o motor/suportes pra puxar e "cuspir" a fita: **`docs/mecanica-fita-perfurada.md`**
(rolete de tração + rolete de pressão; alternativa com roda dentada/sprocket).

## 📚 Bibliotecas a instalar (Arduino IDE → Gerenciador de Bibliotecas)
**FastLED**, **AccelStepper**, **WebSockets** (Links2004), **WiFiManager** (tzapu).
Suporte ESP32 da Espressif. Placa: **ESP32 Dev Module**.

## ▶️ Como gravar
1. `cp firmware/eniac_valvulas_fita/secrets.h.example firmware/eniac_valvulas_fita/secrets.h`
   e preencha o Wi-Fi.
2. Confira `NUM_LEDS` e os pinos no `.ino`.
3. Grave. O Monitor Serial (115200) mostra o IP → abra no celular.

## 🧪 Nota sobre este ambiente
A sessão da Claude **não compila firmware ESP32**. Depois de gerar/alterar código,
**peça um teste de gravação**; se o compilador reclamar (ex.: faltar uma lib), traga o
erro que a gente ajusta.

## 💡 Ideias de evolução (ENIAC)
- Válvulas como **acumulador/contador** (acende conforme "conta").
- Fita com furos = **programa real** (uma coluna = um caractere), sincronizada ao avanço.
- Painel de **fios de programação** (plugboard) decorativo, som de relés, etc.
