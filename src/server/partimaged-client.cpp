/***************************************************************************
                          partimaged-client.cpp  -  manage clients from server
                             -------------------
    begin                : Fri Feb 23 2001
    copyright            : (C) 2001 by Franck Ladurelle
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

#include "partimaged-client.h"
#include "common.h"


// =======================================================
CPartimagedClients::CPartimagedClients()
{
  showDebug(10, "mutex init for clients\n");
  pthread_mutex_init(&mClients, &mClientsAttr);
  Clients = new CIdefRemote[g_uiNbClients+1]; // one more for first client to reject
}

// =======================================================
CPartimagedClients::~CPartimagedClients()
{
  showDebug(10, "mutex destroy for clients\n");
  pthread_mutex_destroy(&mClients);
  delete Clients;
}

// =======================================================
unsigned int CPartimagedClients::New()
{
  unsigned int next = 0;
  bool found = false;

  showDebug(1, "create new clients\n");
  pthread_mutex_lock(&mClients);
  while (!found && next < g_uiNbClients)
    {
      if (!Clients[next].Present)
        found = true;
      else
        next++;
    }

  Clients[next].Present = true;
  pthread_mutex_unlock(&mClients);
  showDebug(1, "new clients created : %d\n", next);
  return found ? next : g_uiNbClients;
}

// =======================================================
void CPartimagedClients::Set(unsigned int client, int sock)
{
  pthread_mutex_lock(&mClients);
  Clients[client].Sock = sock;
  Clients[client].Ssl = NULL;
  
  pthread_mutex_unlock(&mClients);
}

// =======================================================
void CPartimagedClients::Set(unsigned int client, int sock,
   struct sockaddr_in sockaddr, void * _ssl)
{
  pthread_mutex_lock(&mClients);

  Clients[client].Sock = sock;
  Clients[client].SockAddr = sockaddr;
#ifdef HAVE_SSL
  Clients[client].Ssl = (SSL *) _ssl;
#else
  Clients[client].Ssl = (char *) _ssl;
#endif
  pthread_mutex_unlock(&mClients);
}

// =======================================================
void CPartimagedClients::Release(unsigned int client)
{
  pthread_mutex_lock(&mClients);
  showDebug(1, "%d released\n", client);
  shutdown(Clients[client].Sock, SHUT_RDWR);
  Clients[client].Sock = 0;
  Clients[client].Present = false;
  Clients[client].MyPid = 0;
  pthread_mutex_unlock(&mClients);
}

// =======================================================
void CPartimagedClients::ReleaseClientByPid(unsigned int client_pid)
{
  unsigned int next = 0;
  bool found = false;

  pthread_mutex_lock(&mClients);
  while (!found && next < g_uiNbClients)
    {
      if (Clients[next].MyPid == client_pid)
	    {
          found = true;
          showDebug(1, "client %d pid = %d released by pid\n", next, client_pid);
          shutdown(Clients[next].Sock, SHUT_RDWR);
          Clients[next].Sock = 0;
          Clients[next].Present = false;
          Clients[next].MyPid = 0;
	    }
      else
        next++;
    }
  pthread_mutex_unlock(&mClients);
}

// =======================================================
unsigned int CPartimagedClients::SetClientPid(unsigned int client, unsigned int client_pid)
{
  unsigned int ret = 0; 
  
  pthread_mutex_lock(&mClients);
  showDebug(1, "client %d pid = %d set\n", client, client_pid);
  if (Clients[client].Present)
    {
      Clients[client].MyPid = client_pid;
	  ret = 1;
    }
  pthread_mutex_unlock(&mClients);
  return ret;
}

// =======================================================
#ifdef HAVE_SSL
SSL * CPartimagedClients::Ssl(unsigned int client)
{
  SSL * aux;
  pthread_mutex_lock(&mClients);
  aux = Clients[client].Ssl;
  pthread_mutex_unlock(&mClients);
  return aux;
}
#else
char * CPartimagedClients::Ssl(unsigned int client)
{
  char * aux;
  pthread_mutex_lock(&mClients);
  aux = Clients[client].Ssl;
  pthread_mutex_unlock(&mClients);
  return aux;
}
#endif

// =======================================================
int CPartimagedClients::Sock(unsigned int client)
{
  int aux;
  pthread_mutex_lock(&mClients);
  aux = Clients[client].Sock;
  pthread_mutex_unlock(&mClients);
  return aux;
}

// =======================================================
struct sockaddr_in CPartimagedClients::GetSA(unsigned int client)
{
  struct sockaddr_in aux;
  pthread_mutex_lock(&mClients);
  aux = Clients[client].SockAddr;
  pthread_mutex_unlock(&mClients);
  return aux;
}

// =======================================================
void CPartimagedClients::SetSock(unsigned int client, int sock)
{
  pthread_mutex_lock(&mClients);
  Clients[client].Sock = sock;
  pthread_mutex_unlock(&mClients);
}

// =======================================================
void CPartimagedClients::SetSsl(unsigned int client, void * ssl)
{
  pthread_mutex_lock(&mClients);
#ifdef HAVE_SSL
  Clients[client].Ssl = (SSL *) ssl;
#else
  Clients[client].Ssl = (char *) ssl;
#endif
  pthread_mutex_unlock(&mClients);
}
