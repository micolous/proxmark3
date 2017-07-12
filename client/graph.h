//-----------------------------------------------------------------------------
// Copyright (C) 2010 iZsh <izsh at fail0verflow.com>
//
// This code is licensed to you under the terms of the GNU GPL, version 2 or,
// at your option, any later version. See the LICENSE.txt file for the text of
// the license.
//-----------------------------------------------------------------------------
// Graph utilities
//-----------------------------------------------------------------------------

#ifndef GRAPH_H__
#define GRAPH_H__
#include <stdint.h>

void AppendGraph(int redraw, int clock, int bit);
int ClearGraph(int redraw);
//int DetectClock(int peak);
size_t getFromGraphBuf(uint8_t *buff);
int GetAskClock(const char str[], bool printAns, bool verbose);
int GetPskClock(const char str[], bool printAns, bool verbose);
uint8_t GetPskCarrier(const char str[], bool printAns, bool verbose);
uint8_t GetNrzClock(const char str[], bool printAns, bool verbose);
uint8_t GetFskClock(const char str[], bool printAns, bool verbose);
uint8_t fskClocks(uint8_t *fc1, uint8_t *fc2, uint8_t *rf1, bool verbose, int *firstClockEdge);
//uint8_t fskClocks(uint8_t *fc1, uint8_t *fc2, uint8_t *rf1, bool verbose);
bool graphJustNoise(int *BitStream, int size);
void setGraphBuf(uint8_t *buff, size_t size);
#ifdef __cplusplus
extern "C" {
#endif
void save_restoreGB(uint8_t saveOpt);
#ifdef __cplusplus
}
#endif

bool HasGraphData();
void DetectHighLowInGraph(int *high, int *low, bool addFuzz); 

#define GRAPH_SAVE 1
#define GRAPH_RESTORE 0

#endif
