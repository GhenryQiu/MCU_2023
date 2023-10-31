//==============================================================================
//
// Title:		yokogawa.c
// Purpose:		A short description of the implementation.
//
// Created on:	8/25/2020 at 9:11:34 PM by Chenny Wang.
// Copyright:	. All Rights Reserved.
//
//==============================================================================

//==============================================================================
// Include files
#include "yokogawa.h"

//==============================================================================
// Constants
#define VISA_BUF_SIZE 		512
#define YKGW_IDENTITY 		"*IDN?\r"
#define YKGW_REMOTE_MODE 	"COMM:REM ON\r"
#define YKGW_LOCAL_MODE 	"COMM:REM OFF\r"
#define YKGW_ITEM_SIZE 		":NUM:NUM?\r"
#define YKGW_ITEM_NAME 		":NUM:NORM:ITEM%d?\r"
#define YKGW_ITEM_DATA 		":NUM:FORM ASC;:NUM:NORM:NUM %d;:NUM:NORM:VAL?\r"

//==============================================================================
// Types
typedef struct {
	ViSession 	conn;
	double 		*data;
	char 		**itemList;
	int 		numItem;
	char 		*cmdFetchData;
	char  		ipAddress[40];
} YKGW_Cfg;

//==============================================================================
// Static global variables
static YKGW_Cfg *ykgw 	=	NULL;
	

//==============================================================================
// Static functions
int YKGW_Query(const char *cmd, char rxbuf[], short waitMs, char errorMsg[]);
int YKGW_Write(const char *cmd, char errorMsg[]);
int YKGW_BuildSetting(char errorMsg[]);
int YKGW_ConfigItemList(char errorMsg[]);

//==============================================================================
// Global variables

//==============================================================================
// Global functions
int YKGW_Config(const char *ipAddress, char errorMsg[]) {
	int 		error 			= 	0;
	ErrMsg		errMsg			= 	{0};

	YKGW_Close(TRUE);
	nullChk(ykgw = malloc(sizeof(YKGW_Cfg)));
	memset(ykgw, 0, sizeof(YKGW_Cfg));
	strcpy(ykgw->ipAddress, ipAddress);
Error:
	reportError();
	return error;
}


// ----------------------- INITIALIZE HW ----------------------------------------
int YKGW_Initialize(char errorMsg[]) {
	int 		error 			= 	0;
	ErrMsg		errMsg			= 	{0};
	ViSession	resourceMgr 	=	0;
	char 		rxbuf[VISA_BUF_SIZE] 		=	{0};

	if (ykgw==NULL || ykgw->ipAddress==NULL || *ykgw->ipAddress==0) {
		strcpy(errMsg, "yokogawa has not been initialized");
		errChk(VISA_INVALID_NAME);
	}
	errChk(viOpenDefaultRM(&resourceMgr));
	errChk(viOpen(resourceMgr, ykgw->ipAddress, VI_NULL, 1000, &ykgw->conn));
	errChk(viSetAttribute(ykgw->conn, VI_ATTR_TMO_VALUE, 2000));
	errChk(viSetAttribute(ykgw->conn, VI_ATTR_SUPPRESS_END_EN, VI_FALSE));
	errChk(viSetAttribute(ykgw->conn, VI_ATTR_TERMCHAR_EN, VI_TRUE));
	errChk(viSetAttribute(ykgw->conn, VI_ATTR_SEND_END_EN, VI_TRUE));
	errChk(viSetAttribute(ykgw->conn, VI_ATTR_TERMCHAR, 0xA));
	errChk(viFlush(ykgw->conn, VI_IO_IN_BUF));
	Sleep(500);
	errChk(YKGW_Query(YKGW_IDENTITY, rxbuf, 500, errMsg));
	Sleep(100);
	errChk(YKGW_Write(YKGW_REMOTE_MODE, errMsg));
	Sleep(100);
	errChk(YKGW_ConfigItemList(errMsg));
	Sleep(100);
	errChk(YKGW_BuildSetting(errMsg));
	Sleep(100);
Error:
	if (error<0 && *errMsg==0)
		viStatusDesc(ykgw->conn, error, errMsg);
	reportError();
	return error;
}


