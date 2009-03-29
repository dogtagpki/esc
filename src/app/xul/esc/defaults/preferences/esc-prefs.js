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

#pref("toolkit.defaultChromeURI", "chrome://esc/content/settings.xul");


pref("signed.applets.codebase_principal_support",true);

pref("capability.principal.codebase.p0.granted", "UniversalXPConnect");
pref("capability.principal.codebase.p0.id", "file://");

pref("esc.tps.message.timeout","90");

#Do we populate CAPI certs on windows?

pref("esc.windows.do.capi","yes");


#Sample Security Officer Enrollment UI

#pref("esc.security.url","http://test.host.com:7888/cgi-bin/so/enroll.cgi");

#Sample Security Officer Workstation UI

#pref("esc.security.url","https://dhcp-170.sjc.redhat.com:7889/cgi-bin/sow/welcome.cgi");

#Hide the format button or not.

pref("esc.hide.format","no");


#Use this if you absolutely want a global phone home url for all tokens
#Not recommended!

#pref("esc.global.phone.home.url","http:/test.host.com:7888/cgi-bin/home/index.cgi");
