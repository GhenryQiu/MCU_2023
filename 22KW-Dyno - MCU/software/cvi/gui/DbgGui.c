//==============================================================================
//
// Title:		DbgGui.c
// Purpose:		A short description of the implementation.
//
// Created on:	2/22/2021 at 4:53:43 PM by .
// Copyright:	. All Rights Reserved.
//
//==============================================================================

//==============================================================================
// Include files

#include "DbgGui.h"

//==============================================================================
// Constants

//==============================================================================
// Types
typedef struct {
	int 	panel;
	int 	terminate;
	int 	stopped;
} EXEC_Cfg;

//==============================================================================
// Static global variables

//==============================================================================
// Static functions
int CVICALLBACK gui_monitor(void *functionData);
int CVICALLBACK gui_MonitorCanDaemon(void *functionData);
int gui_disableCtrls(int panel, int disable, char errorMsg[]);

//==============================================================================
// Global variables

//==============================================================================
// Global functions
// ------------------------------ MAIN ENTRY -----------------------------------
int main (int argc, char *argv[]) {
	int 		error 			=	0,
				alreadyExist 	=	FALSE,
				cancel 			=	FALSE;
	ErrMsg 		errMsg 			=	{0};
	CONTROLLER_CFG	*gui			=	malloc(sizeof(CONTROLLER_CFG));	
	
	errChk(CheckForDuplicateAppInstance(ACTIVATE_OTHER_INSTANCE, &alreadyExist));
	if (alreadyExist==FALSE) {
		errChk(gui_initialize(gui, &cancel, errMsg));
		if (!cancel)	RunUserInterface();
	}
Error:
	gui_shutdown(gui);
	if (error<0)
		MessagePopup("Error", errMsg);
	return error;
}


// ------------------------- INITIALIZATION ------------------------------------
int gui_initialize(CONTROLLER_CFG *cfg, int *cancel, char errorMsg[]) {
	int 		error 						=	0;
	ErrMsg 		errMsg 						=	{0};
	char 		buffer[512] 				=	{0};
	
	memset(cfg, 0, sizeof(CONTROLLER_CFG));
	errChk(cfg->panelMain = LoadPanel(0, GUI_FILE, PANEL));
	// hook debug console
	utility_hookConsole(cfg->panelMain, PANEL_TEXTBOX, PANEL_PROGRESSBAR);
	errChk(gui_disableCtrls(cfg->panelMain, TRUE, errMsg));
	errChk(SetCtrlAttribute(cfg->panelMain, PANEL_STRIPCHART, ATTR_NUM_TRACES, 5));
	// load configuration
	errChk(YKGW_Config("TCPIP::192.168.0.116::5025::SOCKET", errMsg));
	errChk(SetCtrlAttribute(cfg->panelMain, PANEL_btnPA, ATTR_CALLBACK_DATA, &cfg->terminatePA));
	// set panel title & chart legend
	sprintf(buffer, "Load Controller Diagnostic - %s", PROGRAM_VER);
	errChk(SetPanelAttribute(cfg->panelMain, ATTR_TITLE, buffer));	
	errChk(DisplayPanel(cfg->panelMain));
	utility_puts("launch load controller diagnostic tool...");
Error:
	reportError();
	return error;	
}


// ------------------------- CLOSE RESOURCE ------------------------------------
int gui_shutdown(CONTROLLER_CFG *cfg) {
	if (cfg) {
		if (cfg->panelMain) {		
			cfg->terminatePA = TRUE;
			Sleep(1000);
			DiscardPanel(cfg->panelMain);
		}
		free(cfg);
	}
	return 0;
}


// ---------------------------- CORE DAEMON FOR CAN ----------------------------
int CVICALLBACK gui_MonitorCanDaemon(void *functionData) {
	int 			error 			=	0;
	CAN_Setting		*setting		=	(CAN_Setting *) functionData;
	
	nullChk(setting);
	errChk(ClearStripChart(setting->panel, PANEL_STRIPCHART));
	Sleep(1000);
	while (!setting->terminate) {
		errChk(PlotStripChart(setting->panel, PANEL_STRIPCHART, setting->temp, 5, 0, 0, VAL_FLOAT));
		Sleep(900);
	}
Error:
	if (error<0)
		MessagePopup("Error", GetUILErrorString(error));		
	return error;
}


