/***************************************************************************
                          interface_newt.cpp  -  description
                             -------------------
    begin                : Wed Mar 14 2001
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

#define _FILE_OFFSET_BITS 64

#include <slang.h>
#include <newt.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>

#include "gui_text.h"
#include "interface_newt.h"
#include "errors.h"

// =======================================================
void debugWin(const char *fmt, ...)
{
  va_list args;

  va_start(args, fmt);
  newtWinMessagev(i18n("Newt debug information"), i18n("Ok"), (char *) fmt, args);
  va_end(args);
}

//extern int SLang_Abort_Char;

// =======================================================
/*static int getkeyInterruptHook(void) 
{
    return -1;
}

// =======================================================
int myNewtInit(int nCharInterrupt) 
{
  char * MonoValue, * MonoEnv = "NEWT_MONO";
  
  SLtt_get_terminfo();
  SLtt_get_screen_size();
  
  MonoValue = getenv(MonoEnv);
  if ( MonoValue == NULL ) 
    SLtt_Use_Ansi_Colors = 1;
  else 
    SLtt_Use_Ansi_Colors = 0;
  
  SLsmg_init_smg();
  SLang_init_tty(nCharInterrupt, 0, 0);
  
  newtSetColors(newtDefaultColorPalette);
  
#ifdef SIGWINCH
  SLsignal_intr(SIGWINCH, handleSigwinch);
#endif // SIGWINCH
  
  SLang_getkey_intr_hook = getkeyInterruptHook;
  
  return 0;
}*/


// =======================================================
CInterfaceNewt::CInterfaceNewt(bool bBatchMode): CInterface(bBatchMode)
{
  int cols, rows;
	
  m_bPushDone = false;
  //m_bBatchMode = bBatchMode; // in CInterface
  SLang_init_tty('*', 0, 0); // '*' to send SIGTERM
  //myNewtInit(32);
  newtInit();
  //SLang_Abort_Char = 27;
  newtCls();
  newtGetScreenSize(&cols, &rows);
  newtDrawRootText((int)((cols-14)/2), (int)(rows/2), i18n("Please wait..."));
  newtPushHelpLine("");
  newtRefresh();

}

// =======================================================
CInterfaceNewt::~CInterfaceNewt()
{
  newtFinished();
}

// =======================================================
void CInterfaceNewt::showErrorInternal(const char *szText)
{
  msgBoxError(szText);
}

// =======================================================
void CInterfaceNewt::msgBoxOk(const char *szTitle, const char *szText, ...)
{
  va_list args;
  
  if (!getBatchMode())
    {
      va_start(args, szText);
      newtWinMessagev((char *) szTitle, i18n("Ok"), (char *) szText, args);
      va_end(args);
    }
}

// =======================================================
void CInterfaceNewt::msgBoxCancel(const char *szTitle, const char *szText, ...)
{
  va_list args;
  
  if (!getBatchMode())
    {
      va_start(args, szText);
      newtWinMessagev((char *) szTitle, i18n("Cancel"), (char *) szText, args);
      va_end(args);
    }
}

// =======================================================
void CInterfaceNewt::msgBoxError(const char *szText, ...)
{
  va_list args;
  
  va_start(args, szText);
  newtWinMessagev(i18n("Error"), i18n("Cancel"), (char *) szText, args);
  va_end(args);
}

// =======================================================
int CInterfaceNewt::msgBoxContinueCancel(const char *szTitle, const char *szText, ...)
{
  char szBuf[4096];
  va_list args;
  int nRes;

  va_start(args, szText);
  vsnprintf(szBuf, sizeof(szBuf), szText, args);
  va_end(args);
 
  if (!getBatchMode())
    {
      nRes = newtWinChoice((char *) szTitle, i18n("Continue"), i18n("Cancel"), szBuf);
      return ((nRes == 1) ? MSGBOX_CONTINUE : MSGBOX_CANCEL);
    }
  else
    {
      showDebug(1, "Error: %s\n", szBuf);
      return MSGBOX_CANCEL;
    }
}

