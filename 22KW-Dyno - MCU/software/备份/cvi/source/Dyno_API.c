//==============================================================================
//
// Title:		Dyno_API.c
// Purpose:		A short description of the implementation.
//
// Created on:	2/27/2021 at 1:50:46 PM by .
// Copyright:	. All Rights Reserved.
//
//==============================================================================

//==============================================================================
// Include files
#include "Dyno_API.h"
#include <tcpsupp.h>

//==============================================================================
// Constants
#define VISA_BAUD_RATE 		9600
#define RXBUF_SIZE 			64
#define CMD_SIZE 			6
#define DYNO_TCP_TIMEOUT	100
#define MODBUS_NUM_SLAVE 	5
#define MODBUS_READ_TIMEOUT 180
//==============================================================================
// Types
typedef int (*ReadTorquePtr)(double *torque, double *speed, char errorMsg[]);

typedef enum {
	load_none,
	load_speed_mode,
	load_torque_mode,
} LOAD_Option;

typedef struct {
	unsigned int 	tcpPS;
	ViSession 		modbus;
	ViSession		power_analyzer;
	ViSession 		resourceMgr;
	ViSession		rs232;
	char 			loadController;
	/*char 			tempSensor;*/
	char 			torqueTransducer;
	/*char 			chiller;*/
	/*char 			fluidSensor;*/
	char 			waitForPSCfg;
	/*char 			enableFluidSensorReading;*/
	ReadTorquePtr 	readTorque;
	LOAD_Option 	loadOpt;
} ModBus_Cfg;

typedef struct {
	ViSession 	conn;
	double 		*data;
	char 		**itemList;
	int 		numItem;
	char 		*cmdFetchData;
	char  		ipAddress[32];
} YKGW_Cfg;

//==============================================================================
// Static global variables
static ModBus_Cfg 	*cfg 	= 	NULL;

//==============================================================================
// Static functions
int DYNO_Initialize(const char *visa, char loadController, char torqueTransducer, 
					char tempSensor, char chiller, char fluidSensor, const char *hvps, char errorMsg[]);
int DYNO_OpenVisa(const char *visa, char errorMsg[]);
int DYNO_OpenRS232(const char *visa, char errorMsg[]);
int DYNO_visaQuery(unsigned char cmd[], unsigned char rxbuf[], short rxBufSize, int waitMs, char errorMsg[]);
int DYNO_tcpQuery(unsigned char cmd[], unsigned char rxbuf[], short rxBufSize, int waitMs, char errorMsg[]);
int DYNO_ReadTorque(double *torque, double *speed, char errorMsg[]);
float buf2torque(const char *f);
float buf2speed(const char *f);
float buf2data_rs232(const unsigned char *f);
unsigned char *buildPSCmd(unsigned char cmd[], unsigned char mode, float data);
unsigned int CRC16_2(unsigned char *buf, int len);
int DYNO_SetTorqSpeedCallback(ReadTorquePtr CallbackPtr);
int Modbus_ReadTorque(double *torque, double *speed, char errorMsg[]);
int RS232_ReadTorque(double *torque, double *speed, char errorMsg[]);

//==============================================================================
// Global variables

//==============================================================================
// Global functions
// ------------------- SET CALLBACK FOR TORQUE & SPEED READ --------------------
int DYNO_SetTorqSpeedCallback(ReadTorquePtr CallbackPtr) {
	int error = 0;
	if (cfg) {
		cfg->readTorque = CallbackPtr;
	} else {
		errChk(CONFIG_NOT_INITIALIZED);
	}
Error:
	return error;
}


