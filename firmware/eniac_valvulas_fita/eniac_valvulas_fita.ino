/* =====================================================================
   MAQUETE DO ENIAC  ·  uma ESP32 só
   ---------------------------------------------------------------------
   • FITA DE LED WS2812B  = as VÁLVULAS (tubos a vácuo) do ENIAC:
       brilho/piscar simula o computador "processando".
   • MOTOR DE PASSO NEMA 17 + A4988 = avança e "cospe" a FITA DE PAPEL
       PERFURADO (o programa sendo lido/produzido).
   Controle por um app no navegador (celular/tablet) via Wi-Fi + WebSocket.

   >>> BIBLIOTECAS (Gerenciador de Bibliotecas do Arduino IDE): <<<
     • "FastLED"       (Daniel Garcia)
     • "AccelStepper"  (Mike McCauley)
     • "WebSockets"    (Markus Sattler / Links2004)
     • "WiFiManager"   (tzapu)
   Placa: "ESP32 Dev Module".

   >>> ANTES DE GRAVAR: <<<
     1. Copie "secrets.h.example" para "secrets.h" e ponha seu Wi-Fi.
     2. Ajuste NUM_LEDS e os pinos conforme a sua ligação.
     3. No A4988, regule o LIMITE DE CORRENTE (Vref) para o seu NEMA 17
        ANTES de ligar o motor (veja CLAUDE.md) — evita queimar o motor.

   ⚡ ENERGIA: +5V da fita e +12V do motor são SEPARADOS; só o GND é
      comum (ESP32 + A4988 + fita + fontes). Capacitor 470µF (ou 2×)
      entre VMOT e GND do A4988, perto do driver.
   ===================================================================== */

#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiManager.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <AccelStepper.h>
#include <FastLED.h>
#include "secrets.h"

// ---------------------------------------------------------------------
// PINOS  (seguros no ESP32 — mude conforme sua ligação)
// ---------------------------------------------------------------------
#define LED_PIN     13        // dados da fita WS2812B (DIN)
#define NUM_LEDS    60        // << quantos LEDs/"válvulas" tem a fita
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB

#define STEP_PIN    26        // A4988 STEP
#define DIR_PIN     27        // A4988 DIR
#define EN_PIN      14        // A4988 /ENABLE  (LOW = motor ligado)

// ---------------------------------------------------------------------
CRGB leds[NUM_LEDS];
AccelStepper motor(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);
WiFiMulti wifiMulti;
WebServer http(80);
WebSocketsServer ws(81);

// ----- VÁLVULAS (LED) -----
bool    valvOn   = true;
uint8_t brilho   = 160;                 // 0..255
enum Modo { LIGADO, PROCESSANDO, ONDA, STANDBY };   // estados das válvulas
Modo    modo     = LIGADO;
bool    ledDirty = true;
uint16_t ondaPos = 0;
const CRGB AMBAR = CRGB(255, 110, 20);  // brilho "quente" dos tubos

// ----- FITA DE PAPEL (motor) -----
const float VEL_MAX = 1400.0;           // passos/s máx
float   velFita   = 700.0;              // velocidade de avanço
int     jog       = 0;                  // -1 recuar, +1 avançar (segurar)
unsigned long ultJog = 0;
const unsigned long JOG_TIMEOUT = 400;  // ms sem ping => para (segurança)
bool    rodando   = false;              // "rodar programa": avança contínuo

// =====================================================================
void setup() {
  Serial.begin(115200);
  delay(200);

  pinMode(EN_PIN, OUTPUT);
  digitalWrite(EN_PIN, LOW);            // habilita o A4988
  motor.setMaxSpeed(VEL_MAX);
  motor.setAcceleration(900);
  motor.setSpeed(0);

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS)
         .setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(brilho);
  FastLED.clear(true);

  // Wi-Fi: principal + secundária; se não achar, portal "ENIAC-Setup"
  Serial.print("Conectando no Wi-Fi");
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  wifiMulti.addAP(WIFI_SSID, WIFI_SENHA);
#ifdef WIFI_SSID2
  wifiMulti.addAP(WIFI_SSID2, WIFI_SENHA2);