// =======================================================
int CInterfaceNewt::msgBoxYesNo(const char *szTitle, const char *szText, ...)
{
  char szBuf[4096];
  va_list args;
  int nRes;

  va_start(args, szText);
  vsnprintf(szBuf, sizeof(szBuf), szText, args);
  va_end(args);

  if (!getBatchMode())
    {
      nRes = newtWinChoice((char *) szTitle, i18n("Yes"), i18n("No"), szBuf);
      return ((nRes == 1) ? MSGBOX_YES : MSGBOX_NO);
    }
  else
    {
      showDebug(1, "Error: %s\n", szBuf);
      return MSGBOX_NO;
    }
}

// =======================================================
void CInterfaceNewt::StatusLine(char *str)
{
  char szBuf[1024];
  if (m_bPushDone)
    newtPopHelpLine(); // work only if there is a previous push
  snprintf(szBuf, sizeof(szBuf),
     i18n("%s [* to cancel, CtrlS to pause, CtrlQ to resume]"), str);
  newtPushHelpLine(szBuf); // str
  m_bPushDone = true;
}

// =======================================================
int CInterfaceNewt::guiInitMainWindow(char *szDevice, char *szImageFile, char *szNetworkIP, DWORD *dwServerPort, bool *bSsl)
{
  newtComponent formMain, btnContinue, btnExit, btnAbout;
  newtComponent labelPartition, labelImage, editPartition, editImage, btnImage;
  newtComponent widgetTemp, labelAction, radioSave, radioRestore, radioMbr;
  newtComponent checkNetwork, labelNetwork, labelPort, editNetwork, editPort, checkSsl;
  newtExitStruct event;
  char szTemp[1024];
  int nAction;
  int nRes;

  showDebug(8, "guiInitMainWindow\n");
  
  SNPRINTF(szTemp, "Partition Image %s", PACKAGE_VERSION);
  newtCenteredWindow(67, 20, szTemp);
  
  labelPartition = newtLabel(1, 0, i18n("* Partition to save/restore"));
  editPartition = newtListbox(3, 1, 7, NEWT_FLAG_SCROLL);
  labelImage = newtLabel(1, 9, i18n("* Image file to create/use"));
  editImage = newtEntry(3, 10, szImageFile, 59, NULL, NEWT_FLAG_SCROLL);
  btnImage = newtCompactButton(62, 10, "*");

  labelAction = newtLabel(1, 12, i18n("Action to be done:"));
  radioSave = newtRadiobutton(1, 13, i18n("Save partition into a new image file"), true, NULL);
  radioRestore = newtRadiobutton(1, 14, i18n("Restore partition from an image file"), false, radioSave);
  radioMbr = newtRadiobutton(1, 15, i18n("Restore an MBR from the image file"), false, radioRestore);
  
  checkNetwork = newtCheckbox(1, 17, i18n("Connect to server"), (!!(*szNetworkIP) ? 'X' : ' '), " X", NULL);
#ifdef HAVE_SSL
  checkSsl = newtCheckbox(5, 19, i18n("Encrypt data on the network with SSL"), (*bSsl ? 'X' : ' '), " X", NULL);
#else
  #ifdef MUST_LOGIN
    checkSsl = newtLabel(5, 19,i18n("SSL disabled at compile time"));
  #else
    checkSsl = newtLabel(5, 19,i18n("SSL&login disabled at compile time"));
  #endif
#endif
  labelNetwork = newtLabel(5, 18, i18n("IP/name of the server:"));
  editNetwork = newtEntry(28, 18, szNetworkIP, 25, NULL, 0);
  labelPort = newtLabel(54, 18, i18n("Port:"));
  SNPRINTF(szTemp, "%u", *dwServerPort);
  editPort = newtEntry(60, 18, szTemp, 6, NULL, 0);
  
  btnContinue = newtCompactButton(50, 12, i18n("Next (F5)"));
  btnAbout = newtCompactButton(50, 14, i18n("About"));
  btnExit = newtCompactButton(50, 16, i18n("Exit (F6)"));

  nRes = fillPartitionList(editPartition);
  if (nRes == -1)
    RETURN_int(-1);
  
  formMain = newtForm(NULL, NULL, 0);
  newtFormAddComponents(formMain, labelPartition, labelImage, editPartition, editImage,
                        btnImage, labelAction, radioSave, radioRestore, radioMbr, checkNetwork,
			labelNetwork, editNetwork, labelPort, editPort, checkSsl, 
			btnContinue, btnAbout, btnExit, NULL);
  newtFormAddHotKey(formMain, KEY_EXIT); // Exit
  newtFormAddHotKey(formMain, KEY_OKAY); // Okay

 begin:
  /*widgetTemp = */newtFormRun(formMain, &event);
  widgetTemp = newtFormGetCurrent(formMain);

  if (((event.reason == event.NEWT_EXIT_HOTKEY) && (event.u.key == KEY_EXIT)) || ((event.reason == event.NEWT_EXIT_COMPONENT) && (widgetTemp == btnExit))) 
  //if(widgetTemp == btnExit)
    return OPERATION_EXIT;

  if ((event.reason == event.NEWT_EXIT_COMPONENT) && (widgetTemp == btnAbout))
    {	
      showAboutDialog();
      goto begin;
    }

  if ((event.reason == event.NEWT_EXIT_COMPONENT) && (widgetTemp == btnImage))
    {	
      char *fs_;
      fs_=filesel(newtEntryGetValue(editImage));
      if(fs_) {
        newtEntrySet(editImage, fs_, 1);
//        free(fs_);
        }
      
      goto begin;
    }

  
  if (strcmp(newtEntryGetValue(editImage), "") == 0) // if "image" field empty
    {	
      msgBoxError(i18n("The \"Image\" field is empty. Cannot continue"));
      goto begin;
    }
  
  if (newtCheckboxGetValue(checkNetwork) == 'X')
    {
      if (!(*newtEntryGetValue(editNetwork)))	
	{
	  msgBoxError(i18n("The \"Server IP/Name\" field is empty. Cannot continue"));
	  goto begin;
	}
      
      if (!atoi((char*)newtEntryGetValue(editPort)))
	{
	  msgBoxError(i18n("The \"Server Port\" field is not a valid integer. Cannot continue"));
	  goto begin;
	}
      
    }
  
  // get device
  strcpy(szDevice, (char*)newtListboxGetCurrent(editPartition));
  
  // image file
  strcpy(szImageFile, newtEntryGetValue(editImage));
  
  // network
  if (newtCheckboxGetValue(checkNetwork) == 'X') // If network is activated
    {
      strcpy(szNetworkIP, (char*)newtEntryGetValue(editNetwork));
      *dwServerPort = atoi((char*)newtEntryGetValue(editPort));
#ifdef HAVE_SSL
      *bSsl = (newtCheckboxGetValue(checkSsl) == 'X');
#endif
    }
  else // no network
    {
      *szNetworkIP = 0;
      *dwServerPort = OPTIONS_DEFAULT_SERVERPORT;	
    }
  
  nAction = 0;
  
  if (newtRadioGetCurrent(radioRestore) == radioSave)
    {	
      nAction = OPERATION_SAVE;
    }
  else if (newtRadioGetCurrent(radioRestore) == radioRestore)
    {
      nAction = OPERATION_RESTORE;
    }
  else if (newtRadioGetCurrent(radioRestore) == radioMbr)
    {
      nAction = OPERATION_RESTMBR;
    }
  
  newtFormDestroy(formMain);
  newtPopWindow();
  
  return nAction;
}

