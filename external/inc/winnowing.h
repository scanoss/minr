#ifndef __WINNOWING_H
    #define __WINNOWING_H

#include <stdint.h>    

uint32_t winnowing (char *src, uint32_t *hashes, uint32_t *lines, uint32_t limit);
extern uint8_t GRAM;   // Winnowing gram size in bytes
extern uint8_t WINDOW;  // Winnowing window size in bytes
extern uint32_t MAX_UINT32;

#endif
