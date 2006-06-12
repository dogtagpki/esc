/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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

#ifndef _HTTP_ENGINE_
#define _HTTP_ENGINE_
#include "http.h"
#include "response.h"
#include "request.h"

class __EXPORT Engine {
    public:
        Engine() {};
        ~Engine() {};

        PRFileDesc *_doConnect(PRNetAddr *addr, PRBool SSLOn = PR_FALSE,
                               const PRInt32* cipherSuite = NULL, 
                               PRInt32 count = 0, const char* nickname = NULL,
                               PRBool handshake = PR_FALSE,
                               /*const SecurityProtocols& secprots = SecurityProtocols() ,*/
                               const char *serverName ="localhost",
                               PRIntervalTime iv = PR_SecondsToInterval(30));
        static PRIntervalTime globaltimeout;

        PRFileDesc *_sock;

        PRFileDesc *getSocket() { return _sock;}

        bool connectionClosed ;
        void CloseConnection();

        bool isConnectionClosed() { return connectionClosed;}
};


class __EXPORT HttpEngine: public Engine {
    public:
        HttpEngine() {};
        ~HttpEngine() {};

        PSHttpResponse *makeRequest( PSHttpRequest &request,
             const PSHttpServer& server,
             int timeout = 30, PRBool expectChunked = PR_FALSE,PRBool processStreamed = PR_FALSE);
};

PRBool __EXPORT InitSecurity(char* dbpath, char* certname, char* certpassword,
                             char * prefix ,int verify=1);
PRBool __EXPORT EnableCipher(const char* ciphername);
void  __EXPORT EnableAllSSL3Ciphers();
__EXPORT const char * nscperror_lookup(int error);

#endif