// =======================================================
void CInterfaceNewt::askDescription(char *szDescription, DWORD dwMaxLen)
{
  newtComponent editDesc;
  newtComponent labelText;
  newtComponent form;
  newtComponent btnOk;
  
  memset(szDescription, 0, dwMaxLen);
  
  newtCenteredWindow(65, 18, i18n("Partition description"));
  
  labelText = newtLabel(1, 1, i18n("You can enter a description of the saved partition:"));
  editDesc = newtEntry(1, 3, "", 60, NULL, NEWT_ENTRY_SCROLL);
  btnOk = newtButton(27, 13, i18n("Ok"));
  
  form = newtForm(NULL, NULL, 0);
  newtFormAddComponents(form, labelText, editDesc, btnOk, NULL);
  
  newtRunForm(form);
  newtPopWindow();	
  
  strncpy(szDescription, newtEntryGetValue(editDesc), dwMaxLen);
  newtFormDestroy(form);
}

// =======================================================
int CInterfaceNewt::askText(char *szMessage, char *szTitle, char *szDestText, WORD size)
{
  newtComponent editText;
  newtComponent labelText;
  newtComponent form;
  newtComponent btnOk;
  newtComponent btnCancel;
  newtComponent temp;
  
  *szDestText = 0;
  
  while (!*szDestText)
    {
      newtCenteredWindow(65, 20, szTitle);
      
      labelText = newtTextbox(1, 1, 60, 7, 0/*NEWT_TEXTBOX_SCROLL*/);
      newtTextboxSetText(labelText, szMessage);
      
      editText = newtEntry(1, 9, "", 55, NULL, NEWT_ENTRY_SCROLL);
      btnOk = newtButton(15, 14, i18n("Ok"));
      btnCancel = newtButton(40, 14, i18n("Cancel"));
      
      form = newtForm(NULL, NULL, 0);
      newtFormAddComponents(form, labelText, editText, btnOk, btnCancel, NULL);
      
      temp = newtRunForm(form);
      newtPopWindow();	
      if (temp == btnCancel)
	{	
	  newtFormDestroy(form);
	  RETURN_int(-1);
	}
      
      strncpy(szDestText, newtEntryGetValue(editText), size-1);
      *(szDestText+size-1) = '\0';
      
      if (*szDestText == 0)
	msgBoxError(i18n("Text cannot be empty"));
      
      newtFormDestroy(form);
    }
  
  return 0;
}

