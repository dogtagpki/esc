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

pref("toolkit.defaultChromeURI", "chrome://esc/content/settings.xul");


pref("esc.tps.url","http://test.host.com:7888/nk_service");

#pref("esc.enroll.ui.url","http://test.host.com:7888/cgi-bin/esc.cgi?action=autoenroll");

#pref("esc.enrolled.token.url","http://www.test.com");
pref("esc.hide.on.startup","yes");



pref("signed.applets.codebase_principal_support",true);

pref("capability.principal.codebase.p0.granted", "UniversalXPConnect");
pref("capability.principal.codebase.p0.id", "file://");

pref("esc.tps.message.timeout","90");
