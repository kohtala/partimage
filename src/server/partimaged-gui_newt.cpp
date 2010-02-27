/***************************************************************************
                          partimaged-gui_newt.cpp  -  description
                             -------------------
    begin                : Wed Feb 21 2001
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

#ifdef HAVE_CONFIG_H
  #include <config.h>
#endif

#include "partimaged-gui_newt.h"
#include "partimage.h"
#include "partimaged.h"
#include "common.h"

#include <newt.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <slang.h>

extern bool g_bMustLogin;
extern bool g_bUseSSL;

// =============================================================================
CPartimagedInterfaceNewt::CPartimagedInterfaceNewt()
{
  if (!g_bBeDaemon)
    {
      unsigned int i;
      char * szAux = (char *) malloc (256);
 
      m_labelClient = new newtComponent[g_uiNbClients];
      m_labelClientState = new newtComponent[g_uiNbClients];
      m_labelClientHostname = new newtComponent[g_uiNbClients];
      m_labelClientLocation = new newtComponent[g_uiNbClients];
      m_labelClientSize = new newtComponent[g_uiNbClients];
      SLang_init_tty('*', 0, 0); // '*' to send SIGTERM
      newtInit();
      newtCls();
      newtPushHelpLine("");

      m_Scroll = newtVerticalScrollbar(50,10, 20, NEWT_COLORSET_WINDOW,
				       NEWT_COLORSET_ACTCHECKBOX);
 
      m_formMain = newtForm(m_Scroll, NULL, 0);

#ifdef MUST_LOGIN
  if (g_bMustLogin)
  #ifdef HAVE_SSL
      if (g_bUseSSL)
          snprintf(szAux, 255, "%s %s LOGIN+SSL", i18n("Partimaged"), PACKAGE_VERSION);
      else
  #endif
          snprintf(szAux, 255, "%s %s LOGIN+NOSSL", i18n("Partimaged"), PACKAGE_VERSION);
  else
#endif
  #ifdef HAVE_SSL
      if (g_bUseSSL)
          snprintf(szAux, 255, "%s %s NOLOGIN+SSL", i18n("Partimaged"), PACKAGE_VERSION);
      else
  #endif
          snprintf(szAux, 255, "%s %s NOLOGIN+NOSSL", i18n("Partimaged"), PACKAGE_VERSION);

      *(szAux+255) = '\0';
      newtCenteredWindow(73, 20, szAux);

      m_labelClientTitle = newtLabel(2, 2, "");
      m_labelClientStateTitle = newtLabel(13, 2, "");
      m_labelClientHostnameTitle = newtLabel(22, 2, "");
      m_labelClientLocationTitle = newtLabel(42, 2, "");
      m_labelClientSizeTitle = newtLabel(61, 2, "");

      newtLabelSetText(m_labelClientTitle, i18n("Client #"));
      newtLabelSetText(m_labelClientStateTitle, i18n("State"));
      newtLabelSetText(m_labelClientHostnameTitle, i18n("Host"));
      newtLabelSetText(m_labelClientLocationTitle, i18n("Location"));
      newtLabelSetText(m_labelClientSizeTitle, i18n("Size"));

      newtFormAddComponents(m_formMain, m_labelClientTitle, m_labelClientStateTitle,
			    m_labelClientLocationTitle, m_labelClientSizeTitle, NULL);
      newtRefresh();

      for (i = 0; i < g_uiNbClients; i++)
	{
	  m_labelClient[i] = newtLabel(4, 4+i, "");
	  m_labelClientState[i] = newtLabel(9, 4+i, "");
	  m_labelClientHostname[i] = newtLabel(20, 4+i, "");
	  m_labelClientLocation[i] = newtLabel(42, 4+i, "");
	  m_labelClientSize[i] = newtLabel(61, 4+i, "");
	  newtFormAddComponents(m_formMain, m_labelClient[i], m_labelClientState[i],
				m_labelClientLocation[i], m_labelClientSize[i], NULL);
	}

      for (i = 0; i < g_uiNbClients; i++)
	{
	  snprintf(szAux, 255, "%d", i);
	  *(szAux+255) = '\0';
	  newtLabelSetText(m_labelClient[i], szAux);
	  newtLabelSetText(m_labelClientState[i], i18n("waiting"));
	  newtRefresh();
	}
      free(szAux);
    }
}

// =============================================================================
CPartimagedInterfaceNewt::~CPartimagedInterfaceNewt()
{
  if (!g_bBeDaemon)
    newtFinished();
  delete m_labelClient;
  delete m_labelClientState;
  delete m_labelClientHostname;
  delete m_labelClientLocation;
  delete m_labelClientSize;
}

// =============================================================================
void CPartimagedInterfaceNewt::show()
{
  if (!g_bBeDaemon)
    {
      newtRefresh();
    }
}

// =============================================================================
void CPartimagedInterfaceNewt::SetState(int client, const char * state)
{
  if (!g_bBeDaemon)
    {
      newtLabelSetText(m_labelClientState[client], state);
      newtRefresh();
    }
}

// =============================================================================
void CPartimagedInterfaceNewt::SetHostname(int client, const char * state)
{
  if (!g_bBeDaemon)
    {
      newtLabelSetText(m_labelClientHostname[client], state);
      newtRefresh();
    }
}

// =============================================================================
void CPartimagedInterfaceNewt::SetLocation(int client, const char * locate)
{
  if (!g_bBeDaemon)
    {
      newtLabelSetText(m_labelClientLocation[client], locate);
      newtRefresh();
    }
}

// =============================================================================
void CPartimagedInterfaceNewt::SetSize(int client, QWORD size)
{
  if (!g_bBeDaemon)
    {
      char * szAux = (char *) malloc(64);
      if (szAux)
        {
          formatSize(size, szAux);
          newtLabelSetText(m_labelClientSize[client], szAux);
          newtRefresh();
          free(szAux);
        }
    }
}

// =============================================================================
void CPartimagedInterfaceNewt::SetSize(int client, const char * size)
{
  if (!g_bBeDaemon)
    {
      newtLabelSetText(m_labelClientSize[client], size);
      newtRefresh();
    }
}

// =============================================================================
void CPartimagedInterfaceNewt::Status(char * msg)
{
  char szBuf[1024];
  newtPopHelpLine();
  snprintf(szBuf, sizeof(szBuf),
    i18n("%s [Press * to exit]"), msg);
  newtPushHelpLine(szBuf);
  newtRefresh();
}
