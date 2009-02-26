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

const MY_CERTS_INDEX =  0;

function CertsTabsSelected()
{
    var tabs = document.getElementById("certMgrTabbox");

    if( !tabs)
       return;

    var value = tabs.selectedIndex;

    var importBtn = document.getElementById("mine_restoreButton");
    var deleteBtn = document.getElementById("mine_deleteButton");

    if(value == MY_CERTS_INDEX) {

       if(importBtn) {
           importBtn.setAttribute("hidden","true");
       }

       if(deleteBtn) {
           deleteBtn.setAttribute("hidden","true");
       }

    } else {

        if(importBtn) {
            importBtn.setAttribute("hidden","false");
        }
        if(deleteBtn) {
            deleteBtn.setAttribute("hidden","false");
        }

    }

}
