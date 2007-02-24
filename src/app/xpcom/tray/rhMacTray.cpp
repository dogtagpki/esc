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
#include "MacApplication.h"
NS_IMPL_ISUPPORTS1(rhTray, rhITray)

int rhTray::mInitialized = 0;
WindowRef rhTray::mWnd = NULL;
MenuRef   rhTray::mDockMenu = NULL;
MenuRef   rhTray::mRootMenu = NULL;
ProcessSerialNumber rhTray::mPSN;
EventHandlerRef rhTray::mEventHandlerRef=NULL;
EventHandlerUPP rhTray::mEventHandlerUPP=NULL;

map< nsIBaseWindow *, rhTrayWindowListener *> rhTray::mWindowMap;

#define MENU_ITEM_ID_BASE 5
#define GO_MENU_ID 6

std::list< nsCOMPtr<rhITrayWindNotify> > rhTray::gTrayWindNotifyListeners;

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

     if(!aWindow)
         return NS_OK;

     HRESULT res = RemoveListener(aWindow);

     if(res != S_OK)
     {
         return NS_ERROR_FAILURE;
     }
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

NS_IMETHODIMP rhTray::IsInitializedAlready(PRBool *_retval)
{
    *_retval = 0;

    if(rhTray::mInitialized > 1)
        *_retval = 1;

    rhTray::mInitialized ++;

    return NS_OK;
}

NS_IMETHODIMP rhTray::Sendnotification(const char *aTitle,const char *aMessage,PRUint32 aSeverity,PRUint32 aTimeout, const char *aIcon)
{
    return NS_OK;
}

