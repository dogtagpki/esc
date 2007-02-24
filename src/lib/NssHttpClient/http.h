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

#ifndef _HTTP_SERVER_
#define _HTTP_SERVER_

#include <stdlib.h>
#include <prnetdb.h>
#include <prio.h>
#include <time.h>
#include <plhash.h>
#include <nspr.h>
#include <plstr.h>

#include "PSCommonLib.h"
#include "Cache.h"
#include "Defines.h"
//#include "httpClient/httpc/DebugLogger.h"
//#include "httpClient/httpc/ErrorLogger.h"


#ifdef WIN32
#define __EXPORT __declspec(dllexport)
#else
#define __EXPORT
#endif

extern "C" {

char *GetTStamp(char *aTime,int aSize);

}


typedef bool ( *PSChunkedResponseCallback)(unsigned char *entity_data,unsigned entity_data_len,void *uw, int status);

class PSHttpResponse;

PSHttpResponse *httpSendChunked(char *host_port, char *uri, char *method, char *body,PSChunkedResponseCallback cb,void *uw);

class PSHttpRequest;

class __EXPORT PSHttpServer
{
public:
    PSHttpServer(const char *addr, PRUint16 af);
    ~PSHttpServer();

    long getIp() const;
    long getPort() const;
    const char *getAddr() const;
    void getAddr(PRNetAddr *addr) const;
    void setSSL(PRBool SSLstate);
    PRBool isSSL() const;

    // put a file on the server of size bytes
    PRBool putFile(const char *uri, int size) const;
    PRBool putFile(const char* uri, const char* localFile) const;

private:
    char *_addr;
    PRNetAddr _netAddr;
    PRBool SSLOn;
    PRBool _putFile(PSHttpRequest& rq) const;
    char *_host;
};

typedef __EXPORT enum HttpProtocol_e { HTTPNA    = 0x0, 
                                       HTTP09    = 0x1, 
                                       HTTP10    = 0x2, 
                                       HTTP11    = 0x4, 
                                       HTTPBOGUS = 0x8 } HttpProtocol;

#define NUM_PROTOS 5 // needed for arrays of tests

__EXPORT const char *HttpProtocolToString(HttpProtocol);

class __EXPORT HttpMessage
{
    public:
        HttpMessage(long len = 0, const char* buf = NULL);
        ~HttpMessage();

        PRBool          operator == (const HttpMessage& rhs);

        void addData(long len, const void* buf);

        // set data on the message
        void            setProtocol(HttpProtocol prot);

        // get data about the message
        HttpProtocol    getProtocol() const;


    protected:
        char*               firstline; // first line - may be the request-line or server status
        HttpProtocol        proto;
        long                cl;
};


#endif
