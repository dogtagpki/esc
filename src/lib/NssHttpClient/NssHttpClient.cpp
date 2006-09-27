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

// NssHttpClient.cpp : Defines the entry point for the DLL application.
//
#include "NssHttpClient.h"

#include "HttpClientNss.h"
#include <prlock.h>


static int handleCount = 0;

PRLock*  clientTableLock = NULL;

#define NUM_CLIENTS 50


HttpClientNss *client_table[NUM_CLIENTS];

PRBool __EXPORT InitSecurity(char* dbpath, char* certname, char* certpassword,
                             char * prefix ,int verify=1);

#ifdef WINDOWS
#include "stdafx.h"
BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
                     )
{
    if(ul_reason_for_call == DLL_PROCESS_DETACH)
    {
        if(clientTableLock)
        {
            for(int i = 1; i < NUM_CLIENTS; i++)
            {
                httpDestroyClient(i);
            }

            PR_DestroyLock(clientTableLock);
            clientTableLock = NULL;
                
        }

    }
    return TRUE;
}
#endif


NSS_HTTP_CLIENT_API NSS_HTTP_HANDLE httpAllocateClient()
{

    if(handleCount == 0)
    {
        PRBool result = InitSecurity(NULL, NULL, NULL, NULL,1 );

        if(result == PR_FALSE)
        {
            return 0;
        }

        clientTableLock =  PR_NewLock();

        if(!clientTableLock)
        {
            return 0;

        }

        PR_Lock(clientTableLock);

        handleCount = 1;

    }
    else
    {
        PR_Lock(clientTableLock);
    }

    if(handleCount >= NUM_CLIENTS)
    {
        handleCount = 1;

        if(client_table[handleCount] != NULL)
        {
            PR_Unlock(clientTableLock);
            return 0;
        }

    }

    HttpClientNss *client = new HttpClientNss();

    if(!client)
    {
        PR_Unlock(clientTableLock);
        return 0;

    }
    
    client_table[handleCount] = client;

    int val = handleCount;
    handleCount++;
    
    PR_Unlock(clientTableLock);

    return val;

}

NSS_HTTP_CLIENT_API NSS_HTTP_RESULT httpDestroyClient(NSS_HTTP_HANDLE handle)
{

    if(!clientTableLock) {
        return 0;
    }

    PR_Lock(clientTableLock);

    if(handleCount > NUM_CLIENTS || handleCount <= 0)
    {
        PR_Unlock(clientTableLock);
        return 0;
    }

    HttpClientNss *client = client_table[handle];

    if(!client)
    {
        PR_Unlock(clientTableLock);
        return 1;
    }

    delete client;
    
    client_table[handle] = NULL;

    PR_Unlock(clientTableLock);

    return 1;
}

NSS_HTTP_CLIENT_API NSS_HTTP_RESULT httpSendChunked(char *host_port, char *uri, char *method, char *body,NSChunkedResponseCallback cb,void *cb_uw,NSS_HTTP_HANDLE handle,PRBool doSSL, int messageTimeout )
{
    NSS_HTTP_RESULT res = 0;

    if(!clientTableLock)
    {
        return res;
    }

    if(!handle)
    {
        return res;
    }
    
    PR_Lock(clientTableLock);

    HttpClientNss *client = NULL;

    client = client_table[handle];

    if(!client)
    {
        PR_Unlock(clientTableLock);
        return res;
    }

    PR_Unlock(clientTableLock);

    PSHttpResponse * resp = client->httpSendChunked(host_port,uri,method,body,cb,cb_uw,doSSL,messageTimeout);

    if(!resp)
    {
        res = 0;
    }
    else
    {
        res = 1;
    }

    return res;
}

NSS_HTTP_CLIENT_API NSS_HTTP_RESULT sendChunkedEntityData(int body_len,unsigned char *body,NSS_HTTP_HANDLE handle)
{
    if(!clientTableLock)
    {
        return 0;
    }

    if(!handle || handle >= NUM_CLIENTS || handle < 0)
    {
        return 0;
    }

    PR_Lock(clientTableLock);

    HttpClientNss *client = NULL;

    client = client_table[handle];

    if(!client)
    {
        PR_Unlock(clientTableLock);
        return 0;

    }

    PR_Unlock(clientTableLock);

    PRBool result = client->sendChunkedEntityData(body_len,body);

    return (int) result;

}

NSS_HTTP_CLIENT_API NSS_HTTP_RESULT httpCloseConnection(NSS_HTTP_HANDLE handle)
{
    if(!clientTableLock)
    {
        return 0;
    }

    
    PR_Lock(clientTableLock);

    HttpClientNss *client = NULL;

    client = client_table[handle];

    if(!client)
    {
        PR_Unlock(clientTableLock);
        return 0;

    }

    PR_Unlock(clientTableLock);


    client->CloseConnection();

    return 1;



}
