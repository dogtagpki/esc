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

#define FORCE_PR_LOG 1

#include "rhTray.h"
#include "nsIGenericFactory.h"
#include <prlog.h>
#include <time.h>

NS_IMPL_ISUPPORTS1(rhTray, rhITray)

#include "WinUser.h"

HWND rhTray::mWnd = 0;
int rhTray::mInitialized = 0;
ATOM rhTray::mWndClass = 0;

std::list< nsCOMPtr<rhITrayWindNotify> > rhTray::gTrayWindNotifyListeners;

map< nsIBaseWindow *, rhTrayWindowListener *> rhTray::mWindowMap;
map<unsigned int,string> rhTray::mMenuItemStringMap;

NOTIFYICONDATA rhTray::mIconData;


const TCHAR* LISTENER_INSTANCE = 
  TEXT("_RH_TRAY_WIND_LISTENER_INST");


const TCHAR* LISTENER_CB =
  TEXT("_RH_TRAY_WIND_LISTINER_CB");


static PRLogModuleInfo *trayLog = PR_NewLogModule("tray");

rhTray::rhTray() 
{
  /* member initializers and constructor code */
}

rhTray::~rhTray()
{
    char tBuff[56];
    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::~rhTray\n",GetTStamp(tBuff,56)));


    Cleanup();
  /* destructor code */
}

NS_IMETHODIMP rhTray::Setwindnotifycallback(rhITrayWindNotify *jsNotify)
{
    char tBuff[56];
    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::Setwindnotifycallback\n",GetTStamp(tBuff,56)));

    if(jsNotify)
        AddTrayWindNotifyListener(jsNotify);

    return NS_OK;

}
 /* void unsetwindnotifycallback (in rhITrayWindNotify jsNotify); */

NS_IMETHODIMP rhTray::Unsetwindnotifycallback(rhITrayWindNotify *jsNotify)
{
    char tBuff[56];
    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::Unsetwindnotifycallback\n",GetTStamp(tBuff,56)));

    if(jsNotify)
        RemoveTrayWindNotifyListener(jsNotify);

    return NS_OK;
}

/* void add (); */
NS_IMETHODIMP rhTray::Add(nsIBaseWindow *aWindow)
{
    char tBuff[56];
    NS_ENSURE_ARG(aWindow);

    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::Add %p \n",GetTStamp(tBuff,56),aWindow));
    HRESULT res = Initialize();
    if(res != S_OK)
    {
        return NS_ERROR_FAILURE;
    } 
    
    res = AddListener(aWindow); 

    if(res != S_OK)
        return NS_ERROR_FAILURE;

    return NS_OK;
}

/* void remove (); */
NS_IMETHODIMP rhTray::Remove(nsIBaseWindow *aWindow)
{
    char tBuff[56];
    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::Remove window %p \n",GetTStamp(tBuff,56),aWindow));

    return NS_OK;
}

NS_IMETHODIMP rhTray::Hide(nsIBaseWindow *aWindow)
{

    rhTrayWindowListener *listener = rhTray::mWindowMap[aWindow];

    if(listener)
    {
        listener->HideWindow();

    }

    return NS_OK;

}

NS_IMETHODIMP rhTray::Show(nsIBaseWindow *aWindow)
{

    rhTrayWindowListener *listener = rhTray::mWindowMap[aWindow];

    if(listener)
    {
        listener->ShowWindow();

    }

    return NS_OK;
 
}

NS_IMETHODIMP rhTray::IsInitializedAlready(PRBool *_retval)
{

    *_retval = 0;

    if(rhTray::mInitialized > 1)
        *_retval = 1;

    rhTray::mInitialized ++;


    return NS_OK;

}

/* void hideall (); */
NS_IMETHODIMP rhTray::Hideall() 
{
    HideAllListeners();
    return NS_OK;
}

/* void showall (); */
NS_IMETHODIMP rhTray::Showall()
{
    ShowAllListeners(); 
    return NS_OK;
}

NS_IMETHODIMP rhTray::Sendnotification(const char *aTitle,const char *aMessage,PRUint32 aSeverity,PRUint32 aTimeout, const char *aIcon)
{

    if(aMessage)
    {
        SendBalloonTooltipMessage((char *)aMessage);

    }

    return NS_OK;

}

