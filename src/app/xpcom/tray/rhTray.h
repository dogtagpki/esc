/** BEGIN COPYRIGHT BLOCK
 * This Program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; version 2 of the License.
 *
 * This Program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this Program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA.
 *
 * Copyright (C) 2005 Red Hat, Inc.
 * All rights reserved.
 * END COPYRIGHT BLOCK **/

#ifndef RH_TRAY_H 
#define RH_TRAY_H

#include "rhITray.h"
#include "nsIGenericFactory.h"
#include "nsEmbedString.h"
#include <list>
#include "nsCOMPtr.h"
#include "widget/nsIBaseWindow.h"
#include "widget/nsIWidget.h"
#include <time.h>

extern "C" {
//Utility function to get Time Stamp
    char *GetTStamp(char *aTime,int aSize)
    {
        if(!aTime)
            return NULL;
        int maxSize = 55;
        if(aSize < maxSize)
            return NULL;
        char *tFormat = "[%c]";
        time_t tm = time(NULL);
        struct tm *ptr = localtime(&tm);
        strftime(aTime ,maxSize ,tFormat,ptr);
        return aTime;
    }
}

// Event Defines
#define MENU_EVT        1
#define APP_EVT         2
#define WINDOW_EVT      3

#define SHOW_ALL_WINDOWS  4
#define HIDE_ALL_WINDOWS  5

#define MENU_SHOW         6
#define APP_SHOW          7



#ifdef XP_WIN32
#define NS_REINTERPRET_CAST(__type, __expr)           reinterpret_cast< __type >(__expr)
#define _WIN32_IE  0x0500
#include <windows.h>
#include <shellapi.h>
#include "resource.h"
#include "strsafe.h"

#endif

#ifdef XP_MACOSX

#include <MacWindows.h>
#include <Processes.h>

#endif
#ifdef LINUX

#include <gtk/gtk.h>

extern "C" {
#include "notifytray.h"
}

#define COOLKEY_ICON "components/icon.png"
#endif

#include <map>
#include <string>
using namespace std;
// generate unique ID here with uuidgen

#define RH_TRAY_CID \
{ 0xd7e1bb10, 0x4cf6, 0x4b97, \
{ 0x97, 0x22, 0x9c, 0x18, 0x1f, 0x2c, 0xc3, 0x76}}


#ifdef XP_WIN32
class rhTrayWindowListener
{


public:
    rhTrayWindowListener(HWND aWnd);



    ~rhTrayWindowListener();

    HRESULT Initialize();
    HRESULT Cleanup();
    void ShowWindow();
    void HideWindow();

    

protected:

    static LRESULT CALLBACK WindowProc(
        HWND hwnd,
        UINT aMsg,
        WPARAM wParam,
        LPARAM lParam
    );

    WNDPROC origWindowProc;



private:

    

    HWND mWnd;

};


class rhTray : public rhITray
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_RHITRAY

  rhTray();

private:
  ~rhTray();
protected:
  /* additional members */

     static int mInitialized;


     static HWND mWnd;
     static ATOM mWndClass;
     static  NOTIFYICONDATA mIconData;


  // Collection of nsIBaseWindows

     static map< nsIBaseWindow *, rhTrayWindowListener *> mWindowMap;

     static map<unsigned int,string> mMenuItemStringMap;


     static HRESULT ShowPopupMenu (WORD PopupMenuResource);
     HRESULT AddListener(nsIBaseWindow *aBaseWindow);
     HRESULT RemoveListener(nsIBaseWindow *aBaseWindow);

     static void ShowAllListeners();
     static void HideAllListeners();
     static HRESULT RemoveAllListeners();

  //WindowProc for the tray's dummy window

     static LRESULT CALLBACK WindowProc(
     HWND hwnd,
     UINT aMsg,
     WPARAM wParam,
     LPARAM lParam
   );

   HRESULT SendBalloonTooltipMessage(char *aMessage); 

   HRESULT Initialize();
   HRESULT Cleanup();
   HRESULT CreateEventWindow();
   HRESULT DestroyEventWindow(); 

   HRESULT RemoveIcon();

    // rhTrayWindNotify content

   static std::list< nsCOMPtr<rhITrayWindNotify> > gTrayWindNotifyListeners;


   rhITrayWindNotify* GetTrayWindNotifyListener(rhITrayWindNotify *listener);

   int GetTrayWindNotifyListSize();
   void AddTrayWindNotifyListener(rhITrayWindNotify *listener);
   void RemoveTrayWindNotifyListener(rhITrayWindNotify *listener);
   void ClearTrayWindNotifyList();

   static void NotifyTrayWindListeners(PRUint32 aEvent, PRUint32 aEventData =0,PRUint32 aKeyData=0, PRUint32 aData1=0, PRUint32 aData2=0);
};

#endif


//Linux header info for component
#ifdef LINUX

#define HRESULT int
#define S_OK 1
#define E_FAIL 0

class rhTrayWindowListener
{

public:
    rhTrayWindowListener(GtkWidget *aWnd);

    ~rhTrayWindowListener();

