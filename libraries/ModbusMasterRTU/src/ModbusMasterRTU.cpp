#include "ModbusMasterRTU.h"

ModbusMasterRTU::ModbusMasterRTU(HardwareSerial *port, uint32_t baud)
{
	#ifdef ARDUINO_ARCH_ESP8266
	(*port).begin(baud, MODE);
	#elif ARDUINO_ARCH_ESP32
	(*port).begin(baud, MODE, PINOUT_SERIAL1_RX, PINOUT_SERIAL1_TX);
	#endif
	
	this->port = port;

	if(baud > 19200)
	{
		t1_5 = 750; 
		t3_5 = 1750; 
	}
	else 
	{
		t1_5 = 15000000 / baud;
		t3_5 = 35000000 / baud;
	}
}

bool ModbusMasterRTU::isBusy()
{
	return busy;
}

void ModbusMasterRTU::resetStatus()
{
	busy = true;
	
	status = MODBUS_STATUS_PREPARE;
	
	lastMillis = millis();
}

void ModbusMasterRTU::setTimeout(uint64_t timeout)
{
	this->timeout = timeout;	
}

uint64_t ModbusMasterRTU::getTimeout()
{
	return timeout;
}

void ModbusMasterRTU::setREDE(uint8_t pinREDE)
{
	this->pinREDE = pinREDE;

	pinMode(pinREDE, OUTPUT);
	digitalWrite(pinREDE, LOW);
}

uint8_t ModbusMasterRTU::readHoldingRegisters(const uint8_t id, const uint16_t address, const uint16_t quantity, uint16_t *data, const uint16_t offset)
{
	function = MODBUS_FUNCTION_READ_HOLDING_REGISTERS;
	
	if(status == MODBUS_STATUS_PREPARE)
	{
		prepare();
	}
	else if(status == MODBUS_STATUS_REQUEST)
	{		
		tx[0] = id;
		tx[1] = function;
		
		tx[2] = (uint8_t)(address >> 8);
		tx[3] = (uint8_t)(address & 0xFF);
		tx[4] = (uint8_t)(quantity >> 8);
		tx[5] = (uint8_t)(quantity & 0xFF);
		
		uint16_t calculateCRC = calculateCRC16(tx, 6);
		
		tx[6] = (uint8_t)(calculateCRC & 0xFF);
		tx[7] = (uint8_t)(calculateCRC >> 8);
		
		request(tx, 8);
		
		status = MODBUS_STATUS_RESPONSE;
	}
	else if(status == MODBUS_STATUS_RESPONSE)
	{
		response();
	}
	else if(status == MODBUS_STATUS_SAVE)
	{
		for(uint8_t i = 0; i < quantity; i++)
		{
			data[i + offset] = (rx[2 * i + 3] << 8) | rx[2 * i + 4];
		}
		
		status = MODBUS_STATUS_OK;
	}
	else if(complite())
	{
		busy = false;
	}
	
	return status;
}

uint8_t ModbusMasterRTU::writeMultipleRegisters(const uint8_t id, const uint16_t address, const uint16_t quantity, const uint16_t *data, const uint16_t offset)
{
	function = MODBUS_FUNCTION_WRITE_MULTIPLE_REGISTERS;
	
	if(status == MODBUS_STATUS_PREPARE)
	{
		prepare();
	}
	else if(status == MODBUS_STATUS_REQUEST)
	{		
		tx[0] = id;
		tx[1] = function;
		
		tx[2] = (uint8_t)(address >> 8);
		tx[3] = (uint8_t)(address & 0xFF);
		tx[4] = (uint8_t)(quantity >> 8);
		tx[5] = (uint8_t)(quantity & 0xFF);
		tx[6] = (uint8_t)((2 * quantity) & 0xFF);
		
		for(uint8_t i = 0; i < quantity; i++)
		{
			tx[2 * i + 7] = (uint8_t)(data[i + offset] >> 8);
			tx[2 * i + 8] = (uint8_t)(data[i + offset] & 0xFF);
		}
		
		uint16_t calculateCRC = calculateCRC16(tx, 2 * quantity + 7);
		
		tx[2 * quantity + 7] = (uint8_t)(calculateCRC & 0xFF);
		tx[2 * quantity + 8] = (uint8_t)(calculateCRC >> 8);
		
		request(tx, 2 * quantity + 9);
		
		status = MODBUS_STATUS_RESPONSE;
	}
	else if(status == MODBUS_STATUS_RESPONSE)
	{
		response();
	}
	else if(complite())
	{
		busy = false;
	}
	
	return status;
}

