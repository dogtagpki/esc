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

//Evaulate Password Quality

function EvaluatePasswordQuality()
{
   var qualityImage = document.getElementById("password-image");
   var pw = document.getElementById("pintf").value;
   var pwlength = 0;

   var qualityMeter = document.getElementById("pass-progress-id");


   if(pw)
       pwlength = pw.length;

   if (pwlength>5)
    pwlength=5;

//use of numbers in the password
   var numnumeric = pw.replace (/[0-9]/g, "");
   var numeric=(pw.length - numnumeric.length);
   if (numeric>3)
     numeric=3;

//use of symbols in the password
   var symbols = pw.replace (/\W/g, "");
   var numsymbols=(pw.length - symbols.length);
   if (numsymbols>3)
     numsymbols=3;

//use of uppercase in the password
   var numupper = pw.replace (/[A-Z]/g, "");
   var upper=(pw.length - numupper.length);
   if (upper>3)
     upper=3;

   var pwstrength=((pwlength*10)-20) + (numeric*10) + (numsymbols*15) + (upper*10);

// make sure we're give a value between 0 and 100
  if ( pwstrength < 0 ) {
    pwstrength = 0;
  }
  if ( pwstrength > 100 ) {
    pwstrength = 100;
  }
   if(qualityMeter)
   {
       qualityMeter.setAttribute("value",  pwstrength);

   }
   if(qualityImage)
   {
        if(pwlength==0)
        {
           qualityImage.setAttribute("src","1-none.png");
           return;
        }
        if(pwstrength < 40)
        {
            qualityImage.setAttribute("src", "2-vweak.png");
            return;
        }
        if(pwstrength >= 40 && pwstrength < 50)
        {
            qualityImage.setAttribute("src","3-weak.png");
            return;
        }
        if(pwstrength >=50 && pwstrength < 60)
        {
            qualityImage.setAttribute("src","4-fair.png");
            return; 
        }
        if(pwstrength >= 60 && pwstrength < 80)
        {
           qualityImage.setAttribute("src","5-good.png");
           return;
         }
        if(pwstrength >= 80) 
           qualityImage.setAttribute("src","6-strong.png");
   } 
}

