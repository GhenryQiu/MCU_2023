//==============================================================================
//
// Title:		my_utility.c
// Purpose:		A short description of the implementation.
//
// Created on:	9/5/2020 at 9:58:14 AM by Chenny Wang.
// Copyright:	. All Rights Reserved.
//
//==============================================================================

//==============================================================================
// Include files
#include "my_utility.h"
#ifdef MY_UTILITY_COMBOBOX
#include "combobox.h"
#endif
//==============================================================================
// Constants


//==============================================================================
// Types
typedef struct {
	int 	panel;
	int 	ctrl;
	short 	enable;
} ControlCfg;

typedef struct {
	ControlCfg 	*progressBar;
	ControlCfg 	*listBox;
} ConsoleCfg;

typedef struct {
	short 				continuousMode;
	short 				complete;
	short 				abort;
	int 				totalSec;
	double 				currentStatus;
	CmtThreadFunctionID	id;
	char 				*caller;
	int 				(*func)(const char *reportText);
} UUT_Progress;

//==============================================================================
// Static global variables
static ConsoleCfg 	*conn	= 	NULL;
static UUT_Progress	*pb 	=	NULL;

//==============================================================================
// Static functions
int CVICALLBACK utility_progressBarDaemon(void *functionData);
double utility_calculateElapsedPercentage(CVIAbsoluteTime *start, double duration);

//==============================================================================
// Global variables

//==============================================================================
// Global functions
#ifdef MY_UTILITY_COMBOBOX
// --------- POPULATE COMBO SELECTION FROM DIRECTORY FILE NAMES ----------------
int utility_setComboByPopulateFolder(int panel, int combo, const char *folderPath, char errorMsg[]) {
	int 				error 						=	0,
						status 						=	TRUE;
	ErrMsg 				errMsg 						=	{0};
	char 				folder[MAX_PATHNAME_LEN] 	= 	{0};
	WIN32_FIND_DATAA 	found; 
    HANDLE 				hfind;
	
	errChk(Combo_NewComboBox(panel, combo));
	errChk(Combo_SetComboAttribute(panel, combo, ATTR_COMBO_TEXT_FONT, "Courier New"));
	errChk(utility_checkFolderEx(folderPath, FALSE, errMsg));
	sprintf(folder, "%s\\*.*", folderPath);
	hfind = FindFirstFileA(folder, &found);
	if (hfind !=  INVALID_HANDLE_VALUE) {
		while (status) {
			if (!(strcmp(found.cFileName, ".")==0 || strcmp(found.cFileName, "..")==0))
				errChk(Combo_InsertComboItem(panel, combo, -1, found.cFileName)); 
			status = FindNextFileA(hfind, &found);
		}
	}
	// set combo item
	errChk(Combo_GetComboAttribute(panel, combo, ATTR_COMBO_NUM_ITEMS, &status));
 	if (status)
		errChk(Combo_SetIndex(panel, combo, 0));
Error:
	reportError();
	return error;
}
#endif


// ------------------- GET ABSOLUTE PATH ---------------------------------------
char *utility_getAbsolutePath(const char *relativePath, char *fullPath) {
	int 		error 	= 	0;
	char 		*p 		=	NULL;
	
	errChk(GetModuleDir(__CVIUserHInst, fullPath));
	nullChk(p = strrchr(fullPath, '\\'));
	*p = 0;
	sprintf(fullPath, "%s\\Data\\%s", fullPath, relativePath);
Error:
	return error<0?NULL:fullPath;
}