// ----------------------------- INITIALIZE DYNO -------------------------------
int DYNO_Initialize(const char *visa, char loadController, char torqueTransducer, 
					char tempSensor, char chiller, char fluidSensor, const char *hvps, char errorMsg[]) {
	int 		error 		=	0;
	ErrMsg 		errMsg 		=	{0};
	
	DYNO_Destroy();
	errChk(utility_checkVisaName(visa, errMsg));
	errChk(DYNO_OpenVisa(visa, errMsg));
	error = ConnectToTCPServerEx(&cfg->tcpPS, PS_PORT, hvps, 
								 NULL, NULL, 1000, TCP_ANY_LOCAL_PORT);
	if (error<0) {
		sprintf(errMsg, "-直流电源通讯失败", GetTCPErrorString(error));//INITIALIZE POWER SUPPLY ERROR
		errChk(error);
	}
	errChk(DYNO_SetTorqSpeedCallback(Modbus_ReadTorque));
	cfg->loadController 	= loadController;
	/*cfg->tempSensor 		= tempSensor;*/
	cfg->torqueTransducer 	= torqueTransducer;
	//cfg->chiller 			= chiller;
	//cfg->fluidSensor 		= fluidSensor;
Error:
	reportError();
	return error;
}


// ----------------------------- INITIALIZE DYNO -------------------------------
int DYNO_InitializeFromJson(const char *cfgFilePath, char errorMsg[]) {
	int 		error 								=	0,
				i 									=	0;
	ErrMsg 		errMsg 								=	{0};
	FILE 		*f 									=	NULL;
	char 		data[MODBUS_NUM_SLAVE] 				=	{0},
				psIpAddress[20]						=	{0},
				buf[1024]							=	{0};
	cJSON		*obj 								=	NULL,
				*hwcfg 								=	NULL;
	const char 	*slave[MODBUS_NUM_SLAVE]			=	{"modbus_loadController", 
														"modbus_torqueTransducer", 
										 				"modbus_temperatureSensor", 
														"modbus_chiller", 
														"modbus_fluidSensor"};
	
	errChk(utility_doesFileExists(cfgFilePath, NULL, errMsg));
	nullChk(f = fopen(cfgFilePath, "r"));
	fread(buf, 1, 1024, f);
	nullChk(hwcfg = cJSON_Parse(buf));
	// get hardware resources
	nullChk(obj = cJSON_GetObjectItem(hwcfg, "power_analyzer"));
	errChk(YKGW_Config(obj->valuestring, errMsg));
	// get modbus settings
	for (i = 0; i < MODBUS_NUM_SLAVE; i++) {
		nullChk(obj = cJSON_GetObjectItem(hwcfg, slave[i]));
		data[i] = (char) obj->valueint;
	}
	nullChk(obj = cJSON_GetObjectItem(hwcfg, "power_supply"));
	strcpy(psIpAddress, obj->valuestring);
	nullChk(obj = cJSON_GetObjectItem(hwcfg, "modbus"));
	errChk(DYNO_Initialize(obj->valuestring, data[0], data[1], data[2], data[3], data[4], 
						   psIpAddress, errMsg));
	// check if use RS232 for torque and speed measurement
	nullChk(obj = cJSON_GetObjectItem(hwcfg, "use_rs232"));
	if (obj->valueint) {
		nullChk(obj = cJSON_GetObjectItem(hwcfg, "rs232"));
		errChk(DYNO_OpenRS232(obj->valuestring, errMsg));
		errChk(DYNO_SetTorqSpeedCallback(RS232_ReadTorque));
	} 
Error:
	if (f) 		fclose(f);
	if (hwcfg) 	cJSON_Delete(hwcfg);
	reportError();
	return error;
}


// ----------------------------- DESTROY MODBUS --------------------------------
int DYNO_Destroy(void) {
	if (cfg) {
		if (cfg->rs232>0) 			viClose(cfg->rs232);
		if (cfg->modbus>0)			viClose(cfg->modbus);
		if (cfg->tcpPS>0)	 		DisconnectFromTCPServer(cfg->tcpPS);
		if (cfg->power_analyzer)	viClose(cfg->power_analyzer);
		free(cfg);
		cfg = NULL;
	}
	return 0;
}


