#ifndef _DETECTION_CALL_H_
#define _DETECTION_CALL_H_
#include <stdio.h>
#include <stdbool.h>


void key_init(void);

void key_event_deal_with(void);
char det_call_pin_call(void);
void key_trigger_set(int family);
#endif
