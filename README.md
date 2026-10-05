# 🖥️ Maquete do ENIAC

Maquete do **ENIAC** (1945) controlada por **uma única ESP32**:

- 💡 **Fita de LED WS2812B = as válvulas** (os tubos a vácuo) — brilho/piscar simula o
  computador "processando".
- 📜 **Motor de passo NEMA 17 + A4988 = a fita de papel perfurado** — avança e "cospe" o
  programa.
- 📱 **App no navegador** (Wi-Fi + WebSocket) comanda os dois.

## 📁 Estrutura
```
firmware/
  eniac_valvulas_fita/
    eniac_valvulas_fita.ino   → firmware (válvulas + fita + app)
    secrets.h.example         → modelo de Wi-Fi (copie p/ secrets.h)
docs/
  mecanica-fita-perfurada.md  → como montar o motor/suportes p/ puxar a fita
  mecanica-fita-perfurada.html → o mesmo, com desenho (abra no navegador)
CLAUDE.md                     → contexto + todo o conhecimento do projeto
```

## ▶️ Começar
1. **Bibliotecas** (Arduino IDE → Gerenciador de Bibliotecas): **FastLED**,
   **AccelStepper**, **WebSockets** (Links2004), **WiFiManager** (tzapu). Placa: **ESP32 Dev Module**.
2. `cp firmware/eniac_valvulas_fita/secrets.h.example firmware/eniac_valvulas_fita/secrets.h`
   e preencha o Wi-Fi.
3. Confira `NUM_LEDS` e os pinos no `.ino`, grave e abra o IP (Monitor Serial 115200) no celular.

## 🔌 Ligação rápida
- **A4988:** STEP→GPIO 26, DIR→GPIO 27, /ENABLE→GPIO 14 (LOW liga); **VMOT 12 V** + cap **470 µF**.
- **Fita WS2812B:** DIN→GPIO 13; **+5 V separado** do 12 V do motor; **GND comum**.
- Ajuste o **Vref** do A4988 (corrente baixa p/ papel) **antes** de ligar o motor.

Mais detalhes de montagem mecânica em **`docs/mecanica-fita-perfurada.md`**, e todo o
contexto técnico em **`CLAUDE.md`**.
