/*

 SAVEDATA.C

 Data that should be saved to SRAM.

*/
#include <stdint.h>

uint16_t save_signature; // signature to check if save exists or not
uint8_t high_scores[10]; // 5 speed values * (screen wrap or not)
