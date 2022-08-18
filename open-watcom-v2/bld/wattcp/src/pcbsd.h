#ifndef __PCBSD_H
#define __PCBSD_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <rpc/rpc.h>
#include <netdb.h>

extern int netdbCacheLife;

extern char  *_inet_ntoa (char *s, DWORD x);
extern DWORD  _inet_addr (const char *s);
extern DWORD  _gethostid (void);
extern DWORD  _sethostid (DWORD ip);

extern int   resolve_ip  (DWORD ip, char *name);
extern int   reverse_lookup_myip (void);

extern void  ReadHostFile     (const char *fname);
extern void  ReadServFile     (const char *fname);
extern void  ReadProtoFile    (const char *fname);
extern void  ReadNetworksFile (const char *fname);

extern void  CloseHostFile    (void);
extern void  CloseServFile    (void);
extern void  CloseProtoFile   (void);
extern void  CloseNetworksFile(void);

extern const char *GetHostsFile    (void);
extern const char *GetServFile     (void);
extern const char *GetProtoFile    (void);
extern const char *GetNetworksFile (void);

extern void  ReopenHostFile     (void);
extern void  ReopenServFile     (void);
extern void  ReopenProtoFile    (void);
extern void  ReopenNetworksFile (void);

#ifdef USE_ETHERS
extern void  InitEthersFile  (const char *fname);
extern void  ReadEthersFile  (void);
#endif

#endif
