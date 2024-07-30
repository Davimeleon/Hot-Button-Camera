#ifndef DIGITAL_H
#define DIGITAL_H

#include "Board.h"
#include <math.h>

#define PORTOUT 		1
#define PORTIN			2
#define PORTOUTSCAN		5
#define PORTDAQOUTSCAN	8
#define PORTINSCAN		10
#define PORTDAQINSCAN	16
#define BITOUT			17
#define BITIN		34
#define FIXEDPORT	0
#define PROGPORT	1
#define PROGBIT		2

int TestBoard;
int ChansFound, IOType;
int digInScanOptions, digOutScanOptions;
int daqOutScanOptions, daqInScanOptions;

int FindPortsOfType(int BoardNum, int PortType, int* ProgAbility, int* DefaultPort,
	int* DefaultNumBits, int* FirstBit)
{
	int ThisType, NumPorts, NumBits;
	int InMask, OutMask;
	int PortsFound;
	BOOL PortIsCompatible;
	BOOL CheckBitProg = FALSE;
	BOOL DaqDigitalExists = FALSE;
	int CurPort, configVal;
	int ULStat, firstBit, progAbility;
	int chan, DioDev;
	int BitVals, BitWeight;
	int TotalVal, CurBit;

	// check supported features by trial 
	// and error with error handling disabled
	ULStat = cbErrHandling(DONTPRINT, DONTSTOP);

	*DefaultPort = -1;
	TestBoard = BoardNum;
	CurPort = 0;
	PortsFound = 0;
	*FirstBit = 0;
	*ProgAbility = -1;
	*DefaultNumBits = 0;
	firstBit = 0;
	progAbility = -1;

	ULStat = cbGetConfig(BOARDINFO, BoardNum, 0, BIDINUMDEVS, &NumPorts);
	if (ULStat != NOERRORS) {
		NumPorts = 0;
		return 0;
	}

	if ((PortType == BITOUT) || (PortType == BITIN))
		CheckBitProg = TRUE;
	if ((PortType == PORTOUTSCAN) || (PortType == PORTINSCAN)) {
		// Get digital input scan options, then digital output scan options
		ULStat = cbGetConfig(BOARDINFO, BoardNum, 0, BI_DISCANOPTIONS, &digInScanOptions);
		ULStat = cbGetConfig(BOARDINFO, BoardNum, 0, BI_DOSCANOPTIONS, &digOutScanOptions);
		// Get daq input scan options, then daq output scan options
		ULStat = cbGetConfig(BOARDINFO, BoardNum, 0, BI_DAQISCANOPTIONS, &daqInScanOptions);
		ULStat = cbGetConfig(BOARDINFO, BoardNum, 0, BI_DAQOSCANOPTIONS, &daqOutScanOptions);
		if (PortType == PORTINSCAN)
			if (digInScanOptions == 0) NumPorts = 0;
		if (PortType == PORTOUTSCAN)
			if (digOutScanOptions == 0) NumPorts = 0;
		if (PortType == PORTDAQINSCAN)
			if (daqInScanOptions == 0)
				NumPorts = 0;
			else {
				//Get the number of Daq input channels
				ULStat = cbGetConfig(BOARDINFO, BoardNum, 0, BI_DAQINUMCHANTYPES, &configVal);
				// Get the port types available for DaqInScan
				for (chan = 0; chan < configVal; chan++) {
					ULStat = cbGetConfig(BOARDINFO, BoardNum, chan, BI_DAQICHANTYPE, &configVal);
					if ((configVal == DIGITAL8) | (configVal == DIGITAL16) | (configVal == DIGITAL)) {
						DaqDigitalExists = TRUE;
						break;
					}
					if (!DaqDigitalExists) NumPorts = 0;
				}
			}
		if (PortType == PORTDAQOUTSCAN)
			if (daqOutScanOptions == 0)
				NumPorts = 0;
			else {
				//Get the number of Daq input channels
				ULStat = cbGetConfig(BOARDINFO, BoardNum, 0, 378, &configVal);
				// Get the port types available for DaqInScan
				for (chan = 0; chan < configVal; chan++) {
					ULStat = cbGetConfig(BOARDINFO, BoardNum, 0, 379, &configVal);
					if ((configVal == DIGITAL8) | (configVal == DIGITAL16) | (configVal == DIGITAL)) {
						DaqDigitalExists = TRUE;
						break;
					}
					if (!DaqDigitalExists) NumPorts = 0;
				}
			}
		PortType = PortType & (PORTOUT | PORTIN);
	}

	for (DioDev = 0; DioDev < NumPorts; ++DioDev) {
		progAbility = -1;
		ULStat = cbGetConfig(DIGITALINFO, BoardNum, DioDev, DIINMASK, &InMask);
		ULStat = cbGetConfig(DIGITALINFO, BoardNum, DioDev, DIOUTMASK, &OutMask);
		if ((InMask & OutMask) > 0)
			progAbility = FIXEDPORT;
		ULStat = cbGetConfig(DIGITALINFO, BoardNum, DioDev, DIDEVTYPE, &ThisType);
		if (ULStat == NOERRORS)
			CurPort = ThisType;
		if ((DioDev == 0) && (CurPort == FIRSTPORTCL))
			//a few devices (USB-SSR08 for example)
			//start at FIRSTPORTCL and number the bits
			//as if FIRSTPORTA and FIRSTPORTB exist for
			//compatibiliry with older digital peripherals
			firstBit = 16;

		//check if port is set for requested direction 
		//or can be programmed for requested direction
		PortIsCompatible = FALSE;
		switch (PortType) {
		case (PORTOUT):
			if (OutMask > 0)
				PortIsCompatible = TRUE;
			break;
		case (PORTIN):
			if (InMask > 0)
				PortIsCompatible = TRUE;
			break;
		default:
			PortIsCompatible = FALSE;
			break;
		}
		PortType = (PortType & (PORTOUT | PORTIN));
		if (!PortIsCompatible) {
			if (progAbility != FIXEDPORT) {
				int ConfigDirection;
				ConfigDirection = DIGITALOUT;
				if (PortType == PORTIN)
					ConfigDirection = DIGITALIN;
				if ((CurPort == AUXPORT) && CheckBitProg) {
					//if it's an AuxPort, check bit programmability
					ULStat = cbDConfigBit(BoardNum, AUXPORT, firstBit, ConfigDirection);
					if (ULStat == NOERRORS) {
						//return port to input mode
						ULStat = cbDConfigBit(BoardNum, AUXPORT, firstBit, DIGITALIN);
						progAbility = PROGBIT;
					} else {
						if ((ULStat == NETDEVINUSEBYANOTHERPROC)
							|| (ULStat == NETDEVINUSE)) {
							DisplayMessage(NETDEVINUSE);
							break;
						}
					}
				}
				if (progAbility == -1) {
					//check port programmability
					ULStat = cbDConfigPort(BoardNum, CurPort, ConfigDirection);
					if (ULStat == NOERRORS) {
						//return port to input mode
						ULStat = cbDConfigBit(BoardNum, AUXPORT, firstBit, DIGITALIN);
						progAbility = PROGPORT;
					} else {
						if ((ULStat == NETDEVINUSEBYANOTHERPROC)
							|| (ULStat == NETDEVINUSE)) {
							DisplayMessage(NETDEVINUSE);
							break;
						}
					}
				}
			}
			PortIsCompatible = (progAbility != -1);
		}

		if (PortIsCompatible)
			PortsFound += 1;
		if (*DefaultPort == -1) {
			ULStat = cbGetConfig(DIGITALINFO, BoardNum, DioDev, DINUMBITS, &NumBits);
			if (progAbility == FIXEDPORT) {
				//could have different number of input and output bits
				CurBit = 0;
				TotalVal = 0;
				BitVals = OutMask;
				if (PortType == PORTIN) BitVals = InMask;
				do {
					BitWeight = (int)pow(2, CurBit);
					TotalVal = BitWeight + TotalVal;
					CurBit += 1;
				} while (TotalVal < BitVals);
				NumBits = CurBit;
			}
			if (*DefaultPort == -1) {
				*DefaultNumBits = NumBits;
				*DefaultPort = CurPort;
			}
		}
		if (progAbility == PROGBIT) break;
	}
	ULStat = cbErrHandling(ReportError, HandleError);
	*FirstBit = firstBit;
	*ProgAbility = progAbility;
	return PortsFound;
}

#endif //DIGITAL_H
