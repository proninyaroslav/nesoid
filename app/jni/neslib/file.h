int FASTAPASS(2) FCEU_fopen(char *path, char *mode);
int FASTAPASS(1) FCEU_fclose(int stream);
size_t FASTAPASS(3) FCEU_fread(void *ptr, size_t size, size_t nmemb, int stream);
size_t FASTAPASS(3) FCEU_fwrite(void *ptr, size_t size, size_t nmemb, int stream);
int FASTAPASS(3) FCEU_fseek(int stream, long offset, int whence);
long FASTAPASS(1) FCEU_ftell(int stream);
void FASTAPASS(1) FCEU_rewind(int stream);
int FASTAPASS(2) FCEU_read32(void *Bufo, int fp);
int FASTAPASS(1) FCEU_fgetc(int stream);
long FASTAPASS(1) FCEU_fgetsize(int stream);
int FASTAPASS(1) FCEU_fisarchive(int stream);

void ApplyIPS(FILE *ips, int destf);
int FASTAPASS(1) FCEU_fopen_forcemem(char *path);

#define FCEUD_UTF8fopen fopen

