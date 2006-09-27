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


#include <gtk/gtk.h>
#include <stdlib.h>
#include "notifyareaicon.h"
#include "notifytray.h"
#include "string.h"

#ifdef HAVE_LIB_NOTIFY
#include <libnotify/notify.h>
#endif

/* globals */
static NotifyAreaIcon *notify = NULL;

static GtkWidget *image = NULL;
static GdkPixbuf *blank_icon = NULL;
static GtkWidget *notify_menu = NULL;
static char *notify_image = NULL;

static GtkWidget *notify_box = NULL;


GtkWidget *notify_icon_get_box_widget()
{
    return notify_box;
}


void
notify_icon_embedded_cb(GtkWidget *widget, void *data)
{
	g_print ("notify_icon_embedded_cb\n");
}

void
notify_icon_destroyed_cb(GtkWidget *widget, void *data)
{
	g_print ("notify_icon_destroyed_cb\n");

	g_object_unref(G_OBJECT(notify));
	notify = NULL;
}

void
notify_icon_clicked_cb_local(GtkWidget *button, GdkEventButton *event, void *data)
{
	g_print ("notify_icon_clicked_cb_local.\n");

        if(notify_menu)
        {
            g_print("trying to create popup menu. \n");
            gtk_menu_popup(GTK_MENU(notify_menu),
                                             NULL,
                                             NULL,
                                             NULL,
                                             NULL,
                                             event->button,
                                             event->time);

        }
	if (event->type != GDK_BUTTON_PRESS)
		return;
}

void
notify_icon_destroy()
{
    g_print ("notify_icon_destroy\n");
    if(notify == NULL)
    {
        return;
    }

    g_signal_handlers_disconnect_by_func(G_OBJECT(notify),(void *) G_CALLBACK(notify_icon_destroyed_cb),NULL);

    gtk_widget_destroy(GTK_WIDGET(notify));

    g_object_unref(G_OBJECT(notify));
    notify = NULL;

    if (blank_icon)
         g_object_unref(G_OBJECT(blank_icon));

    blank_icon = NULL;
}

void notify_icon_create_with_image_file(char *image_file)
{
   if(notify_image)
   {
      free(notify_image);
      notify_image = NULL;
   }

   notify_image = strdup(image_file);

   notify_icon_create();
}

void
notify_icon_create()
{
	if (notify) {
             g_print ("Notify icon already created!");
            return;
	}

	notify = notify_area_icon_new("coolkey");

        if(!notify)
        {
             g_print ("notify_area_icon_new() failed!");
        }

        if(!notify_box)
        {
	     notify_box = gtk_event_box_new();
        }

        if(notify_image)
        {
            g_print("about to create image from file %s \n",notify_image);
 
	    image = gtk_image_new_from_file(notify_image);

        }

	g_signal_connect(G_OBJECT(notify), "embedded", G_CALLBACK(notify_icon_embedded_cb), NULL);
	g_signal_connect(G_OBJECT(notify), "destroy", G_CALLBACK(notify_icon_destroyed_cb), NULL);
	gtk_container_add(GTK_CONTAINER(notify_box), image);
	gtk_container_add(GTK_CONTAINER(notify), notify_box);

	if(!gtk_check_version(2,4,0))
		g_object_set(G_OBJECT(notify_box), "visible-window", FALSE, NULL);


	gtk_widget_show_all(GTK_WIDGET(notify));

	/* reference the icon */
	g_object_ref(G_OBJECT(notify));

}

int notify_icon_created_ok()
{
  if(!notify)
  {
       g_print ("notify_icon_created_ok returning 0 because notify is null.");
      return 0;

   }

  if(notify->manager_wnd)
    return 1;
  else
  {

    g_print ("notify_icon_created_ok returning 0 because notify->manager_wnd is null.");
    return 0;

  }

}

int notify_icon_show()
{

    if(!notify)
        return 0;

    gtk_widget_show(GTK_WIDGET(notify));

    return 1;

}

int notify_icon_hide()
{

   if(!notify)
       return 0;

   gtk_widget_hide(GTK_WIDGET(notify));

   return 1;
}

void notify_icon_set_static_tooltip(const gchar *message)
{
    if(!message || !notify)
        return;

    if(notify->tooltips)
    {
        gtk_tooltips_set_tip (notify->tooltips, GTK_WIDGET(notify), message, NULL);
    }
}

void notify_icon_send_tooltip_msg(const gchar *title,const gchar *message,gint severity,gint timeout,const char* icon)
{
    if(!message || !notify)
        return;

    gchar *msg_title = NULL;


    if(!title)
       msg_title = "Notification";
    else
       msg_title = title;
   
    gint msg_timeout = 3000; 
       
    if(timeout > 0  && timeout < 10000)
        msg_timeout = timeout;  

    #ifdef HAVE_LIB_NOTIFY

    if(!notify_is_initted()) 
    {
        notify_init ("ESC");
    }

    g_print("icon %s", icon);
    NotifyNotification *not = notify_notification_new(msg_title, message,icon, GTK_WIDGET(notify));

    if(not)
    {
        notify_notification_set_timeout (not, msg_timeout);

        if(!notify_notification_show (not, NULL))
        {
            g_print ("problem showing notification");
        }

        g_object_unref(G_OBJECT(not));
    }
    else
    {
         g_print ("problem creating notification object!\n");
    }

   #endif

}