// ========================= CALLBACK FUNCTIONS ================================
int CVICALLBACK cbPanel(int panel, int event, void *callbackData,
						int eventData1, int eventData2) {
	if (event==EVENT_CLOSE) {
		EXEC_Cfg 	*exec 		=	NULL;
		
		GetPanelAttribute(panel, ATTR_CALLBACK_DATA, &exec);
		if (exec) {
			exec->terminate = TRUE;
			while (!exec->stopped)
				Sleep(1000);
			free(exec);
		}		
		QuitUserInterface(0);
	}
	return 0;
}


// ------------------------ CALLBACK FOR DIAGNOSTIC ----------------------------
int CVICALLBACK cbBtnDbg (int panel, int control, int event,
					   void *callbackData, int eventData1, int eventData2) {
	int 			error 		=	0,
					status 		=	0;
	ErrMsg 			errMsg 		=	{0};
	CAN_Setting		*setting	=	NULL;
	
	if (event == EVENT_COMMIT) {
		errChk(GetCtrlAttribute(panel, control, ATTR_CTRL_VAL, &status));
		if (status) {
			errChk(zlgCAN_Initialize(errMsg));
			nullChk(setting = malloc(sizeof(CAN_Setting)));
			memset(setting, 0, sizeof(CAN_Setting));
			setting->panel = panel;
			errChk(GetCtrlAttribute(panel, PANEL_numTorqueCmd, ATTR_CTRL_VAL, &setting->torqueCmd));
			errChk(SetCtrlAttribute(panel, control, ATTR_CALLBACK_DATA, setting));
			errChk(CmtScheduleThreadPoolFunction(DEFAULT_THREAD_POOL_HANDLE, ecu_CmdDaemon, setting, &setting->thid));
			errChk(CmtScheduleThreadPoolFunction(DEFAULT_THREAD_POOL_HANDLE, gui_MonitorCanDaemon, setting, NULL));
		} else {
			setting = (CAN_Setting *) callbackData;
			if (setting) {
				setting->terminate = TRUE;
				CmtWaitForThreadPoolFunctionCompletion(DEFAULT_THREAD_POOL_HANDLE, setting->thid, 
													   OPT_TP_PROCESS_EVENTS_WHILE_WAITING);
				CmtReleaseThreadPoolFunctionID(DEFAULT_THREAD_POOL_HANDLE, setting->thid);
				free(setting);
			}
			zlgCAN_Close();
			errChk(SetCtrlAttribute(panel, control, ATTR_CALLBACK_DATA, NULL));
		}
	}
Error:
	if (error<0)
		MessagePopup("Error", errMsg);
	return 0;
}


// ------------------- CALLBACK TO SET TORQUE ------------------------------
int CVICALLBACK cbTorqueCmd (int panel, int control, int event,
							 void *callbackData, int eventData1, int eventData2)
{
	CAN_Setting		*setting	=	NULL;
	int 			error 		=	0;
	ErrMsg 			errMsg 		=	{0};
	
	if (event==EVENT_COMMIT) {
		errChk(GetCtrlAttribute(panel, PANEL_btnCan, ATTR_CALLBACK_DATA, &setting));
		if (setting) {
			float 	torqueCmd 	=	0;
			errChk(GetCtrlAttribute(panel, control, ATTR_CTRL_VAL, &torqueCmd));
			setting->torqueCmd = (torqueCmd+200)*5;
		}
	}
Error:
	if (error<0) {
		strcpy(errMsg, GetUILErrorString(error));
		MessagePopup("Error", errMsg);
	}
	return 0;
}


