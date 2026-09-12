#include <SuplaDevice.h>
#include <supla/control/button.h>
#include <supla/device/status_led.h>
#include <supla/device/supla_ca_cert.h>
#include <supla/network/esp_web_server.h>
#include <supla/network/esp_wifi.h>
#include <supla/network/html/device_info.h>
#include <supla/network/html/protocol_parameters.h>
#include <supla/network/html/wifi_parameters.h>
#include <supla/storage/littlefs_config.h>

#include <supla/sensor/electricity_meter.h>

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
Supla::ESPWifi wifi;
Supla::EspWebServer suplaServer;
Supla::LittleFsConfig configSupla;

auto suplaButtonCfg = new Supla::Control::Button(PINOUT_BUTTON, true, true);

#define MODBUS_MASTER_EM_ID                 3
#define MODBUS_MASTER_EM_ADDRESS_START 0x0000
#define MODBUS_MASTER_EM_ADDRESS_SIZE  0x0040
#define MODBUS_MASTER_EM_ANSWER_TIMEOUT   500

enum ModbusMasterVariablesEM // DTS1946-4P
{
	MODBUS_MASTER_EM_VOLTAGE_L1 = 0x00, // voltage [V]
	MODBUS_MASTER_EM_VOLTAGE_L2 = 0x02, // voltage [V]
	MODBUS_MASTER_EM_VOLTAGE_L3 = 0x04, // voltage [V]
	MODBUS_MASTER_EM_VOLTAGE_L1L2 = 0x06, // voltage [V]
	MODBUS_MASTER_EM_VOLTAGE_L2L3 = 0x08, // voltage [V]
	MODBUS_MASTER_EM_VOLTAGE_L3L1 = 0x0A, // voltage [V]
	MODBUS_MASTER_EM_CURRENT_L1 = 0x0C, // current [A]
	MODBUS_MASTER_EM_CURRENT_L2 = 0x0E, // current [A]
	MODBUS_MASTER_EM_CURRENT_L3 = 0x10, // current [A]
	MODBUS_MASTER_EM_POWER_L1 = 0x12, // power [kWh]
	MODBUS_MASTER_EM_POWER_L2 = 0x14, // power [kWh]
	MODBUS_MASTER_EM_POWER_L3 = 0x16, // power [kWh]
	MODBUS_MASTER_EM_POWER_POSITIVE = 0x34, // energy [kWh]
	MODBUS_MASTER_EM_POWER_REVERSE = 0x36 // energy [kWh]
};

bool masterBigEndian = true;

uint16_t masterTable[MODBUS_MASTER_EM_ADDRESS_SIZE];

ModbusMasterRTU Master(&Serial1, 9600);

auto suplaEM = new Supla::Sensor::ElectricityMeter;

