/***************************************************************************
                          net.cpp  -  basic class for network support
                             -------------------
    begin                : Thu Oct 26 2000
    copyright            : (C) 2000 by Franck Ladurelle
    email                : ladurelf@partimage.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
  #include <config.h>
#endif

#include "net.h"
#include "pathnames.h"

size_t CNet::nSend(CIdefRemote remote, const void * buf, size_t len)
{
  static DWORD nb = 0L;

  size_t res;
  showDebug(3, "begin # %ld for %d bytes \n", nb, len);

#ifdef HAVE_SSL
  showDebug(3, "send avec ssl\n");
  if (remote.Ssl)
    res = SSL_write(remote.Ssl, (char *) buf, len);
  else
#endif
    res = send(remote.Sock, buf, len, MSG_NOSIGNAL); 

  if (res != len)
    showDebug(1, "end: %s\n", strerror(errno));
  showDebug(3, "end ret=%d\n", res);
  nb++;
  return res;
}

  
size_t CNet::nRecv(CIdefRemote remote, void * buf, size_t len)
{
  unsigned int res;
  static DWORD nb = 0L;
  showDebug(3, "begin # %ld for %d\n", nb, len);

  
#ifdef HAVE_SSL
  if (remote.Ssl)
    res = SSL_read(remote.Ssl, (char *) buf, len);
  else
#endif
    res = recv(remote.Sock, buf, len, MSG_WAITALL);

  if ((signed int)res == -1)
    showDebug(1, "end %s\n", strerror(errno));
  else if (res < len)
    showDebug(1, "received %d bytes instead of %d\n", res, len);
  
  showDebug(3, "end nb=%d\n", res);
  nb++;
  return (size_t) res;
}

  
size_t CNet::nRecvMsg(CIdefRemote remote, CMessages * msg)
{
  size_t res;
  void * buf;
  showDebug(3, "begin (*)\n");
  buf = malloc (MESSAGE_LEN);

  res = nRecv(remote, buf, MESSAGE_LEN);

  if (res < sizeof(CMessages))
    showDebug(1, "incomplete message received\n");

  msg->fromString((char *) buf);
  free(buf);
  return res;
}


size_t CNet::nSendMsg(CIdefRemote remote, CMessages * msg)
{
  size_t len;

  len = nSend(remote, msg->toString(), MESSAGE_LEN);

  if (len != sizeof(CMessages))
    showDebug(1, "(*) incomplete message sent (%d instead of %d)\n",
       len, sizeof(CMessages));
  showDebug(3, "end (*)\n");
  return len;
}


char * CNet::Banner(bool bUseSSL, bool bMustLogin)
{
  char * szBanner = (char *) malloc (BANNER_SIZE+1);
  memset(szBanner, ' ', BANNER_SIZE);
  *(szBanner+BANNER_SIZE) = '\0';
  strcpy(szBanner, PACKAGE_NETPROTOVER);
#ifdef HAVE_SSL
  if (bUseSSL)
    { strcat(szBanner, " SSL"); }
#endif
#ifdef MUST_LOGIN
  if (bMustLogin)
    { strcat(szBanner, " LOG"); }
#endif

  showDebug(1, "generated banner: %s\n", szBanner);
  return szBanner;
}

void CNet::CopyProtoVersion(char *szDestBufData, int nDestBufSize, char *szBanner)
{
    char szVersion[BANNER_SIZE];
    int i;

    // partimage versions compatible with network protocol "PiMgNet001"
    const char *szCompatPiMgNet001[]={"0.6.7", "0.6.8", "0.6.9_beta1", "0.6.9_beta2", NULL};

    memset(szVersion, 0, sizeof(szVersion));
    for (i=0; (i<BANNER_SIZE) && (szBanner[i]!=0) && (szBanner[i]!=' '); i++)
        szVersion[i]=szBanner[i];

    snprintf(szDestBufData, nDestBufSize, "%s", szVersion);
    for (i=0; szCompatPiMgNet001[i]!=NULL; i++)
        if (strcmp(szVersion, szCompatPiMgNet001[i])==0)
            snprintf(szDestBufData, nDestBufSize, "PiMgNet001");
}

bool CNet::CompareBanner(char *szBanner1, char *szBanner2, char *szErrorBufDat, int nErrorBufSize)
{
    char szVersion1[BANNER_SIZE];
    char szVersion2[BANNER_SIZE];
    bool bLogin1, bLogin2;
    bool bSsl1, bSsl2;

    memset(szErrorBufDat, 0, nErrorBufSize);
    CopyProtoVersion(szVersion1, sizeof(szVersion1), szBanner1);
    CopyProtoVersion(szVersion2, sizeof(szVersion2), szBanner2);

    bLogin1=strstr(szBanner1, "LOG");
    bLogin2=strstr(szBanner2, "LOG");
    bSsl1=strstr(szBanner1, "SSL");
    bSsl2=strstr(szBanner2, "SSL");

    showDebug(3, "CompareBanner([%s], [%s]): ver1=[%s], ver2=[%s]\n", szBanner1, szBanner2, szVersion1, szVersion2);

    if (strcmp(szVersion1, szVersion2)!=0) // versions differ
    {
        snprintf(szErrorBufDat, nErrorBufSize, "Client and server are using versions with incompatible protocols");
        return BANNERCMP_DIFFVER;
    }
    if (bLogin1 != bLogin2)
    {
        snprintf(szErrorBufDat, nErrorBufSize, "Client and server are using different login options");
        return BANNERCMP_DIFFLOG;
    }
    if (bSsl1 != bSsl2)
    {
        snprintf(szErrorBufDat, nErrorBufSize, "Client and server are using different ssl options");
        return BANNERCMP_DIFFSSL;
    }

    return BANNERCMP_COMPAT; // banners are compatible
}
