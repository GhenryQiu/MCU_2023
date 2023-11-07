//==============================================================================
//
// Title:		zlgInterface.c
// Purpose:		A short description of the implementation.
//
// Created on:	2/21/2021 at 11:15:34 PM by .
// Copyright:	. All Rights Reserved.
//
//==============================================================================

//==============================================================================
// Include files
#include "zlgInterface.h"
#include "ControlCAN.h"

//==============================================================================
// Constants
#define 	MY_CAN_DEVICE 			VCI_USBCAN_E_U
#define 	MY_DEV_INDEX 			0

#define 	ECU_CMD_ID 				0x30D
#define 	ECU_FAULT_ID            0x30E
#define 	ECU_INFO_ID 			0x149
#define 	ECU_TEMP_ID 			0x304
#define     ECU_STATUS And MotTq    0x7E



//==============================================================================
// Types
typedef struct {
	DWORD 				myDev;
	DWORD 				myDevIndex;
	DWORD 				canIndex;
	DWORD 				baudrate;
	short 				started;
	VCI_INIT_CONFIG		setting;
	VCI_CAN_OBJ			*f;
} ZLG_CAN_CFG;

//==============================================================================
// Static global variables
static 	ZLG_CAN_CFG 	*cfg 	=	NULL;

//==============================================================================
// Static functions
int zlgCAN_Write(CAN_Frame_t *frame, char errorMsg[]);
int zlgCAN_Read (CAN_Frame_t *frame, char errorMsg[]);
int zlgCAN_Flush(char errorMsg[]);
int zlgCAN_WaitForFrame(u32 arbid, u8* Payload, u8 PayloadLength, int timeoutMs, char errorMsg[]);
int ecu_CmdECU(ECU_CmdDataIndex mode, float data, char errorMsg[]);

//==============================================================================
// Global variables

//==============================================================================
// Global functions
// ------------------------- INITIALIZE HARDWARE -------------------------------
int zlgCAN_Initialize(char errorMsg[]) {
	int 	error 		=	0;
	ErrMsg 	errMsg 		=	{0};
	
	zlgCAN_Close();
	nullChk(cfg = malloc(sizeof(ZLG_CAN_CFG)));
	memset(cfg, 0, sizeof(ZLG_CAN_CFG));
	nullChk(cfg->f = malloc(sizeof(VCI_CAN_OBJ)));
	cfg->myDev 			= 	VCI_USBCAN_2E_U;
	cfg->baudrate		=	0x060007;//0x060007	(500kbps)  1C0008 (250Kbps)
	cfg->setting.Mode 	= 	0; // normal mode
	// initialize hardware
	if (VCI_OpenDevice(cfg->myDev, cfg->myDevIndex, 0)==0) {
		sprintf(errMsg, "open CAN device(%d) failure(device index=%d)", 
				cfg->myDev, 
				cfg->myDevIndex);
		errChk(OPEN_DEVICE_FAILURE);
	}
	if (cfg->myDev == VCI_USBCAN_2E_U) {
		// have to set baudrate before invoking VCI_InitCan
		if (VCI_SetReference(cfg->myDev, cfg->myDevIndex, cfg->canIndex, 0, &cfg->baudrate)==0) {
			strcpy(errMsg, "set baud rate failure");
			errChk(SET_BAUDRATE_FAILURE);
		}
	} else {
		// baudrate setting if not USBCAN E model
		cfg->setting.Timing0 	=	0x00;
		cfg->setting.Timing1	=	0x1C;
	}
	// initialize CAN
	if (VCI_InitCAN(cfg->myDev, cfg->myDevIndex, cfg->canIndex, &cfg->setting)==0) {
		strcpy(errMsg, "CAN³õÊ¼»¯Ê§°Ü");//CAN initialization failure
		errChk(INITIALIZE_CAN_FAILURE);
	}	
	// start CAN
	if (VCI_StartCAN(cfg->myDev, cfg->myDevIndex, cfg->canIndex)==0) {
		strcpy(errMsg, "Æô¶¯canÊ§°Ü");//start CAN failure
		errChk(START_CAN_FAILURE);
	}
	cfg->started = TRUE;
Error:
	reportError();
	return error;
}


