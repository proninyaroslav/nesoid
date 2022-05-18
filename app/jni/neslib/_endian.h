int write16(uint16 b, FILE *fp);
int write32(uint32 b, FILE *fp);
int read32(void *Bufo, FILE *fp);

#define write32le write32
#define read32le read32