void setup()
{
  suplaButtonCfg->configureAsConfigButton(&SuplaDevice);

  new Supla::Html::DeviceInfo(&SuplaDevice);
  new Supla::Html::WifiParameters;
  new Supla::Html::ProtocolParameters;

  SuplaDevice.setSuplaCACert(suplaCACert);
  SuplaDevice.setSupla3rdPartyCACert(supla3rdCACert);

  SuplaDevice.begin();

  Master.setREDE(PINOUT_REDE);
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

      if(Master.readHoldingRegisters(MODBUS_MASTER_EM_ID, MODBUS_MASTER_EM_ADDRESS_START, MODBUS_MASTER_EM_POWER_REVERSE, masterTable, 0, MODBUS_MASTER_EM_ANSWER_TIMEOUT) == MODBUS_MASTER_STATUS_OK)
      {
        suplaEM->setVoltage(0, (int16_t)(100 * Master.conversionToFloat(Master.conversionToUint32(masterTable[MODBUS_MASTER_EM_VOLTAGE_L1], masterTable[MODBUS_MASTER_EM_VOLTAGE_L1 + 1], masterBigEndian))));
        suplaEM->setVoltage(1, (int16_t)(100 * Master.conversionToFloat(Master.conversionToUint32(masterTable[MODBUS_MASTER_EM_VOLTAGE_L2], masterTable[MODBUS_MASTER_EM_VOLTAGE_L2 + 1], masterBigEndian))));
        suplaEM->setVoltage(2, (int16_t)(100 * Master.conversionToFloat(Master.conversionToUint32(masterTable[MODBUS_MASTER_EM_VOLTAGE_L3], masterTable[MODBUS_MASTER_EM_VOLTAGE_L3 + 1], masterBigEndian))));

        suplaEM->setCurrent(0, (uint32_t)(1000 * Master.conversionToFloat(Master.conversionToUint32(masterTable[MODBUS_MASTER_EM_CURRENT_L1], masterTable[MODBUS_MASTER_EM_CURRENT_L1 + 1], masterBigEndian))));
        suplaEM->setCurrent(1, (uint32_t)(1000 * Master.conversionToFloat(Master.conversionToUint32(masterTable[MODBUS_MASTER_EM_CURRENT_L2], masterTable[MODBUS_MASTER_EM_CURRENT_L2 + 1], masterBigEndian))));
        suplaEM->setCurrent(2, (uint32_t)(1000 * Master.conversionToFloat(Master.conversionToUint32(masterTable[MODBUS_MASTER_EM_CURRENT_L3], masterTable[MODBUS_MASTER_EM_CURRENT_L3 + 1], masterBigEndian))));

        suplaEM->setPowerActive(0, (int64_t)(100000 * Master.conversionToFloat(Master.conversionToUint32(masterTable[MODBUS_MASTER_EM_POWER_L1], masterTable[MODBUS_MASTER_EM_POWER_L1 + 1], masterBigEndian))));
        suplaEM->setPowerActive(1, (int64_t)(100000 * Master.conversionToFloat(Master.conversionToUint32(masterTable[MODBUS_MASTER_EM_POWER_L2], masterTable[MODBUS_MASTER_EM_POWER_L2 + 1], masterBigEndian))));
        suplaEM->setPowerActive(2, (int64_t)(100000 * Master.conversionToFloat(Master.conversionToUint32(masterTable[MODBUS_MASTER_EM_POWER_L3], masterTable[MODBUS_MASTER_EM_POWER_L3 + 1], masterBigEndian))));

        suplaEM->setFwdActEnergy(0, (int64_t)(100000 * Master.conversionToFloat(Master.conversionToUint32(masterTable[MODBUS_MASTER_EM_POWER_POSITIVE], masterTable[MODBUS_MASTER_EM_POWER_POSITIVE + 1], masterBigEndian)) / 3));
        suplaEM->setFwdActEnergy(1, (int64_t)(100000 * Master.conversionToFloat(Master.conversionToUint32(masterTable[MODBUS_MASTER_EM_POWER_POSITIVE], masterTable[MODBUS_MASTER_EM_POWER_POSITIVE + 1], masterBigEndian)) / 3));
        suplaEM->setFwdActEnergy(2, (int64_t)(100000 * Master.conversionToFloat(Master.conversionToUint32(masterTable[MODBUS_MASTER_EM_POWER_POSITIVE], masterTable[MODBUS_MASTER_EM_POWER_POSITIVE + 1], masterBigEndian)) / 3));

        suplaEM->setRvrActEnergy(0, (int64_t)(100000 * Master.conversionToFloat(Master.conversionToUint32(masterTable[MODBUS_MASTER_EM_POWER_REVERSE], masterTable[MODBUS_MASTER_EM_POWER_REVERSE + 1], masterBigEndian)) / 3));
        suplaEM->setRvrActEnergy(1, (int64_t)(100000 * Master.conversionToFloat(Master.conversionToUint32(masterTable[MODBUS_MASTER_EM_POWER_REVERSE], masterTable[MODBUS_MASTER_EM_POWER_REVERSE + 1], masterBigEndian)) / 3));
        suplaEM->setRvrActEnergy(2, (int64_t)(100000 * Master.conversionToFloat(Master.conversionToUint32(masterTable[MODBUS_MASTER_EM_POWER_REVERSE], masterTable[MODBUS_MASTER_EM_POWER_REVERSE + 1], masterBigEndian)) / 3));
      }
    }
  }
}