uint16_t ModbusMasterRTU::conversionToUint16(uint32_t variable, bool bigEndian)
{
	if(bigEndian)
	{
		return (variable >> 16);
	}
	else
	{
		return variable;
	}
}

uint32_t ModbusMasterRTU::conversionToUint32(uint16_t variable0, uint16_t variable1, bool bigEndian)
{
	if(bigEndian)
	{
		return ((variable0 << 16) | variable1);
	}
	else
	{
		return ((variable1 << 16) | variable0);
	}
}

float ModbusMasterRTU::conversionToFloat(uint32_t variable)
{
	return *(float*)&variable;
}

void ModbusMasterRTU::prepare()
{
	if(millis() - lastMillis > 50)
	{
		status = MODBUS_STATUS_REQUEST;
		
		memset(tx, 0, sizeof(tx));
		memset(rx, 0, sizeof(rx));
		
		lastMillis = millis();
	}
}

void ModbusMasterRTU::request(uint8_t *data, uint8_t length)
{
	if(pinREDE != -1)
	{
		digitalWrite(pinREDE, HIGH);
	}
	
	for(uint8_t i = 0; i < length; i++)
	{
		(*port).write(data[i]);
	}

	(*port).flush();

	delayMicroseconds(t3_5);

	if(pinREDE != -1)
	{
		digitalWrite(pinREDE, LOW);
	}
}

void ModbusMasterRTU::response()
{
	if((*port).available())
	{
		lastMillis = millis();
		
		uint8_t frameQuantity = 0;
		
		while((*port).available())
		{
			if(frameQuantity == FRAME_SIZE)
			{
				rx[FRAME_SIZE] = (*port).read();
			}
			else
			{
				rx[frameQuantity++] = (*port).read();
			}
			
			delayMicroseconds(t1_5);
		}
		
		uint16_t calculateCRC = calculateCRC16(rx, frameQuantity - 2);
		
		if(calculateCRC == (((rx[frameQuantity - 1] << 8) | rx[frameQuantity - 2])))
		{
			if(function == MODBUS_FUNCTION_READ_HOLDING_REGISTERS)
			{
				if(rx[1] == MODBUS_FUNCTION_READ_HOLDING_REGISTERS)
				{
					status = MODBUS_STATUS_SAVE;
				}
				else
				{
					status = MODBUS_STATUS_ILLEGAL_DATA_FUNCTION;
				}
			}
			else if(function == MODBUS_FUNCTION_WRITE_MULTIPLE_REGISTERS)
			{
				if(rx[1] == MODBUS_FUNCTION_WRITE_MULTIPLE_REGISTERS)
				{
					status = MODBUS_STATUS_OK;
				}
				else
				{
					status = MODBUS_STATUS_ILLEGAL_DATA_FUNCTION;
				}
			}
		}
		else
		{
			status = MODBUS_STATUS_ERROR_CRC;
		}
	}
	else if(millis() - lastMillis > timeout)
	{
		status = MODBUS_STATUS_ERROR_TIMEOUT;
	}
}

bool ModbusMasterRTU::complite()
{
	return status == MODBUS_STATUS_OK || status == MODBUS_STATUS_ERROR_TIMEOUT || status == MODBUS_STATUS_ERROR_CRC || status == MODBUS_STATUS_ILLEGAL_DATA_FUNCTION || status == MODBUS_STATUS_ILLEGAL_DATA_ADDRESS || status == MODBUS_STATUS_ILLEGAL_DATA_VALUE;
}

uint16_t ModbusMasterRTU::calculateCRC16(uint8_t *data, uint8_t length)
{
	uint16_t crc16 = 0xFFFF;

	for(uint8_t i = 0; i < length; i++)
	{
		crc16 = crc16 ^ data[i];

		for(uint8_t j = 0; j < 8; j++)
		{
			if((crc16 & 1) == 1)
			{
				crc16 = (crc16 >> 1) ^ 0xA001;
			}
			else
			{
				crc16 >>= 1;
			}
		}
	}

	return crc16;
}
