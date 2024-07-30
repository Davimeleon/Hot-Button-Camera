
#ifndef ANALOG_H
#define ANALOG_H

#include "Board.h"

#define ANALOGINPUT		1
#define ANALOGOUTPUT	2
#define ANALOGDAQIN		3
#define ANALOGDAQOUT	4
#define VOLTAGEINPUT	5
#define TEMPERATUREIN	6
#define PRETRIGIN		9
#define ATRIGIN			17

#define RANGENAMELEN	24

//prototypes
void DisplayMessage(int);
//void GetTrigResolution(int* tRes, int* tRange);
int* TestInputRanges(int* DefaultRange, int* numFound);
int* TestOutputRanges(int* DefaultRange, int* numFound);

int TestBoard;
int ADRes, DARes;
int ChansFound, IOType;

int FindAnalogChansOfType(int BoardNum, int AnalogType, int* Resolution, 
	int* DefaultRange, int* DefaultChan, int* DefaultTrig)
{
	short status;
	int defRange;
	long curCount, curIndex;
	int ChansFound;
	int functionType;
	BOOL CheckPretrig, CheckATrig;
	int RangesFound = 0;
	int TestRange, HardRange;
	int ULStat, resAD;
	int* rangeList;
	int ConfigVal;

	// check supported features by trial 
	// and error with error handling disabled
	ULStat = cbErrHandling(DONTPRINT, DONTSTOP);

	TestBoard = BoardNum;
	ChansFound = 0;
	TestRange = 0;
	*DefaultChan = 0;
	//*DefaultTrig = TRIG_POS_EDGE;
	defRange = NOTUSED;

	IOType = (AnalogType & 7);
	switch (IOType) {
	case ANALOGDAQIN:
	case ANALOGINPUT:
	case VOLTAGEINPUT:
		// Get the number of A/D channels
		ULStat = cbGetConfig(BOARDINFO, TestBoard, 0, BINUMADCHANS, &ChansFound);
		if (ULStat != NOERRORS) {
			DisplayMessage(ULStat);
			return ChansFound;
		}
		if (ChansFound > 0) {
			// Get the resolution of A/D
			ULStat = cbGetConfig(BOARDINFO, TestBoard, 0, BIADRES, &resAD);
			if (ULStat == NOERRORS) {
				*Resolution = resAD;
				ADRes = resAD;
			}
			// ToDo  If (AnalogType And &HF00&) > 0 Then CheckInputEvents = True

			// check ranges for a valid default
			rangeList = TestInputRanges(&TestRange, &RangesFound);
			if (RangesFound == 0) return 0;

			ULStat = cbGetConfig(BOARDINFO, TestBoard, 0, BIRANGE, &HardRange);
			if ((HardRange != NOTUSED)
				& (HardRange != 300) & (HardRange != 301)) {
				TestRange = HardRange;
			}
				
			if (RangesFound > 0) defRange = TestRange;
			if (IOType == ANALOGDAQIN) {
				functionType = DAQIFUNCTION;
				ULStat = cbGetIOStatus(BoardNum, &status, &curCount, &curIndex, functionType);
				if (ULStat != NOERRORS) {
					return 0;
				}
			}
		}
		break;
	case TEMPERATUREIN:
		ULStat = cbGetConfig(BOARDINFO, TestBoard, 0, BINUMTEMPCHANS, &ConfigVal);
		if (!ConfigVal)
			return 0;
		return ConfigVal;
	default:
		// Get the number of D/A channels
		ULStat = cbGetConfig(BOARDINFO, TestBoard, 0, BINUMDACHANS, &ChansFound);
		if (ULStat != NOERRORS) {
			DisplayMessage(ULStat);
			return ChansFound;
		}
		if (ChansFound > 0) {
			ULStat = cbGetConfig(BOARDINFO, TestBoard, 0, BIDACRES, &DARes);
			*Resolution = DARes;
			rangeList = TestOutputRanges(&TestRange, &RangesFound);
			if (RangesFound > 0) defRange = TestRange;
			if (IOType == ANALOGDAQOUT)
			{
				functionType = DAQOFUNCTION;
				ULStat = cbGetStatus(BoardNum, &status, &curCount, &curIndex, functionType);
				if (ULStat != NOERRORS) {
					return 0;
				}
			}
		}
		break;
	}

	CheckATrig = ((AnalogType & ATRIGIN) == ATRIGIN);
	if ((ChansFound > 0) & CheckATrig)
	{
		ULStat = cbSetTrigger(TestBoard, TRIGABOVE, 0, 0);
		if (ULStat == NOERRORS) {
			*DefaultTrig = TRIGABOVE;
			//GetTrigResolution();
		} else
			ChansFound = 0;
	}

	CheckPretrig = ((AnalogType & PRETRIGIN) == PRETRIGIN);
	if ((ChansFound > 0) & CheckPretrig)
	{
		// if DaqSetTrigger supported, trigger type is analog
		ULStat = cbDaqSetTrigger(TestBoard, TRIG_IMMEDIATE,
			TRIGABOVE, 0, ANALOG, *DefaultRange, 0.0f, 0.1f, START_EVENT);
		if (ULStat == NOERRORS)
			*DefaultTrig = TRIGABOVE;
		else {
			ULStat = cbSetTrigger(TestBoard, TRIG_POS_EDGE, 0, 0);
			if (ULStat == NOERRORS)
				*DefaultTrig = TRIG_POS_EDGE;
			else
				ChansFound = 0;
		}
	}

	ULStat = cbErrHandling(ReportError, HandleError);
	*DefaultRange = defRange;
	return ChansFound;
}

