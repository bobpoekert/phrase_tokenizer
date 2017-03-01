#pragma once

#include <stdint.h>

typedef uint32_t pt_HashState;

pt_HashState pt_HashState_new();
void pt_HashState_reset(pt_HashState *inp);
uint32_t pt_HashState_crank(pt_HashState *state, char value);
uint32_t pt_HashState_crankCharacter(pt_HashState *inp, char *buffer, size_t *offset, size_t size);
uint32_t pt_hash(char *string, size_t size);
size_t pt_UTF8CharacterLength(char *buffer, size_t offset, size_t size);