/* void settooltipmsg (in string aMessage); */
NS_IMETHODIMP rhTray::Settooltipmsg(const char *aMessage)
{

    char tBuff[56];
    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::Settooltipmsg %s  \n",GetTStamp(tBuff,56),aMessage));

    if(!aMessage)
       return E_FAIL;

    if(strlen(aMessage) >= 64)
     return E_FAIL;

    strcpy(rhTray::mIconData.szTip, aMessage);

   return NS_OK;
}

/* void seticonimage (in string aIcon); */
NS_IMETHODIMP rhTray::Seticonimage(const char *aIcon)
{
    return NS_OK;
}

/* void hideicon (); */
NS_IMETHODIMP rhTray::Hideicon(void)
{
    return NS_OK;
}

/* void showicon (); */
NS_IMETHODIMP rhTray::Showicon(void)
{
    return NS_OK;
}



HRESULT rhTray::SendBalloonTooltipMessage(char *aMessage)
{

    if(!aMessage)
        return E_FAIL;

    if(strlen(aMessage) >= 256)
       return E_FAIL;

    strcpy(rhTray::mIconData.szInfo, aMessage);


    ::Shell_NotifyIcon(NIM_MODIFY,&rhTray::mIconData);

    return S_OK;

}

HRESULT rhTray::Initialize()
{
    char tBuff[56]; 
    if(mInitialized)
    {
        return S_OK;
    }

    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::Initialize  \n",GetTStamp(tBuff,56)));

    CreateMutex(NULL, FALSE, "ESCMutex"); 

    HRESULT res = CreateEventWindow();
    

    if(res != S_OK)
    {
       return E_FAIL;
    }

    HINSTANCE gInstance =   ::GetModuleHandle("rhTray.dll");

    HICON icon = (HICON)::LoadImage(gInstance, // small class icon
                          "components\\esc.ico",
                        IMAGE_ICON,
                       GetSystemMetrics(SM_CXSMICON),
                       GetSystemMetrics(SM_CYSMICON),
                       LR_LOADFROMFILE);


    if(!icon)
    {
        icon = ::LoadIcon(NULL,IDI_WINLOGO);
    }

    rhTray::mIconData.cbSize = sizeof(NOTIFYICONDATA);
    rhTray::mIconData.hWnd = mWnd;
    rhTray::mIconData.uID= 1;
    rhTray::mIconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_INFO ;
    rhTray::mIconData.uCallbackMessage = WM_USER;
    rhTray::mIconData.hIcon = icon;
    rhTray::mIconData.dwInfoFlags = NIIF_INFO;


    rhTray::mIconData.uTimeout = 1000;

    PR_LOG(trayLog,PR_LOG_DEBUG,("%s rhTray::Initialize tray icon handle %d \n",GetTStamp(tBuff,56),icon));


    ::Shell_NotifyIcon( NIM_ADD, &rhTray::mIconData);
    mInitialized = 1;

    return S_OK;
}

HRESULT rhTray::RemoveIcon()
{
    char tBuff[56];
    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::RemoveIcon. \n",GetTStamp(tBuff,56)));

    ::Shell_NotifyIcon(NIM_DELETE,&rhTray::mIconData);

    return S_OK;
}

HRESULT rhTray::Cleanup()
{
    char tBuff[56];
    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::Cleanup.\n",GetTStamp(tBuff,56)));

    RemoveAllListeners();
    DestroyEventWindow();
    RemoveIcon();

    rhTray::mMenuItemStringMap.clear();

    return S_OK;
}

void rhTray::ShowAllListeners()
{

    char tBuff[56];
    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::ShowAllListeners.\n",GetTStamp(tBuff,56)));
    map< nsIBaseWindow *, rhTrayWindowListener *>::iterator i;

    rhTrayWindowListener *cur = NULL;

    for(i = rhTray::mWindowMap.begin(); i!= rhTray::mWindowMap.end(); i++)
    {

        rhTrayWindowListener * cur = (*i).second;

        if(cur)
        {
            cur->ShowWindow();

        }

    }


}

void rhTray::HideAllListeners()
{
    char tBuff[56]; 
    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::HideAllListeners.\n",GetTStamp(tBuff,56)));

    map< nsIBaseWindow *, rhTrayWindowListener *>::iterator i;

    rhTrayWindowListener *cur = NULL;

    for(i = rhTray::mWindowMap.begin(); i!= rhTray::mWindowMap.end(); i++)
    {
        rhTrayWindowListener * cur = (*i).second;

        if(cur)
        {
            cur->HideWindow();
        }

    }

}

