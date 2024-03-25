/**
 * Name: LiMAX17048G
 * Author: Sarojit Koley <koley.sarojit.67@gmail.com>
 * Version: 1.0
 * Description: A library for interfacing the Analog Devices MAXIM MAX17048G+T10
 * 				Lithium fuel gauges. These ICs report the relative state of charge
 * 				of the connected Lithium Ion Polymer battery, and the library 
 * 				can help you configure them and communicate with them
 * Source: https://github.com/SAROJIT67/LiMAX17048G
 * License: Copyright (c) 2024 Sarojit Koley
 *          This library is licensed under the MIT license
 * Filename: LiMAX17048G.cpp
 */


#include "LiMAX17048G.h"
#include "Wire.h"

// Returns a measurement of the voltage of the connected 1S / 2S LiIon battery
// 0-5V range w/ 1.25mV resolution for the MAX17048
// 0-10V range w/ 2.5mV resolution for the MAX17049
double MAX17048G::getCellVolt()
{
    Wire.beginTransmission(MAX1704X_ADDR);
	Wire.write(MAX1704X_VCELL_ADDR);
	Wire.endTransmission(true);
	Wire.requestFrom(MAX1704X_ADDR, (uint8_t)2);
	return ( (Wire.read() << 4) + (Wire.read() >> 4) ) * 0.00125 * _ic;
    delay(1000);   // this delay is importent, otherwise it did not work properly.
}

// Returns the relative state of charge of the connected LiIon Polymer battery
// as a percentage of the full capacity w/ resolution 1/256%
double MAX17048G::getSOC()
{
    Wire.beginTransmission(MAX1704X_ADDR);
	Wire.write(MAX1704X_SOC_ADDR);
	Wire.endTransmission(true);
	Wire.requestFrom(MAX1704X_ADDR, (uint8_t)2);
	return Wire.read() + (double) Wire.read() / 256;
    delay(1000);   // this delay is importent, otherwise it did not work properly.
}

// Returns the production version of the IC
uint16_t MAX17048G::getVersion() 
{
	Wire.beginTransmission(MAX1704X_ADDR);
	Wire.write(MAX1704X_VERSION_ADDR);
	Wire.endTransmission(true);
	Wire.requestFrom(MAX1704X_ADDR, (uint8_t)2);
	return ( Wire.read() << 8 ) + Wire.read();
}

// Return the value used to optimize IC performance to different operating conditions
uint8_t MAX17048G::getCompensateValue() 
{
	Wire.beginTransmission(MAX1704X_ADDR);
	Wire.write(MAX1704X_RCOMP_ADDR);
	Wire.endTransmission(true);
	Wire.requestFrom(MAX1704X_ADDR, (uint8_t)1);
	return Wire.read();
}

// Return the alert threshold as a percentage, below an alert interrupt is generated
uint8_t MAX17048G::getAlertThreshold() 
{
    return ( ~getStatus() & 0x1F ) + 1;
}

// Return the LSByte of the CONFIG register
uint8_t MAX17048G::getStatus()
{
	Wire.beginTransmission(MAX1704X_ADDR);
	Wire.write(MAX1704X_ATHRD_ADDR);
	Wire.endTransmission(true);
	Wire.requestFrom(MAX1704X_ADDR, (uint8_t)1);
	return Wire.read();
}

// Sets a value to the MSB of the CONFIG register used 
// to optimizethe  IC performance to different operating conditions
uint8_t MAX17048G::setCompensation(uint8_t compensation)
{	
	uint8_t status = getStatus();
	Wire.beginTransmission(MAX1704X_ADDR);
	Wire.write(MAX1704X_CONFIG_ADDR);
	Wire.write(compensation);
	Wire.write(status);
	return Wire.endTransmission();
}

// Sets the alert threshold below which an alert interrupt is generated
// The acceptable range is 1-32%. Default threshold is 4%
uint8_t MAX17048G::setAlertThreshold(uint8_t threshold)
{
	if ( threshold > 32 ) threshold = 32;
	else if ( threshold < 1 ) threshold = 1;
	threshold = ( ~threshold + 1 ) & 0x1F;
	
	uint8_t compensation, sleepBit;
	Wire.beginTransmission(MAX1704X_ADDR);
	Wire.write(MAX1704X_CONFIG_ADDR);
	Wire.endTransmission(false);
	Wire.requestFrom(MAX1704X_ADDR, (uint8_t)2);
	compensation = Wire.read();
	sleepBit = Wire.read() & 0x80;
	Wire.beginTransmission(MAX1704X_ADDR);
	Wire.write(MAX1704X_CONFIG_ADDR);
	Wire.write(compensation);
	Wire.write(sleepBit | thrd);
	return Wire.endTransmission();
}

// After an alert interrupt has been generated,
// it clears the alert bit on the CONFIG register
uint8_t MAX17048G::clearAlertInterrupt()
{
	uint8_t compensation = getCompensation();
	uint8_t status = getStatus();
	Wire.beginTransmission(MAX1704X_ADDR);
	Wire.write(MAX1704X_CONFIG_ADDR);
	Wire.write(compensation);
	Wire.write(0xDF & status);
	return Wire.endTransmission();
}

// It puts the MAX1704X to sleep
// All IC operations are halted
uint8_t MAX17048G::sleep()
{
	uint8_t compensation = getCompensation();
	uint8_t threshold = getAlertThreshold();
	
 	Wire.beginTransmission(MAX1704X_ADDR);
	Wire.write(MAX1704X_CONFIG_ADDR);
	Wire.write(compensation);
	Wire.write(0x80 | threshold);
	return Wire.endTransmission();
}

// It wakes the MAX1704X from sleep mode
uint8_t MAX17048G::wake()
{
	uint8_t compensation = getCompensation();
	uint8_t threshold = getAlertThreshold();
	
 	Wire.beginTransmission(MAX1704X_ADDR);
	Wire.write(MAX1704X_CONFIG_ADDR);
	Wire.write(compensation);
	Wire.write(0x7F & threshold);
	return Wire.endTransmission();
    delay(1000);   // this delay is importent, otherwise it did not work properly.
}

// whether the MAX1704X is in sleep mode
boolean MAX17048G::sleeping()
{
	return ( getStatus() & 0x80 ) == 0x80;
}

// It forces the MAX1704X to
// restart fuel-gauge calculations
uint8_t MAX17048G::quickStart()
{
 	Wire.beginTransmission(MAX1704X_ADDR);
	Wire.write(MAX1704X_MODE_ADDR);
	Wire.write(0x40);
	Wire.write(0x00);
	return Wire.endTransmission();
}

// It forces the MAX1704X to completely reset
uint8_t MAX17048G::reset()
{
 	Wire.beginTransmission(MAX1704X_ADDR);
	Wire.write(MAX1704X_COMMAND_ADDR);
	Wire.write(0x54);
	Wire.write(0x00);
	return Wire.endTransmission();
}