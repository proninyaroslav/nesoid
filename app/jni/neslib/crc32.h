#ifdef ZLIB
#include <zlib.h>
#define CalcCRC32 crc32
#else
uint32 CalcCRC32(uint32 crc, uint8 *buf, uint32 len);
#endif
