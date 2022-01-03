#ifndef _stopwatchstate_h
#define _stopwatchstate_h

#include <Arduino.h>

namespace sw
{

extern volatile unsigned long lap_time_ms;
extern volatile int beam_state;

String build_message(bool sync_time);

}


#endif // _stopwatchstate_h