HRESULT rhTray::DestroyEventWindow()
{

    char tBuff[56];
    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::DestroyEventWindow \n",GetTStamp(tBuff,56)));

    ::DestroyWindow(rhTray::mWnd);
    rhTray::mWnd = 0;

    return S_OK;
}

HRESULT rhTray::CreateEventWindow()
{

    char tBuff[56];
    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::CreateEventWindow \n",GetTStamp(tBuff,56)));
    ::SetLastError(0);
    HINSTANCE hInst = ::GetModuleHandle(NULL);

    if(!hInst)
    {
        return E_FAIL;
    }

    if (!mWndClass) {
        WNDCLASS classDef;
        classDef.style          = CS_NOCLOSE | CS_GLOBALCLASS;
        classDef.lpfnWndProc    = WindowProc;
        classDef.cbClsExtra     = 0;
        classDef.cbWndExtra     = 0;
        classDef.hInstance      = hInst;
        classDef.hIcon          = NULL;
        classDef.hCursor        = NULL;
        classDef.hbrBackground  = NULL;
        classDef.lpszMenuName   = NULL;
        classDef.lpszClassName  = TEXT("Enterprise Security");

        mWndClass = ::RegisterClass(&classDef);

        if(!mWndClass)
        {
            return E_FAIL;
        }
     }

     mWnd =
        ::CreateWindow(
        (LPCTSTR)mWndClass,                
        TEXT("Enterprise Security"), 
        WS_MINIMIZE,                          
        CW_USEDEFAULT ,                       
        CW_USEDEFAULT ,                       
        CW_USEDEFAULT,                        
        CW_USEDEFAULT,                        
        ::GetDesktopWindow(),                 
        NULL,                                 
        hInst,                                
        NULL);                                

     if (!mWnd) {
         if (::UnregisterClass((LPCTSTR)mWndClass, hInst))
             mWndClass = NULL;
      }


    return S_OK;
}

