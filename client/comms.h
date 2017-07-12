#ifndef COMMS_H_
#define COMMS_H_

#include <stdbool.h>
#include <pthread.h>

#include "usb_cmd.h"
#include "uart.h"

#ifndef CMD_BUFFER_SIZE
#define CMD_BUFFER_SIZE 50
#endif

#ifndef MAX_DEMOD_BUF_LEN
#define MAX_DEMOD_BUF_LEN (1024*128)
#endif

#ifndef BIGBUF_SIZE
#define BIGBUF_SIZE 40000
#endif

// Max graph trace len: 40000 (bigbuf) * 8 (at 1 bit per sample)
#define MAX_GRAPH_TRACE_LEN (40000 * 8)

typedef struct {
	// If TRUE, continue running the uart_receiver thread.
	bool run;

	// Lock around serial port receives
	pthread_mutex_t recv_lock;
} receiver_arg;

// TODO: Migrate this all to a new pm3_connection struct

// Serial port that we are communicating with the PM3 on.
extern serial_port* port;

// If TRUE, then there is no active connection to the PM3, and we will drop commands sent.
extern bool offline;

// Transmit buffer.
// TODO: Use locks and execute this on the main thread, rather than the receiver
// thread.  Running on the main thread means we need to be careful in the
// flasher, as it means SendCommand is no longer async, and can't be used as a
// buffer for a pending command when the connection is re-established.
extern UsbCommand txcmd;
extern bool txcmd_pending;

// Used by UsbReceiveCommand as a ring buffer for messages that are yet to be
// processed by a command handler (WaitForResponse{,Timeout})
extern UsbCommand cmdBuffer[CMD_BUFFER_SIZE];

// Points to the next empty position to write to
extern int cmd_head;//Starts as 0

// Points to the position of the last unread command
extern int cmd_tail;//Starts as 0

// Demodulation buffer, used by mainly low-frequency tags
extern uint8_t DemodBuffer[MAX_DEMOD_BUF_LEN];
extern size_t DemodBufferLen;
extern int g_DemodStartIdx;
extern int g_DemodClock;

// Sample buffer, to be supplied in GetFromBigBuf and then written in by the device.
extern uint8_t* sample_buf;

// Graphing related globals.
// FIXME: These should eventually make their way into particular UI implementations instead.
extern int GraphBuffer[MAX_GRAPH_TRACE_LEN];
extern int GraphTraceLen;

extern int s_Buff[MAX_GRAPH_TRACE_LEN];
extern double CursorScaleFactor;
extern int PlotGridX;
extern int PlotGridY;
extern int PlotGridXdefault;
extern int PlotGridYdefault;
extern int CursorCPos;
extern int CursorDPos;
extern int flushAfterWrite;  //buzzy
extern int GridOffset;
extern bool GridLocked;
extern bool showDemod;

void SendCommand(UsbCommand *c);
void *uart_receiver(void *targ);
void UsbCommandReceived(UsbCommand *UC);
void clearCommandBuffer();
bool WaitForResponseTimeout(uint64_t cmd, UsbCommand* response, size_t ms_timeout);
bool WaitForResponse(uint64_t cmd, UsbCommand* response);

#endif // COMMS_H_
