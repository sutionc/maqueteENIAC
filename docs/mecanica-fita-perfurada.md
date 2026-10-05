# 📜 Mecânica da fita de papel perfurado (como o motor "cospe" a fita)

Objetivo: o **NEMA 17** puxa a fita de papel de um rolo e a empurra pra fora
por uma fenda, simulando o ENIAC lendo/produzindo a **fita perfurada**.

O jeito mais simples e confiável é um **rolete de tração + rolete de pressão**
(*pinch roller* — igual ao que puxa papel numa impressora). Abaixo a sugestão
principal e uma alternativa mais "autêntica".

---

## ✅ Opção A — Rolete de tração + pressão (recomendada)

A fita passa **espremida** entre um rolete preso no eixo do motor (tração) e um
rolete livre com mola (pressão). O motor gira → a fita anda. Simples, não precisa
que o papel tenha furos de arraste.

### Vista de lado (fluxo do papel)

```
      rolo de papel (bobina)
          (  O  )  ← eixo livre, com leve atrito p/ não desenrolar sozinho
             \
              \  fita
               \
        ┌───────┴────────┐
        │   [o] rolete    │  ← rolete de PRESSÃO (livre, empurrado por mola/elástico)
        │    ‖  ← papel    │
        │   (O) rolete     │  ← rolete de TRAÇÃO (no eixo do NEMA 17)
        └────‖────────────┘
             ‖
             ▼  a fita sai aqui ("cuspida") por uma fenda
```

### Peças
| Peça | Sugestão |
|---|---|
| Motor | **NEMA 17** + driver **A4988** (o mesmo "motor forte" do outro projeto) |
| Suporte do motor | **Bracket em "L" para NEMA 17** (barato, pronto) parafusado na base |
| Rolete de tração | Rolete impresso em 3D com furo de **5 mm** + parafuso de aperto (grub screw) no eixo; OU uma **polia GT2** com um **O-ring/anel de borracha** por cima pra dar aderência |
| Rolete de pressão | Um **rolamento 608/625** livre num eixo curto, OU outro rolete impresso girando solto |
| Mola de pressão | Mola pequena, **elástico** ou espuma, empurrando o rolete de pressão contra o de tração |
| Guias laterais | Duas paredinhas (acrílico/MDF/3D) pra manter a fita alinhada |
| Fenda de saída | Rasgo de ~2 mm de altura e a largura da fita, na parede frontal |
| Bobina | Rolo de **papel de calculadora (~57 mm)** num eixo (uma vareta/dowel) com atrito leve |

### Montagem (passo a passo)
1. Fixe o **bracket em L** do NEMA 17 na base. O eixo do motor fica **horizontal**,
   atravessando por onde a fita vai passar.
2. Encaixe o **rolete de tração** no eixo (furo 5 mm) e aperte o parafuso. Se o furo
   do rolete for 8 mm, use um **acoplador flexível 5→8 mm**.
3. Logo **acima** (ou ao lado) do rolete de tração, monte o **rolete de pressão**
   num bracinho que possa balançar; a **mola/elástico** empurra esse bracinho pro
   rolete de tração. A fita passa **entre os dois**.
4. Ponha as **guias laterais** (a distância = largura da fita + folga pequena) pra
   ela não "serpentear".
5. Coloque a **bobina** num eixo livre atrás, com **atrito leve** (um feltro/elástico
   roçando) pra ela não desenrolar à toa — isso mantém a fita esticada.
6. Passe a fita: bobina → entre os roletes → fenda de saída.

### Dicas de ajuste
- **Pouca força é suficiente:** papel exige quase nada. Regule o **Vref do A4988 BEM BAIXO**
  (motor frio e silencioso). Torque sobrando só esquenta e faz barulho.
- **Microstep 1/8 ou 1/16** (pinos MS1/MS2/MS3) deixa o avanço **suave e silencioso**.
- **Aderência:** se a fita patinar, aumente a pressão da mola ou use borracha melhor no
  rolete de tração (O-ring, fita emborrachada, até uma **borracha de apagar** vira rolete).
- **Quanto anda por passo:** `mm por volta = π × diâmetro do rolete`. Ex.: rolete de
  **20 mm** → ~62,8 mm por volta. Com 200 passos/volta (NEMA 1,8°), cada **passo cheio ≈ 0,31 mm**.
  Rolete maior = fita mais rápida por passo.

---

## 🕳️ Opção B — Roda dentada (sprocket) — mais autêntica

As fitas perfuradas de verdade têm uma **fileira de furinhos de arraste**. Dá pra usar
uma **roda dentada** (sprocket) cujos pinos entram nesses furos e puxam a fita — é o jeito
real dos leitores de fita. Fica lindo, mas exige:
- Furar a fita com **espaçamento constante** (um furador/craft punch regular), e
- Imprimir em 3D uma **roda com pinos no mesmo passo** dos furos.

Mais trabalhoso de acertar o encaixe; a **Opção A** é mais garantida pra feira. Dá pra
combinar: **tração pela Opção A** e furos só **decorativos** na fita.

---

## 🎞️ A fita (o "papel perfurado")
- **Papel de calculadora/adding machine** (bobina ~57 mm) é barato e já parece fita.
- Para o visual perfurado: **fure com furador** em padrão regular, ou **imprima** o padrão
  de furos. Se quiser "programa de verdade", faça os furos representando bits (uma coluna
  = um caractere), como no código de fita real.

---

## ⚙️ Ligações elétricas (resumo — detalhes no CLAUDE.md)
- **A4988:** STEP→GPIO 26, DIR→GPIO 27, /ENABLE→GPIO 14 (LOW liga). MS1/2/3 definem o microstep.
- **VMOT = 12 V** (fonte do motor) com **capacitor 470 µF** entre VMOT e GND, perto do driver.
- **+5 V da fita de LED é SEPARADO** do 12 V do motor; **só o GND é comum** (ESP32 + A4988 + fita + fontes).
- Ajuste o **Vref** (limite de corrente) do A4988 **antes** de ligar o motor.