// ------------------------ OPEN VISA CONNECTION -------------------------------
int DYNO_OpenVisa(const char *visa, char errorMsg[]) {
	int 		error 	=	0;
	ErrMsg 		errMsg 	= 	{0};

	nullChk(cfg = malloc(sizeof(ModBus_Cfg)));
	memset(cfg, 0, sizeof(ModBus_Cfg));
	
	// open visa connection
	errChk(viOpenDefaultRM(&cfg->resourceMgr));
	error = viOpen(cfg->resourceMgr, visa, VI_NULL, 1000, &cfg->modbus);
 	if (error>=0) {
		errChk(viSetAttribute(cfg->modbus, VI_ATTR_ASRL_BAUD, VISA_BAUD_RATE));
		errChk(viSetAttribute(cfg->modbus, VI_ATTR_TMO_VALUE, 1000));
		// enable LINE_FEED for termination byte
		errChk(viSetAttribute(cfg->modbus, VI_ATTR_TERMCHAR, 0x0A));
		errChk(viSetAttribute(cfg->modbus, VI_ATTR_ASRL_DISCARD_NULL, FALSE));
		errChk(viSetAttribute(cfg->modbus, VI_ATTR_TERMCHAR_EN, VI_FALSE));
	 	errChk(viSetAttribute(cfg->modbus, VI_ATTR_ASRL_END_IN, VI_ASRL_END_NONE));
	 	errChk(viSetAttribute(cfg->modbus, VI_ATTR_ASRL_END_OUT, VI_ASRL_END_NONE));
		errChk(viFlush(cfg->modbus, VI_IO_IN_BUF));		
	} else if (error == -1073807246) {
		sprintf(errMsg, "-com口被占用", visa);//COM PORT OCCUPY BY OTHER APPLICATION
		errChk(VISA_OCCUPY_BY_OTHER);
	} else if (error == -1073807343) {
		sprintf(errMsg, "无效串口", visa);//INVALID COM PORT NAME
		errChk(VISA_INVALID_NAME);
	} else {
		sprintf(errMsg, "一般性错误", visa);//GENERIC MODBUS ERROR
		errChk(error);
	}
Error:
	reportError();
	return error;
}


// ------------------------ OPEN RS232 CONNECTION -------------------------------
int DYNO_OpenRS232(const char *visa, char errorMsg[]) {
	int 		error 	=	0;
	ErrMsg 		errMsg 	= 	{0};

	nullChk(cfg);
	error = viOpen(cfg->resourceMgr, visa, VI_NULL, 1000, &cfg->rs232);
 	if (error>=0) {
		errChk(viSetAttribute(cfg->rs232, VI_ATTR_ASRL_BAUD, 2400));
		errChk(viSetAttribute(cfg->rs232, VI_ATTR_TMO_VALUE, 1000));
		// enable LINE_FEED for termination byte
		errChk(viSetAttribute(cfg->rs232, VI_ATTR_TERMCHAR, 0x0A));
		errChk(viSetAttribute(cfg->rs232, VI_ATTR_ASRL_DISCARD_NULL, FALSE));
		errChk(viSetAttribute(cfg->rs232, VI_ATTR_TERMCHAR_EN, VI_FALSE));
	 	errChk(viSetAttribute(cfg->rs232, VI_ATTR_ASRL_END_IN, VI_ASRL_END_NONE));
	 	errChk(viSetAttribute(cfg->rs232, VI_ATTR_ASRL_END_OUT, VI_ASRL_END_NONE));
		errChk(viFlush(cfg->rs232, VI_IO_IN_BUF));		
	} else if (error == -1073807246) {
		sprintf(errMsg, "仪表通讯口被占用", visa);//COM PORT OCCUPY BY OTHER APPLICATION-
		errChk(VISA_OCCUPY_BY_OTHER);
	} else if (error == -1073807343) {
		sprintf(errMsg, "无效串口", visa);//INVALID COM PORT NAME
		errChk(VISA_INVALID_NAME);
	} else {
		sprintf(errMsg, "一般性错误", visa);//GENERIC MODBUS ERROR
		errChk(error);
	}
Error:
	reportError();
	return error;
}


// ------------------------------- READ MODBUS ------------------------------------
int DYNO_ReadModbus(double *torque, double *speed, double *psVoltage, double 
					*psCurrent,/* double temp[], double flowRate[],*/ char errorMsg[]) {
	int 			error 				=	0;
	ErrMsg 			errMsg 				=	{0};
	float 			data[2] 			=	{0};
	
	DisableBreakOnLibraryErrors();
	/*errChk(DYNO_ReadTempSensor(temp, errMsg));*/
	// HVPS	
	data[0] = *torque;
	data[1] = *speed;
	if (!cfg->waitForPSCfg) {
		DYNO_ReadPS(data, data+1, NULL, NULL, NULL, errMsg);
	}
	*psVoltage = data[0];
	*psCurrent = data[1];
	// torque transducer
	*torque = NotANumber();
	*speed = NotANumber();
	DYNO_ReadTorque(torque, speed, errMsg);
	//DYNO_ReadFluidSensor(flowRate, flowRate+1, errMsg);
	EnableBreakOnLibraryErrors();
Error:
	reportError();
	return error;
}


