//==============================================================================
//
// Title:		canInterface.c
// Purpose:		A short description of the implementation.
//
// Created on:	4/1/2021 at 9:39:09 PM by .
// Copyright:	. All Rights Reserved.
//
//==============================================================================

//==============================================================================
// Include files
#include "canInterface.h"

//==============================================================================
// Constants

//==============================================================================
// Types

//==============================================================================
// Static global variables
static CAN_Setting 		*canSetting 	=	NULL;

//==============================================================================
// Static functions

//==============================================================================
// Global variables

//==============================================================================
int CAN_Initialize(char errorMsg[]) {
	return zlgCAN_Initialize(errorMsg);
}


int CAN_Close(void) {
	CAN_StopDaemon();
	return zlgCAN_Close();
}


int CAN_LaunchDaemon(char errorMsg[]) {
	int 		error 			=	0;
	ErrMsg 		errMsg 			=	{0};
	
	if (canSetting==NULL) {
		nullChk(canSetting = malloc(sizeof(CAN_Setting)));
		memset(canSetting, 0, sizeof(CAN_Setting));
		canSetting->controlMode = 1;
		errChk(CAN_SetToruqe(0, errMsg));
		errChk(CmtScheduleThreadPoolFunction(DEFAULT_THREAD_POOL_HANDLE, 
											 ecu_CmdDaemon, canSetting, 
											 &canSetting->thid));
	}
Error:
	reportError();
	return error;
}


int CAN_StopDaemon(void) {
	if (canSetting) {
		canSetting->terminate = TRUE;
		CmtWaitForThreadPoolFunctionCompletion(DEFAULT_THREAD_POOL_HANDLE, 
											   canSetting->thid, OPT_TP_PROCESS_EVENTS_WHILE_WAITING);
		CmtReleaseThreadPoolFunctionID(DEFAULT_THREAD_POOL_HANDLE, canSetting->thid);
		free(canSetting);
	}
Error:
	canSetting = NULL;
	return 0;
}


int CAN_SetToruqe(float torqueCmd, char errorMsg[]) {
	int 	error 	=	0;
	if (canSetting) {
		canSetting->torqueCmd = (torqueCmd+200)*5;
	} else {
		strcpy(errorMsg, "Ť���趨ʧ��");//can daemon is not initialized
		error = -100;
	}
	return error;
}


int CAN_SetControlMode(unsigned char controlMode, char errorMsg[]) {
	int   	error 		=	0;
	if (canSetting) {
		canSetting->controlMode = controlMode;
	} else {
		strcpy(errorMsg, "can daemon not initialized");
		error  = -101;
	}
	return error;
}

int CAN_GetStatus(float status[], unsigned char fault[], float temperature[], char errorMsg[]) {
	int 	error 	=	0;
	if (canSetting) {
		memcpy(status, canSetting->status, sizeof(float )*4);//unsigned short
		memcpy(fault, canSetting->fault, 2);
		memcpy(temperature, canSetting->temp, sizeof(float)*3);//sizeof ���� 3�������� temperatureֵ
	} else {
		strcpy(errorMsg, "��ʼ��ʧ��");//can daemon is not initialized
		error = -100;
	}
	return error;
}