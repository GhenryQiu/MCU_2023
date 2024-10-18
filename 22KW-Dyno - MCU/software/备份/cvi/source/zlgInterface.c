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

#define 	ECU_CMD_ID 				0x70B//30D
#define 	FAULT_AND_STATUS_ID 	0x30E
#define 	ECU_STATUS_ID 			0x07E

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
int ecu_WriteCmd(char IvtrReStCmd, char IvtrReStCmdVld, 
				 unsigned short MotTqCmd, char MotTqCmdVld, 
				 unsigned short MotSpdCmd, char MotSpdCmdVld, 
				 char GearPosnCmd, char GearPosnCmdVld, char errorMsg[]);
int ecu_StartTorqueCtrl(short MotTqCmd, char errorMsg[]);
int ecu_StopTorqueCtrl(char errorMsg[]);


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


// ---------------------------- CORE DAEMON FOR CAN ----------------------------
int CVICALLBACK ecu_CmdDaemon(void *functionData) {
	int 			error 			= 	0;
	long 			count 			= 	0,
					i 				= 	0;
	CAN_Setting		*setting 		=	(CAN_Setting *) functionData;
	ErrMsg			errMsg			= 	{0};
	CAN_Frame_t		f;
	
	errChk(ecu_StartTorqueCtrl(setting->torqueCmd, errMsg));
	errChk(zlgCAN_Flush(errMsg));
	while (!(setting->terminate)) {
		errChk(ecu_WriteCmd(setting->controlMode, 1, setting->torqueCmd, 1, 0, 0, 4, 1, errMsg));
		Sleep(200);
		errChk(count = VCI_GetReceiveNum(cfg->myDev, cfg->myDevIndex, cfg->canIndex));
		for (i = 0; i < count; i++) {
			errChk(zlgCAN_Read(&f, errMsg));
			switch (f.arbID) {
				case FAULT_AND_STATUS_ID: // FaultAndStatus
					memcpy(setting->fault, f.payload, 2);	
					break;
				case 0x7EU: // info
					setting->status[0] = f.payload[1] & 0x0F;
					setting->status[1] = 0.2*((f.payload[0]<<3 & 0x07F8U) + (f.payload[1]>>5 & 0x07U)) - 200;//motortorque
					break;
				case 0x149U:
					setting->status[2] = (f.payload[1]<<1 & 0x01FEU) + (f.payload[2]>>7 & 0x01U); 
					setting->temp[0] = f.payload[4] - 40; 
					break;
				case 0x304U:
					setting->status[3] = ((f.payload[3]<<7 & 0x7F80U) + (f.payload[4]>>1 & 0x007FU)) - 16000;
					setting->temp[1] = f.payload[5] - 40;
					setting->temp[2] = f.payload[6] - 40;
					break;
				default:
					
					break;
			}
		}
	}
Error:
	ecu_StopTorqueCtrl(NULL);
	if (error<0) {
		utility_puts(errMsg);
		MessagePopup("Error", errMsg);
	}
	return error;
}


// ==============================================================================
// ----------------------------- WRITE SINGLE CMD -------------------------------
int ecu_WriteCmd(char IvtrReStCmd, char IvtrReStCmdVld, 
				 unsigned short MotTqCmd, char MotTqCmdVld, 
				 unsigned short MotSpdCmd, char MotSpdCmdVld, 
				 char GearPosnCmd, char GearPosnCmdVld, char errorMsg[]) {
	CAN_Frame_t	f 			=	{.length=5, .arbID=ECU_CMD_ID};					 

	f.payload[0] = (GearPosnCmdVld<<7 & 0x80U) + (GearPosnCmd<<4 & 0x70U) + (IvtrReStCmd<<1 & 0x0EU) + (IvtrReStCmdVld & 0x01U);
	f.payload[1] = MotTqCmd>>3 & 0x00FFU;
	f.payload[2] = (MotTqCmd<<5 & 0x00E0U) + (MotTqCmdVld<<4 & 0x10U) + (MotSpdCmd>>11 & 0x0FU);
	f.payload[3] = MotSpdCmd>>3 & 0x00FFU;
	f.payload[4] = (MotSpdCmd<<5 & 0x000EU) + (MotSpdCmdVld<<4 & 0x10U);
	return zlgCAN_Write(&f, errorMsg);
}


// ------------------------------ START TORQUE CTRL --------------------------------
int ecu_StartTorqueCtrl(short MotTqCmd, char errorMsg[]) {
	int 		error 		=	0;
	ErrMsg 		errMsg 		=	{0};
	
	errChk(ecu_WriteCmd(0, 1, 0, 0, 0, 0, 0, 1, errMsg));
	Sleep(250);
	errChk(ecu_WriteCmd(1, 1, MotTqCmd, 0, 0, 0, 4, 1, errMsg));
	Sleep(250);
	errChk(ecu_WriteCmd(1, 1, MotTqCmd, 1, 0, 0, 4, 1, errMsg));
	Sleep(250);
	//errChk(ecu_WriteCmd(1, 1, MotTqCmd, 1, 0, 0, 4, 1, errMsg));
	//Sleep(250);
Error:
	reportError();
	return error;
}


// ------------------------------- STOP TORQUE CTRL --------------------------------
int ecu_StopTorqueCtrl(char errorMsg[]) {
	int 		error 		=	0;
	ErrMsg 		errMsg 		=	{0};
	
	//errChk(ecu_WriteCmd(1, 1, 0x3E8, 1, 0, 0, 4, 1, errMsg));//0x3E8
	//Sleep(250);
	errChk(ecu_WriteCmd(1, 1, 0x3E8, 0, 0, 0, 4, 1, errMsg));
	Sleep(250);
	errChk(ecu_WriteCmd(0, 1,0x3E8, 0, 0, 0, 0, 0, errMsg));
	Sleep(250);
	errChk(ecu_WriteCmd(0, 0, 0x3E8, 0, 0, 0, 0, 0, errMsg));
	Sleep(250);
	//errChk(ecu_WriteCmd(0, 0, 0, 0, 0, 0, 0, 0, errMsg));
	//Sleep(250);
Error:
	reportError();
	return error;
}