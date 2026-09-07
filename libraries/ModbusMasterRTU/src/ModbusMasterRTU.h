#ifndef MODBUS_MASTER_RTU_H
#define MODBUS_MASTER_RTU_H

#include "Arduino.h"

#define MODE SERIAL_8N1 // data bits | (O) odd, (E) even, (N) no parity | stop bits

#define PINOUT_SERIAL1_RX 18
#define PINOUT_SERIAL1_TX 17

#define FRAME_SIZE  512

enum ModbusError : uint8_t
{
	MODBUS_ERROR_NONE,
	MODBUS_ERROR_ILLEGAL_DATA_FUNCTION = 0x01,
	MODBUS_ERROR_ILLEGAL_DATA_ADDRESS = 0x02,
	MODBUS_ERROR_ILLEGAL_DATA_VALUE = 0x03
};

enum ModbusFunction : uint8_t
{
	MODBUS_FUNCTION_NONE,
	MODBUS_FUNCTION_READ_HOLDING_REGISTERS = 0x03,
	MODBUS_FUNCTION_WRITE_SINGLE_REGISTER = 0x06,
	MODBUS_FUNCTION_WRITE_MULTIPLE_REGISTERS = 0x10
};

enum ModbusStatus : uint8_t
{
	MODBUS_STATUS_PREPARE,
	MODBUS_STATUS_REQUEST,
	MODBUS_STATUS_RESPONSE,
	MODBUS_STATUS_SAVE,
	MODBUS_STATUS_OK,
	MODBUS_STATUS_ERROR_TIMEOUT,
	MODBUS_STATUS_ERROR_CRC,
	MODBUS_STATUS_ILLEGAL_DATA_FUNCTION,
	MODBUS_STATUS_ILLEGAL_DATA_ADDRESS,
	MODBUS_STATUS_ILLEGAL_DATA_VALUE
};

class ModbusMasterRTU
{
	public:

	ModbusMasterRTU(HardwareSerial *port, uint32_t baud);
	
	bool isBusy();
	void resetStatus();
	
	void setTimeout(uint64_t timeout);
	uint64_t getTimeout();
	
	void setREDE(uint8_t pinREDE);
	
	uint8_t readHoldingRegisters(const uint8_t id, const uint16_t address, const uint16_t quantity, uint16_t *data, const uint16_t offset);
	uint8_t writeMultipleRegisters(const uint8_t id, const uint16_t address, const uint16_t quantity, const uint16_t *data, const uint16_t offset);
	
	uint16_t conversionToUint16(uint32_t variable, bool bigEndian);
	uint32_t conversionToUint32(uint16_t variable0, uint16_t variable1, bool bigEndian);
	float conversionToFloat(uint32_t variable);

	protected:

	void prepare();
	void request(uint8_t *data, uint8_t length);
	void response();
	bool complite();
	
	uint16_t calculateCRC16(uint8_t *data, uint8_t length);

	HardwareSerial *port;

	bool busy = true;

	uint8_t pinREDE = -1;
	uint8_t error = MODBUS_ERROR_NONE;
	uint8_t function = MODBUS_FUNCTION_NONE;
	uint8_t status = MODBUS_STATUS_PREPARE;

	uint8_t tx[FRAME_SIZE] = {};
	uint8_t rx[FRAME_SIZE] = {};

	uint16_t t1_5;
	uint16_t t3_5;

	uint64_t timeout = 500;
	uint64_t lastMillis;
};

#endif