// ------------------------------- READ TEMPERATURE -------------------------------
//int DYNO_ReadTempSensor(double data[], char errorMsg[]) {
//	int 			error 				=	0,
//					i 					=	0;
//	ErrMsg 			errMsg 				=	{0};
//	unsigned char 	txbuf[] 			=	{cfg->tempSensor, 0x3, 0x0, 0x0, 0x0, 0x6},
//					rxbuf[17]			=	{0};
//	char			f[2] 				=	{0};
//	errChk(DYNO_visaQuery(txbuf, rxbuf, 17, MODBUS_READ_TIMEOUT, errMsg));
//	for (i = 0; i < 6; i++) {
//		memcpy(f, rxbuf+3+2*i, 2);
//		data[i] = 800.0*((f[0]<<8 & 0xFF00) + f[1])/65536.0-200.0;
//	} 
//Error:
//	reportError();
//	return error;
//}


// ------------------------------- READ TORQUE VALUE -------------------------------
int DYNO_ReadTorque(double *torque, double *speed, char errorMsg[]){
	int 			error 				=	0;
	
	if (cfg) {
		errChk((cfg->readTorque)(torque, speed, errorMsg));
	} else {
		sprintf(errorMsg, "modbus通讯失败");//modbus has not been initialized-
		errChk(CONFIG_NOT_INITIALIZED);
	}
Error:
	return error;
}


// ------------------------------- READ TORQUE VALUE -------------------------------
int Modbus_ReadTorque(double *torque, double *speed, char errorMsg[]){
	int 			error 				=	0;
	ErrMsg 			errMsg 				=	{0};
	unsigned char 	txbuf[] 			=	{cfg->torqueTransducer, 0x4, 0x0, 0x0, 0x0, 0x4},
					rxbuf[12]			=	{0};
	
	errChk(DYNO_visaQuery(txbuf, rxbuf, 12, MODBUS_READ_TIMEOUT, errMsg));
	if (torque)		*torque = buf2torque((char *) rxbuf+3);
	if (speed) 		*speed  = buf2speed((char *) rxbuf+7);
Error:
	reportError();
	return error;
}


// ------------------------------- READ TORQUE VALUE -------------------------------
int RS232_ReadTorque(double *torque, double *speed, char errorMsg[]){
	int 			error 				=	0;
	ErrMsg 			errMsg 				=	{0};
	unsigned char 	txbuf[] 			=	{0x20},
					rxbuf[15]			=	{0};
	unsigned int	count 				=	0;
	
	errChk(viFlush(cfg->rs232, VI_IO_IN_BUF));
	errChk(viWrite(cfg->rs232, txbuf, 1, &count));
	Sleep(MODBUS_READ_TIMEOUT);
	errChk(viRead(cfg->rs232, rxbuf, 15, &count));
	utility_putHex(rxbuf, count);
	if (torque)		*torque = buf2data_rs232(rxbuf);
	if (speed) 		*speed  = buf2data_rs232(rxbuf+5);
Error:
	reportError();
	return error;
}

/*
// ------------------------------- READ FLOW RATE ---------------------------------- 
int DYNO_ReadFluidSensor(double *flowRate1, double *flowRate2, char errorMsg[]){
	int 			error 				=	0;
	ErrMsg 			errMsg 				=	{0};
	unsigned char 	txbuf[] 			=	{cfg->fluidSensor, 0x3, 0x1, 0x0, 0x0, 0x4},
					rxbuf[12]			=	{0};
	
	if (cfg->enableFluidSensorReading) {
		errChk(DYNO_visaQuery(txbuf, rxbuf, 12, MODBUS_READ_TIMEOUT, errMsg));
		if (flowRate1)		*flowRate1 = buf2flowRate(rxbuf+3);
		if (flowRate2) 		*flowRate2 = buf2flowRate(rxbuf+7);
	} else {
		if (flowRate1) 		*flowRate1 = NotANumber();
		if (flowRate2) 		*flowRate2 = NotANumber();
	}
Error:
	reportError();
	return error;
}
*/

