#include <SuplaDevice.h>
#include <supla/control/button.h>
#include <supla/device/status_led.h>
#include <supla/device/supla_ca_cert.h>
#include <supla/network/esp_web_server.h>
#include <supla/network/esp_wifi.h>
#include <supla/network/html/device_info.h>
#include <supla/network/html/protocol_parameters.h>
#include <supla/network/html/wifi_parameters.h>
#include <supla/storage/eeprom.h>
#include <supla/storage/littlefs_config.h>

// SETTINGS
// MCU: ESP32S3
// USB CDC On Boot: Enabled
// CPU Frequency: 240 Mhz
// USB DFU On Boot: Disabled
// Events Run On: Core 1
// Flash Mode: QIO (80 Mhz)
// Flash Size: 4MB
// Arduino Run On: Core 1
// Partition Scheme: Huge APP (3MB / 1MB)
// PSRAM: OPI PSRAM

#define PINOUT_BUTTON 1
#define PINOUT_LED   15

Supla::Device::StatusLed statusLed(PINOUT_LED, true);
Supla::Eeprom eeprom;
Supla::ESPWifi wifi;
Supla::EspWebServer suplaServer;
Supla::LittleFsConfig configSupla;

auto suplaButtonCfg = new Supla::Control::Button(PINOUT_BUTTON, true, true);

void setup()
{
  suplaButtonCfg->configureAsConfigButton(&SuplaDevice);

  new Supla::Html::DeviceInfo(&SuplaDevice);
  new Supla::Html::WifiParameters;
  new Supla::Html::ProtocolParameters;

  SuplaDevice.setSuplaCACert(suplaCACert);
  SuplaDevice.setSupla3rdPartyCACert(supla3rdCACert);

  SuplaDevice.begin();
}

void loop()
{
  SuplaDevice.iterate();
}