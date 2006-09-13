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


#ifndef _NOTIFY_TRAY_H_
#define _NOTIFY_TRAY_H_

extern GtkWidget *notify_icon_get_box_widget();
extern void notify_icon_create();
extern void notify_icon_create_with_image_file(char *image_file_name);
extern void notify_icon_destroy();
extern int notify_icon_created_ok();
extern int notify_icon_show();
extern int notify_icon_hide();
extern void notify_icon_set_static_tooltip(const char *message);
extern void notify_icon_send_tooltip_msg(const char *title,const char *message,gint severity,gint timeout,const char *icon);
#endif /* _NOTIFY_TRAY_H_ */
