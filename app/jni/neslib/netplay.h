#ifdef NETWORK
int InitNetplay(void);
void KillNetplay(void);
void NetplayUpdate(uint16 *JS1, uint16 *JS2);

extern int netplay;
#endif
