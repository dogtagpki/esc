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

#ifndef __NSS_HTTP_CLIENT_H__
#define __NSS_HTT_CLIENT_H__

// The following ifdef block is the standard way of creating macros which make
// exporting from a DLL simpler. All files within this DLL are compiled with
// the COOLKEY_EXPORTS symbol defined on the command line. this symbol should
// not be defined on any project that uses this DLL. This way any other project
// whose source files include this file see COOLKEY_API functions as being
// imported from a DLL, whereas this DLL sees symbols defined with this macro
// as being exported.

#include "nspr.h"

#define NSS_HTTP_CLIENT_API

#define NSS_HTTP_CHUNK_COMPLETE 1
#define NSS_HTTP_CHUNKED_EOF    2

typedef bool ( *NSChunkedResponseCallback)(unsigned char *entity_data,unsigned entity_data_len,void *uw, int status);

typedef int NSS_HTTP_RESULT;

typedef int NSS_HTTP_HANDLE;

extern "C" {

NSS_HTTP_CLIENT_API NSS_HTTP_HANDLE httpAllocateClient();

NSS_HTTP_CLIENT_API NSS_HTTP_RESULT httpDestroyClient(NSS_HTTP_HANDLE handle);

NSS_HTTP_CLIENT_API NSS_HTTP_RESULT httpSendChunked(char *host_port, char *uri, char *method, char *body,NSChunkedResponseCallback cb,void *cb_uw,NSS_HTTP_HANDLE handle,PRBool doSSL = PR_FALSE,int messageTimeout = 30);

NSS_HTTP_CLIENT_API NSS_HTTP_RESULT sendChunkedEntityData(int body_len,unsigned char *body,NSS_HTTP_HANDLE handle);

NSS_HTTP_CLIENT_API NSS_HTTP_RESULT httpCloseConnection(NSS_HTTP_HANDLE handle);

}



#endif
