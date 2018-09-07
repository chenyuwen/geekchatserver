#ifndef __CRC32_H__

#define __CRC32_H__

#define CRC32_INIT	0xffffffff

uint32_t crc32_update(const void * buf, unsigned int size, uint32_t crc);


static inline uint32_t crc32_classic(const void * buf, unsigned int size)
{
	return crc32_update(buf, size, CRC32_INIT) ^ CRC32_INIT;
}

static inline uint32_t crc32_finalize(uint32_t crc)
{
	return crc ^ CRC32_INIT;
}


#endif /* __CRC32_H__ */
