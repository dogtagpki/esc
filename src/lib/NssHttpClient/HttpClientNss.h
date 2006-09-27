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

#ifndef _HTTP_CLIENT_NSS_H_
#define _REQUEST_H_

#include "nspr.h"
#include "http.h"
class HttpEngine;

class HttpClientNss
{
public:
    HttpClientNss();
    ~HttpClientNss();
    
    HttpEngine  *_engine;

    PSHttpRequest *        _request;
    PSHttpResponse*        _response;

    PSHttpResponse *httpSendChunked(char *host_port, char *uri, char *method, char *body,PSChunkedResponseCallback cb,void *uw,PRBool doSSL = PR_FALSE,int messageTimeout =30);

    PRBool sendChunkedEntityData(int body_len,unsigned char * body);
    HttpEngine *getEngine() { return _engine;}

    void CloseConnection();
};

#endif
