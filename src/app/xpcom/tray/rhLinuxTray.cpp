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
#include <gdk/gdkx.h>
#include <stdlib.h>

NS_IMPL_ISUPPORTS1(rhTray, rhITray)

GtkWidget* rhTray::mWnd = NULL;
GtkWidget* rhTray::mIconMenu = NULL;
GtkWidget* rhTray::mIconBoxWidget = NULL;

int rhTray::mInitialized = 0;

std::list< nsCOMPtr<rhITrayWindNotify> > rhTray::gTrayWindNotifyListeners;

map< nsIBaseWindow *, rhTrayWindowListener *> rhTray::mWindowMap;


static PRLogModuleInfo *trayLog = PR_NewLogModule("tray");

static void popup_position(GtkMenu *menu,
                                       gint *x,
                                       gint *y,
                                       gboolean *push_in,
                                       gpointer user_data)
{

  char tBuff[56];
  GtkWidget *icon_box_widget = GTK_WIDGET(user_data);


  if(icon_box_widget)
  {
     GdkScreen* gscreen = gdk_screen_get_default(); 
     GdkWindow* window = icon_box_widget->window;
  
     if(!window)
         return;


     gint width;
     gint height;

     gint px;
     gint py;

     gint screen_width  = 0;
     gint screen_height = 0;

     if(gscreen)
     {
         screen_width  = gdk_screen_get_width(gscreen);
         screen_height = gdk_screen_get_height(gscreen);
     }

     gdk_drawable_get_size(window,&width,&height);

     gdk_window_get_origin(window,
                                     &px,
                                     &py);

    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s popup_position width %d height %d  px %d py %d *x %d *y %d  screen_w %d screen_h %d  \n",GetTStamp(tBuff,56),width,height,px,py,*x,*y,screen_width, screen_height));

    // Are we close to the bottom of the screen? 

    if( screen_width > 0 && screen_height > 0 
        && ( screen_height - py) < (height * 3))
    {
        height = height* -2 ;
    }

     gint x_coord = px;
     gint y_coord = (py + height);

     *x = x_coord;
     *y = y_coord; 
     *push_in = TRUE;

  }

}

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

    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::Add Initialize res %d \n",GetTStamp(tBuff,56),res));

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
    char tBuff[56];
    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::Initialize entering... mInitialized: %d \n",GetTStamp(tBuff,56),mInitialized));

    if(mInitialized)
    {
        return S_OK;
    }

    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::Initialize  \n",GetTStamp(tBuff,56)));
    g_set_print_handler(rhTray::TrayPrintHandler);
    notify_icon_create_with_image_file(COOLKEY_ICON);
 
    HRESULT res = notify_icon_created_ok(); 

    if(res != S_OK)
    {
       return E_FAIL;
    }
    
    mIconBoxWidget = notify_icon_get_box_widget();

    if(mIconBoxWidget)
    {
        g_signal_connect(G_OBJECT(mIconBoxWidget), "button-press-event", G_CALLBACK(rhTray::IconCBProc), NULL);

    }

    res = CreateEventWindow();

    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::Initialize result of CreateIconMenu %d \n",GetTStamp(tBuff,56),res));

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
    char tBuff[56];
    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::Cleanup.\n",GetTStamp(tBuff,56)));

    RemoveAllListeners();
    DestroyEventWindow();
    RemoveIcon();

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

    rhTray::mWnd = 0;
    return S_OK;
}

void rhTray::IconMenuCBProc(GtkWidget *widget, gpointer data)
{
    char tBuff[56];
    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::IconMenuCBProc data %s \n",GetTStamp(tBuff,56),data));

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
        exit(0);
    }
}

HRESULT rhTray::CreateIconMenu()
{
    char tBuff[56];
    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::CreateIconMenu \n",GetTStamp(tBuff,56)));

    if(mIconMenu)
        return E_FAIL;

    mIconMenu = gtk_menu_new ();

    GtkWidget *min_item = gtk_menu_item_new_with_label ("Hide");
    GtkWidget *max_item = gtk_menu_item_new_with_label ("Manage Keys");
    GtkWidget *exit_item = gtk_image_menu_item_new_with_label ("Exit");

    GtkWidget* quit_icon = gtk_image_new_from_stock(GTK_STOCK_QUIT,GTK_ICON_SIZE_SMALL_TOOLBAR);

    if(max_item)
       gtk_menu_shell_append (GTK_MENU_SHELL (mIconMenu), max_item);
    //gtk_menu_shell_append (GTK_MENU_SHELL (mIconMenu), min_item);
    if(exit_item)
    {
        gtk_menu_shell_append (GTK_MENU_SHELL (mIconMenu), exit_item);

        if(quit_icon)
        {
           gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(exit_item), quit_icon);

        }
    }

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
    char tBuff[56];
    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::IconCBProc \n",GetTStamp(tBuff,56)));

    if(event->type != GDK_BUTTON_PRESS)
    {
       return;
    }

    if(event->button == 1)
    {
        PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::IconCBProc Clicked!\n",GetTStamp(tBuff,56)));

        NotifyTrayWindListeners(MENU_EVT,MENU_SHOW);
        rhTray::ShowAllListeners();

        return;
    }

    if(event->button == 2 || event->button == 3)
    {
        if(mIconMenu)
        {
                g_print("trying to create popup menu. \n");
                gtk_menu_popup(GTK_MENU(mIconMenu),
                                             NULL,
                                             NULL,
                        (GtkMenuPositionFunc) popup_position,
                                              mIconBoxWidget,
                                             event->button,
                                             event->time);

       }
    }

}