int* TestInputRanges(int* DefaultRange, int* numFound)
{
	USHORT dataValue;
	int Options, index;
	ULONG dataHRValue;
	int ULStat;
	int TestRange;
	int RangesFound = 0;
	float dataFltValue;
	double dataDblValue;

	static int ValidRanges[49];

	*DefaultRange = NOTUSED;
	TestRange = NOTUSED;
	Options = 0;
	index = 0;
	for (TestRange = BIP5VOLTS; TestRange <= BIP30VOLTS; TestRange++)
	{
		if (ADRes > 16) {
			if ((IOType == ANALOGINPUT) | (IOType == ANALOGDAQIN)) {
				ULStat = cbAIn32(TestBoard, 0, TestRange, &dataHRValue, Options);
				if (ULStat == BADBOARDTYPE) {
					ULStat = cbVIn32(TestBoard, 0, 0, &dataDblValue, 0);
					printf("This device is not compatible with this function.\n\n");
					if (ULStat == NOERRORS)
						printf("   Use 'VIn01' example instead.\n");
					DisplayMessage(BADBOARDTYPE);
					ULStat = BADBOARDTYPE;
					break;
				}
			} else
				ULStat = cbVIn32(TestBoard, 0, 0, &dataDblValue, 0);
		} else {
			if ((IOType == ANALOGINPUT) | (IOType == ANALOGDAQIN)) {
				ULStat = cbAIn(TestBoard, 0, TestRange, &dataValue);
				if (ULStat == BADBOARDTYPE) {
					ULStat = cbVIn(TestBoard, 0, 0, &dataFltValue, 0);
					printf("This device is not compatible with this function.\n\n");
					if (ULStat == NOERRORS)
						printf("   Use 'VIn01' example instead.\n");
					DisplayMessage(BADBOARDTYPE);
					ULStat = BADBOARDTYPE;
					break;
				}
			} else
				ULStat = cbVIn32(TestBoard, 0, 0, &dataDblValue, 0);
		}

		if (ULStat == NOERRORS) {
			if (*DefaultRange == NOTUSED)
				*DefaultRange = TestRange;
			ValidRanges[index] = TestRange;
			index += 1;
			RangesFound += 1;
		} else {
			if ((ULStat == NETDEVINUSEBYANOTHERPROC)
				|| (ULStat == NETDEVINUSE))
			{
				printf("This network device is in use by another process or user.\n\n");
				printf("Check for other users on the network and close any applications\n");
				printf("(such as Instacal) that may be accessing the network device.");
				break;
			}
		}
	}
	ValidRanges[index] = NOTUSED;
	*numFound = RangesFound;
	return 	ValidRanges;
}

