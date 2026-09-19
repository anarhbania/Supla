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

#define PINOUT_REDE  21

#define MODBUS_MASTER_SUPLA_ID                 2
#define MODBUS_MASTER_SUPLA_ADDRESS_START 0x0000
#define MODBUS_MASTER_SUPLA_ADDRESS_SIZE  0x0065
#define MODBUS_MASTER_SUPLA_ANSWER_TIMEOUT   500

enum ModbusMasterVariablesSupla // Supla
{
	MODBUS_MASTER_SUPLA_TEMPERATURE_1 = 0x02,
	MODBUS_MASTER_SUPLA_TEMPERATURE_2 = 0x04,
	MODBUS_MASTER_SUPLA_TEMPERATURE_3 = 0x06,
	MODBUS_MASTER_SUPLA_TEMPERATURE_4 = 0x08,
	MODBUS_MASTER_SUPLA_TEMPERATURE_5 = 0x0A,
	MODBUS_MASTER_SUPLA_TEMPERATURE_6 = 0x0C,
};

bool masterBigEndian = true;

uint16_t masterTable[MODBUS_MASTER_SUPLA_ADDRESS_SIZE];

ModbusMasterRTU Master;

float temperature[6];

void setup()
{
  Serial.begin(115200);

  temperature[0] = 0.0;
  temperature[1] = 10.0;
  temperature[2] = 20.0;
  temperature[3] = 30.0;
  temperature[4] = 40.0;
  temperature[5] = 50.0;

  Master.setSerial(&Serial1, 9600);
  Master.setREDE(PINOUT_REDE);
}

void loop()
{
  static uint32_t lastTime = 0;
  if(millis() - lastTime > 2000)
  {
    lastTime = millis();

    masterTable[MODBUS_MASTER_SUPLA_TEMPERATURE_1] = Master.conversionToUint16(temperature[0], masterBigEndian);
    masterTable[MODBUS_MASTER_SUPLA_TEMPERATURE_1 + 1] = Master.conversionToUint16(temperature[0], !masterBigEndian);
    masterTable[MODBUS_MASTER_SUPLA_TEMPERATURE_2] = Master.conversionToUint16(temperature[1], masterBigEndian);
    masterTable[MODBUS_MASTER_SUPLA_TEMPERATURE_2 + 1] = Master.conversionToUint16(temperature[1], !masterBigEndian);
    masterTable[MODBUS_MASTER_SUPLA_TEMPERATURE_3] = Master.conversionToUint16(temperature[2], masterBigEndian);
    masterTable[MODBUS_MASTER_SUPLA_TEMPERATURE_3 + 1] = Master.conversionToUint16(temperature[2], !masterBigEndian);
    masterTable[MODBUS_MASTER_SUPLA_TEMPERATURE_4] = Master.conversionToUint16(temperature[3], masterBigEndian);
    masterTable[MODBUS_MASTER_SUPLA_TEMPERATURE_4 + 1] = Master.conversionToUint16(temperature[3], !masterBigEndian);
    masterTable[MODBUS_MASTER_SUPLA_TEMPERATURE_5] = Master.conversionToUint16(temperature[4], masterBigEndian);
    masterTable[MODBUS_MASTER_SUPLA_TEMPERATURE_5 + 1] = Master.conversionToUint16(temperature[4], !masterBigEndian);
    masterTable[MODBUS_MASTER_SUPLA_TEMPERATURE_6] = Master.conversionToUint16(temperature[5], masterBigEndian);
    masterTable[MODBUS_MASTER_SUPLA_TEMPERATURE_6 + 1] = Master.conversionToUint16(temperature[5], !masterBigEndian);

    if((Master.writeMultipleRegisters(MODBUS_MASTER_SUPLA_ID, MODBUS_MASTER_SUPLA_ADDRESS_START + MODBUS_MASTER_SUPLA_TEMPERATURE_1, MODBUS_MASTER_SUPLA_TEMPERATURE_6, masterTable, MODBUS_MASTER_SUPLA_TEMPERATURE_1, MODBUS_MASTER_SUPLA_ANSWER_TIMEOUT)) == MODBUS_MASTER_STATUS_OK)
    {
      Serial.println("MultipleRegisters: OK");
    }
    else
    {
      Serial.println("MultipleRegisters: NOK");
    }

    temperature[0]+=0.1;
    temperature[1]+=0.1;
    temperature[2]+=0.1;
    temperature[3]+=0.1;
    temperature[4]+=0.1;
    temperature[5]+=0.1;
  }
}
