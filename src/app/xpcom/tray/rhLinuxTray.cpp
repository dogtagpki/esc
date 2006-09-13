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
#include "notifytray.h"
#include "intl/nsIStringBundle.h"

NS_IMPL_ISUPPORTS1(rhTray, rhITray)

GtkWidget* rhTray::mWnd = NULL;
GtkWidget* rhTray::mIconMenu = NULL;

int rhTray::mInitialized = 0;

std::list< nsCOMPtr<rhITrayWindNotify> > rhTray::gTrayWindNotifyListeners;

map< nsIBaseWindow *, rhTrayWindowListener *> rhTray::mWindowMap;


static PRLogModuleInfo *trayLog = PR_NewLogModule("tray");

rhTray::rhTray() 
{
  /* member initializers and constructor code */

}

rhTray::~rhTray()
{

    PR_LOG( trayLog, PR_LOG_DEBUG, ("rhTray::~rhTray\n"));


    Cleanup();
  /* destructor code */
}

NS_IMETHODIMP rhTray::Setwindnotifycallback(rhITrayWindNotify *jsNotify)
{

    PR_LOG( trayLog, PR_LOG_DEBUG, ("rhTray::Setwindnotifycallback\n"));

    if(jsNotify)
        AddTrayWindNotifyListener(jsNotify);


    return NS_OK;

}
 /* void unsetwindnotifycallback (in rhITrayWindNotify jsNotify); */

NS_IMETHODIMP rhTray::Unsetwindnotifycallback(rhITrayWindNotify *jsNotify)
{


    PR_LOG( trayLog, PR_LOG_DEBUG, ("rhTray::Unsetwindnotifycallback\n"));

    if(jsNotify)
        RemoveTrayWindNotifyListener(jsNotify);

    return NS_OK;
}