LRESULT CALLBACK
rhTray::WindowProc(
    HWND hwnd,
    UINT aMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{

    char tBuff[56];

    switch(aMsg) {
    case WM_USER:

       switch(lParam)
       {
           case WM_LBUTTONDBLCLK:
           case WM_LBUTTONDOWN:

               PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::WindowProc: WM_LBUTTONDBLCLK  \n",GetTStamp(tBuff,56)));
               NotifyTrayWindListeners(MENU_EVT,MENU_SHOW);
               ShowAllListeners();

           break;
           case WM_RBUTTONDOWN:

                PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::WindowProc: WM_RBUTTONDOWN \n",GetTStamp(tBuff,56)));

                HRESULT res =  rhTray::ShowPopupMenu (IDR_MENU1);


                switch(res)
                {

                    case ID_SHOW:

                        NotifyTrayWindListeners(MENU_EVT,MENU_SHOW);

                        ShowAllListeners();
                    break;
                    
                    case IDM_EXIT:

                        PostQuitMessage(0);
                    break;
                };
           break;

       };

      break;
    case WM_CREATE:

           PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::WindowProc: WM_CREATE  \n",GetTStamp(tBuff,56)));
       break;
    default:
       break;
  };

  return ::CallWindowProc(
    DefWindowProc,
    hwnd,
    aMsg,
    wParam,
    lParam);
}

HRESULT rhTray::AddListener(nsIBaseWindow *aWindow)
{

    char tBuff[56];
    nsresult rv;


    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::AddListener %p \n",GetTStamp(tBuff,56),aWindow));
    NS_ENSURE_ARG(aWindow);

    nativeWindow aNativeWindow;
    rv = aWindow->GetParentNativeWindow( &aNativeWindow );


    if(NS_FAILED(rv))
    {
        return E_FAIL;
    }

    HWND hWnd = NS_REINTERPRET_CAST(HWND,aNativeWindow);

    if(!hWnd)
    {
        return E_FAIL;
    }    

    //Now see if it's alreay in the map


    rhTrayWindowListener *already = rhTray::mWindowMap[aWindow];

    if(already)
    {
        PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::AddWindowListener Window already registered  %p \n",GetTStamp(tBuff,56),aWindow));
        return S_OK;

    }

    rhTrayWindowListener *create = new rhTrayWindowListener(hWnd);

    if(!create)
    {
        return E_FAIL;
    }

    mWindowMap[aWindow] = create;
    ::SetProp(hWnd,LISTENER_INSTANCE,(HANDLE) create);

   

    HRESULT res = create->Initialize();

    if(res != S_OK)
       return E_FAIL;
  
    return S_OK; 
}

HRESULT rhTray::ShowPopupMenu (WORD PopupMenuResource)
{
    char tBuff[56];
    HMENU hMenu, hPopup = 0;

    const int numMenuItems = 2;

    hMenu = ::LoadMenu (::GetModuleHandle("rhTray.dll"),
                      MAKEINTRESOURCE (PopupMenuResource));

    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::ShowPopupMenu hMenu %d  error %d\n",GetTStamp(tBuff,56),hMenu,GetLastError()));

    if (hMenu != 0) {
        POINT pt;
        ::GetCursorPos (&pt);

        hPopup = ::GetSubMenu (hMenu, 0);

        int numItems = rhTray::mMenuItemStringMap.size();
    
        PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::ShowPopupMenu num menu item strings : %d\n",GetTStamp(tBuff,56),numItems));
    // Change the menu items text if possible

        MENUITEMINFO mii = {0};
 
        unsigned int menuItemID = 0;

        int i = 0;

        char buffer[256];
 
        if(numItems == numMenuItems )
        {     
            for (i = 0 ;i < numMenuItems; i++)
            {
                mii.cbSize = sizeof(MENUITEMINFO);
                mii.fMask = MIIM_TYPE ;

                char * itemText = (char *) (rhTray::mMenuItemStringMap[i]).c_str();
             
                PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::ShowPopupMenu menutext: %d text %s \n",GetTStamp(tBuff,56),i,itemText));
               if(itemText)
               {

                   if(i == 0)
                   {
                        menuItemID= ID_SHOW;                          
                   }
                   else
                   {
                        menuItemID= IDM_EXIT;

                   }

                   if( GetMenuItemInfo(hPopup,menuItemID,FALSE,&mii))
                   {
                       buffer[0] = 0;

                       if(strlen(itemText) < 256)
                       {
                           strcpy(buffer,itemText);
                       }

                       mii.cch=strlen(buffer);
                       mii.fType = MFT_STRING;
                       mii.dwTypeData= buffer;
                       SetMenuItemInfo(hPopup,menuItemID,FALSE,&mii);

                   }
                   else
                   {
                       PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::ShowPopupMenu Can't GetMenuItemInfo \n",GetTStamp(tBuff,56))); 
                   }
               }

          }

      }

      ::SetForegroundWindow (rhTray::mWnd);

      WORD cmd = ::TrackPopupMenu (hPopup, TPM_RIGHTBUTTON | TPM_RETURNCMD,
                                 pt.x, pt.y, 0, mWnd, NULL);

      ::PostMessage (mWnd, WM_NULL, 0, 0);

      ::DestroyMenu (hMenu);
      return cmd;
    }
    return 0;
}

HRESULT rhTray::RemoveListener(nsIBaseWindow *aBaseWindow)
{
    char tBuff[56];
    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::RemoveWindowListener %p \n",GetTStamp(tBuff,56),aBaseWindow));

    if(!aBaseWindow)
        return S_OK;

    map< nsIBaseWindow *, rhTrayWindowListener *>::iterator i;

    i = rhTray::mWindowMap.find(aBaseWindow);

    if(i != rhTray::mWindowMap.end())
    {
        rhTrayWindowListener *cur = (*i).second;

        if(cur)
        {
            delete cur;
        }
        
        rhTray::mWindowMap.erase(i);
        
    }

    return S_OK;
}

HRESULT rhTray::RemoveAllListeners()
{
    char tBuff[56];
    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::RemoveAllListenesr\n",GetTStamp(tBuff,56)));
    map< nsIBaseWindow *, rhTrayWindowListener *>::iterator i;

    rhTrayWindowListener *cur = NULL;

    for(i = rhTray::mWindowMap.begin(); i!= rhTray::mWindowMap.end(); i++)
    {
        cur = (*i).second;

        if(cur)
        {
            PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::RemoveAllListeners deleting %p\n",GetTStamp(tBuff,56),cur));

            delete cur;

        }

    }

    rhTray::mWindowMap.clear();
    
    return S_OK;
}

/* void setmenuitemtext (in unsigned long aIndex, in string aText); */
NS_IMETHODIMP rhTray::Setmenuitemtext(PRUint32 aIndex, const char *aText)
{
    char tBuff[56]; 
    PR_LOG( trayLog, PR_LOG_DEBUG, ("%sa rhTray::Setmenuitemtext index: %d text: %s\n",GetTStamp(tBuff,56),aIndex, aText));

    if(aIndex >= 0 && aIndex <= 10 && aText)
    {
        rhTray::mMenuItemStringMap[aIndex] = aText;

    }

    return S_OK;
}


//rhTrayWindNotify methods

rhITrayWindNotify* rhTray::GetTrayWindNotifyListener(rhITrayWindNotify *listener)
{
    char tBuff[56];
    std::list<nsCOMPtr<rhITrayWindNotify> >::const_iterator it;

    for(it=gTrayWindNotifyListeners.begin(); it!=gTrayWindNotifyListeners.end(); ++it) {

    if((*it) == listener)
    {
        return (*it);
    }
}

    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhCoolKey::GetNotifyKeyListener:  looking for %p returning NULL. \n",GetTStamp(tBuff,56),listener));

    return nsnull;
}

int rhTray::GetTrayWindNotifyListSize()
{
    return gTrayWindNotifyListeners.size();
}
void rhTray::AddTrayWindNotifyListener(rhITrayWindNotify *listener)
{
    char tBuff[56];
    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::AddTrayWindNotifyListener: %p \n", GetTStamp(tBuff,56),listener));

    if(GetTrayWindNotifyListener(listener ))
    {

        PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::AddTrayWindNotifyListener: %p listener already in list. \n",GetTStamp(tBuff,56),listener));
        return ;

    }

    gTrayWindNotifyListeners.push_back(listener);

}

void rhTray::RemoveTrayWindNotifyListener(rhITrayWindNotify *listener)
{
    if(!GetTrayWindNotifyListener(listener))
    {
        return;
    }

    gTrayWindNotifyListeners.remove(listener);


    listener = NULL;
}

void rhTray::ClearTrayWindNotifyList()
{

     while (gTrayWindNotifyListeners.size() > 0) {
         rhITrayWindNotify * node = (gTrayWindNotifyListeners.front()).get();

         node = NULL;

         gTrayWindNotifyListeners.pop_front();
     }

}

void rhTray::NotifyTrayWindListeners(PRUint32 aEvent, PRUint32 aEventData,PRUint32 aKeyData,PRUint32 aData1, PRUint32 aData2)
{

      //Now notify all the listeners of the event

    std::list< nsCOMPtr <rhITrayWindNotify> >::const_iterator it;
    for(it=gTrayWindNotifyListeners.begin(); it!=gTrayWindNotifyListeners.end(); ++it) {

        PRBool claimed = 0;

        ((rhITrayWindNotify *) (*it))->RhTrayWindEventNotify(aEvent,aEventData, aKeyData, aData1, aData2, &claimed);

    }

}


//rhTrayWindowListener Methods

rhTrayWindowListener::rhTrayWindowListener(HWND aWnd)
{ 
    mWnd = aWnd;
    origWindowProc = NULL;

}

rhTrayWindowListener::~rhTrayWindowListener()
{
    char tBuff[56];
    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTrayWindowListener::~rhTrayWindowListener.\n",GetTStamp(tBuff,56)));

    Cleanup();

}

LRESULT CALLBACK
rhTrayWindowListener::WindowProc(
    HWND hwnd,
    UINT aMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    char tBuff[56];
    WNDPROC wndProc = (WNDPROC) NULL; 

    int eventClaimed = 0;

    int show = 0;

    static int firstTime = 1;

    rhTrayWindowListener *me = (rhTrayWindowListener *) ::GetProp(hwnd,LISTENER_INSTANCE );

    if(!me || !me->origWindowProc)
    {
        wndProc = DefWindowProc;
    }
    else
    {
        wndProc = me->origWindowProc;
    }

    switch(aMsg) {
        case WM_NCLBUTTONDOWN:
            switch(wParam)
            {
                case HTMINBUTTON:

                    if(me)
                    {
                        //me->HideWindow();
                    }

                    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTrayWindowListener Minimize\n",GetTStamp(tBuff,56)));
                     //eventClaimed = 1;
                break;

                case HTMAXBUTTON:
                    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTrayWindowListener:: Maximize  \n",GetTStamp(tBuff,56)));
                    //eventClaimed = 1;
                break;
                 
                case HTCLOSE:

                    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTrayWindowListener Close! \n",GetTStamp(tBuff,56)));

                    if(me)
                    {
                        //me->HideWindow();
                    }

                    //eventClaimed = 1;

                break;

            };
 
        break;

        case WM_ACTIVATE:

             switch (LOWORD(wParam)) {
                 case WA_ACTIVE:
                 case WA_CLICKACTIVE:

                     PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTrayWindowListener ACTIVATE! \n",GetTStamp(tBuff,56)));

                     
                 break;
                 default:
                 break;
             };

        break;

        case WM_SHOWWINDOW:

        char buff[500];
        sprintf(buff,"WM_SHOW wParam %d lParam %d",wParam,lParam);

        if(firstTime)
        {
            firstTime = 1;
          
            eventClaimed = 1;

        }


        PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTrayWindowListener WM_SHOWWINDOW wParam %d lParam %d! \n",GetTStamp(tBuff,56),wParam, lParam));

        show = (int) wParam;

        if(lParam == 0)
        {    
            PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTrayWindowListener  WM_SHOW called from ShowWindow or HideWindow \n",GetTStamp(tBuff,56)));
        }

        break;

        case WM_SYSCOMMAND:
            switch(wParam)
            {
                case SC_CLOSE:
                    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTrayWindowListener:: WM_SYSCOMMAND SC_CLOSE  \n",GetTStamp(tBuff,56)));
                break;

                default:
                break;
            };

        break;
     
        case WM_SIZE:
            switch(wParam)
            {
                case SIZE_MINIMIZED:

                    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTrayWindowListener::  WM_SIZE SIZE_MINIMIZE \n",GetTStamp(tBuff,56)));

                break;
                default:
                break;
            };

        break;
 
        default:
        break;

    };

    if(eventClaimed)
    {
        PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTrayWindowListener:: Event claimed \n",GetTStamp(tBuff,56)));
        return FALSE;
    }

    return ::CallWindowProc(
        wndProc,
        hwnd,
        aMsg,
        wParam,
        lParam);
}

HRESULT rhTrayWindowListener::Cleanup()
{

    char tBuff[56];
    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTrayWindowListener::Cleanup. \n",GetTStamp(tBuff,56)));

    return S_OK;

}

HRESULT rhTrayWindowListener::Initialize()
{
    if(mWnd)
    {
      
      origWindowProc = (WNDPROC) ::GetWindowLongPtr(mWnd,GWLP_WNDPROC);
      ::SetWindowLongPtr(mWnd,GWLP_WNDPROC,(LONG_PTR) rhTrayWindowListener::WindowProc);

    }

    return S_OK;
}


void rhTrayWindowListener::ShowWindow()
{
    char tBuff[56];
    if(mWnd)
    {
         PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTrayWindowListener:: ShowWindow \n",GetTStamp(tBuff,56)));
         //::ShowWindow(mWnd,SW_SHOW);

         //::ShowWindow(mWnd,SW_RESTORE);

    }

}

void rhTrayWindowListener::HideWindow()
{
    char tBuff[56];
    if(mWnd)
    {

        PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTrayWindowListener:: Hide Window \n",GetTStamp(tBuff,56)));

        ::ShowWindow(mWnd,SW_HIDE);

        //Change style to nix the taskbar button

        ::SetWindowLongPtr(mWnd,GWL_EXSTYLE,GetWindowLongPtr(mWnd,GWL_STYLE) | WS_EX_TOOLWINDOW);

         PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTrayWindowListener:: Hide Window , try to hide owner window too. \n",GetTStamp(tBuff,56)));
        ::ShowWindow(mWnd,SW_HIDE);

    }
}

NS_GENERIC_FACTORY_CONSTRUCTOR(rhTray);


//rhTray Module Implementation

static const nsModuleComponentInfo components[] =
{
  { "rhTray",
    RH_TRAY_CID,
    "@redhat.com/rhTray",
    rhTrayConstructor
  }
};

NS_IMPL_NSGETMODULE(rhTrayModule, components)