// -------------------------- CLEAR PLOT TAB PAGE -------------------------------
int utility_setCtrlArray(int panel, int controlArray, CtrlArray_Mode mode, int value, char errorMsg[]) {
	int 	error 		=	0,
			ctrlArray 	=	0,
			count 		=	0,
			i 			=	0,
			ctrl 		=	0;
	
	errChk(ctrlArray = GetCtrlArrayFromResourceID(panel, controlArray));
	errChk(GetNumCtrlArrayItems(ctrlArray, &count));
	for (i=0; i<count; i++) {
		errChk(ctrl = GetCtrlArrayItem(ctrlArray, i));
		switch (mode) {
			case ctrlArray_value:
				errChk(SetCtrlAttribute(panel, ctrl, ATTR_CTRL_VAL, (double) value));
				break;
			case ctrlArray_dimmed:
				errChk(SetCtrlAttribute(panel, ctrl, ATTR_DIMMED, value));
				break;	
			case ctrlArray_chart:
				errChk(ClearStripChart(panel, ctrl));
				break;
		}
	}
Error:
	if (error<0)	sprintf(errorMsg, "%s->%s", __FUNCTION__, GetUILErrorString(error));
	return error;
}


// -----------------------------------------------------------------------------
int utility_hookConsole(int panel, int list, int progressBar) {
	if (panel>0) {
		if (conn==NULL) {
			conn = malloc(sizeof(ConsoleCfg));
			conn->listBox 		=	NULL;
			conn->progressBar	=	NULL;
		}
		if (progressBar>0) {
			conn->progressBar 	= 	malloc(sizeof(ControlCfg));
			conn->progressBar->panel 	= panel;
			conn->progressBar->ctrl	 	= progressBar;
			conn->progressBar->enable	= 1;
		}
		if (list>0) {
			conn->listBox 		=	malloc(sizeof(ControlCfg));
			conn->listBox->panel		= panel;
			conn->listBox->ctrl 		= list;
			conn->listBox->enable		= 1;
		}
	}
	return 0;
}


// ------------------------ RESET CONSOLE --------------------------------
int utility_resetConsole(void) {
	if (conn) {
		if (conn->listBox)
			DeleteTextBoxLines(conn->listBox->panel, 
							   conn->listBox->ctrl, 0, -1);
		if (conn->progressBar)
			SetCtrlAttribute(conn->progressBar->panel, 
							 conn->progressBar->ctrl, 
							 ATTR_CTRL_VAL, 0.0);
	}
	return 0;
}


// ------------------------ DESTROY CONTROL ------------------------------
int utility_destroyConsole(void) {
	if (conn) {
		if (conn->listBox)
			free(conn->listBox);
		if (conn->progressBar)
			free(conn->progressBar);
		free(conn);
	}
	return 0;
}


// ------------------------ ENABLE CONSOLE -------------------------------
int utility_enableConsole(Console_Mode mode, short enable) {
	if (conn) {
		if (conn->listBox && mode!=Console_progressBar) {
			conn->listBox->enable = enable;
		}
		if (conn->progressBar && mode!=Console_listBox) {
			conn->progressBar->enable = enable;
		}
	}	
	return enable;
}


// ------------------------- OUPUT TO CONSOLE ----------------------------
int utility_puts(const char *reportText) {
	int 		count 	=	0;
	ControlCfg 	*cfg	=	NULL;
	if (conn) {
		cfg = conn->listBox;
		if (cfg && cfg->enable && reportText) {
			InsertTextBoxLine(cfg->panel, cfg->ctrl, -1, reportText);
			GetNumTextBoxLines(cfg->panel, cfg->ctrl, &count);
			if (count>32) {
				SetCtrlAttribute(cfg->panel, cfg->ctrl, 
								 ATTR_FIRST_VISIBLE_LINE, count-32);
			}
		}
	}
	return 0;
}


// ------------------------- OUPUT TO CONSOLE ----------------------------
int utility_putHex(const unsigned char *reportText, int dataSize) {
	int 		count 	=	0;
	ControlCfg 	*cfg	=	NULL;
	char 		*f 		=	NULL;
	
	if (conn) {
		cfg = conn->listBox;
		if (cfg && cfg->enable && reportText) {
			f = malloc(dataSize*3+10);
			memset(f, 0, dataSize*3+10);
			for (int i = 0; i<dataSize; i++) {
				sprintf(f, "%s%02x-", f, reportText[i]);
			}
			f[strlen(f)-1] = 0;
			InsertTextBoxLine(cfg->panel, cfg->ctrl, -1, f);
			GetNumTextBoxLines(cfg->panel, cfg->ctrl, &count);
			if (count>32) {
				SetCtrlAttribute(cfg->panel, cfg->ctrl, 
								 ATTR_FIRST_VISIBLE_LINE, count-32);
			}
		}
	}
	if (f) 	free(f);
	return 0;
}


