/**************************************************************************/
/* LabWindows/CVI User Interface Resource (UIR) Include File              */
/*                                                                        */
/* WARNING: Do not add to, delete from, or otherwise modify the contents  */
/*          of this include file.                                         */
/**************************************************************************/

#include <userint.h>

#ifdef __cplusplus
    extern "C" {
#endif

     /* Panels and Controls: */

#define  PANEL                            1       /* callback function: cbPanel */
#define  PANEL_btnCan                     2       /* control type: textButton, callback function: cbBtnDbg */
#define  PANEL_TEXTBOX                    3       /* control type: textBox, callback function: (none) */
#define  PANEL_PROGRESSBAR                4       /* control type: scale, callback function: (none) */
#define  PANEL_btnVISA                    5       /* control type: textButton, callback function: cbModbus */
#define  PANEL_btnTorque                  6       /* control type: command, callback function: cbModbus */
#define  PANEL_btnTempSensor              7       /* control type: command, callback function: cbModbus */
#define  PANEL_btnMonitor                 8       /* control type: textButton, callback function: cbModbus */
#define  PANEL_numSpeed                   9       /* control type: numeric, callback function: (none) */
#define  PANEL_numTemp                    10      /* control type: numeric, callback function: (none) */
#define  PANEL_numTorque                  11      /* control type: numeric, callback function: (none) */
#define  PANEL_btnChiller                 12      /* control type: textButton, callback function: cbModbus */
#define  PANEL_btnGetHV                   13      /* control type: command, callback function: cbModbus */
#define  PANEL_btnLoad                    14      /* control type: textButton, callback function: cbModbus */
#define  PANEL_numLoadSpeedSet            15      /* control type: numeric, callback function: (none) */
#define  PANEL_btnSetHV                   16      /* control type: textButton, callback function: cbModbus */
#define  PANEL_numOCP                     17      /* control type: numeric, callback function: (none) */
#define  PANEL_numOVP                     18      /* control type: numeric, callback function: (none) */
#define  PANEL_numCurrent                 19      /* control type: numeric, callback function: (none) */
#define  PANEL_numVoltage                 20      /* control type: numeric, callback function: (none) */
#define  PANEL_btnPA                      21      /* control type: textButton, callback function: cbModbus */
#define  PANEL_numTorqueCmd               22      /* control type: scale, callback function: cbTorqueCmd */
#define  PANEL_DECORATION                 23      /* control type: deco, callback function: (none) */
#define  PANEL_btnDebug                   24      /* control type: command, callback function: cbModbus */
#define  PANEL_numericDebug               25      /* control type: numeric, callback function: (none) */
#define  PANEL_ringCtrlMode               26      /* control type: ring, callback function: cbCtrlMode */
#define  PANEL_STRIPCHART                 27      /* control type: strip, callback function: (none) */


     /* Control Arrays: */

#define  CTRLARRAY                        1

     /* Menu Bars, Menus, and Menu Items: */

          /* (no menu bars in the resource file) */


     /* Callback Prototypes: */

int  CVICALLBACK cbBtnDbg(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK cbCtrlMode(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK cbModbus(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK cbPanel(int panel, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK cbTorqueCmd(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);


#ifdef __cplusplus
    }
#endif