// Declare globals. This should only be included, exactly once, in files with a
// main() function.  These will be mostly replaced with a pm3_connection struct
// in comms.h.

// Serial port that we are communicating with the PM3 on.
serial_port* port;

// If TRUE, then there is no active connection to the PM3, and we will drop commands sent.
bool offline;

// Transmit buffer.
// TODO: Use locks and execute this on the main thread, rather than the receiver
// thread.  Running on the main thread means we need to be careful in the
// flasher, as it means SendCommand is no longer async, and can't be used as a
// buffer for a pending command when the connection is re-established.
UsbCommand txcmd;
bool txcmd_pending;

// Used by UsbReceiveCommand as a ring buffer for messages that are yet to be
// processed by a command handler (WaitForResponse{,Timeout})
UsbCommand cmdBuffer[CMD_BUFFER_SIZE];

// Points to the next empty position to write to
int cmd_head;//Starts as 0

// Points to the position of the last unread command
int cmd_tail;//Starts as 0

// Demodulation buffer, used by mainly low-frequency tags
uint8_t DemodBuffer[MAX_DEMOD_BUF_LEN];
size_t DemodBufferLen;
int g_DemodStartIdx;
int g_DemodClock;

// Sample buffer, to be supplied in GetFromBigBuf and then written in by the device.
uint8_t* sample_buf;

// Graphing related globals.
// FIXME: These should eventually make their way into particular UI implementations instead.
int GraphBuffer[MAX_GRAPH_TRACE_LEN];
int GraphTraceLen;

int s_Buff[MAX_GRAPH_TRACE_LEN];
double CursorScaleFactor;
int PlotGridX;
int PlotGridY;
int PlotGridXdefault;
int PlotGridYdefault;
int CursorCPos;
int CursorDPos;
int flushAfterWrite;  //buzzy
int GridOffset;
bool GridLocked;
bool showDemod;

uint8_t g_debugMode;

