#ifndef MODBUS_MASTER_RTU_H
#define MODBUS_MASTER_RTU_H

#include "Arduino.h"

#define MODE SERIAL_8N1 // data bits | (O) odd, (E) even, (N) no parity | stop bits

#define PINOUT_SERIAL1_RX 18
#define PINOUT_SERIAL1_TX 17

#define FRAME_SIZE  512

enum ModbusMasterError : uint8_t
{
	MODBUS_MASTER_ERROR_NONE,
	MODBUS_MASTER_ERROR_ILLEGAL_DATA_FUNCTION = 0x01,
	MODBUS_MASTER_ERROR_ILLEGAL_DATA_ADDRESS = 0x02,
	MODBUS_MASTER_ERROR_ILLEGAL_DATA_VALUE = 0x03
};

enum ModbusMasterFunction : uint8_t
{
	MODBUS_MASTER_FUNCTION_NONE,
	MODBUS_MASTER_FUNCTION_READ_HOLDING_REGISTERS = 0x03,
	MODBUS_MASTER_FUNCTION_WRITE_SINGLE_REGISTER = 0x06,
	MODBUS_MASTER_FUNCTION_WRITE_MULTIPLE_REGISTERS = 0x10
};

enum ModbusMasterStatus : uint8_t
{
	MODBUS_MASTER_STATUS_PREPARE,
	MODBUS_MASTER_STATUS_REQUEST,
	MODBUS_MASTER_STATUS_RESPONSE,
	MODBUS_MASTER_STATUS_SAVE,
	MODBUS_MASTER_STATUS_OK,
	MODBUS_MASTER_STATUS_ERROR_TIMEOUT,
	MODBUS_MASTER_STATUS_ERROR_CRC,
	MODBUS_MASTER_STATUS_ILLEGAL_DATA_FUNCTION,
	MODBUS_MASTER_STATUS_ILLEGAL_DATA_ADDRESS,
	MODBUS_MASTER_STATUS_ILLEGAL_DATA_VALUE
};

class ModbusMasterRTU
{
	public:

	ModbusMasterRTU(HardwareSerial *port, uint32_t baud);

	uint8_t readHoldingRegisters(const uint8_t id, const uint16_t address, const uint16_t quantity, uint16_t *data, const uint16_t offset, uint64_t timeout);
	uint8_t writeSingleRegisters(const uint8_t id, const uint16_t address, const uint16_t data, const uint16_t offset, uint64_t timeout);
	uint8_t writeMultipleRegisters(const uint8_t id, const uint16_t address, const uint16_t quantity, const uint16_t *data, const uint16_t offset, uint64_t timeout);

	void setREDE(uint8_t pinREDE);

	uint16_t conversionToUint16(uint32_t variable, bool bigEndian);
	uint32_t conversionToUint32(uint16_t variable0, uint16_t variable1, bool bigEndian);
	float conversionToFloat(uint32_t variable);

	protected:

	void prepare();

	void sendRequest();
	void readResponse();

	uint16_t calculateCRC16(uint8_t *data, uint8_t length);

	HardwareSerial *port;

	uint8_t pinREDE = -1;
	
	uint8_t status;

	uint8_t tx[FRAME_SIZE] = {};
	uint8_t rx[FRAME_SIZE] = {};

	uint16_t txQuantity = 0;
	uint16_t rxQuantity = 0;
	uint16_t rxQuantityResponse = 0;

	uint16_t t1_5;
	uint16_t t3_5;

	uint64_t timeout = 0;
	uint64_t lastMillis = 0;
};

#endif
