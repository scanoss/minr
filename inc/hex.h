#ifndef __HEX_H
    #define __HEX_H

void hex_to_bin(char *hex, uint32_t len, uint8_t *out);

char *bin_to_hex(uint8_t *bin, uint32_t len);
uint16_t uint16(uint8_t *data);
void uint32_reverse(uint8_t *data);
#endif