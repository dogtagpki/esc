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

#include <sys/types.h>

#include <stdio.h>
#ifndef WIN32

#else 
#include <windows.h>
#endif 

#include "http.h"
#include "request.h"
#include "response.h"
#include "engine.h"

#include "string.h"

#include "HttpClientNss.h"



HttpClientNss::HttpClientNss()
{
    _request = NULL;
    _response = NULL;

    _engine = NULL;
    
}

HttpClientNss::~HttpClientNss()
{
    if(_response)
    {
        delete _response;
    }

    if(_engine)
    {
        delete _engine;
    }
}

/*
Send a http message with a persistant transfer chunked encoded message type

*/

PSHttpResponse *HttpClientNss::httpSendChunked(char *host_port, char *uri, char *method, char *body,PSChunkedResponseCallback cb,void *uw,PRBool doSSL,int messageTimeout )
{
    
    PSHttpServer server(host_port, PR_AF_INET);
 
    PSHttpRequest request( &server, uri, HTTP11, 0 );
    _request = &request;

    int timeout = 30;

    if(messageTimeout >= 0)
    {
        timeout = messageTimeout;

    }
    
    
    request.setSSL(doSSL);
   
    request.addHeader("Transfer-Encoding", (const char *) "chunked");
 
    request.addHeader( "Content-Type", "text/plain" );

    if(cb)
    {
        request.setChunkedCallback(cb);
        request.setChunkedCallbackUserWord(uw);
    }

    if(body)
    {
        request.setChunkedEntityData((int)strlen(body),body);
    }

    _engine = new HttpEngine();

    if(!_engine)
        return NULL;


    PSHttpResponse *resp =  _engine->makeRequest( request, server, timeout /*_timeout*/ , PR_FALSE /* expect chunked*/,PR_TRUE /* process streamed */);

    _response = resp;

    if(resp && resp->getStatus() != 200)
    {
        return NULL;

    }

    return resp;
    
}
void HttpClientNss::CloseConnection()
{
    if(_engine)
    {
        _engine->CloseConnection();
    }

}
PRBool HttpClientNss::sendChunkedEntityData(int body_len,unsigned char *body)
{
    int timeout = PR_TicksPerSecond()*60;

    char chunked_message[4096];
    if(body_len == 0 || !body || ((body_len + 50) > 4096))
    {
        return PR_FALSE;
    }

    if( !_request)
    {    
        return PR_FALSE;
    }

    HttpEngine *engine= getEngine();

    if(!engine)
    {
        return PR_FALSE;
    }
    
    PRFileDesc *socket = _engine->getSocket();

    if(!socket)
    {
        return PR_FALSE;
    }

    sprintf(chunked_message,"%X\r\n%s\r\n",body_len,body);

    int bytes = PR_Send(socket, chunked_message, strlen(chunked_message), 0, timeout);

    if(bytes < 0)
    {
        return PR_FALSE;
    }
    else
    {
        return PR_TRUE;
    }
}


