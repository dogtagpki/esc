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

#ifndef _NOTIFY_AREA_ICON_H
#define _NOTIFY_AREA_ICON_H

#include <gtk/gtkplug.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>

G_BEGIN_DECLS
	
struct _NotifyAreaIcon
{

  GtkPlug parent;

  guint counter;
  
  Atom selection_atom;
  Atom system_tray_opcode_atom;
  Window manager_wnd;

  GtkTooltips *tooltips;
};

struct _NotifyAreaIconClass
{
    GtkPlugClass parent_class;
};

typedef struct _NotifyAreaIcon NotifyAreaIcon;
typedef struct _NotifyAreaIconClass NotifyAreaIconClass;

extern  NotifyAreaIcon *notify_area_icon_new(const gchar *name);

guint        notify_area_icon_send_msg(NotifyAreaIcon *icon,
					   gint         timeout,
					   const char  *msg,
					   gint         length);

GType notify_area_icon_get_type (void);
					    
G_END_DECLS

#endif /* NOTIFY_AREA_ICON_H */
