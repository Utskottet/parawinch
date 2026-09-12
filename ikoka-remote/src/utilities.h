#pragma once
/*----------------------------------------------------------
 *  Fork Ikoka Nano — Seeed XIAO nRF52840 Plus + EBYTE E22-900M30S
 *  Pin mapping: SEEED_XIAO_NRF52840_PLUS variant
 *  Source: "Fork Ikoka Nano.kicad_pcb" netlist (U1 pad -> net)
 *
 *  Differences vs. remote/src/utilities.h (XIAO + Wio SX1262 B2B):
 *    - LoRa moves off the B2B pins onto D0-D3 / D7
 *    - buzzer moves off D7 onto the J4 piezo pair D18/D19
 *    - buttons move off D6/D0 onto the J1 header D11/D12
 *---------------------------------------------------------*/

// ═══════════════════════════════════════════════════════
// LoRa E22-900M30S (SX1262 + PA + LNA)
// U1 pad  -> net           -> E22 pin
//   D0    -> SX126X_CS     -> 19 NSS
//   D1    -> SX126X_DIO1   -> 13 DIO1
//   D2    -> SX126X_BUSY   -> 14 BUSY
//   D3    -> SX126X_RESET  -> 15 NRST   (also MT3608B boost EN, see below)
//   D7    -> SX126X_RXEN   ->  6 RXEN
// E22 TXEN (7) is bonded to E22 DIO2 (8) on the PCB — the SX1262 drives its
// own TX enable, so there is no MCU pin for it (setDio2AsRfSwitch).
// ═══════════════════════════════════════════════════════
#define LORA_CS_PIN     D0    // SPI Chip Select
#define LORA_DIO1_PIN   D1    // IRQ / interrupt
#define LORA_BUSY_PIN   D2    // Busy signal
#define LORA_RST_PIN    D3    // Reset  + E22 5 V rail enable
#define LORA_RXEN_PIN   D7    // RX/TX switch: HIGH = RX, LOW = TX

// SPI bus pins (standard XIAO SPI, unchanged from the Wio build)
#define LORA_SCK_PIN    D8
#define LORA_MOSI_PIN   D10
#define LORA_MISO_PIN   D9

// The E22 runs off VCC_E22, a ~5.2 V MT3608B boost whose EN pin is tied to
// SX126X_RESET. Reset asserted (LOW) therefore also cuts the module's supply:
//   RST HIGH -> boost on  -> E22 powered
//   RST LOW  -> boost off -> E22 completely unpowered (300k pulldown = default)
// Nothing to do at runtime beyond letting the rail settle before the first SPI
// transaction, and pulling RST low again on the way into System OFF.
#define LORA_BOOT_SETTLE_MS  10

// ═══════════════════════════════════════════════════════
// Silkscreen pads -> BSP pin indices
//
// Careful: on the XIAO nRF52840 Plus, Seeed's BSP macros D17 and D19 do NOT
// match the pads printed on the board. That pair is swapped:
//
//   silkscreen pad      real GPIO    BSP index    BSP macro claims
//   ---------------     ---------    ---------    ----------------
//   D17                 P1.03           38        D19
//   D18                 P1.05           37        D18          (agrees)
//   D19                 P1.07           36        D17
//
// Verified on hardware with the selftest build: driving the J4 piezo's first
// leg (KiCad pad "D18") works on BSP 37, and the circuit only completes when
// BSP 36 is grounded -- BSP 38 grounded is silent. So the pad the KiCad
// netlist calls "D19" is physically P1.07 = BSP 36.
//
// The KiCad netlist names pads by silkscreen, so these PAD_* names follow the
// silkscreen too and the table above is the only place the swap is handled.
// Never use the bare D17/D19 macros on this board.
// ═══════════════════════════════════════════════════════
#define PAD_D11         D11   // 30, P0.15  -- BSP and silkscreen agree
#define PAD_D12         D12   // 31, P0.19  -- agree
#define PAD_D13         D13   // 32, P1.01  -- agree
#define PAD_D17          38   // P1.03      -- NOT the BSP's D17
#define PAD_D18         D18   // 37, P1.05  -- agree
#define PAD_D19          36   // P1.07      -- NOT the BSP's D19

// ═══════════════════════════════════════════════════════
// Buttons — J1 header (1=BUTTON_1 .. 4=BUTTON_4, 5=GND)
//   J1-1 -> pad D11 (P0.15)   UP     confirmed on hardware
//   J1-2 -> pad D12 (P0.19)   DOWN   confirmed on hardware
//   J1-3 -> pad D13 (P1.01), J1-4 -> pad D17 (P1.03) spare, both untested
// Active LOW against J1-5, internal pullups.
// ═══════════════════════════════════════════════════════
#define BTN_UP_PIN      PAD_D11   // UP: increment state
#define BTN_DN_PIN      PAD_D12   // DOWN: decrement / emergency stop

// ═══════════════════════════════════════════════════════
// Buzzer — J4 piezo, one leg per pin through a 100R series resistor
//   pad D18 (P1.05) -> R4 -> PIEZO_A   driven by tone()
//   pad D19 (P1.07) -> R5 -> PIEZO_B   held LOW as the return path
// Single-ended drive, same as the Wio build's single buzzer pin. High drive
// mode matters here -- at default drive strength the piezo is barely audible.
// ═══════════════════════════════════════════════════════
#define BUZZER_PIN      PAD_D18   // tone() output
#define BUZZER_GND_PIN  PAD_D19   // piezo return, parked LOW