int* TestOutputRanges(int* DefaultRange, int* numFound)
{
	short dataValue = 0;
	int ULStat;
	int TestRange;
	int RangesFound = 0;
	int configVal, index;
	int numRangesFound = 0;

	static int ValidRanges[49];

	*DefaultRange = NOTUSED;
	TestRange = -5;
	index = 0;
	for (TestRange = BIP5VOLTS; TestRange <= BIP30VOLTS; TestRange++)
	{
		ULStat = cbAOut(TestBoard, 0, TestRange, dataValue);
		if (ULStat == NOERRORS) {
			ULStat = cbGetConfig(2, TestBoard, 0, BIDACRANGE, &configVal);
			if (ULStat == NOERRORS) {
				// fixed range
				*DefaultRange = configVal;
				ValidRanges[index] = configVal;
				index += 1;
				numRangesFound += 1;
				break;
			}
		} else {
			TestRange = NOTUSED;
			for (TestRange = BIP5VOLTS; TestRange <= BIP30VOLTS; TestRange++) {
				ULStat = cbAOut(TestBoard, 0, TestRange, dataValue);
				if (ULStat == NOERRORS) {
					if (*DefaultRange == NOTUSED)
						* DefaultRange = TestRange;
					break;
				}
				else {
					if ((ULStat == NETDEVINUSEBYANOTHERPROC)
						|| (ULStat == NETDEVINUSE))
					{
						printf("This network device is in use by another process or user.\n\n");
						printf("Check for other users on the network and close any applications\n");
						printf("(such as Instacal) that may be accessing the network device.");
						break;
					}
				}
			}
		}
		if (ULStat == NOERRORS) {
			if (*DefaultRange == NOTUSED)
				* DefaultRange = TestRange;
			ValidRanges[index] = TestRange;
			index += 1;
			RangesFound += 1;
		}
	}
	ValidRanges[index] = NOTUSED;
	*numFound = numRangesFound;
	return 	ValidRanges;
}

int* GetRangeList(int subSystem, int* DefaultRange, int* numFound)
{
	int DefRange;
	int numRangesFound;
	int ULStat;
	int* rangeArr = NULL;

	// ToDo fill ValidRanges?
	// check supported ranges by trial 
	// and error with error handling disabled
	ULStat = cbErrHandling(DONTPRINT, DONTSTOP);

	if (subSystem == ANALOGOUTPUT)
		rangeArr = TestOutputRanges(&DefRange, &numRangesFound);
	else
		rangeArr = TestInputRanges(&DefRange, &numRangesFound);

	*numFound = numRangesFound;
	*DefaultRange = DefRange;

	ULStat = cbErrHandling(ReportError, HandleError);
	return rangeArr;
	//return numRangesFound;
}