// --------------------------------- START/STOP CHILLER ---------------------------- 
//int DYNO_EnableChiller(unsigned char enable, char errorMsg[]) {
//	unsigned char 	txbuf[] 			=	{cfg->chiller, 0x6, 0x2, 0x0, 0x0, enable?0x72:0x70};
//	//cfg->enableFluidSensorReading 		= 	enable;
//	return DYNO_visaQuery(txbuf, NULL, 0, 100, errorMsg);
//}


// --------------------------------- READ PS STATUS --------------------------------
int DYNO_ReadPS(float *outputVoltage, float *outputCurrent, char *isStopped, char *emergencyState, char *alertCode, char errorMsg[]) {
	int 			error 				=	0;
	ErrMsg 			errMsg 				=	{0};
	unsigned char 	txbuf[] 			=	{0x1, 0x3, 0x0, 0x1, 0x0, 0x5},
					rxbuf[15]			=	{0};
	
	errChk(DYNO_tcpQuery(txbuf, rxbuf, 15, MODBUS_READ_TIMEOUT, errMsg));
	if (isStopped) 		*isStopped 		= rxbuf[4];
	if (outputVoltage)	*outputVoltage 	= ((rxbuf[5]<<8 & 0xFF00) + rxbuf[6])/10.0;
	if (outputCurrent) 	*outputCurrent 	= ((rxbuf[7]<<8 & 0xFF00) + rxbuf[8])/10.0;
	if (emergencyState)	*emergencyState	= rxbuf[10];
	if (alertCode) 		*alertCode		= rxbuf[12];
Error:
	reportError();
	return error;
}


// --------------------------------- SET HV POWER ----------------------------------
int DYNO_ConfigurePS(float voltage, float current, unsigned char enable, float ovp, float ocp, char errorMsg[]) {
	int 			error 				=	0;
	ErrMsg 			errMsg 				=	{0};
	unsigned char 	txbuf[] 			=	{0x1, 0x6, 0x3, 0x0, 0x0, 0x0};
	
	cfg->waitForPSCfg = TRUE;
	Sleep(500);
	if (enable) {
		// voltage
		errChk(DYNO_tcpQuery(buildPSCmd(txbuf, 0xED, voltage), NULL, 0, 200, errMsg));
		// current
		errChk(DYNO_tcpQuery(buildPSCmd(txbuf, 0xEE, current), NULL, 0, 200, errMsg));
		// ovp
		errChk(DYNO_tcpQuery(buildPSCmd(txbuf, 0xF0, ovp), NULL, 0, 200, errMsg));
		// ocp
		errChk(DYNO_tcpQuery(buildPSCmd(txbuf, 0xF1, ocp), NULL, 0, 200, errMsg));
	}
	// enable/disable power supply
	txbuf[2] = 0x0;
	txbuf[3] = 0x1;
	txbuf[4] = 0x0;
	txbuf[5] = enable?0xFF:0xAA;
	errChk(DYNO_tcpQuery(txbuf, NULL, 0, 100, errorMsg));
Error:
	cfg->waitForPSCfg = FALSE;
	reportError();
	return error;
}


// ------------------------------- SET LOAD MODE ------------------------------------
int DYNO_SetLoadMode(Load_Mode loadMode, char errorMsg[]) {
	int 			error 				=	0;
	ErrMsg 			errMsg 				=	{0};
	unsigned char 	txbuf[] 			=	{cfg->loadController, 
											0x6, 0xF, 0xA6, 0x0, loadMode};
	errChk(DYNO_visaQuery(txbuf, NULL, 0, 100, errMsg));
	if (loadMode == load_stop) {
		errChk(DYNO_SetLoadSpeed(0, errMsg));
	}
Error:
	reportError();
	return error;
}


