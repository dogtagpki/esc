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

#include <string.h>
#include <unistd.h>
#include <gdk/gdkx.h>
#include "notifyareaicon.h"

#define SYSTEM_TRAY_REQUEST_DOCK    0
#define SYSTEM_TRAY_BEGIN_MESSAGE   1
#define SYSTEM_TRAY_CANCEL_MESSAGE  2
         
static void notify_area_icon_class_init(NotifyAreaIconClass *theClass);
static void notify_area_icon_init(NotifyAreaIcon *icon);
static void notify_area_icon_unrealize (GtkWidget *widget);

static void notify_area_icon_update_manager_wnd (NotifyAreaIcon *icon);

static GtkPlugClass *plug_parent_class  = NULL;

GType
notify_area_icon_get_type (void)
{
  static GType my_type = 0;

  my_type = g_type_from_name("CoolKeyTrayIcon");

  if (my_type == 0)
    {
      static const GTypeInfo my_info =
      {
        sizeof (NotifyAreaIconClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) notify_area_icon_class_init,
        NULL, /* class_finalize */
        NULL, /* class_data */
        sizeof (NotifyAreaIcon),
        0,    /* n_preallocs */
        (GInstanceInitFunc) notify_area_icon_init
      };

      my_type = g_type_register_static (GTK_TYPE_PLUG, "CoolKeyTrayIcon", &my_info,(GTypeFlags) 0);
    }
  else 
  {
      if (plug_parent_class == NULL) {
          notify_area_icon_class_init((NotifyAreaIconClass *)g_type_class_peek(my_type));
      }
  }

  return my_type;
}


static void
notify_area_icon_init (NotifyAreaIcon *icon)
{
  icon->counter = 1;
  
  gtk_widget_add_events (GTK_WIDGET (icon), GDK_PROPERTY_CHANGE_MASK);
}

static void
notify_area_icon_class_init (NotifyAreaIconClass *theClass)
{
  GtkWidgetClass *widget_klass = (GtkWidgetClass *)theClass;

  plug_parent_class =  (GtkPlugClass *) g_type_class_peek_parent (theClass);

  widget_klass->unrealize = notify_area_icon_unrealize;

}


static void
notify_area_icon_unrealize (GtkWidget *widget)
{
  NotifyAreaIcon *icon =  (NotifyAreaIcon *) widget;

  GdkWindow *root_window;

  g_print("notify_area_icon_unrealize \n");
  if (icon->manager_wnd != None)
    {
      GdkWindow *gdkwin;
      gdkwin = gdk_window_lookup (icon->manager_wnd);

    }

  root_window = gdk_window_lookup (gdk_x11_get_default_root_xwindow ());

  if (GTK_WIDGET_CLASS (plug_parent_class)->unrealize)
    (* GTK_WIDGET_CLASS (plug_parent_class)->unrealize) (widget);
}

static void
notify_area_icon_send_manager_msg (NotifyAreaIcon *icon,
				    long         message,
				    Window       window,
				    long         data1,
				    long         data2,
				    long         data3)
{
  XClientMessageEvent ev;
  Display *display;
 
  g_print("notify_area_icon_send_manager_msg \n");
 
  ev.type = ClientMessage;
  ev.window = window;
  ev.message_type = icon->system_tray_opcode_atom;
  ev.format = 32;
  ev.data.l[0] = gdk_x11_get_server_time (GTK_WIDGET (icon)->window);
  ev.data.l[1] = message;
  ev.data.l[2] = data1;
  ev.data.l[3] = data2;
  ev.data.l[4] = data3;

  display = gdk_display;
  
  gdk_error_trap_push ();
  XSendEvent (display,
	      icon->manager_wnd, False, NoEventMask, (XEvent *)&ev);
  XSync (display, False);
  gdk_error_trap_pop ();
}

static void
notify_area_icon_send_dock_request (NotifyAreaIcon *icon)
{

    g_print("notify_area_icon_send_dock_request \n");
    notify_area_icon_send_manager_msg (icon,
				      SYSTEM_TRAY_REQUEST_DOCK,
				      icon->manager_wnd,
				      gtk_plug_get_id (GTK_PLUG (icon)),
				      0, 0);
}

