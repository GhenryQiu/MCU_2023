//==============================================================================
//
// Title:       my_utility.h
// Purpose:     A short description of the interface.
//
// Created on:  4/30/2017 at 6:33:51 PM by Chenny Wang.
// Copyright:   GalaTech, Inc.. All Rights Reserved.
//
//==============================================================================

#ifndef __my_utility_H__
#define __my_utility_H__

#ifdef __cplusplus
    extern "C" {
#endif

//==============================================================================
// Include files
#include <windows.h>
#include <ansi_c.h>
#include <toolbox.h>
#include <formatio.h>
#include "git_version.h"

//==============================================================================
// Constants
#ifndef errChk
#define errChk(fCall) if (error = (fCall), error < 0) \
{goto Error;} else
#endif
		
#define reportError()	if (error < 0)	sprintf(errorMsg, "%s->%s", __FUNCTION__, errMsg);

#define ERRMSG_SIZE     	1024
#define DOUBLE_SEPARATION 	"============================="
#define SINGLE_SEPARATION  	"-----------------------------"
#define COMPLETE_SEPARATION "---------- COMPLETE ---------"
#define TIMESTAMP_LENGTH 	24

		
#define NULL_OR_EMPTY_STRING 			-10000		
#define INVALID_TAB_PAGE_NAME			-10001
#define OPEN_FILE_FAILURE				-10002
#define CAN_NOT_FIND_SEQ				-10003
#define VISA_EMPTY_RESOURCE_NAME		-10004
#define VISA_DEVICE_HAS_NOT_INITIALIZED -10005
#define VISA_LIBRARY_API_ERROR 			-10006
#define VISA_READ_TIMEOUT				-10007
#define INVALID_FILE_PATH 				-10008
#define NO_COM_PORT_DETECTED 			-10009
#define VISA_RESOURCE_NOT_AVAILABLE 	-10010
#define MENU_HANDLE_NOT_INITIALIZED 	-10011
#define VISA_OCCUPY_BY_OTHER 			-10012
#define VISA_INVALID_NAME				-10013
#define INVALID_PLATFORM_REGISTRY		-10014	
#define OPEN_REGISTRY_FAILURE 			-10015
#define PROGRESSBAR_NOT_HOOKED 			-10016
#define PROGRESSBAR_OCCUPIED			-10017
#define APP_INSTANCCE_ALREADY_EXIST		-10018
#define USB_NOT_CONNECTED				-10019
#define MODEM_FIRMWARE_NOT_FOUND 		-10020
#define KEYWORD_NOT_DETECTED 			-10021
#define VISA_KEYWORD_NOT_DETECTED 		-10021
#define INVALID_CMD_UPGRAD_SW 			-10022
#define SW_VERSION_DOES_NOT_MATCH		-10023
#define ENTER_UBOOT_FAILURE 			-10024
#define SET_CONFIG_FAILURE 				-10025
#define TEST_IS_NOT_STARTED				-10026		
#define CONFIG_NOT_INITIALIZED 			-10027
#define SPEED_SET_OUT_OF_RANGE 			-10028
#define TCP_NO_ENOUGH_DATA 				-10029
#define TCP_NACK_ERROR 					-10030
#define TCP_RETURN_LESS_EXPECTED 		-10031
#define TCP_PACKET_SIZE_LESS_EXPECTED	-10032
#define NO_ADDITIONAL_DATA_FOUND		-10033
#define LOAD_NOT_IN_SPEED_MODE 			-10034
#define TORQUE_SET_OUT_OF_RANGE 		-10035
#define LOAD_SET_TORQUE_MODE_FAILURE 	-10036
		
//==============================================================================
// Types
typedef char 			ErrMsg[ERRMSG_SIZE];

typedef struct {
	const char 	*uirName;
	int 		panelName;
	const char 	*tabName;
} TabPageCfg;

typedef enum {
	Console_all,
	Console_progressBar,
	Console_listBox,
} Console_Mode;

typedef enum {
	ctrlArray_dimmed,
	ctrlArray_value,
	ctrlArray_chart,
} CtrlArray_Mode;

typedef int (*funcWrite2Console)(const char *reportText);

//==============================================================================
// External variables

//==============================================================================
// Global functions
double 			utility_getExecutionTime(CVIAbsoluteTime start);
CVIAbsoluteTime utility_getTimeOut(CVIAbsoluteTime *start, double durationSec);
int  			utility_isTimeout(CVIAbsoluteTime *timeout);
char 			*utility_getTimestamp(char *time_stamp, short standard);
// file & folder processing
char *utility_getAbsolutePath(const char *relativePath, char *fullPath);
int utility_doesFileExists	(const char *filePath, char basename[], char errorMsg[]);
int utility_checkFolderEx	(const char *folderPath, short createIfNotExist, char errorMsg[]);
#ifdef MY_UTILITY_COMBOBOX
int utility_setComboByPopulateFolder(int panel, int combo, const char *folderPath, char errorMsg[]);
#endif
// ctrlArray operation
int utility_setCtrlArray	(int panel, int controlArray, CtrlArray_Mode mode, int value, char errorMsg[]);
int utility_insertTabPages	(int panel, int tabCtrl, TabPageCfg tabCfg[], int numTabPage, char errorMsg[]);
int utility_checkVisaName	(const char *visa, char errorMsg[]);
// console & progress bar
int utility_hookConsole		(int panel, int ctrl, int progressBar);
int utility_resetConsole	(void);
int utility_destroyConsole	(void);
int utility_enableConsole	(Console_Mode mode, short enable);
int utility_puts			(const char *reportText);
int utility_putHex			(const unsigned char *reportText, int dataSize);
int utility_updateText		(const char *reportText);
int utility_launchProgressBar(const char *callerFuncName, int totalSec, funcWrite2Console f,
							  short currentStatus, short continuousMode, char errorMsg[]);
int utility_stopProgressBar	(funcWrite2Console f, int error, char errorMsg[]);

#ifdef __cplusplus
    }
#endif

#endif  /* ndef __my_utility_H__ */
