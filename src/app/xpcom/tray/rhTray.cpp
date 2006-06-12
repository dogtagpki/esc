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

#include "rhTray.h"
#include "nsIGenericFactory.h"
#include <prlog.h>

NS_IMPL_ISUPPORTS1(rhTray, rhITray)

#include "Winuser.h"

HWND rhTray::mWnd = 0;
int rhTray::mInitialized = 0;
ATOM rhTray::mWndClass = 0;



map< nsIBaseWindow *, rhTrayWindowListener *> rhTray::mWindowMap;
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

    PR_LOG( trayLog, 5, ("rhTray::~rhTray\n"));


    Cleanup();
  /* destructor code */
}

/* void add (); */
NS_IMETHODIMP rhTray::Add(nsIBaseWindow *aWindow)
{
    

    NS_ENSURE_ARG(aWindow);

    PR_LOG( trayLog, 5, ("rhTray::Add %p \n",aWindow));
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
     PR_LOG( trayLog, 5, ("rhTray::Remove window %p \n",aWindow));


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

NS_IMETHODIMP rhTray::Sendnotification(const char *aMessage)
{

    if(aMessage)
    {
        SendBalloonTooltipMessage((char *)aMessage);

    }

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

    if(mInitialized)
    {
        return S_OK;
    }

    PR_LOG( trayLog, 5, ("rhTray::Initialize  \n"));

    //MessageBox(NULL,"Inside rhTray::Initialize!",NULL,NULL);

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

    PR_LOG(trayLog,5,("rhTray::Initialize tray icon handle %d \n",icon));


    ::Shell_NotifyIcon( NIM_ADD, &rhTray::mIconData);
    mInitialized = 1;

    return S_OK;
}

HRESULT rhTray::RemoveIcon()
{
    PR_LOG( trayLog, 5, ("rhTray::RemoveIcon. \n"));

    ::Shell_NotifyIcon(NIM_DELETE,&rhTray::mIconData);


    return S_OK;
}

HRESULT rhTray::Cleanup()
{
    PR_LOG( trayLog, 5, ("rhTray::Cleanup.\n"));

    RemoveAllListeners();
    DestroyEventWindow();
    RemoveIcon();

    return S_OK;
}

void rhTray::ShowAllListeners()
{

    PR_LOG( trayLog, 5, ("rhTray::ShowAllListeners.\n"));
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
    PR_LOG( trayLog, 5, ("rhTray::HideAllListeners.\n"));

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

    PR_LOG( trayLog, 5, ("rhTray::DestroyEventWindow \n"));

    ::DestroyWindow(rhTray::mWnd);
    rhTray::mWnd = 0;

    return S_OK;
}

HRESULT rhTray::CreateEventWindow()
{

    PR_LOG( trayLog, 5, ("rhTray::CreateEventWindow \n"));
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


  switch(aMsg) {
    case WM_USER:

       switch(lParam)
       {
           case WM_LBUTTONDBLCLK:

               PR_LOG( trayLog, 5, ("rhTray::WindowProc: WM_LBUTTONDBLCLK  \n"));
               ShowAllListeners();

           break;
           case WM_RBUTTONDOWN:

                PR_LOG( trayLog, 5, ("rhTray::WindowProc: WM_RBUTTONDOWN \n"));

                HRESULT res = rhTray::ShowPopupMenu (IDR_MENU1);


                switch(res)
                {

                    case ID_SHOW:

                        ShowAllListeners();
                    break;
                    
                    case ID_HIDE:

                        HideAllListeners();
                    break;
                 
                    case IDM_EXIT:

                        PostQuitMessage(0);
                    break;
                };
           break;

       };

      break;
    case WM_CREATE:

           PR_LOG( trayLog, 5, ("rhTray::WindowProc: WM_CREATE  \n"));
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

    nsresult rv;


    PR_LOG( trayLog, 5, ("rhTray::AddListener %p \n",aWindow));
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
        PR_LOG( trayLog, 5, ("rhTray::AddWindowListener Window already registered  %p \n",aWindow));
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
  HMENU hMenu, hPopup = 0;

  hMenu = ::LoadMenu (::GetModuleHandle("rhTray.dll"),
                      MAKEINTRESOURCE (PopupMenuResource));

  PR_LOG( trayLog, 5, ("rhTray::ShowPopupMenu hMenu %d  error %d\n",hMenu,GetLastError()));

  if (hMenu != 0) {
    POINT pt;
    ::GetCursorPos (&pt);

    hPopup = ::GetSubMenu (hMenu, 0);

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

    PR_LOG( trayLog, 5, ("rhTray::RemoveWindowListener %p \n",aBaseWindow));

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

    PR_LOG( trayLog, 5, ("rhTray::RemoveAllListenesr\n"));
    map< nsIBaseWindow *, rhTrayWindowListener *>::iterator i;

    rhTrayWindowListener *cur = NULL;

    for(i = rhTray::mWindowMap.begin(); i!= rhTray::mWindowMap.end(); i++)
    {

        cur = (*i).second;

        if(cur)
        {


            PR_LOG( trayLog, 5, ("rhTray::RemoveAllListeners deleting %p\n",cur));

            delete cur;

        }

    }

    rhTray::mWindowMap.clear();
    
    return S_OK;

}


//rhTrayWindowListener Methods

rhTrayWindowListener::rhTrayWindowListener(HWND aWnd)
{ 
    mWnd = aWnd;
    origWindowProc = NULL;

}

rhTrayWindowListener::~rhTrayWindowListener()
{

    PR_LOG( trayLog, 5, ("rhTrayWindowListener::~rhTrayWindowListener.\n"));

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
                         me->HideWindow();
                         
                     }

                    PR_LOG( trayLog, 5, ("rhTrayWindowListener Minimize\n"));
                     eventClaimed = 1;
                break;

                case HTMAXBUTTON:
                    PR_LOG( trayLog, 5, ("rhTrayWindowListener:: Maximize  \n"));
                    eventClaimed = 1;
                break;
                 
                case HTCLOSE:

                    PR_LOG( trayLog, 5, ("rhTrayWindowListener Close! \n"));

                    if(me)
                    {
                        me->HideWindow();
                    }

                    eventClaimed = 1;

                break;

            };
 
        break;

        case WM_ACTIVATE:

             switch (LOWORD(wParam)) {
                 case WA_ACTIVE:
                 case WA_CLICKACTIVE:

                     PR_LOG( trayLog, 5, ("rhTrayWindowListener ACTIVATE! \n"));

                     
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


        PR_LOG( trayLog, 5, ("rhTrayWindowListener WM_SHOWWINDOW wParam %d lParam %d! \n",wParam, lParam));

        show = (int) wParam;


        if(lParam == 0)
        {    

                  PR_LOG( trayLog, 5, ("rhTrayWindowListener  WM_SHOW called from ShowWindow or HideWindow \n"));


        }

        break;

        case WM_SYSCOMMAND:
            switch(wParam)
            {
                case SC_CLOSE:

                    PR_LOG( trayLog, 5, ("rhTrayWindowListener:: WM_SYSCOMMAND SC_CLOSE  \n"));
                break;

                default:
                break;
            };

        break;
     
        case WM_SIZE:
            switch(wParam)
            {
                case SIZE_MINIMIZED:

                    PR_LOG( trayLog, 5, ("rhTrayWindowListener::  WM_SIZE SIZE_MINIMIZE \n"));

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
    PR_LOG( trayLog, 5, ("rhTrayWindowListener:: Event claimed \n"));
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

    PR_LOG( trayLog, 5, ("rhTrayWindowListener::Cleanup. \n"));

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
    if(mWnd)
    {
         PR_LOG( trayLog, 5, ("rhTrayWindowListener:: ShowWindow \n"));
         ::ShowWindow(mWnd,SW_SHOW);

         ::ShowWindow(mWnd,SW_RESTORE);

    }

}

void rhTrayWindowListener::HideWindow()
{
    if(mWnd)
    {

        PR_LOG( trayLog, 5, ("rhTrayWindowListener:: Hide Window \n"));

        ::ShowWindow(mWnd,SW_MINIMIZE);
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
