//==============================================================================
//
// Title:		yokogawa.h
// Purpose:		A short description of the interface.
//
// Created on:	8/25/2020 at 9:11:34 PM by Chenny Wang.
// Copyright:	. All Rights Reserved.
//
//==============================================================================

#ifndef __yokogawa_H__
#define __yokogawa_H__

#ifdef __cplusplus
    extern "C" {
#endif

//==============================================================================
// Include files
#include <visa.h>
#include "my_utility.h"
		
//==============================================================================
// Constants
#define 	NUM_INSTR_SIGNALS 		16
#define 	YKGW_ITEM_LIST 			{"U,1", "U,2", "U,3", "U,SIGMA", "I,1", "I,2", "I,3", "I,SIGMA", "P,1", "P,2", "P,3", "P,SIGMA", "LAMB,1", "LAMB,2", "LAMB,3", "LAMB,SIGMA"};

//==============================================================================
// Types

//==============================================================================
// External variables

//==============================================================================
// Global functions
int YKGW_Config(const char *ipAddress, char errorMsg[]);
int YKGW_Initialize(char errorMsg[]);
int YKGW_Close(short destroyAll);
int YKGW_Measure(char errorMsg[]);
int YKGW_Read(double *data, int numSignal, char errorMsg[]);
int YKGW_FetchItemNames(char ***itemList, int *numItem, char errorMsg[]);
int CVICALLBACK YokogawaDaemon(void *functionData);

#ifdef __cplusplus
    }
#endif

#endif  /* ndef __yokogawa_H__ */
