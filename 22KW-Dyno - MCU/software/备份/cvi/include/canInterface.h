//==============================================================================
//
// Title:		canInterface.h
// Purpose:		A short description of the interface.
//
// Created on:	4/1/2021 at 9:39:09 PM by .
// Copyright:	. All Rights Reserved.
//
//==============================================================================

#ifndef __canInterface_H__
#define __canInterface_H__

#ifdef __cplusplus
    extern "C" {
#endif

//==============================================================================
// Include files
#include "zlgInterface.h"

//==============================================================================
// Constants

//==============================================================================
// Types

//==============================================================================
// External variables

//==============================================================================
// Global functions
int CAN_Initialize(char errorMsg[]);
int CAN_Close(void);
int CAN_LaunchDaemon(char errorMsg[]);
int CAN_StopDaemon(void);
int CAN_SetToruqe(float torqueCmd, char errorMsg[]);
int CAN_SetControlMode(unsigned char controlMode, char errorMsg[]);
int CAN_GetStatus(float status[], unsigned char fault[], float temperature[], char errorMsg[]);

#ifdef __cplusplus
    }
#endif

#endif  /* ndef __canInterface_H__ */
