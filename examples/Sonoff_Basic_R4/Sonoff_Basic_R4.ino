#include <SuplaDevice.h>
#include <supla/control/button.h>
#include <supla/control/relay.h>
#include <supla/device/status_led.h>
#include <supla/device/supla_ca_cert.h>
#include <supla/network/esp_web_server.h>
#include <supla/network/esp_wifi.h>
#include <supla/network/html/device_info.h>
#include <supla/network/html/protocol_parameters.h>
#include <supla/network/html/wifi_parameters.h>
#include <supla/storage/eeprom.h>
#include <supla/storage/littlefs_config.h>

#define PINOUT_LED    6
#define PINOUT_BUTTON 9
#define PINOUT_RELAY  4

Supla::Device::StatusLed statusLed(PINOUT_LED, false);
Supla::Eeprom eeprom;
Supla::ESPWifi wifi;
Supla::EspWebServer suplaServer;
Supla::LittleFsConfig configSupla;

auto suplaButtonCfg = new Supla::Control::Button(PINOUT_BUTTON, true, true);
auto suplaRelay = new Supla::Control::Relay(PINOUT_RELAY);

void setup() 
{
  suplaButtonCfg->configureAsConfigButton(&SuplaDevice);
  suplaButtonCfg->addAction(Supla::TOGGLE, suplaRelay, Supla::ON_CLICK_1);
	
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