// ------------------------------ START LOAD ----------------------------------------
int DYNO_StartLoad(short start, char errorMsg[]) {
	Load_Mode mode = (Load_Mode) start;
	return DYNO_SetLoadMode(mode, errorMsg);
}


// ------------------------------ SET LOAD SPEED ------------------------------------
int DYNO_SetLoadSpeed(unsigned short loadSpeed, char errorMsg[]) {
	unsigned char 	txbuf[] 			=	{cfg->loadController, 
											0x6, 0x10, 0x10, 0x0, 0x0};
	txbuf[4] = loadSpeed>>8 & 0x00FFU;
	txbuf[5] = loadSpeed & 0x00FFU;
	return DYNO_visaQuery(txbuf, NULL, 0, 100, errorMsg);
}


// ------------------------------ SET LOAD TO TORQUE MODE --------------------------
int LOAD_SetTorqueMode(unsigned char enable, char errorMsg[]) {
	int 			error 				=	0;
	ErrMsg 			errMsg 				=	{0};
	unsigned char 	txbuf[] 			=	{cfg->loadController, 
											0x06, 0x0F, 0xBE, 0x00, enable},
					loadStatus[] 		=	{cfg->loadController, 0x03, 0x1B, 0x5B, 0x00, 0x01},
					loadTorqComm[] 		=	{cfg->loadController, 0x06, 0x0F, 0xC7, 0x00, 0x01},
					rxbuf[6] 			=	{0};	
	
	errChk(DYNO_visaQuery(txbuf, NULL, 0, 100, errMsg));
	if (enable) {
		errChk(DYNO_visaQuery(loadStatus, rxbuf, 6, 200, errMsg));
		switch (rxbuf[4]) {
			case 0:
				cfg->loadOpt = load_speed_mode;
				strcpy(errMsg, "转速模式，不能启动扭矩模式");//load in speed mode. can't configure to torque mode
				errChk(LOAD_SET_TORQUE_MODE_FAILURE);
				break;
			case 2:
				cfg->loadOpt = load_torque_mode;
				break;
			default:
				cfg->loadOpt = load_none;
				sprintf(errMsg, "load can't be configured to torque mode(%d)", rxbuf[3]);
				errChk(LOAD_SET_TORQUE_MODE_FAILURE);
				break;
		}
		errChk(DYNO_visaQuery(loadTorqComm, NULL, 0, 100, errMsg));
	} else {
		cfg->loadOpt = load_none;
		errChk(LOAD_SetTorque(0, errMsg));
	}
Error:
	reportError();
	return error;
}


// ------------------------------ SET LOAD TORQUE ----------------------------------
int LOAD_SetTorque(double torqueSet, char errorMsg[]) {
	int 			error 				=	0;
	ErrMsg 			errMsg 				=	{0};
	short 			torque 				=	10*torqueSet;
	unsigned char 	txbuf[] 			=	{cfg->loadController, 
											0x06, 0x10, 0x07, 0x00, 0x00};	
	if (cfg->loadOpt == load_torque_mode ) {
		txbuf[4] = torque>>8 & 0x00FFU;
		txbuf[5] = torque & 0x00FFU;
		errChk(DYNO_visaQuery(txbuf, NULL, 0, 100, errMsg));
	} /*else {
	strcpy(errMsg, "load is not configured correctly. set load to torque mode first");
    errChk(LOAD_SET_TORQUE_MODE_FAILURE);
	}*/
Error:
	reportError();
	return error;
}


// #################################################################################
// ---------------------------------- HELP FUNCTION --------------------------------
// #################################################################################
float buf2torque(const char *f) {
	float fff 	=	0;
	memcpy(&fff, f, 4);
	return (fff-10000)*200/5000;
}


float buf2speed(const char *f) {
	float fff 	=	0;
	memcpy(&fff, f, 4);
	return fff;
}

float buf2data_rs232(const unsigned char *f) {
	float fff = 0;
	char p[10] = {0};
	unsigned char x = 0;
	x = (f[4] & 0x3FU);
	if ((f[4]>>6) & 0x01U) {
		x *= -1;
	}
	sprintf(p, "%x%02x%02x%02x", f[0], f[1], f[2], f[3]);
	sscanf(p, "%f", &fff);
	fff = fff / pow(10, 8-1-x);
	if (f[4] & 0b10000000U)
		fff *= -1;
	return fff;
}