// ---------------------- DESTROY CNV DATA CONNECTION ---------------------------
int YKGW_Close(short destroyAll) {
	int		error 	=	0; 		
	ErrMsg 	errMsg 	= 	{0};
	
	if (ykgw) {
		if (ykgw->itemList) {
			for (int i = 0; i < ykgw->numItem; i++)
				free(ykgw->itemList[i]);
			free(ykgw->itemList);
			ykgw->itemList = NULL;
		}
		//
		if (ykgw->data)	{
			free(ykgw->data);
			ykgw->data = NULL;
		}
		if (ykgw->conn)	{
			error = YKGW_Write(YKGW_LOCAL_MODE, errMsg);
			if (error<0)
				MessagePopup("Error", "error");
			viClose(ykgw->conn);
			ykgw->conn = 0;
		}
		if (destroyAll) {
			free(ykgw);
			ykgw = NULL;
		}
	}
	return 0;	
}


// ----------------------- BUILD SETTING ----------------------------------------
int YKGW_BuildSetting(char errorMsg[]) {
	int 		error 			= 	0;
	ErrMsg		errMsg			= 	{0};

	//errChk(YKGW_FetchItemNames(&ykgw->itemList, &ykgw->numItem, errMsg));
	ykgw->numItem = NUM_INSTR_SIGNALS;
	nullChk(ykgw->data = malloc(ykgw->numItem*sizeof(double)));
	memset(ykgw->data, 0, ykgw->numItem*sizeof(double));
	nullChk(ykgw->cmdFetchData = malloc(strlen(YKGW_ITEM_DATA)+1));
	sprintf(ykgw->cmdFetchData, YKGW_ITEM_DATA, ykgw->numItem);
Error:
	reportError();
	return error;
}


// ----------------------- CORE DAEMON FOR YOKOGAWA ----------------------------
int CVICALLBACK YokogawaDaemon(void *functionData) {
	int 		error 					= 	0,
				*abort 					=	(int *) functionData;
	ErrMsg		errMsg					= 	{0};
	
	errChk(YKGW_Initialize(errMsg));
	utility_puts("power analyzer daemon...started");
	while (!(*abort)) {
		errChk(YKGW_Measure(errMsg));
		Sleep(200);
	}
Error:
	YKGW_Close(FALSE);
	utility_puts("power analyzer daemon...stopped");
	if (error<0)
		MessagePopup("Error", errMsg);
	return error;
}


// ------------------------ READ DATA(SHARED VARIABLE)---------------------------
int YKGW_Read(double *data, int numSignal, char errorMsg[]) {
	int error = 0;
	if (data && ykgw && ykgw->data) {
		if (numSignal==NUM_INSTR_SIGNALS) {
			memcpy(data, ykgw->data, sizeof(double)*numSignal);
		} else {
			data[0] = ykgw->data[1];/*dc V*/
			data[1] = ykgw->data[5]; /*dc I*/
			data[2] = ykgw->data[9];/* dc power*/
			data[3] = ykgw->data[3]; /*ac V*/
			data[4] = ykgw->data[7];  /*ac i*/
			data[5] = ykgw->data[11];  /*AC POWER*/
			data[6] = data[5]*100/data[2]; 
		}
	} else {
		strcpy(errorMsg, "null pointer for data(YKGW_READ)");
		errChk(NULL_OR_EMPTY_STRING);
	}
Error:
	return error;
}


// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// ----------------------------- CONFIG ITEM LIST --------------------------------
int YKGW_ConfigItemList(char errorMsg[]) {
	int 		error 							=	0,
				i 								=	0;
	ErrMsg 		errMsg 							=	{0};
	char 		cmd[128]						=	{0},
				txbuf[2048]						=	{0};
	const char 	*itemList[NUM_INSTR_SIGNALS] 	=	YKGW_ITEM_LIST; 			
	
	sprintf(txbuf, ":NUM:NUM %d;", NUM_INSTR_SIGNALS);
	for (i = 0; i<NUM_INSTR_SIGNALS; i++) {
		sprintf(cmd, ":NUM:ITEM%d %s;", i+1, itemList[i]);
		strcat(txbuf, cmd);
	}
	txbuf[strlen(txbuf)-1] = 0xC;
	errChk(YKGW_Write(txbuf, errMsg));
Error:
	reportError();
	return error;
}


