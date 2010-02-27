/***************************************************************************
                          netclient.h  -  description
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

#ifndef _NETCLIENT_H_
#define _NETCLIENT_H_

#ifdef HAVE_CONFIG_H
  #include <config.h>
#endif

#include "net.h"
#include "netdb.h"
#include "exceptions.h"

// ================================================
class CNetClient : public CNet
{
private:
  CIdefRemote IdfServer;

#ifdef HAVE_SSL
  SSL_CTX * ctx;
  X509 * server_cert;
#if OPENSSL_VERSION_NUMBER >= 0x10000000L
  SSL_METHOD const * meth;
#else
  SSL_METHOD * meth;
#endif // OPENSSL_VERSION_NUMBER

#endif
  bool m_bUseSSL;
  bool m_bMustLogin;
  
public:
  CNetClient(bool bMustLogin, bool bUseSSL);
  void Connect(char * adr, unsigned short int port);
  unsigned int SendPass(char * login, char * pass);

  size_t Send(const void * buf, size_t len){return nSend(IdfServer, buf, len);} 
  size_t Recv(void * buf, size_t len)      {return nRecv(IdfServer, buf, len);}
  size_t SendMsg(CMessages * msg)             {return nSendMsg(IdfServer, msg);}
  size_t RecvMsg(CMessages * msg)             {return nRecvMsg(IdfServer, msg);}
  CExceptions * RecvExcep(const char * msg);
};

#endif // _NETCLIENT_H_