void GetRangeInfo(int Range, char* RangeString, double* RangeVolts)
{
	strcpy(RangeString, "");
	switch (Range)
	{
	case NOTUSED:
		strcpy(RangeString, "NOTUSED");
		*RangeVolts = 0.0;
		break;
	case BIP60VOLTS:
		strcpy(RangeString, "BIP60VOLTS");
		*RangeVolts = 120.0;
		break;
	case BIP30VOLTS:
		strcpy(RangeString, "BIP30VOLTS");
		*RangeVolts = 60.0;
		break;
	case BIP20VOLTS:
		strcpy(RangeString, "BIP20VOLTS");
		*RangeVolts = 40.0;
		break;
	case BIP15VOLTS:
		strcpy(RangeString, "BIP15VOLTS");
		*RangeVolts = 30.0;
		break;
	case BIP10VOLTS:
		strcpy(RangeString, "BIP10VOLTS");
		*RangeVolts = 20.0;
		break;
	case BIP5VOLTS:
		strcpy(RangeString, "BIP5VOLTS");
		*RangeVolts = 10.0;
		break;
	case BIP4VOLTS:
		strcpy(RangeString, "BIP4VOLTS");
		*RangeVolts = 8.0;
		break;
	case BIP2PT5VOLTS:
		strcpy(RangeString, "BIP2PT5VOLTS");
		*RangeVolts = 5.0;
		break;
	case BIP2VOLTS:
		strcpy(RangeString, "BIP2VOLTS");
		*RangeVolts = 4.0;
		break;
	case BIP1PT25VOLTS:
		strcpy(RangeString, "BIP1PT25VOLTS");
		*RangeVolts = 2.5;
		break;
	case BIP1VOLTS:
		strcpy(RangeString, "BIP1VOLTS");
		*RangeVolts = 2.0;
		break;
	case BIPPT625VOLTS:
		strcpy(RangeString, "BIPPT625VOLTS");
		*RangeVolts = 1.25;
		break;
	case BIPPT5VOLTS:
		strcpy(RangeString, "BIPPT5VOLTS");
		*RangeVolts = 1.0;
		break;
	case BIPPT1VOLTS:
		strcpy(RangeString, "BIPPT1VOLTS");
		*RangeVolts = 0.2;
		break;
	case BIPPT05VOLTS:
		strcpy(RangeString, "BIPPT05VOLTS");
		*RangeVolts = 0.1;
		break;
	case BIPPT312VOLTS:
		strcpy(RangeString, "BIPPT312VOLTS");
		*RangeVolts = 0.624;
		break;
	case BIPPT25VOLTS:
		strcpy(RangeString, "BIPPT25VOLTS");
		*RangeVolts = 0.5;
		break;
	case BIPPT2VOLTS:
		strcpy(RangeString, "BIPPT2VOLTS");
		*RangeVolts = 0.4;
		break;
	case BIPPT156VOLTS:
		strcpy(RangeString, "BIPPT156VOLTS");
		*RangeVolts = 0.3125;
		break;
	case BIPPT125VOLTS:
		strcpy(RangeString, "BIPPT125VOLTS");
		*RangeVolts = 0.25;
		break;
	case BIPPT078VOLTS:
		strcpy(RangeString, "BIPPT078VOLTS");
		*RangeVolts = 0.15625;
		break;
	case BIPPT01VOLTS:
		strcpy(RangeString, "BIPPT01VOLTS");
		*RangeVolts = 0.02;
		break;
	case BIPPT005VOLTS:
		strcpy(RangeString, "BIPPT005VOLTS");
		*RangeVolts = 0.01;
		break;
	case BIP1PT67VOLTS:
		strcpy(RangeString, "BIP1PT67VOLTS");
		*RangeVolts = 3.34;
		break;
	case UNI10VOLTS:
		strcpy(RangeString, "UNI10VOLTS");
		*RangeVolts = 10.0;
		break;
	case UNI5VOLTS:
		strcpy(RangeString, "UNI5VOLTS");
		*RangeVolts = 5.0;
		break;
	case UNI4VOLTS:
		strcpy(RangeString, "UNI4VOLTS");
		*RangeVolts = 4.096;
		break;
	case UNI2PT5VOLTS:
		strcpy(RangeString, "UNI2PT5VOLTS");
		*RangeVolts = 2.5;
		break;
	case UNI2VOLTS:
		strcpy(RangeString, "UNI2VOLTS");
		*RangeVolts = 2.0;
		break;
	case UNI1PT25VOLTS:
		strcpy(RangeString, "UNI1PT25VOLTS");
		*RangeVolts = 1.25;
		break;
	case UNI1VOLTS:
		strcpy(RangeString, "UNI1VOLTS");
		*RangeVolts = 1.0;
		break;
	case UNIPT25VOLTS:
		strcpy(RangeString, "UNIPT25VOLTS");
		*RangeVolts = 0.25;
		break;
	case UNIPT2VOLTS:
		strcpy(RangeString, "UNIPT2VOLTS");
		*RangeVolts = 0.2;
		break;
	case UNIPT1VOLTS:
		strcpy(RangeString, "UNIPT1VOLTS");
		*RangeVolts = 0.1;
		break;
	case UNIPT05VOLTS:
		strcpy(RangeString, "UNIPT05VOLTS");
		*RangeVolts = 0.05;
		break;
	case UNIPT01VOLTS:
		strcpy(RangeString, "UNIPT01VOLTS");
		*RangeVolts = 0.01;
		break;
	case UNIPT02VOLTS:
		strcpy(RangeString, "UNIPT02VOLTS");
		*RangeVolts = 0.02;
		break;
	case UNI1PT67VOLTS:
		strcpy(RangeString, "UNI1PT67VOLTS");
		*RangeVolts = 1.67;
		break;
	case MA4TO20:
		strcpy(RangeString, "MA4TO20");
		*RangeVolts = 16.0;
		break;
	case MA2TO10:
		strcpy(RangeString, "MA2to10");
		*RangeVolts = 8.0;
		break;
	case MA1TO5:
		strcpy(RangeString, "MA1TO5");
		*RangeVolts = 4.0;
		break;
	case MAPT5TO2PT5:
		strcpy(RangeString, "MAPT5TO2PT5");
		*RangeVolts = 2.0;
		break;
	case MA0TO20:
		strcpy(RangeString, "MA0TO20");
		*RangeVolts = 20.0;
		break;
	case BIPPT025AMPS:
		strcpy(RangeString, "BIPPT025A");
		*RangeVolts = 0.05;
		break;
	case BIPPT025VOLTSPERVOLT:
		strcpy(RangeString, "BIPPT025VPERV");
		*RangeVolts = 0.05;
		break;
	default:
		strcpy(RangeString, "NOTUSED");
		*RangeVolts = 0.0;
		break;
	}
}

