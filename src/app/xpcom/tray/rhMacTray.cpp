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
#include "MacApplication.h"
NS_IMPL_ISUPPORTS1(rhTray, rhITray)

int rhTray::mInitialized = 0;
WindowRef rhTray::mWnd = NULL;
ProcessSerialNumber rhTray::mPSN;
EventHandlerRef rhTray::mEventHandlerRef=NULL;
EventHandlerUPP rhTray::mEventHandlerUPP=NULL;

map< nsIBaseWindow *, rhTrayWindowListener *> rhTray::mWindowMap;


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

    ShowApp();
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

NS_IMETHODIMP rhTray::Sendnotification(const char *aMessage)
{

    return NS_OK;

}

void rhTray::ShowApp()
{

    PR_LOG( trayLog, 5, ("rhTray::Show app!  \n"));
   
    ::ShowHideProcess(&rhTray::mPSN,TRUE);
    ::SetFrontProcess(&rhTray::mPSN);

}

void rhTray::HideApp()
{

    ::ShowHideProcess(&rhTray::mPSN,FALSE);

}


HRESULT rhTray::Initialize()
{

    if(mInitialized)
        return S_OK;

    PR_LOG( trayLog, 5, ("rhTray::Initialize  dock:  \n"));


    OSErr pRes =  GetCurrentProcess (
        &mPSN 
    );

    PR_LOG( trayLog, 5, ("rhTray::Initialize App PID result %d  \n",pRes));
  
     
    HRESULT res = CreateApplicationListener();

     PR_LOG( trayLog, 5, ("rhTray::Initialize result of CreateApplicationListener %d \n",res));

    if(res != S_OK)
    {
        return E_FAIL;
    }

    mInitialized = 1;

    return S_OK;
}

HRESULT rhTray::RemoveIcon()
{
    PR_LOG( trayLog, 5, ("rhTray::RemoveIcon. \n"));



    return S_OK;
}

HRESULT rhTray::Cleanup()
{
    PR_LOG( trayLog, 5, ("rhTray::Cleanup.\n"));

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
 
    return S_OK;
}

HRESULT rhTray::CreateApplicationListener()
{
    EventTargetRef target  = GetApplicationEventTarget();

    if(!target)
        return E_FAIL;

    PR_LOG( trayLog, 5, ("rhTray::CreateApplicationListener . app target %p\n",target));
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

    ShowApp();
    PR_LOG( trayLog, 5, ("rhTray::ShowAllListeners.\n"));
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
    PR_LOG( trayLog, 5, ("rhTray::HideAllListeners.\n"));

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

    PR_LOG( trayLog, 5, ("rhTray::DestroyEventWindow \n"));


    return S_OK;
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
        PR_LOG( trayLog, 5, ("rhTray::AddWindowListener Window already registered  %p \n",aWindow));
        return S_OK;

    }


    PR_LOG( trayLog, 5, ("rhTray::AddWindowListener top level widget  %p \n",hWnd));

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

    PR_LOG( trayLog, 5, ("rhTray::RemoveWindowListener %p \n",aBaseWindow));

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

    PR_LOG( trayLog, 5, ("rhTray::RemoveAllListenesr\n"));
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

