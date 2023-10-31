//==============================================================================
//
// Title:		Dyno_API.h
// Purpose:		A short description of the interface.
//
// Created on:	2/27/2021 at 1:50:46 PM by .
// Copyright:	. All Rights Reserved.
//
//==============================================================================

#ifndef __Dyno_API_H__
#define __Dyno_API_H__

#ifdef __cplusplus
    extern "C" {
#endif

//==============================================================================
// Include files
#include "my_utility.h"
#include "yokogawa.h"
#include "cJSON.h"
		
//==============================================================================
// Constants
#define PS_PORT 			10
		
//==============================================================================
// Types
typedef enum {
	LoadController,
	TempSensor,
	TorqueTransducer,
	Chiller,
	HVPower,
} ModBus_Slave;

typedef struct {
	ModBus_Slave 	slave;
	char 			port[12];
	char 			slaveId;
} ModBus_SlaveCfg;

typedef enum {
	load_stop 		=	0,
	load_rotateCW	=	1,
	load_rotateCCW  =	2,
} Load_Mode;

//==============================================================================
// External variables

//==============================================================================
// Global functions
int DYNO_InitializeFromJson(const char *cfgFilePath, char errorMsg[]);
int DYNO_Destroy(void);
int DYNO_CalculateCrc(unsigned char buffer[], unsigned char dataSize);
//int DYNO_ReadTempSensor(double data[], char errorMsg[]);
int DYNO_ReadTorque(double *torque, double *speed, char errorMsg[]);
//int DYNO_ReadFluidSensor(double *flowRate1, double *flowRate2, char errorMsg[]);
int DYNO_SetLoadMode(Load_Mode loadMode, char errorMsg[]);
int DYNO_StartLoad(short start, char errorMsg[]);
int DYNO_SetLoadSpeed(unsigned short loadSpeed, char errorMsg[]);
int DYNO_EnableChiller(unsigned char enable, char errorMsg[]);
int DYNO_ConfigurePS(float voltage, float current, unsigned char enable, float ovp, float ocp, char errorMsg[]);
int DYNO_ReadPS(float *outputVoltage, float *outputCurrent, char *isStopped, char *emergencyState, char *alertCode, char errorMsg[]);
int DYNO_ConfigurePowerAnalyzer(const char *ipAddress, char errorMsg[]);
int DYNO_ReadPowerAnalyzer(float data[], char errorMsg[]);
int DYNO_ReadModbus(double *torque, double *speed, double *psVoltage, double 
					*psCurrent,/* double temp[], double flowRate[],*/ char errorMsg[]);
//float buf2flowRate(const unsigned char *f);

int LOAD_SetTorqueMode(unsigned char enable, char errorMsg[]);
int LOAD_SetTorque(double torqueSet, char errorMsg[]);

#ifdef __cplusplus
    }
#endif

#endif  /* ndef __Dyno_API_H__ */