static void
notify_area_icon_update_manager_wnd(NotifyAreaIcon *icon)
{
  Display *xdisplay;
 
  xdisplay = gdk_display;
  
  if (icon->manager_wnd != None)
    {
      GdkWindow *gdkwin;

      gdkwin = gdk_window_lookup (icon->manager_wnd);
      
    }
  
  XGrabServer (xdisplay);
  
  icon->manager_wnd = XGetSelectionOwner (xdisplay, icon->selection_atom);

  if(icon->manager_wnd == None)
  {

      /* Let's loop through for up to 7 seconds until the
         notification applet comes on line */

      const int maxIters = 7;
      const int sleepInterval = 1000000;

      int i = 0;
      for(i = 0; i < maxIters ; i++)
      {

          XUngrabServer (xdisplay);
          XFlush (xdisplay);
          g_print("XGetSelectionOwner failed try again iter: %d ... \n",i);

          usleep(sleepInterval);


          XGrabServer (xdisplay);
          icon->manager_wnd = XGetSelectionOwner (xdisplay, icon->selection_atom); 

          if(icon->manager_wnd == None)
          {
              g_print("XGetSelectionOwner failed try again! \n");
          }
          else
          {

              g_print("XGetSelectionOwner succeeded ! \n");
              break;
          }

      }

  }

  if (icon->manager_wnd != None)
    XSelectInput (xdisplay,
		  icon->manager_wnd, StructureNotifyMask);

  XUngrabServer (xdisplay);
  XFlush (xdisplay);
  
  if (icon->manager_wnd != None)
    {
      GdkWindow *gdkwin;

      gdkwin = gdk_window_lookup (icon->manager_wnd);
     
     g_print("update_manager_wnd gdkwin %p \n",gdkwin); 

      /* Send dock request */
      notify_area_icon_send_dock_request (icon);
    }

}

NotifyAreaIcon *
notify_area_icon_new(const gchar *name)
{
  NotifyAreaIcon *icon;
  char buff[256];
  GdkWindow *root;

  Screen *xscreen = DefaultScreenOfDisplay (gdk_display);

  g_return_val_if_fail (xscreen != NULL,NULL);

  g_print("notify_area_icon_new_for_xscreen \n");

  icon = (NotifyAreaIcon *) g_object_new(notify_area_icon_get_type ()
, NULL);


  g_print ("result of g_object_new() %p",icon);


  if(!icon)
  {
       g_print ("icon is null returning...");
       return icon;
  }

  gtk_window_set_title (GTK_WINDOW (icon), name);

  gtk_plug_construct (GTK_PLUG (icon), 0);  
  
  gtk_widget_realize (GTK_WIDGET (icon)); 

  g_snprintf (buff, sizeof (buff),
	      "_NET_SYSTEM_TRAY_S%d",
	      XScreenNumberOfScreen (xscreen));
  
  icon->selection_atom = XInternAtom (DisplayOfScreen (xscreen),
				      buff, False);
  
  icon->system_tray_opcode_atom = XInternAtom (DisplayOfScreen (xscreen),
					       "_NET_SYSTEM_TRAY_OPCODE", False);

  notify_area_icon_update_manager_wnd(icon); 

   g_print ("attempted to update_manager_wnd: %p",(void *)icon->manager_wnd);

  root = gdk_window_lookup (gdk_x11_get_default_root_xwindow ());
 
  icon->tooltips = gtk_tooltips_new ();

  return icon;
}

guint
notify_area_icon_send_msg (NotifyAreaIcon *icon,
			    gint         timeout,
			    const char *message,
			    gint         msg_len)
{
  guint counter;
 
  g_print("notify_area_tray_icon_send_msg \n"); 
  g_return_val_if_fail (timeout >= 0, 0);
  g_return_val_if_fail (message != NULL, 0);
		     
  if (icon->manager_wnd == None)
    return 0;

  int len = strlen(message);

  counter = icon->counter++;
  
  notify_area_icon_send_manager_msg (icon, SYSTEM_TRAY_BEGIN_MESSAGE,
				      (Window)gtk_plug_get_id (GTK_PLUG (icon)),
				      timeout, len, counter);

  int message_chunk_size = 20;

  gdk_error_trap_push ();

  /* Send the message in chunks  of 20*/

  while (len > 0)
  {
      XClientMessageEvent ev;
      Display *xdisplay;

     
      xdisplay = GDK_DISPLAY_XDISPLAY (gtk_widget_get_display (GTK_WIDGET (icon))); 
      ev.type = ClientMessage;
      ev.window = (Window)gtk_plug_get_id (GTK_PLUG (icon));
      ev.format = 8;
      ev.message_type = XInternAtom (xdisplay,
				     "_NET_SYSTEM_TRAY_MESSAGE_DATA", False);
      if (len > message_chunk_size) 
	{
	  memcpy ((char *)&ev.data, message, message_chunk_size);
	  len -= message_chunk_size;
	  message += message_chunk_size;
	}
      else
	{
	  memcpy ((char *)&ev.data, message, len);
	  len = 0;
	}

      XSendEvent (xdisplay,
		  icon->manager_wnd, False, StructureNotifyMask, (XEvent *)&ev);
      XSync (xdisplay, False);
  }
  gdk_error_trap_pop ();

  return counter;
}