#endif
  unsigned long t0 = millis();
  while (wifiMulti.run(8000) != WL_CONNECTED && millis() - t0 < 12000) { delay(300); Serial.print("."); }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nSem rede conhecida. Abrindo portal 'ENIAC-Setup'...");
    WiFiManager wm;
    wm.setConfigPortalTimeout(180);
    if (!wm.autoConnect("ENIAC-Setup")) { Serial.println("Portal expirou; reiniciando..."); delay(500); ESP.restart(); }
  }
  WiFi.setAutoReconnect(true);
  Serial.println(" conectado!");
  Serial.print("Rede: "); Serial.println(WiFi.SSID());

  http.on("/", []() { http.send_P(200, "text/html", PAGINA); });
  http.begin();
  ws.begin();
  ws.onEvent(wsEvent);

  Serial.println("========================================");
  Serial.print("ABRA NO CELULAR:  http://"); Serial.println(WiFi.localIP());
  Serial.println("========================================");

  ledDirty = true;
}

// =====================================================================
void loop() {
  static unsigned long tWifi = 0;
  if (WiFi.status() != WL_CONNECTED && millis() - tWifi > 5000) { tWifi = millis(); wifiMulti.run(6000); }

  ws.loop();
  http.handleClient();

  // ---- MOTOR (fita) ----
  if (jog != 0 && millis() - ultJog > JOG_TIMEOUT) jog = 0;   // watchdog do "segurar"
  if (jog != 0) {                                   // jog manual (prioridade)
    motor.setSpeed(jog > 0 ? velFita : -velFita);
    motor.runSpeed();
  } else if (rodando) {                             // "rodar programa": avança sempre
    motor.setSpeed(velFita);
    motor.runSpeed();
  }

  // ---- VÁLVULAS (LED) ----
  // FastLED.show() desliga interrupções ~1ms; para não roubar passos do
  // motor, atualizamos só a cada ~25ms e quando há mudança/animação.
  static unsigned long tLed = 0;
  if (valvOn && (modo == PROCESSANDO || modo == ONDA)) ledDirty = true;  // anima
  if (ledDirty && millis() - tLed > 25) {
    tLed = millis();
    render();
    FastLED.show();
    ledDirty = false;
  }
}

// =====================================================================
void render() {
  FastLED.setBrightness(brilho);
  if (!valvOn) { fill_solid(leds, NUM_LEDS, CRGB::Black); return; }
  switch (modo) {
    case LIGADO:   fill_solid(leds, NUM_LEDS, AMBAR); break;
    case STANDBY:  fill_solid(leds, NUM_LEDS, CRGB(60, 22, 4)); break;
    case PROCESSANDO:                                  // tubos "computando"
      fill_solid(leds, NUM_LEDS, CRGB(90, 34, 6));     // base morna fraca
      for (int i = 0; i < NUM_LEDS; i++) if (random8() < 45) leds[i] = CRGB(255, 150, 35);
      break;
    case ONDA:                                         // dado "correndo" pela fita
      fadeToBlackBy(leds, NUM_LEDS, 48);
      leds[ondaPos % NUM_LEDS] = AMBAR;
      ondaPos++;
      break;
  }
}

// =====================================================================
void wsEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t len) {
  if (type != WStype_TEXT) return;
  String m = String((char*)payload).substring(0, len);
  int sep = m.indexOf(':');
  String cmd = sep >= 0 ? m.substring(0, sep) : m;
  String val = sep >= 0 ? m.substring(sep + 1) : "";

  // ---- fita (motor) ----
  if      (cmd == "mA") { jog = (val == "1") ? +1 : 0; ultJog = millis(); }   // avançar (cuspir)
  else if (cmd == "mR") { jog = (val == "1") ? -1 : 0; ultJog = millis(); }   // recuar
  else if (cmd == "vel"){ velFita = constrain(val.toInt(), 50, (int)VEL_MAX); }
  else if (cmd == "rodar"){ rodando = (val == "1"); modo = rodando ? PROCESSANDO : LIGADO; ledDirty = true; }

  // ---- válvulas (LED) ----
  else if (cmd == "on") { valvOn = (val == "1"); ledDirty = true; }
  else if (cmd == "bri"){ brilho = constrain(val.toInt(), 0, 255); ledDirty = true; }
  else if (cmd == "modo"){
    if      (val == "ligado")     modo = LIGADO;
    else if (val == "processando")modo = PROCESSANDO;
    else if (val == "onda")       modo = ONDA;
    else if (val == "standby")    modo = STANDBY;
    ledDirty = true;
  }
}