/* void settooltipmsg (in string aMessage); */
NS_IMETHODIMP rhTray::Settooltipmsg(const char *aMessage)
{
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

void rhTray::ShowApp()
{
    char tBuff[56];
    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::Show app!  \n",GetTStamp(tBuff,56)));
  
    ::ShowHideProcess(&rhTray::mPSN,TRUE);
    ::SetFrontProcess(&rhTray::mPSN);
}

void rhTray::HideApp()
{
    ::ShowHideProcess(&rhTray::mPSN,FALSE);
}


HRESULT rhTray::Initialize()
{
    char tBuff[56];
    if(mInitialized)
        return S_OK;

    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::Initialize  dock:  \n",GetTStamp(tBuff,56)));

    OSErr pRes =  GetCurrentProcess (
        &mPSN 
    );

    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::Initialize App PID result %d  \n",GetTStamp(tBuff,56),pRes));
     
    HRESULT res = CreateApplicationListener();

    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::Initialize result of CreateApplicationListener %d \n",GetTStamp(tBuff,56),res));

    if(res != S_OK)
    {
        return E_FAIL;
    }

    //Take care of the menu stuff

    MenuRef tMenu;
    CreateNewMenu(1, 0, &tMenu);

    MenuItemIndex item;
    AppendMenuItemTextWithCFString( tMenu, CFSTR("Show Manage Smart Cards"),  0,MENU_ITEM_ID_BASE , &item );

    if(tMenu)
    {
        OSStatus result =  SetApplicationDockTileMenu (tMenu);

        if(result == noErr)
        {
            PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::Initialize result of SetApplicationDockTileMenu %d \n",GetTStamp(tBuff,56),result));
            mDockMenu = GetApplicationDockTileMenu();

            PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::Initialize result of GetApplicationDockTileMenu: %d . \n",GetTStamp(tBuff,56),mDockMenu));
        }
    }

    MenuRef tGoMenu;
    ::CreateNewMenu(1,0,&tGoMenu);

    if(tGoMenu)
    {
        SetMenuID (tGoMenu,GO_MENU_ID);
    }
    else
    {
        return S_OK;
    }

    MenuRef tRootMenu;
    ::CreateNewMenu(0, 0, &tRootMenu);

    if(!tRootMenu)
    {
        return S_OK;
    }

    MenuItemIndex goItem;

    ::AppendMenuItemTextWithCFString( tGoMenu, CFSTR("Show Manage Smart Cards"),  0,MENU_ITEM_ID_BASE , &goItem );

    ::SetMenuTitleWithCFString( tGoMenu, CFSTR("Go") );

    OSStatus rootResult = ::SetRootMenu(tRootMenu);

    if(rootResult == noErr)
    {
        mRootMenu = AcquireRootMenu();

        MenuItemIndex myMenuIndex;
        AppendMenuItemTextWithCFString( tRootMenu, NULL, 0, 0, &myMenuIndex );
        SetMenuItemHierarchicalMenu(tRootMenu, myMenuIndex, tGoMenu); 
    }

    mInitialized = 1;
    return S_OK;
}

HRESULT rhTray::RemoveIcon()
{
    char tBuff[56];
    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::RemoveIcon. \n",GetTStamp(tBuff,56)));
    return S_OK;
}

HRESULT rhTray::Cleanup()
{
    char tBuff[56];
    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::Cleanup.\n",GetTStamp(tBuff,56)));

    RemoveAllListeners();
    DestroyEventWindow();
    RemoveIcon();

    if(mEventHandlerRef)
    {
        ::RemoveEventHandler(mEventHandlerRef);
    }

    if(mEventHandlerUPP)
    {
        ::DisposeEventHandlerUPP(mEventHandlerUPP);
    }

    if(mDockMenu)
    {
        ::ReleaseMenu(mDockMenu);
    }

    MenuRef goMenu = GetMenuHandle (GO_MENU_ID);

    if(goMenu)
    {
        ::ReleaseMenu(goMenu);
    }

    if(mRootMenu)
    {
        ::ReleaseMenu(mRootMenu);
    }
 
    return S_OK;
}

HRESULT rhTray::CreateApplicationListener()
{
    char tBuff[56];
    EventTargetRef target  = GetApplicationEventTarget();

    if(!target)
        return E_FAIL;

    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::CreateApplicationListener . app target %p\n",GetTStamp(tBuff,56),target));
    int numTypes = 4;

    rhTray::mEventHandlerUPP = NewEventHandlerUPP(rhTray::ApplicationProc);

    if(!rhTray::mEventHandlerUPP)
        return E_FAIL;
    
    EventTypeSpec eventTypes[]= { {kEventClassApplication, kEventAppActivated } , {kEventClassApplication , kEventAppDeactivated }, {kEventClassCommand, kEventCommandProcess}};

    InstallEventHandler(target,rhTray::mEventHandlerUPP,numTypes,eventTypes, (void *) this,&mEventHandlerRef); 
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
        cur = (*i).second;

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
        cur = (*i).second;

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

    return S_OK;
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

    nsIWidget *widget= nsnull;

    WindowRef hWnd = (WindowRef) aNativeWindow;

    //Now see if it's alreay in the map

    rv = aWindow->GetParentWidget(&widget);

    if(widget)
    {
        hWnd  =(WindowRef) widget->GetNativeData(NS_NATIVE_DISPLAY);
    }
    rhTrayWindowListener *already = rhTray::mWindowMap[aWindow];

    if(already)
    {
        PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::AddWindowListener Window already registered  %p \n",GetTStamp(tBuff,56),aWindow));
        return S_OK;
    }

    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::AddWindowListener top level widget  %p \n",GetTStamp(tBuff,56),hWnd));

    rhTrayWindowListener *create = new rhTrayWindowListener(hWnd);

    if(!create)
    {
        return E_FAIL;
    }

    mWindowMap[aWindow] = create;

    HRESULT res = create->Initialize();

    if(res != S_OK)
       return E_FAIL;
  
    return S_OK; 
}

// From Code Project
HRESULT rhTray::ShowPopupMenu ()
{
  return S_OK;
}

HRESULT rhTray::RemoveListener(nsIBaseWindow *aBaseWindow)
{
    char tBuff[56];
    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::RemoveWindowListener %p \n",GetTStamp(tBuff,56),aBaseWindow));

    if(!aBaseWindow)
        return S_OK;

    rhTrayWindowListener *cur = NULL;
    map< nsIBaseWindow *, rhTrayWindowListener *>::iterator i;

    i = rhTray::mWindowMap.find(aBaseWindow);
    
    if(i != rhTray::mWindowMap.end())
    {
        cur = (*i).second;

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
    // On the Mac , we support only one menu item

    if(aIndex == 0 && aText)
    {
        PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhCoolKey::Setmenuitemtext  aIndex: %d text %s. \n",GetTStamp(tBuff,56),aIndex,aText));

        MenuRef outMenu;
        MenuItemIndex theIndex;

        OSStatus result = GetIndMenuItemWithCommandID (
             mDockMenu,
             MENU_ITEM_ID_BASE + aIndex,
             1,
             &outMenu,
             &theIndex
        );

        PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhCoolKey::Setmenuitemtext  Result of menu item: %d. \n",GetTStamp(tBuff,56),result));
   
        if(result == noErr)
        {
            PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhCoolKey::Setmenuitemtext changing item index:    %d . \n",GetTStamp(tBuff,56),theIndex));
            CFStringRef cfStr= CFStringCreateWithCString (
                NULL,
                aText,
                kCFStringEncodingASCII
            );

            OSStatus  result = SetMenuItemTextWithCFString (
               mDockMenu,
               theIndex ,
               cfStr 
            );

            PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhCoolKey::Setmenuitemtext  Result of setting item text: %d. \n",GetTStamp(tBuff,56),result));

        }

        // Now take care of the root menu, provide exact same item here

       MenuRef tGoMenu = GetMenuHandle (GO_MENU_ID);

       if(!tGoMenu)
       {
           return S_OK;
       }

       MenuRef goOutMenu;
       MenuItemIndex theGoIndex;

       OSStatus resultRoot = GetIndMenuItemWithCommandID (
             tGoMenu,
             MENU_ITEM_ID_BASE + aIndex,
             1,
             &goOutMenu,
             &theGoIndex
        );

        PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhCoolKey::Setmenuitemtext  Result of menu item for go menu: %d. \n",GetTStamp(tBuff,56),result));
  
        if(resultRoot == noErr)
        {
            PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhCoolKey::Setmenuitemtext changing item index:    %d . For go  menu. \n",GetTStamp(tBuff,56),theIndex));
            CFStringRef cfStr= CFStringCreateWithCString (
                NULL,
                aText,
                kCFStringEncodingASCII
            );

            OSStatus  result = SetMenuItemTextWithCFString (
               tGoMenu,
               theGoIndex ,
               cfStr
            );

            PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhCoolKey::Setmenuitemtext  Result of setting item text for root menu: %d. \n",GetTStamp(tBuff,56),result));

        }
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
    PR_LOG( trayLog, PR_LOG_DEBUG, ("rhTray::AddTrayWindNotifyListener: %p \n",GetTStamp(tBuff,56),
listener));

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
    char tBuff[56];
      //Now notify all the listeners of the event

    std::list< nsCOMPtr <rhITrayWindNotify> >::const_iterator it;
    for(it=gTrayWindNotifyListeners.begin(); it!=gTrayWindNotifyListeners.end(); ++it) {

        PRBool claimed = 0;

        PR_LOG(trayLog, PR_LOG_DEBUG, ("%s rhTray::NotifyTrayWindListener:   . \n",GetTStamp(tBuff,56)));
        ((rhITrayWindNotify *) (*it))->RhTrayWindEventNotify(aEvent,aEventData, aKeyData, aData1, aData2, &claimed);

    }

}

pascal OSStatus rhTray::ApplicationProc(EventHandlerCallRef nextHandler, EventRef aEvent, void *userData)
{
    char tBuff[56];
    OSStatus result = eventNotHandledErr;

    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::ApplicationProc .\n",GetTStamp(tBuff,56)));

    int theEvent = GetEventKind(aEvent);
    int theClass = GetEventClass(aEvent);

    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::ApplicationProc  class %d event: %d \n",GetTStamp(tBuff,56),theClass,theEvent));

    switch(theClass)
    {
        case kEventClassApplication: 

            switch(theEvent)
            {
                 case kEventAppActivated:
                     PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::ApplicationProc App activated! \n",GetTStamp(tBuff,56)));
                     result = noErr;
                  break;

                 case kEventAppDeactivated:
                     PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::ApplicationProc App deactivated! \n",GetTStamp(tBuff,56)));
                     result = noErr;
                 break;

            };

        break;

        case kEventClassCommand:

            PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::ApplicationProc App kEventClassCommand! \n",GetTStamp(tBuff,56)));
             HICommand commandStruct; 

             GetEventParameter (aEvent, kEventParamDirectObject, 
                    typeHICommand, NULL, sizeof(HICommand), 
                    NULL, &commandStruct);
            
            switch(commandStruct.commandID)
            {
                 case kHICommandHide:
                     PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::ApplicationProc App kHICommandHide! \n",GetTStamp(tBuff,56)));
                 break; 
               
                 case kHICommandSelectWindow:
                     PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::ApplicationProc App kHICommandSelectWindow! \n",GetTStamp(tBuff,56)));
                 break;

                 case kHICommandClose:
                     PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::ApplicationProc App kHICommandClose! \n",GetTStamp(tBuff,56)));
                 break;

                 case kHICommandQuit:
                     PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::ApplicationProc App kHICommandQuit! \n",GetTStamp(tBuff,56)));

                 break;

                 case MENU_ITEM_ID_BASE:
                     PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::ApplicationProc App Manage Smart Cards! \n",GetTStamp(tBuff,56)));
                     NotifyTrayWindListeners(MENU_EVT,MENU_SHOW);

                 break;
            };


        break;

        default:

        break;
    } 

    return result;
}

//rhTrayWindowListener Methods

rhTrayWindowListener::rhTrayWindowListener(WindowRef aWnd)
{ 
    mWnd = aWnd;
}

rhTrayWindowListener::~rhTrayWindowListener()
{
    char tBuff[56];
    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTrayWindowListener::~rhTrayWindowListener.\n",GetTStamp(tBuff,56)));

    Cleanup();
}

HRESULT rhTrayWindowListener::Initialize()
{
    char tBuff[56];
    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTrayWindowListener::Initialize \n",GetTStamp(tBuff,56)));

    mEventHandlerUPP = NewEventHandlerUPP(rhTrayWindowListener::WindowProc);

    if(!mEventHandlerUPP)
        return E_FAIL;

    EventTargetRef target = GetWindowEventTarget(mWnd);

    int numTypes = 2;

    EventTypeSpec eventTypes[]= { {kEventClassWindow, kEventWindowClose } , {kEventClassWindow , kEventWindowHidden }};

    //::InstallStandardEventHandler(target);
    ::InstallEventHandler(target,mEventHandlerUPP,numTypes,eventTypes, (void *) this,&mEventHandlerRef); 

    ShowWindow();

    return S_OK;
}

void rhTrayWindowListener::ShowWindow()
{
    char tBuff[56];
    if(mWnd)
    {
         PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTrayWindowListener:: ShowWindow \n",GetTStamp(tBuff,56)));

         if(IsWindowCollapsed(mWnd))
        { 
            PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTrayWindowListener:: ShowWindow :  uncollapsing collapsed window. \n",GetTStamp(tBuff,56)));
            //::CollapseWindow(mWnd,FALSE);
         }

         if(!IsWindowVisible(mWnd))
         {
             PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTrayWindowListener:: ShowWindow : Window not visible showing...  \n",GetTStamp(tBuff,56)));
             //::ShowWindow(mWnd);
         }


         ::BringToFront(mWnd);
         PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTrayWindowListener:: ShowWindow :  \n",GetTStamp(tBuff,56)));

         rhTray::ShowApp();

    }
}

void rhTrayWindowListener::HideWindow()
{
    char tBuff[56];
    if(mWnd)
    {
         PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTrayWindowListener:: HideWindow \n",GetTStamp(tBuff,56)));
    }

}

pascal OSStatus rhTrayWindowListener::WindowProc(EventHandlerCallRef nextHandler, EventRef aEvent, void *userData)
{
    char tBuff[56];
    OSStatus result = eventNotHandledErr;

    rhTrayWindowListener * self = (rhTrayWindowListener *) userData;

    int theEvent = GetEventKind(aEvent);

    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTrayWindowListener::WindowProc event: %d \n",GetTStamp(tBuff,56),theEvent));
    switch(theEvent)
    {

        case kEventWindowClose:

            PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTrayWindowListener::WindowProc attempting Window close! \n",GetTStamp(tBuff,56)));

        break;

        case kEventWindowHidden:

             PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTrayWindowListener::WindowProc attempting Window hide! \n",GetTStamp(tBuff,56)));
        break;

        case kEventWindowClosed:

            PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTrayWindowListener::WindowProc Window closed! \n",GetTStamp(tBuff,56)));
        break;

        case kEventMouseDown:
            PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTrayWindowListener::WindowProc mouse down! \n",GetTStamp(tBuff,56)));
        break;

    }

    return result;
}

HRESULT rhTrayWindowListener::Cleanup()
{
    char tBuff[56];
    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTrayWindowListener::Cleanup. \n",GetTStamp(tBuff,56)));

    if(mEventHandlerRef)
    {
        ::RemoveEventHandler(mEventHandlerRef);
    }

    if(mEventHandlerUPP)
    {
        ::DisposeEventHandlerUPP(mEventHandlerUPP);
    }

    return S_OK;
}


NS_GENERIC_FACTORY_CONSTRUCTOR(rhTray)


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
