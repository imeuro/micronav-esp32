/*
 * Test mirato: inizializzazione display GC9A01 via LovyanGFX invece di
 * Adafruit_GC9A01A, usando la stessa identica configurazione pannello del
 * demo ufficiale del produttore (Factory_samples.ino), per verificare se
 * risolve il problema "schermo nero dopo spegnimento completo / nessun
 * disegno visibile" osservato con Adafruit_GC9A01A.
 *
 * Non tocca il firmware principale. Sketch separato e isolato.
 *
 * Pin e config pannello identici a:
 * 1.28inch_ESP32-2424S012/1-Demo/Demo_Arduino/1_1_Factory_samples/Factory_samples/Factory_samples.ino
 */

#define LGFX_USE_V1
#include <Arduino.h>
#include <LovyanGFX.hpp>

// Riusa i dati del boot logo gia' generati per il firmware principale
#include "../src/boot_logo.h"

#define DISPLAY_BL_PIN 3

class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_GC9A01 _panel_instance;
  lgfx::Bus_SPI _bus_instance;

public:
  LGFX(void) {
    {
      auto cfg = _bus_instance.config();
      cfg.spi_host = SPI2_HOST;
      cfg.spi_mode = 0;
      cfg.freq_write = 80000000;
      cfg.freq_read = 20000000;
      cfg.spi_3wire = true;
      cfg.use_lock = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      cfg.pin_sclk = 6;
      cfg.pin_mosi = 7;
      cfg.pin_miso = -1;
      cfg.pin_dc = 2;

      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }

    {
      auto cfg = _panel_instance.config();
      cfg.pin_cs = 10;
      cfg.pin_rst = -1;
      cfg.pin_busy = -1;

      cfg.memory_width = 240;
      cfg.memory_height = 240;
      cfg.panel_width = 240;
      cfg.panel_height = 240;
      cfg.offset_x = 0;
      cfg.offset_y = 0;
      cfg.offset_rotation = 0;
      cfg.dummy_read_pixel = 8;
      cfg.dummy_read_bits = 1;
      cfg.readable = false;
      cfg.invert = true;
      cfg.rgb_order = false;
      cfg.dlen_16bit = false;
      cfg.bus_shared = false;

      _panel_instance.config(cfg);
    }

    setPanel(&_panel_instance);
  }
};

LGFX tft;

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("\n========================================");
  Serial.println("Test LovyanGFX - Boot Logo");
  Serial.print("Build: ");
  Serial.print(__DATE__);
  Serial.print(" ");
  Serial.println(__TIME__);
  Serial.println("========================================");

  // Backlight spento durante init, come nel firmware principale
  pinMode(DISPLAY_BL_PIN, OUTPUT);
  digitalWrite(DISPLAY_BL_PIN, LOW);

  Serial.println("[Test] tft.init()...");
  bool ok = tft.init();
  Serial.print("[Test] tft.init() risultato: ");
  Serial.println(ok ? "OK" : "FALLITO");

  Serial.println("[Test] fillScreen(BLACK)...");
  tft.fillScreen(TFT_BLACK);

  // I dati del boot logo sono stati generati per Adafruit_GFX (byte order
  // diverso da quello di default di LovyanGFX): servirebbe lo swap.
  tft.setSwapBytes(true);

  Serial.println("[Test] pushImage boot logo...");
  tft.pushImage(boot_logo_data_offset_x, boot_logo_data_offset_y,
                boot_logo_data_width, boot_logo_data_height,
                boot_logo_data);

  tft.setSwapBytes(false);

  Serial.println("[Test] Cerchio rosso di controllo...");
  tft.fillCircle(120, 180, 20, TFT_RED);

  Serial.println("[Test] Testo di controllo...");
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(1);
  tft.setCursor(70, 210);
  tft.print("LovyanGFX test");

  digitalWrite(DISPLAY_BL_PIN, HIGH);
  Serial.println("[Test] Backlight acceso, rendering completato.");
  Serial.println("[Test] Setup completato, nessun loop attivo.");
}

void loop() {
  delay(1000);
}