// =======================================================
int CInterfaceNewt::askLogin(char *szLogin, char *szPasswd, WORD size)
{
  newtComponent editLoginText;
  newtComponent editPasswdText;
  newtComponent labelLoginText;
  newtComponent labelPasswdText;

  newtComponent form;
  newtComponent btnOk;
  newtComponent btnCancel;
  newtComponent temp;
 
  int nOk = 0; 
  
  while (!nOk)
    {
      newtCenteredWindow(65, 20, i18n("Password required"));
      
      labelLoginText = newtTextbox(1, 5, 60, 1, 0);
      newtTextboxSetText(labelLoginText, i18n("Enter your login"));
      editLoginText = newtEntry(1, 6, "", 55, NULL, NEWT_ENTRY_SCROLL);

      labelPasswdText = newtTextbox(1, 9, 60, 1, 0);
      newtTextboxSetText(labelPasswdText, i18n("Enter your password"));
      editPasswdText = newtEntry(1, 10, "", 55, NULL, NEWT_FLAG_HIDDEN);

      btnOk = newtButton(15, 14, i18n("Ok"));
      btnCancel = newtButton(40, 14, i18n("Cancel"));
      
      form = newtForm(NULL, NULL, 0);
      newtFormAddComponents(form, labelLoginText, editLoginText, 
         labelPasswdText, editPasswdText, btnOk, btnCancel, NULL);
      
      temp = newtRunForm(form);
      newtPopWindow();	
      if (temp == btnCancel)
	{	
	  newtFormDestroy(form);
	  RETURN_int(-1);
	}
      
      strncpy(szLogin, newtEntryGetValue(editLoginText), size-1);
      *(szLogin+size-1) = '\0';
      strncpy(szPasswd, newtEntryGetValue(editPasswdText), size-1);
      *(szPasswd+size-1) = '\0';
      
      if (!(*szLogin) || !(*szPasswd))
	msgBoxError(i18n("Text cannot be empty"));
      else
        nOk = 1;
      
      newtFormDestroy(form);
    }
  
  return 0;
}

// =======================================================
WORD CInterfaceNewt::askNewPath(char * szOrigFilename, DWORD dwVolume, char * szPath, char * szNewPath, WORD size)
{
	char szMess[2048];
	
	SNPRINTF(szMess, i18n("Disk full! Can't write next volume file "
	"(%s.%.3d)\nto %s\nPlease, enter another directory path "
	"(without filename):"), szOrigFilename, dwVolume, szPath);
	
	return askText(szMess, i18n("New Volume location"), szNewPath, size);
}

