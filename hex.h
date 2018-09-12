#ifndef __HEX_H__
#define __HEX_H__
#include <stdio.h>

static inline int hex_to_ascii(char *dst, unsigned char *src, int srclen)
{
	static const char *numlists = {"0123456789ABCDEF"};
	int i = 0;
	for(i=0; i<srclen; i++) {
		dst[i * 2 + 0] = numlists[(src[i] >> 4) & 0x0F];
		dst[i * 2 + 1] = numlists[(src[i] >> 0) & 0x0F];
	}
	return 0;
}

static inline int dump_buffer(const char *buffer, int len)
{
	int i = 0;
	for(i=0; i<len; i++) {
		if(buffer[i] <= 0xF) {
			printf("0x0%X ", buffer[i]);
		} else {
			printf("0x%X ", buffer[i]);
		}
		if((i % 20) == 19) {
			printf("\n");
		}
	}
	printf("\n");
	return 0;
}

#endif /*__HEX_H__*/