// ---------------------------- GET ITEM NAMES -----------------------------------
int YKGW_FetchItemNames(char ***itemList, int *numItem, char errorMsg[]) {
	int 	error 					= 	0,
			i 						=	0;
	ErrMsg	errMsg					= 	{0};
	char 	txbuf[VISA_BUF_SIZE] 	=	{0},
			rxbuf[VISA_BUF_SIZE] 	=	{0},
			tmp[8]					=	{0},
			*p 						=	NULL,
			**f 					=	NULL;
	
	if (numItem) 
		*numItem = NUM_INSTR_SIGNALS;
	nullChk(f = malloc(NUM_INSTR_SIGNALS*sizeof(char *)));
	for (i = 0; i<NUM_INSTR_SIGNALS; i++) {
		sprintf(txbuf, YKGW_ITEM_NAME, i+1);
		sprintf(tmp, "ITEM%d ", i+1);
		errChk(YKGW_Query(txbuf, rxbuf, 100, errMsg));
		nullChk(p = strstr(rxbuf, tmp));
		nullChk(f[i]=malloc(strlen(p)));
		strcpy(f[i], p+strlen(tmp));
	}
	*itemList = f;
Error:
	reportError();
	return error;
}


// ----------------------------- READ CNV DATA -----------------------------------
int YKGW_Measure(char errorMsg[]) {
	int 	error 					= 	0,
			i 						=	0;
	ErrMsg	errMsg					= 	{0};
	char 	rxbuf[VISA_BUF_SIZE]	=	{0},
			tmp[12] 				=	{0},
			*p 						=	NULL,
			*q 						=	NULL;

	if (ykgw==NULL) {
		strcpy(errMsg, "功率计通讯失败");//Yokogawa is not initialized
		errChk(VISA_DEVICE_HAS_NOT_INITIALIZED);		
	}
	errChk(YKGW_Query(ykgw->cmdFetchData, rxbuf, 200, errMsg));
	p = rxbuf;
	for (i=0; i<ykgw->numItem; i++) {
		q = strchr(p, ',');
		if (q) {
			strncpy(tmp, p, q-p);
			tmp[q-p] = 0;
			if (strcmp(tmp, "INF"))
				errChk(sscanf(tmp, "%lf", &ykgw->data[i]));
			else
				ykgw->data[i] = -99999;
			p = q + 1;
		} else {
			if (strcmp(p, "INF")) 
				errChk(sscanf(p, "%lf", &ykgw->data[i]));
			else
				ykgw->data[i] = -99999;
		}
	}
Error:
	reportError();
	return error;
}


// -------------------------------- BASIC QUERY -----------------------------------
int YKGW_Query(const char *cmd, char rxbuf[], short waitMs, char errorMsg[]) {
	int 	error 	= 	0,
			count 	=	0;
	ErrMsg	errMsg	= 	{0};
	
	if (ykgw) {
		errChk(viWrite(ykgw->conn, (unsigned char *) cmd, strlen(cmd), &count));
		Sleep(waitMs);
		errChk(viRead(ykgw->conn, (unsigned char *) rxbuf, VISA_BUF_SIZE, &count));
		rxbuf[count] = 0;
		utility_puts(rxbuf);
	} else {
		strcpy(errMsg, "功率计读取失败");//Yokogawa is not initialized
		errChk(VISA_DEVICE_HAS_NOT_INITIALIZED);
	}
Error:
	if (error<0 && *errMsg==0) 
		viStatusDesc(ykgw->conn, error, errMsg);
	reportError();
	return error;
}


// -------------------------------- WRITE ONLY -----------------------------------
int YKGW_Write(const char *cmd, char errorMsg[]) {
	int 	error 	= 	0,
			count 	=	0;
	ErrMsg	errMsg	= 	{0};
	
	if (ykgw) {
		errChk(viWrite(ykgw->conn, (unsigned char *) cmd, strlen(cmd), &count));
	} else {
		strcpy(errMsg, "功率计写入失败");//Yokogawa is not initialized
		errChk(VISA_DEVICE_HAS_NOT_INITIALIZED);
	}
Error:
	if (error<0 && *errMsg==0) 
		viStatusDesc(ykgw->conn, error, errMsg);
	reportError();
	return error;
}
