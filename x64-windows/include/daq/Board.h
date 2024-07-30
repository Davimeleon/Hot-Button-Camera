#ifndef BOARD_H
#define BOARD_H

#include <conio.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
	
//#include "cbw.h"

#define BI_ADNUMTRIGTYPES	318
#define BI_ADTRIGTYPE		319
#define BI_DISCANOPTIONS	367
#define BI_DOSCANOPTIONS	368
#define BI_DAQISCANOPTIONS	370
#define BI_DAQOSCANOPTIONS	371
#define BI_DAQINUMCHANTYPES 376
#define BI_DAQICHANTYPE		377
#define BI_DAQONUMCHANTYPES 378
#define BI_DAQOCHANTYPE		379

#define CHANTYPENAMELEN		22
#define PORTTYPENAMELEN		22

int ReportError, HandleError;
BOOL GeneralError;

void MoveCursor(int x, int y);
void DisplayMessage(int ErrorNumber);


void InitUL(void)
{
	//  Initiate error handling
	//   activating error handling will trap errors like
	//   bad channel numbers and non-configured conditions.
	//   Parameters:
	//     MccDaq.ErrorReporting.PrintAll :all warnings and errors encountered will be printed
	//     MccDaq.ErrorHandling.StopAll   :if an error is encountered, the program will stop

	int ULStat;
	GeneralError = FALSE;
	ReportError = PRINTALL;
	HandleError = STOPALL;
	ULStat = cbErrHandling(ReportError, HandleError);
}

int GetNameOfBoard(int TestBoard, char* BoardName)
{
	int ULStat, retVal;

	retVal = 1;
	strcpy(BoardName, "unknown");
	ULStat = cbErrHandling(DONTPRINT, DONTSTOP);
	ULStat = cbFlashLED(TestBoard);
	if (ULStat == BADBOARDTYPE) {
		// FlashLED not supported, use another function
		// to check for live device
		ULStat = cbStopBackground(TestBoard, AIFUNCTION);
		if (ULStat != NOERRORS) {
			DisplayMessage(ULStat);
			retVal = 0;
		}
	}
	if ((ULStat == BADBOARD) | (ULStat == DEADDEV) 
		| (ULStat == CFGFILENOTFOUND)) {
		DisplayMessage(ULStat);
		retVal = 0;
	}
	ULStat = cbGetBoardName(TestBoard, BoardName);
	if (ULStat != NOERRORS) {
		DisplayMessage(ULStat);
		retVal = 0;
	}
	return retVal;
}

void GetNameOfChanType(int chanType, char* typeName)
{
	switch (chanType & 0xFF) {
	case(ANALOG):
		strcpy(typeName, "ANALOG");
		break;
	case(DIGITAL8):
		strcpy(typeName, "DIGITAL8");
		break;
	case(DIGITAL16):
		strcpy(typeName, "DIGITAL16");
		break;
	case(CTR16):
		strcpy(typeName, "CTR16");
		break;
	case(CTR32LOW):
		strcpy(typeName, "CTR32LOW");
		break;
	case(CTR32HIGH):
		strcpy(typeName, "CTR32HIGH");
		break;
	case(CJC):
		strcpy(typeName, "CJC");
		break;
	case(TC):
		strcpy(typeName, "TC");
		break;
	case(ANALOG_SE):
		strcpy(typeName, "ANALOG_SE");
		break;
	case(ANALOG_DIFF):
		strcpy(typeName, "ANALOG_DIFF");
		break;
	case(SETPOINTSTATUS):
		strcpy(typeName, "SETPOINTSTATUS");
		break;
	case(CTRBANK0):
		strcpy(typeName, "CTRBANK0");
		break;
	case(CTRBANK1):
		strcpy(typeName, "CTRBANK1");
		break;
	case(CTRBANK2):
		strcpy(typeName, "CTRBANK2");
		break;
	case(CTRBANK3):
		strcpy(typeName, "CTRBANK3");
		break;
	case(PADZERO):
		strcpy(typeName, "PADZERO");
		break;
	case(DIGITAL):
		strcpy(typeName, "DIGITAL");
		break;
	case(CTR):
		strcpy(typeName, "CTR");
		break;
	default:
		strcpy(typeName, "Unknown");
		break;
	}
}

