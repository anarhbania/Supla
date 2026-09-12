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

uint8_t ModbusMasterRTU::readHoldingRegisters(const uint8_t id, const uint16_t address, const uint16_t quantity, uint16_t *data, const uint16_t offset, uint64_t timeout)
{
	this->timeout = timeout;

	prepare();

	tx[0] = id;
	tx[1] = MODBUS_MASTER_FUNCTION_READ_HOLDING_REGISTERS;

	tx[2] = (uint8_t)(address >> 8);
	tx[3] = (uint8_t)(address & 0xFF);
	tx[4] = (uint8_t)(quantity >> 8);
	tx[5] = (uint8_t)(quantity & 0xFF);

	uint16_t calculateCRC = calculateCRC16(tx, 6);

	tx[6] = (uint8_t)(calculateCRC & 0xFF);
	tx[7] = (uint8_t)(calculateCRC >> 8);

	txQuantity = 8;
	sendRequest();

	rxQuantityResponse = 5 + 2 * quantity;
	readResponse();

	if(rxQuantity)
	{
		if(calculateCRC16(rx, rxQuantity - 2) == (uint16_t)((rx[rxQuantity - 1] << 8) | rx[rxQuantity - 2]))
		{
			if(id == rx[0])
			{
				if(MODBUS_MASTER_FUNCTION_READ_HOLDING_REGISTERS == rx[1])
				{
					for(uint16_t i = 0; i < rx[2] / 2; i+=2)
					{
						data[i] = (uint16_t)(rx[3 + i] << 8) | rx[4 + i];
					}

					status = MODBUS_MASTER_STATUS_OK;
				}
			}
		}
		else
		{
			status = MODBUS_MASTER_STATUS_ERROR_CRC;
		}
	}
	else
	{
		status = MODBUS_MASTER_STATUS_ERROR_TIMEOUT;
	}

	return status;
}

void ModbusMasterRTU::setREDE(uint8_t pinREDE)
{
	this->pinREDE = pinREDE;

	pinMode(pinREDE, OUTPUT);
	digitalWrite(pinREDE, LOW);
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
		txQuantity = 0;
		rxQuantity = 0;
		rxQuantityResponse = 0;

		memset(tx, 0, sizeof(tx));
		memset(rx, 0, sizeof(rx));
	}
}

void ModbusMasterRTU::sendRequest()
{
	if(pinREDE != -1)
	{
		digitalWrite(pinREDE, HIGH);
	}

	for(uint8_t i = 0; i < txQuantity; i++)
	{
		(*port).write(tx[i]);
	}

	delayMicroseconds(t3_5);

	(*port).flush();

	if(pinREDE != -1)
	{
		digitalWrite(pinREDE, LOW);
	}
}

void ModbusMasterRTU::readResponse()
{
	lastMillis = millis();
	
	while((millis() - lastMillis < timeout) && rxQuantity < rxQuantityResponse)
	{
		if((*port).available())
		{
			while((*port).available())
			{
				if(rxQuantity == FRAME_SIZE)
				{
					rx[FRAME_SIZE] = (*port).read();
				}
				else
				{
					rx[rxQuantity++] = (*port).read();
				}
				
				delayMicroseconds(t1_5);
			}
		}
	}
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