// ------------------- CALLBACK TO SET TORQUE ------------------------------
int CVICALLBACK cbCtrlMode (int panel, int control, int event,
							 void *callbackData, int eventData1, int eventData2)
{
	CAN_Setting		*setting	=	NULL;
	int 			error 		=	0;
	ErrMsg 			errMsg 		=	{0};
	
	if (event==EVENT_COMMIT) {
		errChk(GetCtrlAttribute(panel, PANEL_btnCan, ATTR_CALLBACK_DATA, &setting));
		if (setting) {
			errChk(GetCtrlAttribute(panel, control, ATTR_CTRL_VAL, &setting->controlMode));
		}
	}
Error:
	if (error<0) {
		strcpy(errMsg, GetUILErrorString(error));
		MessagePopup("Error", errMsg);
	}
	return 0;
}


// ------------------------ CALLBACK FOR MODBUS ----------------------------
int CVICALLBACK cbModbus (int panel, int control, int event,
					   void *callbackData, int eventData1, int eventData2) {
	int 			error 		=	0,
					status 		=	0;
	ErrMsg 			errMsg 		=	{0};
	double 			f[6];
	
	if (event == EVENT_COMMIT) {
		switch(control) {
			case PANEL_btnVISA:
				errChk(GetCtrlAttribute(panel, control, ATTR_CTRL_VAL, &status));
				if (status) {
					errChk(DYNO_InitializeFromJson("HW_Config.json", errMsg));
				} else {
					DYNO_Destroy();
				}
				errChk(gui_disableCtrls(panel, !status, errMsg));
				break;
			case PANEL_btnTorque:
				errChk(DYNO_ReadTorque(f, f+1, errMsg));
				errChk(SetCtrlAttribute(panel, PANEL_numTorque, ATTR_CTRL_VAL, f[0]));
				errChk(SetCtrlAttribute(panel, PANEL_numSpeed, ATTR_CTRL_VAL, f[1]));
				break;
		/*	case PANEL_btnTempSensor:
				//errChk(DYNO_ReadTempSensor(f, errMsg));
				errChk(SetCtrlAttribute(panel, PANEL_numTemp, ATTR_CTRL_VAL, f[0]));
				break;*/
			case PANEL_btnChiller:
				//errChk(DYNO_EnableChiller((unsigned char) status, errMsg));
				break;
			/*case PANEL_btnDebug:
				const unsigned char  p[] = {0x2B, 0x02, 0x3E, 0xC7};
				errChk(SetCtrlAttribute(panel, PANEL_numericDebug, ATTR_CTRL_VAL, f[1])//(double) buf2flowRate(p)
									   );*/
			case PANEL_btnLoad:
				errChk(GetCtrlAttribute(panel, control, ATTR_CTRL_VAL, &status));
				if (status) {
					unsigned short	speed = 0;
					errChk(GetCtrlAttribute(panel, PANEL_numLoadSpeedSet, ATTR_CTRL_VAL, &speed));
					errChk(DYNO_SetLoadSpeed(speed, errMsg));
					errChk(DYNO_SetLoadMode(load_rotateCW, errMsg));
				} else {
					errChk(DYNO_SetLoadMode(load_stop, errMsg));
				}
				break;
			case PANEL_btnSetHV:
				errChk(GetCtrlAttribute(panel, control, ATTR_CTRL_VAL, &status));
				if (status) {
					float 	data[4];
					errChk(GetCtrlAttribute(panel, PANEL_numVoltage, ATTR_CTRL_VAL, data));
					errChk(GetCtrlAttribute(panel, PANEL_numCurrent, ATTR_CTRL_VAL, data+1));
					errChk(GetCtrlAttribute(panel, PANEL_numOVP, 	 ATTR_CTRL_VAL, data+2));
					errChk(GetCtrlAttribute(panel, PANEL_numOCP, 	 ATTR_CTRL_VAL, data+3));
					errChk(DYNO_ConfigurePS(data[0], data[1], TRUE, data[2], data[3], errMsg));					
				} else {
					errChk(DYNO_ConfigurePS(0, 0, FALSE, 0, 0, errMsg));
				}
				break;
			case PANEL_btnGetHV:
				float outputVoltage, outputCurrent;
				char alertCode;
				errChk(DYNO_ReadPS(&outputVoltage, &outputCurrent, NULL, NULL, &alertCode, errMsg));
				sprintf(errMsg, "output voltage: %f V\noutput current: %fA\nalert code: %d", outputVoltage, outputCurrent, alertCode);
				utility_puts(errMsg);
				break;
			case PANEL_btnPA:
				int *tpa = (int *) callbackData;
				errChk(GetCtrlAttribute(panel, control, ATTR_CTRL_VAL, &status));
				if (status) {
					*tpa = FALSE;
					errChk(CmtScheduleThreadPoolFunction(DEFAULT_THREAD_POOL_HANDLE, YokogawaDaemon, tpa, NULL));
				} else {
					*tpa = TRUE;
				}
				break;
			case PANEL_btnMonitor:
				EXEC_Cfg 	*exec 		=	NULL;
				errChk(GetCtrlAttribute(panel, control, ATTR_CTRL_VAL, &status));
				errChk(SetCtrlAttribute(panel, PANEL_btnTorque, ATTR_DIMMED, status));
				errChk(SetCtrlAttribute(panel, PANEL_btnTempSensor, ATTR_DIMMED, status));
				errChk(SetCtrlAttribute(panel, PANEL_btnVISA, ATTR_DIMMED, status));
				if (status) {
					nullChk(exec = malloc(sizeof(EXEC_Cfg)));
					memset(exec, 0, sizeof(EXEC_Cfg));
					exec->panel = panel;
					errChk(SetPanelAttribute(panel, ATTR_CALLBACK_DATA, exec));
					// launch new thread
					errChk(CmtScheduleThreadPoolFunction(DEFAULT_THREAD_POOL_HANDLE, gui_monitor, exec, NULL));
				} else {
					// terminate
					errChk(GetPanelAttribute(panel, ATTR_CALLBACK_DATA, &exec));
					if (exec) {
						exec->terminate = TRUE;
						while (!exec->stopped)
							Sleep(1000);
						errChk(SetPanelAttribute(panel, ATTR_CALLBACK_DATA, NULL));
						free(exec);
					}
				}
				break;
		}
	}
Error:
	if (error<0)
		MessagePopup("Error", errMsg);
	return 0;
}