// ---------------------------- CLOSE CAN SESSION -----------------------------
int zlgCAN_Close(void) {
	if (cfg) {
		VCI_CloseDevice(cfg->myDev, cfg->myDevIndex);
		if (cfg->f)
			free(cfg->f);
		free(cfg);
		cfg = NULL;
	}
	return 0;
}


// ---------------------------- WRITE SINGLE FRAME ----------------------------
int zlgCAN_Write(CAN_Frame_t *frame, char errorMsg[]) {
	int 		error 		=	0;
	ErrMsg 		errMsg 		=	{0};
	
	if (cfg && cfg->started) {
		memset(cfg->f, 0, sizeof(VCI_CAN_OBJ));
		cfg->f->ID 		= frame->arbID;
		cfg->f->DataLen = frame->length;
		memcpy(cfg->f->Data, frame->payload, 8);
		if (VCI_Transmit(cfg->myDev, cfg->myDevIndex, cfg->canIndex, cfg->f, 1)!=1) {
			strcpy(errMsg, "write CAN frame error");
			errChk(CAN_WRITE_FRAME_ERROR);
		}
	} else {
		strcpy(errMsg, "can¿¨Ð´ÈëÊ§°Ü");//CAN device is not initialized
		errChk(CAN_DEV_NOT_INITIALIZED);
	}
Error:
	reportError();
	return error;
}


// ----------------------------- READ SINGLE FRAME ----------------------------
int zlgCAN_Read(CAN_Frame_t *frame, char errorMsg[]) {
	int 		error 		=	0;
	ErrMsg 		errMsg 		=	{0};
	VCI_CAN_OBJ	f;
	
	if (cfg && cfg->started) {
		error = VCI_Receive(cfg->myDev, cfg->myDevIndex, cfg->canIndex, &f, 1, 250);
		if (error == 1) {
			frame->arbID = f.ID;
			frame->length = f.DataLen;
			memcpy(frame->payload, f.Data, 8);
		} else if (error == 0xFFFFFFFF) {
			strcpy(errMsg, "read CAN frame error");
			errChk(CAN_READ_FRAME_ERROR);
		}
	} else {
		strcpy(errMsg, "CAN¶ÁÈ¡Ê§°Ü");//CAN device is not initialized
		errChk(CAN_DEV_NOT_INITIALIZED);
	}
Error:
	reportError();
	return error;
}


// ----------------------------- FLUSH BUFFER ---------------------------------
int zlgCAN_Flush(char errorMsg[]) {
	int 		error 		=	0;
	ErrMsg 		errMsg 		=	{0};
	
	nullChk(cfg);
	error = VCI_ClearBuffer(cfg->myDev, cfg->myDevIndex, cfg->canIndex);
	error -= 1;
Error:
	reportError();
	return error;
}

// ------------------------------- WAIT FOR CAN ID -----------------------------
int zlgCAN_WaitForFrame(u32 arbid, u8* Payload, u8 PayloadLength, int timeoutMs, char errorMsg[]) {
	int 			error 		=	0;
	ErrMsg 			errMsg 		=	{0};
	VCI_CAN_OBJ		f 			=	{.ID=0, .Data = {0}};
	
	VCI_Receive(cfg->myDev, cfg->myDevIndex, cfg->canIndex, &f, PayloadLength, timeoutMs);	
	if (f.ID==arbid) {
		memcpy(Payload, f.Data, PayloadLength);
	} else {
		sprintf(errMsg, "wait for frame(arbid=%x) timeout(%d ms)", arbid, timeoutMs);
		errChk(WAIT_FOR_FRAME_TIMEOUT);
	}
Error:
	reportError();
	return error;
}


