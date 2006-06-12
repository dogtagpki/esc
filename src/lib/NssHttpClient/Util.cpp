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
#include <prmem.h>
#include "pk11func.h"
#include "Util.h"


#ifdef XP_WIN32
#define NSAPI_PUBLIC __declspec(dllexport)
#else /* !XP_WIN32 */
#define NSAPI_PUBLIC
#endif /* !XP_WIN32 */

Util::Util ()
{
}

Util::~Util ()
{
}

NSAPI_PUBLIC int Util::ascii2numeric (char c)
{
    int num;
    switch (c) {
        case '0': case '1': case '2':case '3':case '4':case '5':
        case '6': case '7': case '8': case '9':
            num = c - '0';
            break;
        default:
            num = -1;
            break;
    }
    return num;
}