// =====================================================================
const char PAGINA[] PROGMEM = R"HTML(
<!doctype html><html lang="pt-br"><head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>ENIAC · painel</title>
<style>
  :root{ --bg:#140f08; --card:#1f1710; --ink:#f3e9db; --soft:#c0a784; --line:#3a2c1b;
         --ambar:#e8912b; --ambar2:#c9731a; --dim:#2a2016; }
  *{box-sizing:border-box} body{margin:0;background:var(--bg);color:var(--ink);
    font-family:ui-monospace,Menlo,Consolas,monospace;padding:16px;max-width:560px;margin:0 auto}
  h1{font-size:19px;margin:2px 0;letter-spacing:.04em} .sub{color:var(--soft);font-size:12px;margin-bottom:14px}
  .card{background:var(--card);border:1px solid var(--line);border-radius:12px;padding:14px;margin:10px 0}
  .lbl{font-size:11px;letter-spacing:.14em;text-transform:uppercase;color:var(--soft);margin-bottom:9px}
  .row{display:flex;gap:10px;flex-wrap:wrap} .row>*{flex:1}
  button{font:inherit;border:1px solid var(--line);border-radius:10px;padding:14px;color:#1a120a;
    font-weight:700;background:var(--ambar)}
  button:active{transform:translateY(1px)} .alt{background:var(--dim);color:var(--ink)}
  input[type=range]{width:100%} .num{color:var(--ambar)}
  #st{color:var(--soft)}
</style></head><body>
  <h1>⚙ ENIAC — painel de controle</h1>
  <div class="sub">uma ESP32 · válvulas (fita LED) + fita de papel perfurado (NEMA 17) · <span id="st">conectando…</span></div>

  <div class="card">
    <div class="lbl">📜 Fita de papel — segurar para mover</div>
    <div class="row">
      <button id="bR" class="alt">◀ Recuar</button>
      <button id="bA">Avançar ▶ (cuspir)</button>
    </div>
    <div class="row" style="margin-top:10px">
      <button id="run" class="alt">▶ Rodar programa</button>
    </div>
    <div style="margin-top:12px">
      <div class="lbl">Velocidade da fita: <span class="num" id="vV">700</span> passos/s</div>
      <input type="range" min="50" max="1400" value="700" id="vel">
    </div>
  </div>

  <div class="card">
    <div class="lbl">💡 Válvulas</div>
    <div class="row">
      <button onclick="send('on:1')">Ligar</button>
      <button class="alt" onclick="send('on:0')">Desligar</button>
    </div>
    <div class="row" style="margin-top:10px">
      <button onclick="send('modo:ligado')">Ligado</button>
      <button onclick="send('modo:processando')">Processando</button>
      <button class="alt" onclick="send('modo:onda')">Onda</button>
      <button class="alt" onclick="send('modo:standby')">Standby</button>
    </div>
    <div style="margin-top:12px">
      <div class="lbl">Brilho: <span class="num" id="bV">160</span></div>
      <input type="range" min="0" max="255" value="160" id="bri">
    </div>
  </div>

<script>
  let ws, ready=false, running=false;
  function conectar(){
    ws=new WebSocket('ws://'+location.hostname+':81/');
    ws.onopen =()=>{ready=true; st.textContent='conectado';};
    ws.onclose=()=>{ready=false; st.textContent='reconectando…'; setTimeout(conectar,1200);};
  }
  const st=document.getElementById('st'); conectar();
  function send(s){ if(ready) ws.send(s); }

  function hold(btn, cmd){
    let t=null;
    const go=(e)=>{e.preventDefault(); send(cmd+':1'); t=setInterval(()=>send(cmd+':1'),150);};
    const stop=()=>{ if(t){clearInterval(t);t=null;} send(cmd+':0'); };
    btn.addEventListener('pointerdown',go);
    ['pointerup','pointerleave','pointercancel'].forEach(ev=>btn.addEventListener(ev,stop));
  }
  hold(document.getElementById('bA'),'mA');
  hold(document.getElementById('bR'),'mR');

  const run=document.getElementById('run');
  run.onclick=()=>{ running=!running; send('rodar:'+(running?1:0));
    run.textContent = running ? '⏸ Parar programa' : '▶ Rodar programa'; };

  const vel=document.getElementById('vel');
  vel.oninput=()=>{ vV.textContent=vel.value; send('vel:'+vel.value); };
  const bri=document.getElementById('bri');
  bri.oninput=()=>{ bV.textContent=bri.value; send('bri:'+bri.value); };
  const vV=document.getElementById('vV'), bV=document.getElementById('bV');
</script>
</body></html>
)HTML";