// ---------- File Selection Dialogue: contribution from Vadim S. (vvs1@slackware.ru) ---------
typedef struct fde
{
	char	*name;
	struct stat st;
} fde;

// to avoid localized things... maybe, not good
const char *months[]={"Jan","Feb","Mar","Apr", "May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};

static fde *dir_here(newtComponent lb, char *dr)
{
	struct dirent **namelist;
	int n, sz2, i;
	double sz1;
	char szb[20];
	char buf[128], *bf; // really, we have newt listbox->width-7 (7 is for frame with listbox)
	time_t tt, ttt;
	struct tm *tmm;
	fde *dp, *dpp;
	
	tt=time(NULL);
	bf=buf;
	
	dp=(fde*)malloc(sizeof(fde));
	dp->name=dirname(strdup(dr));
	stat(dp->name,&dp->st);
	dpp=dp;
	newtListboxAppendEntry(lb, "..", dp);
	n = scandir(dr, &namelist, 0, alphasort);
	if(n < 0)
	{
		// TODO: replace with newt iface
		perror("scandir");
	} 
	else
	{
		for(i=0; i < n; i++)
		{
			if(strcmp(namelist[i]->d_name,".") != 0 &&strcmp(namelist[i]->d_name,"..") != 0)
			{
				dp=(fde*)malloc(sizeof(fde));
				dp->name=(char*)malloc(strlen(dr)+2+strlen(namelist[i]->d_name));
				strcpy(dp->name,dr);  
				if(strlen(dr) > 1) strcat(dp->name,"/"); strcat(dp->name,namelist[i]->d_name);
				stat(dp->name,&dp->st);
				tmm=localtime(&dp->st.st_mtime);
				ttt=tt-dp->st.st_mtime;
				if(dp->st.st_size >= 1024*1024*1024) // GiB: 999.99
				{
					sz1=(double)dp->st.st_size/1024.0/1024.0/1024.0;
					sprintf(szb,i18n("%6.2f GiB"),sz1);
				}
				else if(dp->st.st_size >= 1024*1024) // MiB: 999.99
				{
					sz1=(float)dp->st.st_size/1024.0/1024.0;
					sprintf(szb,i18n("%6.2f MiB"),sz1);
				}
				else // standard
				{
					sz2=dp->st.st_size; // avoid warnings
					sprintf(szb,i18n("%7d Bytes"),sz2);
				}
				
				sprintf(buf," %.30s %.40s",namelist[i]->d_name,"                                                                  ");
				if(ttt < 60*66*30*365) // current year
				sprintf(buf+31," |%s| %s %02d %02d:%02d",szb, months[tmm->tm_mon],tmm->tm_mday,tmm->tm_hour,tmm->tm_min);
				else 
				sprintf(buf+31," |%s| %s %02d %02d",szb,months[tmm->tm_mon], tmm->tm_mday,1900+tmm->tm_year);
				
				if(S_ISDIR(dp->st.st_mode))
				{
					buf[0]='/';
					newtListboxInsertEntry(lb, buf, dp,dpp);
					dpp=dp;
				}
				else
				{
					newtListboxAppendEntry(lb, buf, dp);
				}
			}
			free(namelist[i]);
		}
		free(namelist);
	}
	
	return dp;
}

// selects the current string in the listbox
static void set_lb_cursor(newtComponent lb, char *key) 
{
	int cnt, i;
	fde *sela;
	cnt=newtListboxItemCount(lb);
	for(i=0; i < cnt; i++)
	{
		newtListboxGetEntry(lb, i, NULL, (void**)&sela);
		if(strcmp(sela->name,key) == 0)
		{
			newtListboxSetCurrent(lb , i);
			break;
		}
	}
}

// return a malloc()ed path
char *filesel(char *dr)
{
	newtComponent	lb, b1, b2, b3, l1, l2, f, f1;
	const char	*enr2, *enr3;
	char		*curv;
	char		curvv[PATH_MAX]; // stupid, but who cares...
	int		i, cnt;
	fde		*sel, *sela;
	char		cwd[PATH_MAX], cwdcd[PATH_MAX], *ret=NULL;
	struct		newtExitStruct es;
	struct		stat st;
	
	newtCenteredWindow(65, 15, i18n("Select path"));
	
	f = newtForm(NULL, NULL, 0);
	
	f1 = newtForm(NULL, NULL, 0);
	b1 = newtCompactButton(59, 14, i18n("Ok"));
	b3 = newtCompactButton(45, 14, i18n("Cancel"));
	
	l2 = newtEntry(1, 14, "", 25, &enr3, NEWT_FLAG_SCROLL);
	b2 = newtCompactButton(26, 14, i18n("Create dir"));
	
	lb = newtListbox(1, 2, 12, /*NEWT_FLAG_MULTIPLE |*/ /*NEWT_FLAG_BORDER | */
		NEWT_FLAG_RETURNEXIT |
		NEWT_FLAG_SCROLL | NEWT_FLAG_SHOWCURSOR);
	
	if(dr && strlen(dr) > 0) 
	{
		// let it show the file or dir at any case
		l1 = newtEntry(1, 0, dr, 60, &enr2, NEWT_FLAG_SCROLL | NEWT_FLAG_DISABLED);
		stat(dr,&st);
		if(S_ISDIR(st.st_mode)) 
		{
			dir_here(lb,dr);
			strcpy(cwd,dr);
		} else
		{
			dir_here(lb,dirname(dr));
			strcpy(cwd,dirname(dr));
		}
	}
	else
	{
		// if NULL, show the current dir
		l1 = newtEntry(1, 0, get_current_dir_name(), 60, &enr2, NEWT_FLAG_SCROLL | NEWT_FLAG_DISABLED);
		dir_here(lb,get_current_dir_name());
		strcpy(cwd,get_current_dir_name());
	}
	
	newtListboxSetWidth(lb,63);
	newtFormAddComponents(f, lb,l1, l2, b2, b1, b3, NULL);
	newtRefresh();
	newtFormSetTimer(f, 200);
	
	do
	{
		newtFormRun(f, &es);
		
		if (es.reason == es.NEWT_EXIT_COMPONENT && es.u.co == lb) 
		{
			sel=(fde*)newtListboxGetCurrent(lb);
			if (sel && S_ISDIR(sel->st.st_mode))
			{
				cnt=newtListboxItemCount(lb);
				// delete items there:
				for(i=0; i < cnt; i++) 
				{
					newtListboxGetEntry(lb, i, &curv, (void**)&sela);
					if(sela != sel) 
					{
						free(sela->name);
						free(sela);
					}
					else
					{
						strcpy(curvv, curv);
					}
				}
				
				newtListboxClear(lb);
				
				dir_here(lb,sel->name);
				if(strcmp(curvv,"..") == 0) set_lb_cursor(lb,cwdcd);
				strcpy(cwd,sel->name);
				strcpy(cwdcd,sel->name);
				newtEntrySet(l1, sel->name, 1);
				if(sel)
				{
						free(sel->name);
						free(sel);
				}
			} 
			if(sel && S_ISREG(sel->st.st_mode)) // is a directory
			{
				newtEntrySet(l1, sel->name, 1);
			}
		}
		else if (es.reason == es.NEWT_EXIT_COMPONENT && es.u.co == b2) 
		{
			if(strlen(enr3) > 0) 
			{
				strcpy(cwdcd,cwd);
				strcat(cwdcd,"/");
				strcat(cwdcd,enr3);
				if(mkdir(cwdcd,0755) == 0) 
				{
					newtListboxClear(lb);
					dir_here(lb,cwd);
					set_lb_cursor(lb,cwdcd);
					newtEntrySet(l2, "", 1);
				}
			}
		}
		else if (es.reason == es.NEWT_EXIT_COMPONENT && es.u.co == b1)
		{
			ret=strdup(enr2);
		}
	}
	while (es.reason != es.NEWT_EXIT_COMPONENT || es.u.co == lb || es.u.co == b2);
	
	newtFormDestroy(f);
	newtPopWindow();
	
	return ret;
}
