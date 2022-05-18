void flushcache(void *beginning_addr, void *end_addr, unsigned int flags);
void block_or(void *src, size_t n, int pat);
void block_and(void *src, size_t n, int pat);
void block_andor(void *src, size_t n, int andpat, int orpat);
void memset32(int *dest, int c, int count);
void spend_cycles(int c); // utility
void soft_scale(void *dst, unsigned short *pal, int line_offs, int lines);
void convert2RGB555(unsigned short *dst, unsigned char *src, unsigned short *pal, int count);

