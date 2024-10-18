//==============================================================================
//
// Title:		DbgGui.h
// Purpose:		A short description of the interface.
//
// Created on:	2/22/2021 at 4:53:43 PM by .
// Copyright:	. All Rights Reserved.
//
//==============================================================================

#ifndef __DbgGui_H__
#define __DbgGui_H__

#ifdef __cplusplus
    extern "C" {
#endif

//==============================================================================
// Include files

#include "zlgInterface.h"
#include "Dyno_API.h"
#include "can_gui.h"

//==============================================================================
// Constants
#define GUI_FILE	"can_gui.uir"
		
//==============================================================================
// Types
typedef struct {
	int 	panelMain;
	int 	terminatePA;
} CONTROLLER_CFG;

//==============================================================================
// External variables

//==============================================================================
// Global functions
int gui_initialize(CONTROLLER_CFG *cfg, int *cancel, char errorMsg[]);
int gui_shutdown(CONTROLLER_CFG *cfg);

#ifdef __cplusplus
    }
#endif

#endif  /* ndef __DbgGui_H__ */