pascal OSStatus rhTray::ApplicationProc(EventHandlerCallRef nextHandler, EventRef aEvent, void *userData)
{

    OSStatus result = eventNotHandledErr;

    PR_LOG( trayLog, 5, ("rhTray::ApplicationProc .\n"));


    int theEvent = GetEventKind(aEvent);
    int theClass = GetEventClass(aEvent);

    PR_LOG( trayLog, 5, ("rhTray::ApplicationProc  class %d event: %d \n",theClass,theEvent));

    switch(theClass)
    {

        case kEventClassApplication: 

            switch(theEvent)
            {

                 case kEventAppActivated:
                   PR_LOG( trayLog, 5, ("rhTray::ApplicationProc App activated! \n"));
                   break;

                 case kEventAppDeactivated:

                     PR_LOG( trayLog, 5, ("rhTray::ApplicationProc App deactivated! \n"));
                     result = noErr;
                 break;
                 

            };

        break;

        case kEventClassCommand:

            PR_LOG( trayLog, 5, ("rhTray::ApplicationProc App kEventClassCommand! \n"));


             HICommand commandStruct; 

             GetEventParameter (aEvent, kEventParamDirectObject, 
                    typeHICommand, NULL, sizeof(HICommand), 
                    NULL, &commandStruct);
            
            switch(commandStruct.commandID)
            {
                 case kHICommandHide:
                     PR_LOG( trayLog, 5, ("rhTray::ApplicationProc App kHICommandHide! \n"));
                 break; 
               
                 case kHICommandSelectWindow:
                     PR_LOG( trayLog, 5, ("rhTray::ApplicationProc App kHICommandSelectWindow! \n"));
                 break;

                 case kHICommandClose:
                     PR_LOG( trayLog, 5, ("rhTray::ApplicationProc App kHICommandClose! \n"));
                 break;

                 case kHICommandQuit:
                     PR_LOG( trayLog, 5, ("rhTray::ApplicationProc App kHICommandQuit! \n"));

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
    PR_LOG( trayLog, 5, ("rhTrayWindowListener::~rhTrayWindowListener.\n"));

    Cleanup();
}

HRESULT rhTrayWindowListener::Initialize()
{
    PR_LOG( trayLog, 5, ("rhTrayWindowListener::Initialize \n"));

    mEventHandlerUPP = NewEventHandlerUPP(rhTrayWindowListener::WindowProc);

    if(!mEventHandlerUPP)
        return E_FAIL;

    EventTargetRef target = GetWindowEventTarget(mWnd);

    int numTypes = 2;

    EventTypeSpec eventTypes[]= { {kEventClassWindow, kEventWindowClose } , {kEventClassWindow , kEventWindowHidden }};

    //::InstallStandardEventHandler(target);
    ::InstallEventHandler(target,mEventHandlerUPP,numTypes,eventTypes, (void *) this,&mEventHandlerRef); 

    return S_OK;
}

void rhTrayWindowListener::ShowWindow()
{

    if(mWnd)
    {

         PR_LOG( trayLog, 5, ("rhTrayWindowListener:: ShowWindow \n"));

         if(IsWindowCollapsed(mWnd))
        { 
            PR_LOG( trayLog, 5, ("rhTrayWindowListener:: ShowWindow :  uncollapsing collapsed window. \n"));
            ::CollapseWindow(mWnd,FALSE);
         }


         if(!IsWindowVisible(mWnd))
         {
             PR_LOG( trayLog, 5, ("rhTrayWindowListener:: ShowWindow : Window not visible showing...  \n"));
             ::ShowWindow(mWnd);
         }


         //::BringToFront(mWnd);
         PR_LOG( trayLog, 5, ("rhTrayWindowListener:: ShowWindow :  \n"));

         rhTray::ShowApp();

    }
}

void rhTrayWindowListener::HideWindow()
{
    if(mWnd)
    {

         PR_LOG( trayLog, 5, ("rhTrayWindowListener:: HideWindow \n"));

         //OSStatus res = ::CollapseWindow(mWnd,TRUE);

         //::HideWindow(mWnd);

         rhTray::HideApp();

         PR_LOG( trayLog, 5, ("rhTrayWindowListener:: HideWindow  \n"));

    }

}

pascal OSStatus rhTrayWindowListener::WindowProc(EventHandlerCallRef nextHandler, EventRef aEvent, void *userData)
{

    OSStatus result = eventNotHandledErr;

    rhTrayWindowListener * self = (rhTrayWindowListener *) userData;

    int theEvent = GetEventKind(aEvent);

    PR_LOG( trayLog, 5, ("rhTrayWindowListener::WindowProc event: %d \n",theEvent));
    switch(theEvent)
    {

        case kEventWindowClose:

            PR_LOG( trayLog, 5, ("rhTrayWindowListener::WindowProc attempting Window close! \n"));

            if(self)
                self->HideWindow();
            //result = noErr;
        break;

        case kEventWindowHidden:

             PR_LOG( trayLog, 5, ("rhTrayWindowListener::WindowProc attempting Window hide! \n"));
        break;

        case kEventWindowClosed:

            PR_LOG( trayLog, 5, ("rhTrayWindowListener::WindowProc Window closed! \n"));
        break;

        case kEventMouseDown:

            PR_LOG( trayLog, 5, ("rhTrayWindowListener::WindowProc mouse down! \n"));
        break;

    }

    return result;
}

HRESULT rhTrayWindowListener::Cleanup()
{

    PR_LOG( trayLog, 5, ("rhTrayWindowListener::Cleanup. \n"));

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
