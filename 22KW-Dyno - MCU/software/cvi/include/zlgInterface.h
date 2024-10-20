//==============================================================================
//
// Title:		zlgInterface.h
// Purpose:		A short description of the interface.
//
// Created on:	2/21/2021 at 11:15:34 PM by mrs qiu
// Copyright:	. All Rights Reserved.
//
//==============================================================================

#ifndef __zlgInterface_H__
#define __zlgInterface_H__

#ifdef __cplusplus
    extern "C" {
#endif

//==============================================================================
// Include files
#include "my_utility.h"

//==============================================================================
// Constants
#ifndef _NI_u8_DEFINED_
#define _NI_u8_DEFINED_
    typedef unsigned char      u8;
#endif
#ifndef _NI_u16_DEFINED_
#define _NI_u16_DEFINED_
    typedef unsigned short     u16;
#endif
#ifndef _NI_u32_DEFINED_
#define _NI_u32_DEFINED_
    typedef uint32_t      	   u32;
#endif
#ifndef _NI_u64_DEFINED_
#define _NI_u64_DEFINED_
#if (defined(_MSC_VER) || defined(_CVI_))
    typedef unsigned __int64     u64;
#elif defined(__GNUC__)
    typedef unsigned long long int   u64;
#endif
#endif
	
#define 	OPEN_DEVICE_FAILURE			-61001
#define 	INITIALIZE_CAN_FAILURE		-61002
#define 	START_CAN_FAILURE			-61003		
#define 	SET_BAUDRATE_FAILURE		-61004
#define 	CAN_READ_FRAME_ERROR 		-61005
#define 	CAN_WRITE_FRAME_ERROR		-61006
#define 	INVALID_RETURN_DATA			-61007
#define 	WAIT_FOR_FRAME_TIMEOUT 		-61008
#define 	CAN_DEV_NOT_INITIALIZED 	-61009
	
//==============================================================================
// Types
// CAN msg definition
typedef struct CAN_Frame_t {
    u32 arbID;
    u8 	messageType;
    u8 	payload[8];
    u8 	length;
    u64 timeStamp;
} CAN_Frame_t;

typedef struct {
	int 					panel;
	int 					terminate;
	unsigned char 			controlMode;
	unsigned short 			torqueCmd;
	float 					temp[3];
	unsigned char 			fault[2];
	float                  status[4];
	CmtThreadFunctionID 	thid;
} CAN_Setting;


//==============================================================================
// External variables

//==============================================================================
// Global functions

int zlgCAN_Initialize(char errorMsg[]);
int zlgCAN_Close(void);
int CVICALLBACK ecu_CmdDaemon(void *functionData);

#ifdef __cplusplus
    }
#endif

#endif  /* ndef __zlgInterface_H__ */
