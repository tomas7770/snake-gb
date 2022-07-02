/*

 SAVEDATA.H

 Data that should be saved to SRAM.

*/
#include <stdint.h>

#ifndef __savedata_h_INCLUDE
#define __savedata_h_INCLUDE

extern uint16_t save_signature; // signature to check if save exists or not
extern uint8_t high_scores[10]; // 5 speed values * (screen wrap or not)

#endif
