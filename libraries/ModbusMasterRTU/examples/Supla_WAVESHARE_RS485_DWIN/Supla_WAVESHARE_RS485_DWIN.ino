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

#define MODBUS_MASTER_DWIN_ID                 1
#define MODBUS_MASTER_DWIN_ADDRESS_START 0x1000
#define MODBUS_MASTER_DWIN_ADDRESS_SIZE  0x0040
#define MODBUS_MASTER_DWIN_ANSWER_TIMEOUT   500

enum ModbusMasterVariablesDWIN // HMI
{
	MODBUS_MASTER_DWIN_PARAMETER_1 = 0x01,
	MODBUS_MASTER_DWIN_PARAMETER_2 = 0x02
};

bool masterBigEndian = true;

uint16_t masterTable[MODBUS_MASTER_DWIN_ADDRESS_SIZE];

ModbusMasterRTU Master(&Serial1, 9600);

void setup()
{
  Serial.begin(115200);

  Master.setREDE(PINOUT_REDE);
}

void loop()
{
  static uint32_t lastTime = 0;
  if(millis() - lastTime > 2000)
  {
    lastTime = millis();

    if(Master.writeSingleRegisters(MODBUS_MASTER_DWIN_ID, MODBUS_MASTER_DWIN_ADDRESS_START, masterTable[MODBUS_MASTER_DWIN_PARAMETER_1], MODBUS_MASTER_DWIN_PARAMETER_1, MODBUS_MASTER_DWIN_ANSWER_TIMEOUT) == MODBUS_MASTER_STATUS_OK)
    {
      Serial.println("SAVE PARAMETER 1: OK");
    }

    delay(1000);

    if(Master.writeSingleRegisters(MODBUS_MASTER_DWIN_ID, MODBUS_MASTER_DWIN_ADDRESS_START, masterTable[MODBUS_MASTER_DWIN_PARAMETER_2], MODBUS_MASTER_DWIN_PARAMETER_2, MODBUS_MASTER_DWIN_ANSWER_TIMEOUT) == MODBUS_MASTER_STATUS_OK)
    {
      Serial.println("SAVE PARAMETER 2: OK");
    }

    masterTable[MODBUS_MASTER_DWIN_PARAMETER_1]++;
    masterTable[MODBUS_MASTER_DWIN_PARAMETER_2]++;
  }
}