// -------------------------------- MONITOR THREAD ------------------------------------
int CVICALLBACK gui_monitor(void *functionData) {
	int 		error 	=	0;
	ErrMsg 		errMsg 	=	{0};
	EXEC_Cfg 	*cfg  	=	(EXEC_Cfg *) functionData;
	double 		torque 	=	0,
				speed 	=	0,
				temp[6]	=	{0};
	
	utility_launchProgressBar(__FUNCTION__, 60, utility_puts, 0, TRUE, errMsg);
	while (!cfg->terminate) {
		Sleep(200);
		errChk(DYNO_ReadTorque(&torque, &speed, errMsg));
		Sleep(200);
		//errChk(DYNO_ReadTempSensor(temp, errMsg));
		errChk(SetCtrlAttribute(cfg->panel, PANEL_numTorque, ATTR_CTRL_VAL, torque));
		errChk(SetCtrlAttribute(cfg->panel, PANEL_numSpeed, ATTR_CTRL_VAL, speed));
		errChk(SetCtrlAttribute(cfg->panel, PANEL_numTemp, ATTR_CTRL_VAL,temp[0]));
	}
Error:
	cfg->stopped = TRUE;
	utility_stopProgressBar(utility_puts, error, errMsg);
	return error;
}


int gui_disableCtrls(int panel, int disable, char errorMsg[]) {
	int 	error 		=	0,
			count 		=	0,
			i 			=	0,
			ctrlArray 	=	0,
			ctrl 		=	0;
	ErrMsg 	errMsg 		=	{0};

	errChk(ctrlArray = GetCtrlArrayFromResourceID(panel, CTRLARRAY));
	errChk(GetNumCtrlArrayItems(ctrlArray, &count));
	for (i = 0; i<count; i++) {
		errChk(ctrl = GetCtrlArrayItem(ctrlArray, i));
		errChk(SetCtrlAttribute(panel, ctrl, ATTR_DIMMED, disable));
	}
Error:
	reportError();
	return error;
}
