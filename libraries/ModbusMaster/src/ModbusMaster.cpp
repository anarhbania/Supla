#include "ModbusMaster.h"

ModbusMaster::ModbusMaster(HardwareSerial *port, uint32_t baud, uint8_t slaveID, uint16_t registersAddress, uint16_t *registers, uint16_t registersSize, uint64_t timeout)
{
	#ifdef ARDUINO_ARCH_ESP8266
	(*port).begin(baud, MODE);
	#elif ARDUINO_ARCH_ESP32
	(*port).begin(baud, MODE, PINOUT_SERIAL1_RX, PINOUT_SERIAL1_TX);
	#endif
	
	this->port = port;
	this->slaveID = slaveID;
	this->registersAddress = registersAddress;
	this->registers = registers;
	this->registersSize = registersSize;
	this->timeout = timeout;

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

void ModbusMaster::setREDE(uint8_t pinREDE)
{
	this->pinREDE = pinREDE;

	pinMode(pinREDE, OUTPUT);
	digitalWrite(pinREDE, LOW);
}

uint16_t ModbusMaster::conversionToUint16(uint32_t variable, bool bigEndian)
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

uint32_t ModbusMaster::conversionToUint32(uint16_t variable0, uint16_t variable1, bool bigEndian)
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

float ModbusMaster::conversionToFloat(uint32_t variable)
{
	return *(float*)&variable;
}

void ModbusMaster::sendAnswer(uint8_t length)
{
	if(pinREDE != -1)
	{
		digitalWrite(pinREDE, HIGH);
	}
	
	for(uint8_t i = 0; i < length; i++)
	{
		(*port).write(frame[i]);
	}

	(*port).flush();

	delayMicroseconds(t3_5);

	if(pinREDE != -1)
	{
		digitalWrite(pinREDE, LOW);
	}
}

void ModbusMaster::sendException(uint8_t function, uint8_t exception)
{
	frame[0] = slaveID;
	frame[1] = (0x80 | function);
	frame[2] = exception;

	uint16_t calculateCRC = ModbusSlave::calculateCRC16(3);
	frame[3] = calculateCRC >> 8;
	frame[4] = calculateCRC & 0xFF;

	ModbusSlave::sendAnswer(5);
}

uint16_t ModbusMaster::calculateCRC16(uint8_t length)
{
	uint16_t crc16 = 0xFFFF;

	for(uint8_t i = 0; i < length; i++)
	{
		crc16 = crc16 ^ frame[i];

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