// ---------------------------- COMMAND ECU ------------------------------------
int ecu_CmdECU(ECU_CmdDataIndex mode, float data, char errorMsg[]) {
	int 		error 		=	0;
	ErrMsg 		errMsg 		=	{0};
	CAN_Frame_t	f 			=	{.length=6, .arbID=ECU_CMD_ID};
	
	memset(f.payload, 0, 8);
	f.payload[0] = ECU_CanCmd;
	f.payload[1] = mode;
	switch (mode) {
		case ECU_Mode_UnlockPswd:
			f.payload[5] = ((u32) data) >> 24 & 0x000000FF;
			f.payload[4] = ((u32) data) >> 16 & 0x000000FF;
			f.payload[3] = ((u32) data) >> 8  & 0x000000FF;
			f.payload[2] = ((u32) data) & 0x000000FF;
			break;
		case ECU_Mode_Torque:
			memcpy(f.payload+2, &data, 4);
			break;
		case ECU_Mode_Enable:
		case ECU_Mode_ControlMode: 

		case ECU_Mode_ExternMode:
			f.payload[2] = (u8) data;
			break;
	}
	
	errChk(zlgCAN_Write(&f, errMsg));
Error:
	if (errorMsg)	reportError();
	return error;	
}


// ---------------------------- CORE DAEMON FOR CAN ----------------------------
int CVICALLBACK ecu_CmdDaemon(void *functionData) {
	int 			error 			= 	0,
					j 				=	0;
	long 			count 			= 	0,
					i 				= 	0;
	CAN_Setting		*setting 		=	(CAN_Setting *) functionData;
	ErrMsg			errMsg			= 	{0};
	CAN_Frame_t		f;
	
	errChk(ecu_CmdECU(ECU_Mode_UnlockPswd, 9344, errMsg));
	Sleep(250);
	errChk(ecu_CmdECU(ECU_Mode_Enable, 1, errMsg));
	Sleep(250);
	errChk(ecu_CmdECU(ECU_Mode_ControlMode, setting->controlMode, errMsg));
	Sleep(250);
	errChk(ecu_CmdECU(ECU_Mode_ExternMode, 2, errMsg));
	Sleep(250);
	errChk(ecu_CmdECU(ECU_Mode_Torque, setting->torqueCmd, errMsg));
	Sleep(250);
	errChk(zlgCAN_Flush(errMsg));
	while (!(setting->terminate)) {
		errChk(ecu_CmdECU(ECU_Mode_Torque, setting->torqueCmd, errMsg));
		Sleep(200);
		if (setting->setCtrlMode) {
			errChk(ecu_CmdECU(ECU_Mode_ControlMode, setting->controlMode, errMsg));
			setting->setCtrlMode = FALSE;
		}
		Sleep(200);
		errChk(count = VCI_GetReceiveNum(cfg->myDev, cfg->myDevIndex, cfg->canIndex));
		for (i = 0; i < count; i++) {
			errChk(zlgCAN_Read(&f, errMsg));
			switch (f.arbID) {
				case 0xC65D1D1: // FaultAndStatus
					memcpy(setting->fault, f.payload, 4);	
					break;
				case 0xC65D1D2: // info
					setting->status[0] = (f.payload[1]<<8 & 0xFF00) + f.payload[0];
					setting->status[1] = (f.payload[3]<<8 & 0xFF00) + f.payload[2];
					setting->status[2] = (f.payload[5]<<8 & 0xFF00) + f.payload[4];
					setting->status[3] = f.payload[6];
					setting->status[4] = f.payload[7]>0;
					break;
				case 0xC65D1D3: // temp 
					for (j=0; j<5; j++)
						setting->temp[j] = f.payload[j]-40;
					break;
				default:
					
					break;
			}
		}
	}
Error:
	if (setting->torqueCmd>1) {
		for (int i = 0; i < 10; i++) {
			ecu_CmdECU(ECU_Mode_Torque, 1, NULL);
			Sleep(400);
		}
	}
	if (setting->terminate ==TRUE){
	ecu_CmdECU(ECU_Mode_Torque, 0, NULL);
	Sleep(250);
	ecu_CmdECU(ECU_Mode_ControlMode, 0, NULL);
	}
	if (error<0) {
		utility_puts(errMsg);
		MessagePopup("Error", errMsg);
	}
	return error;
}