// ------------------------- OUTPUT SINGLE ENTRY --------------------------
int utility_updateText(const char *reportText) {
	ControlCfg 	*cfg	=	NULL;
	if (conn) {
		cfg = conn->listBox;
		if (cfg && cfg->enable && reportText) {
			SetCtrlAttribute(cfg->panel, cfg->ctrl, ATTR_CTRL_VAL, reportText);
		}
	}
	return 0;
}


// --------------------- CALCULATE EXECUTION TIME -------------------------
double utility_getExecutionTime(CVIAbsoluteTime	start) {
	CVIAbsoluteTime		end;
	CVITimeInterval		interval;
	double 				executionTime;
	
	GetCurrentCVIAbsoluteTime(&end);
	SubtractCVIAbsoluteTimes(end, start, &interval);
	CVITimeIntervalToSeconds(interval, &executionTime);
	return executionTime;
}


// --------------------- CALCULATE TIMEOUT CVITIME ------------------------
CVIAbsoluteTime utility_getTimeOut(CVIAbsoluteTime *start, double durationSec) {
	CVITimeInterval		interval;	
	CVIAbsoluteTime		end;
	CVITimeIntervalFromSeconds(durationSec, &interval);
	GetCurrentCVIAbsoluteTime(start);
	AddToCVIAbsoluteTime(*start, interval, &end);
	return end;
}


// --------------------- IS TIMEOUT ---------------------------------------
int utility_isTimeout(CVIAbsoluteTime *timeout) {
	CVIAbsoluteTime current;
	int 			status 	=	0;
	GetCurrentCVIAbsoluteTime(&current);
	CompareCVIAbsoluteTimes(current, *timeout, &status);
	return status>=0;
	
}


// --------------------- CALCULATE COMPLETE PERCENTAGE --------------------
double utility_calculateElapsedPercentage(CVIAbsoluteTime *start, double duration) {
	CVIAbsoluteTime current;
	CVITimeInterval	interval;
	double 			f 			=	0;
	GetCurrentCVIAbsoluteTime(&current);
	SubtractCVIAbsoluteTimes(current, *start, &interval);
	CVITimeIntervalToSeconds(interval, &f);
	return (f*100/(duration+1));
}


// --------------------- GET TIMESTAMP ------------------------------------
char *utility_getTimestamp(char *time_stamp, short standard) {
	LPSYSTEMTIME lpSystemTime = malloc(sizeof(SYSTEMTIME));

	if (time_stamp) {	
		GetLocalTime(lpSystemTime);
		if (standard) {
			sprintf(time_stamp, "%d-%02d-%02d %02d:%02d:%02d.%03d", 
				lpSystemTime->wYear,
				lpSystemTime->wMonth,
				lpSystemTime->wDay,
				lpSystemTime->wHour,
				lpSystemTime->wMinute,
				lpSystemTime->wSecond,
				lpSystemTime->wMilliseconds);
		} else {
			sprintf(time_stamp, "%d-%02d-%02d_%02d-%02d-%02d", 
				lpSystemTime->wYear,
				lpSystemTime->wMonth,
				lpSystemTime->wDay,
				lpSystemTime->wHour,
				lpSystemTime->wMinute,
				lpSystemTime->wSecond);	
		}
	}
	free(lpSystemTime);
	return time_stamp;
}