void GetNameOfPortType(int portType, char* portTypeStr)
{
	switch (portType)
	{
	case(AUXPORT0):
		strcpy(portTypeStr, "AUXPORT0");
		break;
	case(AUXPORT1):
		strcpy(portTypeStr, "AUXPORT1");
		break;
	case(AUXPORT2):
		strcpy(portTypeStr, "AUXPORT2");
		break;
	case(FIRSTPORTA):
		strcpy(portTypeStr, "FIRSTPORTA");
		break;
	case(FIRSTPORTB):
		strcpy(portTypeStr, "FIRSTPORTB");
		break;
	case(FIRSTPORTCL):
		strcpy(portTypeStr, "FIRSTPORTCL");
		break;
	case(FIRSTPORTCH):
		strcpy(portTypeStr, "FIRSTPORTCH");
		break;
	case(SECONDPORTA):
		strcpy(portTypeStr, "SECONDPORTA");
		break;
	case(SECONDPORTB):
		strcpy(portTypeStr, "SECONDPORTBA");
		break;
	case(SECONDPORTCL):
		strcpy(portTypeStr, "SECONDPORTCL");
		break;
	case(SECONDPORTCH):
		strcpy(portTypeStr, "SECONDPORTCH");
		break;
	case(THIRDPORTA):
		strcpy(portTypeStr, "THIRDPORTA");
		break;
	case(THIRDPORTB):
		strcpy(portTypeStr, "THIRDPORTB");
		break;
	case(THIRDPORTCL):
		strcpy(portTypeStr, "THIRDPORTCL");
		break;
	case(THIRDPORTCH):
		strcpy(portTypeStr, "THIRDPORTCH");
		break;
	case(FOURTHPORTA):
		strcpy(portTypeStr, "FOURTHPORTA");
		break;
	case(FOURTHPORTB):
		strcpy(portTypeStr, "FOURTHPORTB");
		break;
	case(FOURTHPORTCL):
		strcpy(portTypeStr, "FOURTHPORTCL");
		break;
	case(FOURTHPORTCH):
		strcpy(portTypeStr, "FOURTHPORTCH");
		break;
	case(FIFTHPORTA):
		strcpy(portTypeStr, "FIFTHPORTA");
		break;
	case(FIFTHPORTB):
		strcpy(portTypeStr, "FIFTHPORTB");
		break;
	case(FIFTHPORTCL):
		strcpy(portTypeStr, "FIFTHPORTCL");
		break;
	case(FIFTHPORTCH):
		strcpy(portTypeStr, "FIFTHPORTCH");
		break;
	case(SIXTHPORTA):
		strcpy(portTypeStr, "SIXTHPORTA");
		break;
	case(SIXTHPORTB):
		strcpy(portTypeStr, "SIXTHPORTB");
		break;
	case(SIXTHPORTCL):
		strcpy(portTypeStr, "SIXTHPORTCL");
		break;
	case(SIXTHPORTCH):
		strcpy(portTypeStr, "SIXTHPORTCH");
		break;
	case(SEVENTHPORTA):
		strcpy(portTypeStr, "SEVENTHPORTA");
		break;
	case(SEVENTHPORTB):
		strcpy(portTypeStr, "SEVENTHPORTB");
		break;
	case(SEVENTHPORTCL):
		strcpy(portTypeStr, "SEVENTHPORTCL");
		break;
	case(SEVENTHPORTCH):
		strcpy(portTypeStr, "SEVENTHPORTCH");
		break;
	case(EIGHTHPORTA):
		strcpy(portTypeStr, "EIGHTHPORTA");
		break;
	case(EIGHTHPORTB):
		strcpy(portTypeStr, "EIGHTHPORTB");
		break;
	case(EIGHTHPORTCL):
		strcpy(portTypeStr, "EIGHTHPORTCL");
		break;
	case(EIGHTHPORTCH):
		strcpy(portTypeStr, "EIGHTHPORTCH");
		break;
	default:
		strcpy(portTypeStr, "Invalid Port");
		break;
	}
}

void DisplayMessage(int ErrorNumber)
{
	int ULStat;
	char ErrMessage[ERRSTRLEN];


	if (ErrorNumber != NOERRORS) {
		ULStat = cbGetErrMsg(ErrorNumber, ErrMessage);
		printf("\a\n");
		printf("\n  Error %u occurred (%s)\n", ErrorNumber, ErrMessage);
		if ((ErrorNumber == BADBOARD) | (ErrorNumber == DEADDEV)
			| (ErrorNumber == CFGFILENOTFOUND))
			printf("\n\n  Verify your device is properly installed with Instacal.\n");
		GeneralError = TRUE;
	} else {
		printf("\nPress any key...\n");
	}
	while (!_kbhit()) {
		Sleep(500);
	}
}

/***************************************************************************
*
* Name:      ClearScreen
* Arguments: ---
* Returns:   ---
*
* Clears the screen.
*
***************************************************************************/

#define BIOS_VIDEO   0x10

void ClearScreen(void)
{
	COORD coordOrg = { 0, 0 };
	DWORD dwWritten = 0;
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	if (INVALID_HANDLE_VALUE != hConsole)
		FillConsoleOutputCharacter(hConsole, ' ', 80 * 50, coordOrg, &dwWritten);

	MoveCursor(0, 0);

	return;
}


/***************************************************************************
*
* Name:      MoveCursor
* Arguments: x,y - screen coordinates of new cursor position
* Returns:   ---
*
* Positions the cursor on screen.
*
***************************************************************************/


void MoveCursor(int x, int y)
{
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	if (INVALID_HANDLE_VALUE != hConsole)
	{
		COORD coordCursor;
		coordCursor.X = (short)x;
		coordCursor.Y = (short)y;
		SetConsoleCursorPosition(hConsole, coordCursor);
	}

	return;
}


/***************************************************************************
*
* Name:      GetTextCursor
* Arguments: x,y - screen coordinates of new cursor position
* Returns:   *x and *y
*
* Returns the current (text) cursor position.
*
***************************************************************************/

void GetTextCursor(int* x, int* y)
{
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	*x = -1;
	*y = -1;
	if (INVALID_HANDLE_VALUE != hConsole)
	{
		GetConsoleScreenBufferInfo(hConsole, &csbi);
		*x = csbi.dwCursorPosition.X;
		*y = csbi.dwCursorPosition.Y;
	}

	return;
}


#endif //UTILITIES_H
