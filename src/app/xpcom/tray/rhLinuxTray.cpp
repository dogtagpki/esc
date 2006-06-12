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


GtkWidget* rhTray::mWnd = NULL;
GtkWidget* rhTray::mIconMenu = NULL;

int rhTray::mInitialized = 0;

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

NS_IMETHODIMP rhTray::Sendnotification(const char *aMessage)
{


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



HRESULT rhTray::Initialize()
{

    if(mInitialized)
    {
        return S_OK;
    }

    PR_LOG( trayLog, 5, ("rhTray::Initialize  \n"));


    g_set_print_handler(rhTray::TrayPrintHandler);
   
    HRESULT res = E_FAIL; 

    if(res != S_OK)
    {
       return E_FAIL;
    }

    res = CreateEventWindow();

     PR_LOG( trayLog, 5, ("rhTray::Initialize result of CreateIconMenu %d \n",res));

    if(res != S_OK)
    {
        return E_FAIL;
    }

    mInitialized = 1;

    return S_OK;
}

HRESULT rhTray::RemoveIcon()
{
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

    rhTray::mWnd = 0;

    return S_OK;
}

void rhTray::IconMenuCBProc(GtkWidget *widget, gpointer data)
{

     PR_LOG( trayLog, 5, ("rhTray::IconMenuCBProc data %s \n",data));


    if(!strcmp((char *)data,"icon.min"))
    {
         rhTray::HideAllListeners();
    }

    if(!strcmp((char *)data,"icon.max"))
    {
         rhTray::ShowAllListeners();
    }

    if(!strcmp((char *)data,"icon.exit"))
    {
        gtk_main_quit();
    }
}

HRESULT rhTray::CreateIconMenu()
{

    PR_LOG( trayLog, 5, ("rhTray::CreateIconMenu \n"));

    if(mIconMenu)
        return E_FAIL;

    mIconMenu = gtk_menu_new ();

    GtkWidget *min_item = gtk_menu_item_new_with_label ("Hide");
    GtkWidget *max_item = gtk_menu_item_new_with_label ("Show");
    GtkWidget *exit_item = gtk_menu_item_new_with_label ("Exit");

    gtk_menu_shell_append (GTK_MENU_SHELL (mIconMenu), max_item);
    gtk_menu_shell_append (GTK_MENU_SHELL (mIconMenu), min_item);
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

     PR_LOG( trayLog, 5, ("rhTray::IconCBProc \n"));


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

    PR_LOG( trayLog, 5, ("rhTray::CreateEventWindow \n"));

    HRESULT res = CreateIconMenu();
    return res;
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

    GtkWidget *hWnd = NULL;
    GdkWindow *gWnd =(GdkWindow *) aNativeWindow;

    gdk_window_get_user_data (gWnd,(void **)&hWnd);


    PR_LOG(trayLog,5, ("rhTray::AddListener is widget %p .\n",hWnd));
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

    PR_LOG( trayLog, 5, ("rhTray::AddWindowListener current level widget  %p \n",hWnd));

    hWnd = gtk_widget_get_toplevel( hWnd);

    PR_LOG( trayLog, 5, ("rhTray::AddWindowListener top level widget  %p \n",hWnd));

   if (GTK_WIDGET_TOPLEVEL (hWnd))
   {
       PR_LOG( trayLog, 5, ("rhTray::AddWindowListener is really a top level widget  %p \n",hWnd));

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


void rhTray::TrayPrintHandler(const gchar *string)
{

    PR_LOG( trayLog, 5, ("rhTray:TrayPrintHandler. : %s \n",(char *) string));

}
//rhTrayWindowListener Methods

rhTrayWindowListener::rhTrayWindowListener(GtkWidget* aWnd)
{ 
    mWnd = aWnd;
}

rhTrayWindowListener::~rhTrayWindowListener()
{
    PR_LOG( trayLog, 5, ("rhTrayWindowListener::rhTrayWindowListener.\n"));
}

HRESULT rhTrayWindowListener::Initialize()
{
    PR_LOG( trayLog, 5, ("rhTrayWindowListener::Initialize \n"));

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

         PR_LOG( trayLog, 5, ("rhTrayWindowListener:: ShowWindow \n"));
    }

}

void rhTrayWindowListener::HideWindow()
{
    if(mWnd)
    {
        gtk_widget_hide(mWnd);
        PR_LOG( trayLog, 5, ("rhTrayWindowListener:: Hide Window  %p\n",mWnd));
    }
}

void rhTrayWindowListener::WndDestroyCBProc( GtkWidget *widget,
                     gpointer   data )
{

    g_print("WndDestroyCBProc \n");

    PR_LOG( trayLog, 5, ("rhTrayWindowListener::WndDestroyCBProc \n"));

}

gboolean rhTrayWindowListener::WndDeleteCBProc( GtkWidget *widget,
                              GdkEvent  *event,
                              gpointer   data )
{
    g_print("WndDeleteCBProc\n");

    PR_LOG( trayLog, 5, ("rhTrayWindowListener::WndDeleteCBProc \n"));

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