//float buf2flowRate(const unsigned char *f) {
//	unsigned char ff[] 	= 	{f[1], f[0], f[3], f[2]};
//	float fff 	=	0;
//	memcpy(&fff, ff, 4);
//	return fff;
//}


// ------------------------------ MODBUS CRC CALCULATION ---------------------------
unsigned int CRC16_2(unsigned char *buf, int len) 
{  
  unsigned int crc = 0xFFFF;
  for (int pos = 0; pos < len; pos++) 
  {
  crc ^= (unsigned int)buf[pos];    // XOR byte into least sig. byte of crc

  for (int i = 8; i != 0; i--) {    // Loop over each bit
    if ((crc & 0x0001) != 0) {      // If the LSB is set
      crc >>= 1;                    // Shift right and XOR 0xA001
      crc ^= 0xA001;
    }
    else                            // Else LSB is not set
      crc >>= 1;                    // Just shift right
    }
  }

  return crc;
}


// ------------------------------- CALCULATE CRC -----------------------------------
int DYNO_CalculateCrc(unsigned char buffer[], unsigned char dataSize) {
	unsigned crc = CRC16_2(buffer, dataSize);
	buffer[dataSize+1] = (unsigned char) ((crc >> 8) & 0x00FF);
	buffer[dataSize] = (unsigned char) crc & 0x00FF;
	return 0;
}


// ---------------------------------- VISA QUERY -----------------------------------
int DYNO_visaQuery(unsigned char cmd[], unsigned char rxbuf[], short rxBufSize, int waitMs, char errorMsg[]) {
	int 			error 				=	0;
	ErrMsg 			errMsg 				=	{0};
	unsigned int 	count				=	0;
	unsigned char 	txbuf[CMD_SIZE+3]	=	{0};
	
	errChk(viFlush(cfg->modbus, VI_IO_IN_BUF));
	memcpy(txbuf, cmd, CMD_SIZE);
	// calculate crc
	errChk(DYNO_CalculateCrc(txbuf, CMD_SIZE));
	// write cmd
	utility_putHex(txbuf, CMD_SIZE+2);
	errChk(viWrite(cfg->modbus, txbuf, CMD_SIZE+2, &count));
	Sleep(waitMs);
	if (rxbuf) {
		errChk(viRead(cfg->modbus, rxbuf, rxBufSize, &count));
		utility_putHex(rxbuf, count);
	}
Error:
	reportError();
	return error;
}


// ---------------------------------- VISA QUERY -----------------------------------
int DYNO_tcpQuery(unsigned char cmd[], unsigned char rxbuf[], short rxBufSize, int waitMs, char errorMsg[]) {
	int 			error 				=	0;
	ErrMsg 			errMsg 				=	{0};
	unsigned char 	txbuf[CMD_SIZE+3]	=	{0},
					buf[24] 			=	{0};
	
	
	// read old buffer
	ClientTCPRead(cfg->tcpPS, buf, 24, 100);
	// start new cmd
	memcpy(txbuf, cmd, CMD_SIZE);
	// calculate crc
	errChk(DYNO_CalculateCrc(txbuf, CMD_SIZE));
	// write cmd
	utility_putHex(txbuf, CMD_SIZE+2);
	errChk(ClientTCPWrite(cfg->tcpPS, txbuf, CMD_SIZE+2, DYNO_TCP_TIMEOUT));
	Sleep(waitMs);
	if (rxbuf) {
		errChk(ClientTCPRead(cfg->tcpPS, rxbuf, rxBufSize, DYNO_TCP_TIMEOUT));
		utility_putHex(rxbuf, rxBufSize);
	}
Error:
	reportError();
	return error;
}


// ---------------------------- BUILD PS CMD ----------------------------------------
unsigned char *buildPSCmd(unsigned char cmd[], unsigned char mode, float data) {
	unsigned short f = data*10;
	cmd[2] = 0x3;
	cmd[3] = mode;
	cmd[4] = f>>8 & 0x00FF;
	cmd[5] = f & 0x00FF;
	return cmd;
}