    HRESULT Initialize();
    void ShowWindow();
    void HideWindow();

    //Callbacks

    static gboolean WndDeleteCBProc( GtkWidget *widget,
                              GdkEvent  *event,
                              gpointer   data );

    static void WndDestroyCBProc( GtkWidget *widget,
                     gpointer   data );
protected:


private:

    GtkWidget *mWnd;

};

class rhTray : public rhITray
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_RHITRAY

  rhTray();

private:
  ~rhTray();
protected:
  /* additional members */

     static int mInitialized;

     static GtkWidget *mWnd;

  // Icon menu related

    static GtkWidget *mIconMenu;
    static GtkWidget *mIconBoxWidget;
    static void IconMenuCBProc(GtkWidget *widget, gpointer data);
    HRESULT CreateIconMenu();     

  // Collection of nsIBaseWindows

     static map< nsIBaseWindow *, rhTrayWindowListener *> mWindowMap;

     static HRESULT ShowPopupMenu ();
     HRESULT AddListener(nsIBaseWindow *aBaseWindow);
     HRESULT RemoveListener(nsIBaseWindow *aBaseWindow);

     static void ShowAllListeners();
     static void HideAllListeners();
     static HRESULT RemoveAllListeners();

    // Callbacks
    static void IconCBProc(GtkWidget *button, GdkEventButton *event, void *data);

    HRESULT Initialize();
    HRESULT Cleanup();
    HRESULT CreateEventWindow();
    HRESULT DestroyEventWindow();

    HRESULT RemoveIcon();

    //Gtk print handler

    static void TrayPrintHandler(const gchar *string);


     // rhTrayWindNotify content

   static std::list< nsCOMPtr<rhITrayWindNotify> > gTrayWindNotifyListeners;


   rhITrayWindNotify* GetTrayWindNotifyListener(rhITrayWindNotify *listener);

   int GetTrayWindNotifyListSize();
   void AddTrayWindNotifyListener(rhITrayWindNotify *listener);
   void RemoveTrayWindNotifyListener(rhITrayWindNotify *listener);
   void ClearTrayWindNotifyList();

   static void NotifyTrayWindListeners(PRUint32 aEvent, PRUint32 aEventData =0,PRUint32 aKeyData=0, PRUint32 aData1=0, PRUint32 aData2=0);
};

#endif
#ifdef XP_MACOSX 

#define HRESULT int
#define S_OK 1
#define E_FAIL 0

class rhTrayWindowListener
{

public:
    rhTrayWindowListener(WindowRef aWnd);

    ~rhTrayWindowListener();

    HRESULT Initialize();
    void ShowWindow();
    void HideWindow();
    HRESULT Cleanup();

    //Callbacks

protected:

    static pascal OSStatus WindowProc(EventHandlerCallRef nextHandler, EventRef aEvent, void *userData);

    EventHandlerRef mEventHandlerRef;
    EventHandlerUPP mEventHandlerUPP;
    
    WindowRef mWnd; 
private:


};

class rhTray : public rhITray
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_RHITRAY

  rhTray();

private:
  ~rhTray();
protected:
  /* additional members */

     static int mInitialized;

     static WindowRef mWnd;
     static ProcessSerialNumber mPSN;

     static EventHandlerRef mEventHandlerRef;
     static EventHandlerUPP mEventHandlerUPP;
     static MenuRef mDockMenu;
     static MenuRef mRootMenu;

  // Icon menu related

    HRESULT CreateApplicationListener();

  // Collection of nsIBaseWindows

     static map< nsIBaseWindow *, rhTrayWindowListener *> mWindowMap;

public:
     static void ShowApp();
     static void HideApp();

protected:
     static HRESULT ShowPopupMenu ();
     HRESULT AddListener(nsIBaseWindow *aBaseWindow);
     HRESULT RemoveListener(nsIBaseWindow *aBaseWindow);

     static void ShowAllListeners();
     static void HideAllListeners();
     static HRESULT RemoveAllListeners();

    // Callbacks

    static pascal OSStatus ApplicationProc(EventHandlerCallRef nextHandler, EventRef aEvent, void *userData);
   // static void IconCBProc(GtkWidget *button, GdkEventButton *event, void *data);

    HRESULT Initialize();
    HRESULT Cleanup();
    HRESULT CreateEventWindow();
    HRESULT DestroyEventWindow();

    HRESULT RemoveIcon();


     // rhTrayWindNotify content

   static std::list< nsCOMPtr<rhITrayWindNotify> > gTrayWindNotifyListeners;


   rhITrayWindNotify* GetTrayWindNotifyListener(rhITrayWindNotify *listener);

   int GetTrayWindNotifyListSize();
   void AddTrayWindNotifyListener(rhITrayWindNotify *listener);
   void RemoveTrayWindNotifyListener(rhITrayWindNotify *listener);
   void ClearTrayWindNotifyList();

   static void NotifyTrayWindListeners(PRUint32 aEvent, PRUint32 aEventData =0,PRUint32 aKeyData=0, PRUint32 aData1=0, PRUint32 aData2=0);

};

#endif

#endif




