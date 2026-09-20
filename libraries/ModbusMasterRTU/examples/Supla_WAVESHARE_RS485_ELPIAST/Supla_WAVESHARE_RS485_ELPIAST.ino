#include <SuplaDevice.h>
#include <supla/control/button.h>
#include <supla/device/status_led.h>
#include <supla/network/esp_web_server.h>
#include <supla/network/esp_wifi.h>
#include <supla/network/html/device_info.h>
#include <supla/network/html/protocol_parameters.h>
#include <supla/network/html/wifi_parameters.h>
#include <supla/storage/eeprom.h>
#include <supla/storage/littlefs_config.h>

#include <supla/clock/clock.h>
#include <supla/control/hvac_base.h>
#include <supla/control/internal_pin_output.h>
#include <supla/network/html/time_parameters.h>
#include <supla/sensor/virtual_thermometer.h>

#include <ModbusMasterRTU.h>

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

#define PINOUT_REDE  21

Supla::Device::StatusLed statusLed(PINOUT_LED, true);
Supla::Eeprom eeprom;
Supla::ESPWifi wifi;
Supla::EspWebServer suplaServer;
Supla::LittleFsConfig configSupla;

auto suplaButtonCfg = new Supla::Control::Button(PINOUT_BUTTON, true, true);

#define MODBUS_MASTER_ELPIAST_ID                 1
#define MODBUS_MASTER_ELPIAST_ADDRESS_START 0x0000
#define MODBUS_MASTER_ELPIAST_ADDRESS_SIZE  0x000A
#define MODBUS_MASTER_ELPIAST_ANSWER_TIMEOUT   500

enum ModbusMasterVariablesELPIAST // ELPIAST
{
	MODBUS_MASTER_ELPIAST_TEMPERATURE = 0x00,
	MODBUS_MASTER_ELPIAST_SET_TEMPERATURE = 0x02,
	MODBUS_MASTER_ELPIAST_SET_MODE = 0x03
};

bool masterBigEndian = true;

uint16_t masterTable[MODBUS_MASTER_ELPIAST_ADDRESS_SIZE];

ModbusMasterRTU Master;

auto suplaThermometer = new Supla::Sensor::VirtualThermometer;

auto suplaThermostatOutput = new Supla::Control::InternalPinOutput(-1);
auto suplaThermostat = new Supla::Control::HvacBase(suplaThermostatOutput);

void setup()
{
  Serial.begin(115200);

  suplaButtonCfg->configureAsConfigButton(&SuplaDevice);

  new Supla::Html::DeviceInfo(&SuplaDevice);
  new Supla::Html::WifiParameters;
  new Supla::Html::ProtocolParameters;
  
  new Supla::Clock;
  new Supla::Html::TimeParameters(&SuplaDevice);

  suplaThermostat->setMainThermometerChannelNo(0);

  Master.setSerial(&Serial1, 9600);
  Master.setREDE(PINOUT_REDE);

  SuplaDevice.setInitialMode(Supla::InitialMode::StartInCfgMode);
  SuplaDevice.begin();
}

void loop()
{
  SuplaDevice.iterate();

  if(SuplaDevice.getDeviceMode() == Supla::DEVICE_MODE_NORMAL)
  {
    static uint32_t lastTime = 0;
    if(millis() - lastTime > 2000)
    {
      lastTime = millis();

      if(Master.readHoldingRegisters(MODBUS_MASTER_ELPIAST_ID, MODBUS_MASTER_ELPIAST_ADDRESS_START + MODBUS_MASTER_ELPIAST_TEMPERATURE, 2, masterTable, MODBUS_MASTER_ELPIAST_TEMPERATURE, MODBUS_MASTER_ELPIAST_ANSWER_TIMEOUT) == MODBUS_MASTER_STATUS_OK)
      {
        suplaThermometer->setValue(Master.conversionToFloat(Master.conversionToUint32(masterTable[MODBUS_MASTER_ELPIAST_TEMPERATURE], masterTable[MODBUS_MASTER_ELPIAST_TEMPERATURE + 1], masterBigEndian)));
      }

      delay(1000);

      if(Master.writeMultipleRegisters(MODBUS_MASTER_ELPIAST_ID, MODBUS_MASTER_ELPIAST_ADDRESS_START + MODBUS_MASTER_ELPIAST_SET_TEMPERATURE, 2, masterTable, MODBUS_MASTER_ELPIAST_SET_TEMPERATURE, MODBUS_MASTER_ELPIAST_ANSWER_TIMEOUT) == MODBUS_MASTER_STATUS_OK)
      {
        masterTable[MODBUS_MASTER_ELPIAST_SET_TEMPERATURE] = (uint16_t)suplaThermostat->getTemperatureSetpointHeat();

        if(!suplaThermostat->isThermostatDisabled())
        {
          if(suplaThermostat->isManualModeEnabled())
          {
            masterTable[MODBUS_MASTER_ELPIAST_SET_MODE] = 0x01;
          }
          else
          {
            masterTable[MODBUS_MASTER_ELPIAST_SET_MODE] = 0x02;
          }
        }
        else
        {
          masterTable[MODBUS_MASTER_ELPIAST_SET_MODE] = 0x00;
        }
      }
    }
  }
}
