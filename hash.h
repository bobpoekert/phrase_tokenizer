#pragma once

#include <stdint.h>

typedef pt_HashState uint32_t;

pt_HashState pt_HashState_new();
void pt_HashState_reset(pt_HashState *inp);
uint32_t pt_HashState_crank(pt_HashState state, char value);