/* void add (); */
NS_IMETHODIMP rhTray::Add(nsIBaseWindow *aWindow)
{
    

    NS_ENSURE_ARG(aWindow);

    PR_LOG( trayLog, PR_LOG_DEBUG, ("rhTray::Add %p \n",aWindow));
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
     PR_LOG( trayLog, PR_LOG_DEBUG, ("rhTray::Remove window %p \n",aWindow));

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

   if(aMessage)
       notify_icon_send_tooltip_msg(aTitle,aMessage,aSeverity,aTimeout,aIcon);

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

/* void settooltipmsg (in string aMessage); */
NS_IMETHODIMP rhTray::Settooltipmsg(const char *aMessage)
{

    if(aMessage)
        notify_icon_set_static_tooltip(aMessage);

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


    notify_icon_hide();

    return NS_OK;
}

/* void showicon (); */
NS_IMETHODIMP rhTray::Showicon(void)
{

    notify_icon_show();

    return NS_OK;
}

HRESULT rhTray::Initialize()
{

    if(mInitialized)
    {
        return S_OK;
    }

    PR_LOG( trayLog, PR_LOG_DEBUG, ("rhTray::Initialize  \n"));


    g_set_print_handler(rhTray::TrayPrintHandler);
  
    notify_icon_create_with_image_file(COOLKEY_ICON);
 
    HRESULT res = notify_icon_created_ok(); 

    if(res != S_OK)
    {
       return E_FAIL;
    }

    
    GtkWidget *icon_widget = notify_icon_get_box_widget();

    if(icon_widget)
    {
        g_signal_connect(G_OBJECT(icon_widget), "button-press-event", G_CALLBACK(rhTray::IconCBProc), NULL);

    }

    res = CreateEventWindow();

     PR_LOG( trayLog, PR_LOG_DEBUG, ("rhTray::Initialize result of CreateIconMenu %d \n",res));

    if(res != S_OK)
    {
        return E_FAIL;
    }

    mInitialized = 1;

    notify_icon_hide();

    return S_OK;
}

HRESULT rhTray::RemoveIcon()
{
    return S_OK;
}

HRESULT rhTray::Cleanup()
{
    PR_LOG( trayLog, PR_LOG_DEBUG, ("rhTray::Cleanup.\n"));

    RemoveAllListeners();
    DestroyEventWindow();
    RemoveIcon();

    return S_OK;
}

void rhTray::ShowAllListeners()
{

    PR_LOG( trayLog, PR_LOG_DEBUG, ("rhTray::ShowAllListeners.\n"));
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
    PR_LOG( trayLog, PR_LOG_DEBUG, ("rhTray::HideAllListeners.\n"));

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

    PR_LOG( trayLog, PR_LOG_DEBUG, ("rhTray::DestroyEventWindow \n"));

    rhTray::mWnd = 0;

    return S_OK;
}

void rhTray::IconMenuCBProc(GtkWidget *widget, gpointer data)
{

     PR_LOG( trayLog, PR_LOG_DEBUG, ("rhTray::IconMenuCBProc data %s \n",data));


    if(!strcmp((char *)data,"icon.min"))
    {
         rhTray::HideAllListeners();
    }

    if(!strcmp((char *)data,"icon.max"))
    {
         NotifyTrayWindListeners(MENU_EVT,MENU_SHOW);
         rhTray::ShowAllListeners();
    }

    if(!strcmp((char *)data,"icon.exit"))
    {
        gtk_main_quit();
    }
}

HRESULT rhTray::CreateIconMenu()
{

    PR_LOG( trayLog, PR_LOG_DEBUG, ("rhTray::CreateIconMenu \n"));

    if(mIconMenu)
        return E_FAIL;

    mIconMenu = gtk_menu_new ();

    GtkWidget *min_item = gtk_menu_item_new_with_label ("Hide");
    GtkWidget *max_item = gtk_menu_item_new_with_label ("Manage Keys");
    GtkWidget *exit_item = gtk_menu_item_new_with_label ("Exit");

    gtk_menu_shell_append (GTK_MENU_SHELL (mIconMenu), max_item);
    //gtk_menu_shell_append (GTK_MENU_SHELL (mIconMenu), min_item);
    gtk_menu_shell_append (GTK_MENU_SHELL (mIconMenu), exit_item);

    g_signal_connect(G_OBJECT (min_item), "activate",
                              G_CALLBACK (rhTray::IconMenuCBProc),
                              (gpointer) "icon.min");
    g_signal_connect(G_OBJECT (max_item), "activate",
                              G_CALLBACK (rhTray::IconMenuCBProc),
                              (gpointer) "icon.max");

    g_signal_connect (G_OBJECT (exit_item), "activate",
                              G_CALLBACK (rhTray::IconMenuCBProc),
                              (gpointer) "icon.exit");

    gtk_widget_show (min_item);
    gtk_widget_show (max_item);
    gtk_widget_show (exit_item);

    return S_OK;
}

void rhTray::IconCBProc(GtkWidget *button, GdkEventButton *event, void *data)
{

     PR_LOG( trayLog, PR_LOG_DEBUG, ("rhTray::IconCBProc \n"));


    if(event->type != GDK_BUTTON_PRESS)
    {
       return;

    }

    if(event->button == 1)
    {

        rhTray::ShowAllListeners();

        return;


    }


    if(mIconMenu)
    {
            g_print("trying to create popup menu. \n");
            gtk_menu_popup(GTK_MENU(mIconMenu),
                                             NULL,
                                             NULL,
                                             NULL,
                                             NULL,
                                             event->button,
                                             event->time);

   }

}

HRESULT rhTray::CreateEventWindow()
{

    PR_LOG( trayLog, PR_LOG_DEBUG, ("rhTray::CreateEventWindow \n"));

    HRESULT res = CreateIconMenu();
    return res;
}

HRESULT rhTray::AddListener(nsIBaseWindow *aWindow)
{

    nsresult rv;


    PR_LOG( trayLog, PR_LOG_DEBUG, ("rhTray::AddListener %p \n",aWindow));
    NS_ENSURE_ARG(aWindow);

     nativeWindow aNativeWindow;
    rv = aWindow->GetParentNativeWindow( &aNativeWindow );


    if(NS_FAILED(rv))
    {
        return E_FAIL;
    }

    GtkWidget *hWnd = NULL;
    GdkWindow *gWnd =(GdkWindow *) aNativeWindow;

    gdk_window_get_user_data (gWnd,(void **)&hWnd);


    PR_LOG(trayLog,PR_LOG_DEBUG, ("rhTray::AddListener is widget %p .\n",hWnd));
    if(!hWnd)
    {
        return E_FAIL;
    }    

    //Now see if it's alreay in the map


    rhTrayWindowListener *already = rhTray::mWindowMap[aWindow];

    if(already)
    {
        PR_LOG( trayLog, PR_LOG_DEBUG, ("rhTray::AddWindowListener Window already registered  %p \n",aWindow));
        return S_OK;

    }

    PR_LOG( trayLog, PR_LOG_DEBUG, ("rhTray::AddWindowListener current level widget  %p \n",hWnd));

    hWnd = gtk_widget_get_toplevel( hWnd);

    PR_LOG( trayLog, PR_LOG_DEBUG, ("rhTray::AddWindowListener top level widget  %p \n",hWnd));

   if (GTK_WIDGET_TOPLEVEL (hWnd))
   {
       PR_LOG( trayLog, PR_LOG_DEBUG, ("rhTray::AddWindowListener is really a top level widget  %p \n",hWnd));

   }
    rhTrayWindowListener *create = new rhTrayWindowListener(hWnd);

    if(!create)
    {
        return E_FAIL;
    }

    mWindowMap[aWindow] = create;
    //::SetProp(hWnd,LISTENER_INSTANCE,(HANDLE) create);

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

    PR_LOG( trayLog, PR_LOG_DEBUG, ("rhTray::RemoveWindowListener %p \n",aBaseWindow));

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

    PR_LOG( trayLog, PR_LOG_DEBUG, ("rhTray::RemoveAllListenesr\n"));
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

//rhTrayWindNotify methods

rhITrayWindNotify* rhTray::GetTrayWindNotifyListener(rhITrayWindNotify *listener)
{

    std::list<nsCOMPtr<rhITrayWindNotify> >::const_iterator it;

    for(it=gTrayWindNotifyListeners.begin(); it!=gTrayWindNotifyListeners.end(); ++it) {

    if((*it) == listener)
    {
        return (*it);
    }
}

    PR_LOG( trayLog, PR_LOG_DEBUG, ("rhCoolKey::GetNotifyKeyListener:  looking for %p returning NULL. \n",listener));

    return nsnull;



}

int rhTray::GetTrayWindNotifyListSize()
{
    return gTrayWindNotifyListeners.size();

}


void rhTray::AddTrayWindNotifyListener(rhITrayWindNotify *listener)
{

        PR_LOG( trayLog, PR_LOG_DEBUG, ("rhTray::AddTrayWindNotifyListener: %p \n",
listener));

    if(GetTrayWindNotifyListener(listener ))
    {

         PR_LOG( trayLog, PR_LOG_DEBUG, ("rhTray::AddTrayWindNotifyListener: %p listener already in list. \n",listener));

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

    PR_LOG( trayLog, PR_LOG_DEBUG, ("rhTray::NotifyTrayWindListeners \n"));
     
      //Now notify all the listeners of the event

    std::list< nsCOMPtr <rhITrayWindNotify> >::const_iterator it;
    for(it=gTrayWindNotifyListeners.begin(); it!=gTrayWindNotifyListeners.end(); ++it) {

        PRBool claimed = 0;

        PR_LOG( trayLog, PR_LOG_DEBUG, ("NotifyTrayWindListeners about to notify \n"));
        ((rhITrayWindNotify *) (*it))->RhTrayWindEventNotify(aEvent,aEventData, aKeyData, aData1, aData2, &claimed);


    }

}


void rhTray::TrayPrintHandler(const gchar *string)
{

    PR_LOG( trayLog, PR_LOG_DEBUG, ("rhTray:TrayPrintHandler. : %s \n",(char *) string));

}
//rhTrayWindowListener Methods

rhTrayWindowListener::rhTrayWindowListener(GtkWidget* aWnd)
{ 
    mWnd = aWnd;
}

rhTrayWindowListener::~rhTrayWindowListener()
{
    PR_LOG( trayLog, PR_LOG_DEBUG, ("rhTrayWindowListener::rhTrayWindowListener.\n"));
}

HRESULT rhTrayWindowListener::Initialize()
{
    PR_LOG( trayLog, PR_LOG_DEBUG, ("rhTrayWindowListener::Initialize \n"));

    if(mWnd)
    {
        //Connect the various callbacks for this window

        g_signal_connect(GTK_OBJECT (mWnd), "delete_event",
                      GTK_SIGNAL_FUNC (rhTrayWindowListener::WndDeleteCBProc), this);     
        g_signal_connect(GTK_OBJECT (mWnd), "destroy", GTK_SIGNAL_FUNC(rhTrayWindowListener::WndDestroyCBProc),this); 
    }

    return S_OK;
}


void rhTrayWindowListener::ShowWindow()
{
    if(mWnd)
    {
         gtk_widget_show(mWnd);

         
         gtk_window_deiconify(GTK_WINDOW(mWnd));

         PR_LOG( trayLog, PR_LOG_DEBUG, ("rhTrayWindowListener:: ShowWindow \n"));
    }

}

void rhTrayWindowListener::HideWindow()
{
    if(mWnd)
    {
        gtk_widget_hide(mWnd);
        PR_LOG( trayLog, PR_LOG_DEBUG, ("rhTrayWindowListener:: Hide Window  %p\n",mWnd));
    }
}

void rhTrayWindowListener::WndDestroyCBProc( GtkWidget *widget,
                     gpointer   data )
{

    g_print("WndDestroyCBProc \n");

    PR_LOG( trayLog, PR_LOG_DEBUG, ("rhTrayWindowListener::WndDestroyCBProc \n"));

}

gboolean rhTrayWindowListener::WndDeleteCBProc( GtkWidget *widget,
                              GdkEvent  *event,
                              gpointer   data )
{
    g_print("WndDeleteCBProc\n");

    PR_LOG( trayLog, PR_LOG_DEBUG, ("rhTrayWindowListener::WndDeleteCBProc \n"));

    rhTrayWindowListener *me = (rhTrayWindowListener *) data;

    if(!me)
    {
        return TRUE;
    }

    return TRUE;
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
