Gstreamer talker application

Supports one gstreamer pipeline with two sinks, using appsink or fdsink interface.
Two interconnected state machines are used. One manages the pipeline state, the
other the stream(s)/sink(s) state.
A single thread is used to manage the different state machines.

The application main loop listens to avdecc events (to create/destroy streams),
avb library events (to request more stream data for transmission) and timer events.

The pipeline state machine has the following states:

GST_STATE_STARTING - transition state to GST_STATE_STARTED
GST_STATE_STARTED - pipeline is started. At least one stream is started.
GST_STATE_STOPPED - pipeline is stopped. All streams are stopped.
GST_STATE_LOOP - pipeline is stopped. All streams are looping. Waiting for timeout before switching to GST_STATE_STARTING.

and events:

GST_EVENT_START - start event from stream state machine
GST_EVENT_STOP - stop event from stream state machine
GST_EVENT_LOOP - loop event from gst state machine, when all streams are in looping state
GST_EVENT_TIMER	- timer event from thread loop timeout


  +---------------------------------------------------------------- Yes -------------------------+
  |       +-------------No-------------+                                                         |
  |       |                            |                               ----                      +
  |       |    +---- LOOP --> | Are all streams looping? | -- Yes -+-> LOOP -- Timer --> | Timeout reached | -+
  v       |    |                                                   |   ----                                   |
--------  |  -------                                               +--------------------------- No -----------+
STARTING -+> STARTED 
--------     -------                
  ^            |                                                      ----
  |            +---- STOP --> | Are all streams stopped? | -- Yes --> STOP -- START --+ 
  |                                                                   ----            |
  +-----------------------------------------------------------------------------------+


The stream state machines has the following states:
STREAM_STATE_CONNECTING - transition state to CONNECTED
STREAM_STATE_CONNECTED - avb stream is open and being polled, actively reading from media stack
STREAM_STATE_DISCONNECTING - transition state to DISCONNECTED
STREAM_STATE_DISCONNECTED - avb stream is closed, not reading from media stack. Sink data is read from timer event in pipeline state machine.
STREAM_STATE_WAIT_DATA - avb stream is open but not polled, actively reading from media stack (based on a timer). On timeout transition to STREAM_STATE_LOOP 
STREAM_STATE_LOOP - avb stream is open but not polled, not reading from media stack. On timeout, loop media file and transition to STREAM_STATE_CONNECTING 

and events:
STREAM_EVENT_CONNECT - Connect event from control handler
STREAM_EVENT_DISCONNECT - Disconnect event from control handler
STREAM_EVENT_DATA - Data event from stream file descriptor
STREAM_EVENT_TIMER - Timer event from thread loop
STREAM_EVENT_LOOP_DONE - Loop done event from pipeline state machine


Stream synchronization
The gstreamer appsink interface provides media data buffers and presentation timestamps for those buffers (absolute time
in gPTP timebase) to the application. The application uses the lower 32bit of the buffer PTS (with an 15ms offset) to build
SYNC events used by the media driver interface. These events are passed to the avtp stack and used to build avtp timestamps.
The 15ms offset is introduced to avoid having timestamps in the past due to scheduling delay at the gstreamer interface level.
Synchronization of the two generated streams (on the talker side) depends on the precision of the timestamps provided by the
gstreamer framework.

