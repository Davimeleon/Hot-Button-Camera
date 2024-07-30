#ifndef COUNTERS_H
#define COUNTERS_H

#include "Board.h"

#define CTR8254 	1
#define CTR9513		2
#define CTR8536		3
#define CTR7266		4
#define CTREVENT	5
#define CTRSCAN		6
#define CTRTMR		7
#define CTRQUAD		8
#define CTRPULSE	9

#define BI_CTRSCANOPTIONS	369


int TestBoard;
int ChansFound, IOType;

int FindCountersOfType(int BoardNum, int CounterType, int* DefaultCtr)
{
	int NumCounters, CtrsFound;
	int CounterNum;
	int ThisType;
	int ULStat, CtrDev;

	// check supported features by trial 
	// and error with error handling disabled
	ULStat = cbErrHandling(DONTPRINT, DONTSTOP);

	TestBoard = BoardNum;
	CtrsFound = 0;
	ULStat = cbGetConfig(BOARDINFO, BoardNum, 0, BICINUMDEVS, &NumCounters);
	if (ULStat != NOERRORS) {
		NumCounters = 0;
		return 0;
	}
	*DefaultCtr = -1;
	for (CtrDev = 0; CtrDev < NumCounters; CtrDev++) {
		ULStat = cbGetConfig(COUNTERINFO, BoardNum, CtrDev, CICTRTYPE, &ThisType);
		if (ThisType == CounterType) {
			ULStat = cbGetConfig(COUNTERINFO, BoardNum, CtrDev, CICTRNUM, &CounterNum);
			if (ULStat == NOERRORS) {
				CtrsFound += 1;
				if (*DefaultCtr == -1)
					*DefaultCtr = CounterNum;
			}
		}
	}
	return CtrsFound;
}

#endif //COUNTERS_H