HRESULT rhTray::CreateEventWindow()
{
    char tBuff[56];
    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::CreateEventWindow \n",GetTStamp(tBuff,56)));

    HRESULT res = CreateIconMenu();
    return res;
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

    GtkWidget *hWnd = NULL;
    GdkWindow *gWnd =(GdkWindow *) aNativeWindow;

    gdk_window_get_user_data (gWnd,(void **)&hWnd);

    PR_LOG(trayLog,PR_LOG_DEBUG, ("%s rhTray::AddListener is widget %p .\n",GetTStamp(tBuff,56),hWnd));
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

    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::AddWindowListener current level widget  %p \n",GetTStamp(tBuff,56),hWnd));

    hWnd = gtk_widget_get_toplevel( hWnd);

    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::AddWindowListener top level widget  %p \n",GetTStamp(tBuff,56),hWnd));

    if (GTK_WIDGET_TOPLEVEL (hWnd))
    {
        PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::AddWindowListener is really a top level widget  %p \n",GetTStamp(tBuff,56),hWnd));
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
    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::Setmenuitemtext:  index: %d text %s. \n",GetTStamp(tBuff,56),aIndex,aText));

    if(!aText)
        return S_OK;

    if(!mIconMenu)
        return S_OK;

    if(aIndex < 0 || aIndex > 10)
        return S_OK; 

    GList *iterate = NULL;

    GList*  children = gtk_container_get_children (GTK_CONTAINER (mIconMenu));

    unsigned int i = 0;
    for (iterate = children; iterate; iterate=iterate->next)
    {
        PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::Setmenuitemtext:  index: %d \n",GetTStamp(tBuff,56),i));
        if(aIndex == i)
        {
             PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::Setmenuitemtext:  About to reset text of item %p. \n",GetTStamp(tBuff,56),(void *) iterate->data));
             if(iterate->data)
             {
                GtkWidget *label = gtk_bin_get_child(GTK_BIN(iterate->data)); 

                if(label)
                {
                    gtk_label_set_text(GTK_LABEL(label),aText);
                }
             }

             break;
        }

        i++;
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
    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::AddTrayWindNotifyListener: %p \n", GetTStamp(tBuff,56),
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
    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray::NotifyTrayWindListeners \n",GetTStamp(tBuff,56)));
     
      //Now notify all the listeners of the event

    std::list< nsCOMPtr <rhITrayWindNotify> >::const_iterator it;
    for(it=gTrayWindNotifyListeners.begin(); it!=gTrayWindNotifyListeners.end(); ++it) {

        PRBool claimed = 0;

        PR_LOG( trayLog, PR_LOG_DEBUG, ("%s NotifyTrayWindListeners about to notify \n",GetTStamp(tBuff,56)));
        ((rhITrayWindNotify *) (*it))->RhTrayWindEventNotify(aEvent,aEventData, aKeyData, aData1, aData2, &claimed);

    }

}


void rhTray::TrayPrintHandler(const gchar *string)
{
    char tBuff[56];
    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTray:TrayPrintHandler. : %s \n",GetTStamp(tBuff,56),(char *) string));
}
//rhTrayWindowListener Methods

rhTrayWindowListener::rhTrayWindowListener(GtkWidget* aWnd)
{ 
    mWnd = aWnd;
}

rhTrayWindowListener::~rhTrayWindowListener()
{
    char tBuff[56];
    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTrayWindowListener::rhTrayWindowListener.\n",GetTStamp(tBuff,56)));
}

HRESULT rhTrayWindowListener::Initialize()
{
    char tBuff[56];
    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTrayWindowListener::Initialize \n",GetTStamp(tBuff,56)));

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
    char tBuff[56];
    if(mWnd)
    {
         GtkWidget *widget = NULL;

         widget = GTK_WIDGET(mWnd);

         if(widget->window)
         {
             gdk_x11_window_set_user_time (widget->window, gdk_x11_get_server_time (widget->window));
             if(GTK_WIDGET_VISIBLE(mWnd))
             {
                 gdk_window_show(widget->window);
                 gdk_window_raise(widget->window); 

             }
             else
             {
                 gtk_widget_show(widget);
             }
         }

         PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTrayWindowListener:: ShowWindow \n",GetTStamp(tBuff,56)));
    }
}

void rhTrayWindowListener::HideWindow()
{
    char tBuff[56];
    if(mWnd)
    {
        gtk_widget_hide(mWnd);
        PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTrayWindowListener:: Hide Window  %p\n",GetTStamp(tBuff,56),mWnd));
    }
}

void rhTrayWindowListener::WndDestroyCBProc( GtkWidget *widget,
                     gpointer   data )
{
    char tBuff[56];
    g_print("WndDestroyCBProc \n");

    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTrayWindowListener::WndDestroyCBProc \n",GetTStamp(tBuff,56)));

}

gboolean rhTrayWindowListener::WndDeleteCBProc( GtkWidget *widget,
                              GdkEvent  *event,
                              gpointer   data )
{
    char tBuff[56];
    g_print("WndDeleteCBProc\n");

    PR_LOG( trayLog, PR_LOG_DEBUG, ("%s rhTrayWindowListener::WndDeleteCBProc \n",GetTStamp(tBuff,56)));

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
