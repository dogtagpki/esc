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

var parentWindow = window.opener;
var gStringBundle=null;

loadStringBundle();

function doOperation()
{
  //alert("doOperation opener " + parentWindow + " nam " + window.name);

  var pin = GetLocalPINValue();

  if(!pin)
      return;

  parentWindow.SetPINValue(pin);

  var theOperation = window.name;

  if(theOperation == "resetpin") 
     parentWindow.DoResetSelectedCoolKeyPIN();

  if(theOperation == "enroll")
     parentWindow.DoEnrollCoolKey();

  window.close(); 
}


function GetLocalPINValue()
{

  var pintf_obj = document.getElementById("pintf");
  var reenterpintf_obj = document.getElementById("reenterpintf");


  var pinVal = null;
  var rpinVal = null;


  if(pintf_obj)
       pinVal =  pintf_obj.value;

  if(reenterpintf_obj)
      rpinVal =  reenterpintf_obj.value;

  if (!pinVal && pintf_obj)
  {
      MyAlert(getBundleString("errorProvideTokenPIN"));
      return null;
  }

  if ( pinVal != rpinVal )
  {
      MyAlert(getBundleString("errorMatchPinValues"));
      return null;
  }

  return pinVal;
}

function PasswordLoad()
{
    window.sizeToContent();
    var pintf_obj = document.getElementById("pintf");
    if(pintf_obj)
        pintf_obj.focus();
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

