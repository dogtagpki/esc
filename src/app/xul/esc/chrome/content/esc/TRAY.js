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

// ESC Icon Tray functionality

var gStringBundle=null;

var gTray = null;

var gBaseWindow = 0;


loadStringBundle();

//Initialize tray XPCOM object

 // GECKO ONLY initialization
try {
   netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
   gTray =  Components.classes["@redhat.com/rhTray"].getService();


      gTray = gTray.QueryInterface(Components.interfaces.rhITray);

      gBaseWindow = getBaseWindow();



 } catch(e) {
}


TrayAddWindow();

function getBaseWindow(  ) {
    var rv;
    try
    {
        var requestor =
            window.QueryInterface(Components.interfaces.nsIInterfaceRequestor);
        var nav =
            requestor.getInterface(Components.interfaces.nsIWebNavigation);
        var dsti =
            nav.QueryInterface(Components.interfaces.nsIDocShellTreeItem);
        var owner = dsti.treeOwner;
        requestor =
            owner.QueryInterface(Components.interfaces.nsIInterfaceRequestor);
        rv = requestor.getInterface(Components.interfaces.nsIXULWindow);
        rv = rv.docShell;
        rv = rv.QueryInterface(Components.interfaces.nsIDocShell);
        rv = rv.QueryInterface(Components.interfaces.nsIBaseWindow);

    }
    catch (ex)
    {
        rv = null;
        /* ignore no-interface exception */
    }
    return rv;
}

function GetTrayIsInitialized()
{

   var rv= 0;

   if(!gTray)
       return rv;


   try
   {
        netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

        rv = gTray.isInitializedAlready();


   }
   catch (ex)
   {
      alert(getBundleString("errorTrayIsInitialized"));
      rv = null;
      /* ignore no-interface exception */
   }
   return rv;


}

function ShowAllWindows()
{

    if( !gTray || !gBaseWindow)
        return;


    try {

        netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

        gTray.showall();

            } catch(e) {
                alert(getBundleString("errorShowAllWindows") + e);
                return;
            }


}

function HideAllWindows()
{

    if( !gTray || !gBaseWindow)
        return;

    try {

        netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

        gTray.hideall();

            } catch(e) {
                alert(getStringBundle("errorHideAllWindows") + e);
                return;
            }

}


function ShowWindow()
{

    if( !gTray || !gBaseWindow)
        return;


    try {

        netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

        gTray.show(gBaseWindow);

            } catch(e) {
                alert(getBundleString("errorShowWindow"));
                return;
            }


}

function HideWindow()
{

    if(!gTray || !gBaseWindow)
        return ;
        
    try {
         
        netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
        
        gTray.hide(gBaseWindow);
        
            } catch(e) {
                alert(getBundleString("errorHideWindow"));
                return;
            }   
            

}

function TrayRemoveWindow()
{

    if(gTray && gBaseWindow)
    {

         try {

        netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

            gTray.remove(gBaseWindow);

            } catch(e) {
                alert(getBundleString("errorRemoveWindow") + e);
                return;
            }

    }

}

function TrayAddWindow()
{

    if(gTray && gBaseWindow)
    {

         try {

        netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

            gTray.add(gBaseWindow);

            } catch(e) {
                // no need to bother the user if the tray fails

                gTray = null;
                return;
            }


    }




}

function TraySendNotificationMessage(aMessage)
{


    if(gTray && gBaseWindow)
{

     try {

    netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect")


        gTray.sendnotification(aMessage);

        } catch(e) {
            alert(getBundleString("errorTrayNotification") + e);
            return;
        }


}




}

//String bundling related functions

function loadStringBundle()
{
    gStringBundle = document.getElementById("esc_strings");
}

function getBundleString(string_id)
{

    var str = null;

    if(!string_id || !gStringBundle)
       return null;

    str = gStringBundle.getString(string_id);

    return str;

}

