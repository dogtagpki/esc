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

var gWindNotify = null;



loadStringBundle();

//
// Tray event Notify callback
//
function jsWindNotify()  {}

jsWindNotify.prototype = {

  rhTrayWindEventNotify: function(aEvent,aEventData,aKeyData,aData1,aData2)
  {
     //alert("rhTrayWindEventNotify!!!! event " + aEvent);

     NotifyESCOfTrayEvent(aEvent,aEventData,aKeyData,aData1,aData2);
  },

  QueryInterface: function(iid)
  {
     //alert("iid: " + iid);
     if(!iid.equals(Components.interfaces.rhITrayWindNotify) &&
         !iid.equals(Components.interfaces.nsISupports))
      {
          MyAlert(getBundleString("errorJsNotifyInterface"));
          throw Components.results.NS_ERROR_NO_INTERFACE;
      }
      return this;
  }
};



//Initialize tray XPCOM object

 // GECKO ONLY initialization
try {
   netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
   gTray =  Components.classes["@redhat.com/rhTray"].getService();


      gTray = gTray.QueryInterface(Components.interfaces.rhITray);

      gBaseWindow = getBaseWindow();


      if(gTray)
      {
           gWindNotify = new jsWindNotify;
           if(gWindNotify)
               gTray.setwindnotifycallback(gWindNotify);

           //alert("setting tray notify callback " + gWindNotify);
      }


 } catch(e) {

     MyAlert("e " + e);
}

TrayAddWindow();

TrayShowTooltip(getBundleString("escTitle"));

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
      MyAlert(getBundleString("errorTrayIsInitialized"));
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
                MyAlert(getBundleString("errorShowAllWindows") + e);
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
                MyAlert(getStringBundle("errorHideAllWindows") + e);
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
                MyAlert(getBundleString("errorShowWindow"));
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
                MyAlert(getBundleString("errorHideWindow"));
                return;
            }   
            

}

function TrayRemoveWindowNotify()
{

    if(gTray && gBaseWindow && gWindNotify)
    {
         try {

            netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

            gTray.unsetwindnotifycallback(gWindNotify);
            

            } catch(e) {
                return;
            }


   }
}

function TrayRemoveWindow(doPreserveNotifyCallback)
{
    if(gTray && gBaseWindow)
    {
        try {

        netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

            gTray.remove(gBaseWindow);

            } catch(e) {
                MyAlert(getBundleString("errorRemoveWindow") + e);
                return;
            }

        if(gWindNotify && !doPreserveNotifyCallback)
               gTray.unsetwindnotifycallback(gWindNotify);
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

function TraySendNotificationMessage(aTitle,aMessage,aSeverity,aTimeout,aIcon)
{
    if(!gHiddenPage)
        return;

    if(gTray && gBaseWindow)
    {

        try {

        netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect")


        gTray.sendnotification(aTitle,aMessage,aSeverity,aTimeout,aIcon);

        } catch(e) {
            alert(getBundleString("errorTrayNotification") + e);
            return;
        }

    }

} 


function TrayHideNotificationIcon()
{
    if(!gHiddenPage)
        return;

    if(gTray && gBaseWindow)
    {

        try {

        netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect")

        gTray.hideicon();

        } catch(e) {
            return;
        }

    }

}

function TrayLoadedOK()
{
      var result = 0;

      if(gTray)
          result = 1;

      return result;
}

function TrayShowNotificationIcon()
{
    if(!gHiddenPage)
        return;

    if(gTray && gBaseWindow)
    {

        try {

        netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
        gTray.settooltipmsg(getBundleString("escTitle"));
        gTray.showicon();

        } catch(e) {
            return;
        }

    }

}

function TrayShowTooltip(aMessage)
{
    if(!aMessage)
        return;

    if(gTray && gBaseWindow)
    {

        try {

        netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect")
;
        gTray.settooltipmsg(aMessage);

        } catch(e) {
            return;
        }

    }

}


function SetMenuItemsText()
{

  var manageKeys=getBundleString("menuManageKeys");

  var exit= getBundleString("menuExit");

  TrayChangeMenuItemText(0,manageKeys);
  TrayChangeMenuItemText(1,exit);

}

function TrayChangeMenuItemText(aIndex, aText)
{
    if(gTray && gBaseWindow)
    {
        try {
        netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect")
        gTray.setmenuitemtext(aIndex,aText);
        gTray.showicon();
        } catch(e) {
alert("exception " + e);
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