void GetTrigResolution(int* tRes, int* tRange)
{
	static int ATrigRes;
	static int ATrigRange;

	int BoardID, TrigSource;
	int ULStat;

	ATrigRange = 0;
	ATrigRes = 0;
	ULStat = cbGetConfig(TestBoard, 2, 0, BIADTRIGSRC, &TrigSource);
	ULStat = cbGetConfig(BOARDINFO, TestBoard, 0, BIBOARDTYPE, &BoardID);
	if (ULStat == NOERRORS) {
		switch (BoardID) {
		case 95:
		case 96:
		case 97:
		case 98:
		case 102:
		case 165:
		case 166:
		case 167:
		case 168:
		case 177:
		case 178:
		case 179:
		case 180:
		case 203:
		case 204:
		case 205:
		case 213:
		case 214:
		case 215:
		case 216:
		case 217:
		{
			//PCI-DAS6030, 6031, 6032, 6033, 6052
			//USB-1602HS, 1602HS-2AO, 1604HS, 1604HS-2AO
			//PCI-2511, 2513, 2515, 2517, USB-2523, 2527, 2533, 2537
			//USB-1616HS, 1616HS-2, 1616HS-4, 1616HS-BNC
			ATrigRes = 12;
			ATrigRange = 20;
			if (TrigSource > 0) ATrigRange = -1;
		}
		break;
		case 101:
		case 103:
		case 104:
		{
			//PCI-DAS6040, 6070, 6071
			ATrigRes = 8;
			ATrigRange = 20;
			if (TrigSource > 0) ATrigRange = -1;
		}
		break;
		default:
		{
			ATrigRes = 0;
			ATrigRange = -1;
		}
		break;
		}
	}
	*tRes = ATrigRes;
	*tRange = ATrigRange;
}

#endif //ANALOG_H