// --------------------- CHECK IF FILE EXISTS -----------------------------
int utility_doesFileExists(const char *filePath, char basename[], char errorMsg[]) {
	int 	error 		= 	0;
	ErrMsg 	errMsg 		= 	{0};
	DWORD	dwAttrib	= 	GetFileAttributes(filePath);
	
	if (filePath==NULL || strlen(filePath)<4) {
		strcpy(errMsg, "file path can not be empty or NULL");
		errChk(INVALID_FILE_PATH);
	}
	if (dwAttrib == INVALID_FILE_ATTRIBUTES) {
		sprintf(errMsg, "invalid file path(%s)", filePath);
		errChk(OPEN_FILE_FAILURE);
	}
	if (basename) {
		SplitPath(filePath, NULL, NULL, basename);
	}
Error:
	reportError();
	return error;
}


// --------------------- CREATE FOLDER RECURSIVELY -----------------------------
int utility_checkFolderEx(const char *folderPath, short createIfNotExist, char errorMsg[]) {
	int 			error 		= 	0;
	ErrMsg 			errMsg 		= 	{0};
	DWORD			dwAttrib	= 	GetFileAttributes(folderPath);
	char 			*buffer 	=	NULL,
					*p 			=	NULL;
	
	if (folderPath==NULL || *folderPath==0) {
		strcpy(errMsg, "folder path can not be empty");
		errChk(NULL_OR_EMPTY_STRING);
	}
	if (dwAttrib == INVALID_FILE_ATTRIBUTES) {
		if (createIfNotExist) {
			if (MakeDir(folderPath)<0) {
				nullChk(buffer = malloc(strlen(folderPath)+1));
				strcpy(buffer, folderPath);
				nullChk(p = strrchr(buffer, '\\'));
				*p = 0;
				errChk(utility_checkFolderEx(buffer, TRUE, errMsg));
				errChk(MakeDir(folderPath));
			}
		} else {
			sprintf(errMsg, "invalid folder path(%s)", folderPath);
			errChk(OPEN_FILE_FAILURE);
		}
	}	
Error:
	reportError();
	return error;	
}


// ------------------------------ IS VISA NAME VALID ------------------------------
int utility_checkVisaName(const char *visa, char errorMsg[]) {
	int 			error 		= 	0,
					index	=	0;
	ErrMsg 			errMsg 		= 	{0}; 

	if (visa && *visa) {
		if (Scan(visa, "COM%d", &index)<0) {
			sprintf(errMsg, "invalid visa resource name(%s)", visa);
			errChk(VISA_RESOURCE_NOT_AVAILABLE);
		} 
	} else {
		strcpy(errMsg, "COM port name can not be empty");
		errChk(NULL_OR_EMPTY_STRING);
	}
Error:
	reportError();
	return error<0?error:index;	
}


// ----------------------------- INSERT TAB PAGES -----------------------------
int utility_insertTabPages(int panel, int tabCtrl, TabPageCfg tabCfg[], int numTabPage, char errorMsg[]) {
	int 			error 		= 	0,
					subpanel 	=	0,
					i 			=	0;
	
	for (i = 0; i<numTabPage; i++) {
		errChk(subpanel = LoadPanel(panel, tabCfg[i].uirName, tabCfg[i].panelName));
		errChk(InsertPanelAsTabPage(panel, tabCtrl, i, subpanel));
		// set tab page label
		errChk(SetTabPageAttribute(panel, tabCtrl, i, ATTR_LABEL_TEXT, tabCfg[i].tabName));
	}
	errChk(SetTabPageAttribute(panel, tabCtrl, VAL_ALL_OBJECTS, ATTR_LABEL_FONT, "Courier New"));
	errChk(SetTabPageAttribute(panel, tabCtrl, VAL_ALL_OBJECTS, ATTR_LABEL_POINT_SIZE, 13));
	errChk(SetActiveTabPage(panel, tabCtrl, 0));
Error:
	if (error<0) {
		sprintf(errorMsg, "%s->%s(panel=%d; tab=%d; numTabPage=%d)", 
				__FUNCTION__, GetUILErrorString(error), panel, tabCtrl, numTabPage);
	}
	return error;
}


// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// ------------------- LAUNCH PROGRESSBAR IN BK ---------------------------------
int utility_launchProgressBar(const char *callerFuncName, int totalSec, funcWrite2Console f,
							  short currentStatus, short continuousMode, char errorMsg[]) {
	int 	error 	=	0;
	ErrMsg 	errMsg 	=	{0};
	
	if (conn==NULL || conn->progressBar==NULL) {
		strcpy(errMsg, "panel ctrls are not hooked");
		errChk(PROGRESSBAR_NOT_HOOKED);
	}
	
	if (pb==NULL) {
		nullChk(pb = malloc(sizeof(UUT_Progress)));
		memset(pb, 0, sizeof(UUT_Progress));
		pb->complete 		= FALSE;
		pb->totalSec 		= totalSec;
		pb->currentStatus	= currentStatus;
		pb->continuousMode	= continuousMode;
		pb->caller			= callerFuncName;
		pb->func			= f;

		error = CmtScheduleThreadPoolFunction(DEFAULT_THREAD_POOL_HANDLE, 
									  utility_progressBarDaemon, pb, &pb->id);
		if (error<0) {
			CmtGetErrorMessage(error, errMsg);
			errChk(error);
		}
	} else {
		sprintf(errMsg, "progress bar is running in another daemon(%s)", pb->caller);
		errChk(PROGRESSBAR_OCCUPIED);
	}
Error:
	reportError();
	return error;
}


// --------------------------- STOP PROGRESS BAR ---------------------------------
int utility_stopProgressBar(funcWrite2Console f, int error, char errorMsg[]) {
	ErrMsg 	errMsg = {0};
	
	if (conn && conn->progressBar && pb) {
		sprintf(errMsg, "[Err] %s", errorMsg);
		if (error<0) {
			if (f) (f)(errMsg); 
			Sleep(1000); // need some time for launching progressbar daemon
		}
		if (error != PROGRESSBAR_OCCUPIED) {
			if (pb->id) {
				pb->complete 	= TRUE;
				pb->abort 		= (error<0);
				CmtWaitForThreadPoolFunctionCompletion(DEFAULT_THREAD_POOL_HANDLE, 
									pb->id, OPT_TP_PROCESS_EVENTS_WHILE_WAITING);
				CmtReleaseThreadPoolFunctionID(DEFAULT_THREAD_POOL_HANDLE, pb->id);
			}
			free(pb);
			pb = NULL;
		}
	}
	return 0;
}


// ------------------------------ RUN PROGRESSBAR ---------------------------------
int CVICALLBACK utility_progressBarDaemon(void *functionData) {
	int 			error 		= 	0,
					interval 	=	0;
	UUT_Progress 	*progress	=	(UUT_Progress *) functionData;
	ControlCfg 		*cfg 		=	NULL;
	CVIAbsoluteTime	start, end;
	
	nullChk(conn);
	nullChk(progress);
	cfg = conn->progressBar;
	if (cfg) {
		progress->complete 		= 	FALSE;
		progress->abort 		= 	FALSE;
		progress->currentStatus	=	0;
		interval = (progress->totalSec*10>1000)?1000:progress->totalSec*10;
		end = utility_getTimeOut(&start, progress->totalSec);
		while (!(progress->complete || utility_isTimeout(&end))) {
			progress->currentStatus = utility_calculateElapsedPercentage(&start, progress->totalSec);
			if (progress->continuousMode)
				progress->currentStatus = (int) progress->currentStatus % 100;
			SetCtrlAttribute(cfg->panel, cfg->ctrl, 
							 ATTR_CTRL_VAL, progress->currentStatus);
			Sleep(interval);
		}
		// update final status
		if (!progress->abort)
			SetCtrlAttribute(cfg->panel, cfg->ctrl, ATTR_CTRL_VAL, 100.0);
	}
	if (progress->func)
		(progress->func)(COMPLETE_SEPARATION);
Error:
	return 0;
}