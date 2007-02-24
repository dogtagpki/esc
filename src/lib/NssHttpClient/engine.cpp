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

#include <nspr.h>
#include "sslproto.h"
#include <prerror.h>

#include "ssl.h"
#include "nss.h"
#include "pk11func.h"
#include "cert.h"
#include "certt.h"
#include "sslerr.h"
#include "secerr.h"

#include "engine.h"
#include "http.h"

#include "Defines.h"

char* certName = NULL;
char* password = NULL;
int ciphers[32];
int cipherCount = 0;
int _doVerifyServerCert = 1;

PRIntervalTime Engine::globaltimeout = PR_TicksPerSecond()*30;

/**
 * Function: SECStatus myBadCertHandler()
 * <BR>
 * Purpose: This callback is called when the incoming certificate is not
 * valid. We define a certain set of parameters that still cause the
 * certificate to be "valid" for this session, and return SECSuccess to cause
 * the server to continue processing the request when any of these conditions
 * are met. Otherwise, SECFailure is return and the server rejects the 
 * request.
 */
SECStatus myBadCertHandler( void *arg, PRFileDesc *socket ) {

    SECStatus    secStatus = SECFailure;
    PRErrorCode    err;

    /* log invalid cert here */

    if ( !arg ) {
        return secStatus;
    }

    *(PRErrorCode *)arg = err = PORT_GetError();

    /* If any of the cases in the switch are met, then we will proceed   */
    /* with the processing of the request anyway. Otherwise, the default */    
    /* case will be reached and we will reject the request.              */

    switch (err) {
    case SEC_ERROR_INVALID_AVA:
    case SEC_ERROR_INVALID_TIME:
    case SEC_ERROR_BAD_SIGNATURE:
    case SEC_ERROR_EXPIRED_CERTIFICATE:
    case SEC_ERROR_UNKNOWN_ISSUER:
    case SEC_ERROR_UNTRUSTED_CERT:
    case SEC_ERROR_CERT_VALID:
    case SEC_ERROR_EXPIRED_ISSUER_CERTIFICATE:
    case SEC_ERROR_CRL_EXPIRED:
    case SEC_ERROR_CRL_BAD_SIGNATURE:
    case SEC_ERROR_EXTENSION_VALUE_INVALID:
    case SEC_ERROR_CA_CERT_INVALID:
    case SEC_ERROR_CERT_USAGES_INVALID:
    case SEC_ERROR_UNKNOWN_CRITICAL_EXTENSION:
    case SEC_ERROR_EXTENSION_NOT_FOUND: // Added by Rob 5/21/2002
        secStatus = SECSuccess;
    break;
    default:
        secStatus = SECFailure;
    break;
    }

    return secStatus;
}


PRBool __EXPORT InitSecurity(char* certDir, char* certname, char* certpassword, char *prefix,int verify ) {
  
    password = NULL;

    PR_Init( PR_USER_THREAD, PR_PRIORITY_NORMAL, 0 );

    SECStatus stat;  
    
    stat = NSS_SetDomesticPolicy();
    SSL_CipherPrefSetDefault( SSL_RSA_WITH_NULL_MD5, PR_TRUE );

    _doVerifyServerCert = verify;

     return PR_TRUE;
}


int ssl2Suites[] = {
    SSL_EN_RC4_128_WITH_MD5,                    /* A */
    SSL_EN_RC4_128_EXPORT40_WITH_MD5,           /* B */
    SSL_EN_RC2_128_CBC_WITH_MD5,                /* C */
    SSL_EN_RC2_128_CBC_EXPORT40_WITH_MD5,       /* D */
    SSL_EN_DES_64_CBC_WITH_MD5,                 /* E */
    SSL_EN_DES_192_EDE3_CBC_WITH_MD5,           /* F */
    0
};

int ssl3Suites[] = {
    SSL_FORTEZZA_DMS_WITH_FORTEZZA_CBC_SHA,     /* a */
    SSL_FORTEZZA_DMS_WITH_RC4_128_SHA,          /* b */
    SSL_RSA_WITH_RC4_128_MD5,                   /* c */
    SSL_RSA_WITH_3DES_EDE_CBC_SHA,              /* d */
    SSL_RSA_WITH_DES_CBC_SHA,                   /* e */
    SSL_RSA_EXPORT_WITH_RC4_40_MD5,             /* f */
    SSL_RSA_EXPORT_WITH_RC2_CBC_40_MD5,         /* g */
    SSL_FORTEZZA_DMS_WITH_NULL_SHA,             /* h */
    SSL_RSA_WITH_NULL_MD5,                      /* i */
    SSL_RSA_FIPS_WITH_3DES_EDE_CBC_SHA,         /* j */
    SSL_RSA_FIPS_WITH_DES_CBC_SHA,              /* k */
    TLS_RSA_EXPORT1024_WITH_DES_CBC_SHA,        /* l */
    TLS_RSA_EXPORT1024_WITH_RC4_56_SHA,         /* m */
    0
};

void disableAllCiphersOnSocket(PRFileDesc* sock) {
    int i;
    int numsuites = SSL_NumImplementedCiphers;

    /* disable all the cipher suites for that socket */
    for (i = 0; i<numsuites; i++) {
        SSL_CipherPrefSet(sock, SSL_ImplementedCiphers[i], SSL_NOT_ALLOWED);
    }
}

void __EXPORT EnableAllSSL3Ciphers(PRFileDesc* sock) {
    int i =0;
    while (ssl3Suites[i]) {
        SSL_CipherPrefSet(sock, ssl3Suites[i], SSL_ALLOWED);
    }
}
 
PRBool __EXPORT EnableCipher(const char* cipherString) {
     int ndx;
  
     if (!cipherString) {
        return PR_FALSE;
     }

     while (0 != (ndx = *cipherString++)) {
        int* cptr;
        int cipher;

        if (! isalpha(ndx)) {
           continue;
        }
        cptr = islower(ndx) ? ssl3Suites : ssl2Suites;
        for (ndx &= 0x1f; (cipher = *cptr++) != 0 && --ndx > 0; ) {
            /* do nothing */;
        }
        ciphers[cipherCount++] = cipher;
     }

     return PR_TRUE;
}

SECStatus certcallback (
    void *arg,
    PRFileDesc *fd,
    PRBool checksig,
    PRBool isServer) {
     return SECSuccess; // always succeed
}

/**
 * Function: SECStatus myAuthCertificate()
 * <BR>
 * Purpose: This function is our custom certificate authentication handler.
 * <BR>
 * Note: This implementation is essentially the same as the default 
 *       SSL_AuthCertificate().
 */
extern "C" {
static SECStatus myAuthCertificate( void *arg,
                             PRFileDesc *socket, 
                             PRBool checksig,
                             PRBool isServer ) {

    SECCertUsage        certUsage;
    CERTCertificate *   cert;
    void *              pinArg;
    char *              hostName = NULL;
    SECStatus           secStatus = SECSuccess;

    if ( !arg || !socket ) {
        return SECFailure;
    }

    /* Define how the cert is being used based upon the isServer flag. */

    certUsage = isServer ? certUsageSSLClient : certUsageSSLServer;

    cert = SSL_PeerCertificate( socket );
    
    pinArg = SSL_RevealPinArg( socket );

    // Skip the server cert verification fconditionally, because our test
    // servers do not have a valid root CA cert.
    if ( _doVerifyServerCert ) {


        secStatus = CERT_VerifyCertNow( (CERTCertDBHandle *)arg,
                                        cert,
                                        checksig ,
                                        certUsage,
                                        pinArg );
        if( SECSuccess != secStatus ) {

        }
    }

    /* If this is a server, we're finished. */
    if (isServer || secStatus != SECSuccess) {
        return secStatus;
    }

    /* Certificate is OK.  Since this is the client side of an SSL
     * connection, we need to verify that the name field in the cert
     * matches the desired hostname.  This is our defense against
     * man-in-the-middle attacks.
     */

    /* SSL_RevealURL returns a hostName, not an URL. */
    hostName = SSL_RevealURL( socket );

    if (hostName && hostName[0]) {
        secStatus = CERT_VerifyCertName( cert, hostName );
        if( SECSuccess != secStatus ) {

        }
    } else {
        secStatus = SECFailure;

    }

    if( hostName != NULL ) {
        PR_Free( hostName );
        hostName = NULL;
    }

    return secStatus;
}


/* Function: SECStatus ownGetClientAuthData()
 *
 * Purpose: This callback is used by SSL to pull client certificate 
 * information upon server request.
 */
static SECStatus ownGetClientAuthData(void *arg, PRFileDesc *socket,
                    CERTDistNames *caNames,
                    CERTCertificate **pRetCert,/*return */
                    SECKEYPrivateKey **pRetKey) {
    CERTCertificate *               cert = NULL;
    SECKEYPrivateKey *              privKey = NULL;
    void *                          proto_win = NULL;
    SECStatus                       rv = SECFailure;
    char *                localNickName = (char *)arg;

    proto_win = SSL_RevealPinArg(socket);
  
 
    if (localNickName) {
      
     cert = PK11_FindCertFromNickname(localNickName, proto_win);
        if (cert) {
      
            privKey = PK11_FindKeyByAnyCert(cert, proto_win);
            if (privKey) {
             
                    rv = SECSuccess;
            } else {
                    if( cert != NULL ) {
                        CERT_DestroyCertificate( cert );
                        cert = NULL;
                    }
            }
        }
        else {
           
        }

        if (rv == SECSuccess) {
            *pRetCert = cert;
            *pRetKey  = privKey;
        }

        
        return rv;
    }
    else {
          
    }

    char* chosenNickName = certName ? (char *)PL_strdup(certName) : NULL;
    if (chosenNickName) {
        cert = PK11_FindCertFromNickname(chosenNickName, proto_win);
        if (cert) {
            privKey = PK11_FindKeyByAnyCert(cert, proto_win);
            if (privKey) {
                rv = SECSuccess;
            } else {
                if( cert != NULL ) {
                    CERT_DestroyCertificate( cert );
                    cert = NULL;
                }
            }
        }
    } else {
        /* no nickname given, automatically find the right cert */
        CERTCertNicknames *     names;
        int                     i;

        names = CERT_GetCertNicknames(  CERT_GetDefaultCertDB(), 
                                        SEC_CERT_NICKNAMES_USER,
                                        proto_win);

        if (names != NULL) {
            for( i=0; i < names->numnicknames; i++ ) {


                cert = CERT_FindUserCertByUsage(CERT_GetDefaultCertDB(),names->nicknames[i],certUsageSSLClient,PR_FALSE,proto_win);


                if (!cert) {
                    continue;
                }

                /* Only check unexpired certs */
                if (CERT_CheckCertValidTimes(cert, PR_Now(), PR_FALSE) != 
                    secCertTimeValid) {
                    if( cert != NULL ) {
                        CERT_DestroyCertificate( cert );
                        cert = NULL;
                    }
                    continue;
                }

                rv = NSS_CmpCertChainWCANames(cert, caNames);

                if (rv == SECSuccess) {
                    privKey = PK11_FindKeyByAnyCert(cert, proto_win);
                    if (privKey) {
                        // got the key
                        break;
                    }

                    // cert database password was probably wrong
                    rv = SECFailure;
                    break;
                };
            } /* for loop */
            CERT_FreeNicknames(names);
        } // names
    } // no nickname chosen

    if (rv == SECSuccess) {
        *pRetCert = cert;
        *pRetKey  = privKey;
    }

    if( chosenNickName != NULL ) {
        free( chosenNickName );
        chosenNickName = NULL;
    }

    return rv;
}
} // extern "C"

void nodelay(PRFileDesc* fd) {
    PRSocketOptionData opt;
    PRStatus rv;

    opt.option = PR_SockOpt_NoDelay;
    opt.value.no_delay = PR_FALSE;

    rv = PR_GetSocketOption(fd, &opt);
    if (rv == PR_FAILURE) {
        return;
    }

    opt.option = PR_SockOpt_NoDelay;
    opt.value.no_delay = PR_TRUE;
    rv = PR_SetSocketOption(fd, &opt);
    if (rv == PR_FAILURE) {
        return;
    }

    return;
}


void Engine::CloseConnection()
{
    connectionClosed = true;

    if(_sock)
    {
        PR_Close(_sock);
        _sock = NULL;
    }
}
/**
 * Returns a file descriptor for I/O if the HTTP connection is successful
 * @param addr PRnetAddr structure which points to the server to connect to
 * @param SSLOn boo;elan to state if this is an SSL client
 */
PRFileDesc * Engine::_doConnect(PRNetAddr *addr, PRBool SSLOn,
                                const PRInt32* cipherSuite, 
                                PRInt32 count, const char *nickName,
                                PRBool handshake,
                                /*const SecurityProtocols& secprots,*/
                                const char *serverName, PRIntervalTime timeout) {

    PRFileDesc *tcpsock = NULL;
    PRFileDesc *sock = NULL;
    connectionClosed = false;

    tcpsock = PR_OpenTCPSocket(addr->raw.family);
   

    if (!tcpsock) {

        return NULL;
    }

    nodelay(tcpsock);

    if (PR_TRUE == SSLOn) {
        sock=SSL_ImportFD(NULL, tcpsock);


        if (!sock) {
            //xxx log
            if( tcpsock != NULL ) {
                PR_Close( tcpsock );
                tcpsock = NULL;
            }
            return NULL;
        }

        SSL_SetPKCS11PinArg (sock,0);

        int error = 0;
        PRBool rv = SSL_OptionSet(sock, SSL_SECURITY, 1);
        if ( SECSuccess == rv ) {
            rv = SSL_OptionSet(sock, SSL_HANDSHAKE_AS_CLIENT, 1);
        }
        if ( SECSuccess == rv ) {
            rv = SSL_OptionSet(sock, SSL_ENABLE_SSL3, PR_TRUE);
        }
        if ( SECSuccess == rv ) {
            rv = SSL_OptionSet(sock, SSL_ENABLE_TLS, PR_TRUE);
        }
        if ( SECSuccess != rv ) {
            error = PORT_GetError();
            if( sock != NULL ) {
                PR_Close( sock );
                sock = NULL;
            }

            return NULL;
        }

        rv = SSL_GetClientAuthDataHook( sock,
                                        ownGetClientAuthData,
                                        (void*)nickName);
        if ( SECSuccess != rv ) {
            error = PORT_GetError();
            if( sock != NULL ) {
                PR_Close( sock );
                sock = NULL;
            }

            return NULL;
        }
        
        rv = SSL_AuthCertificateHook(sock,
                                     (SSLAuthCertificate)myAuthCertificate,
                                     (void *)CERT_GetDefaultCertDB()); 

        if (rv != SECSuccess ) {
            if( sock != NULL ) {
                PR_Close( sock );
                sock = NULL;
            }
            return NULL;
        }

        PRErrorCode errCode = 0;

        rv = SSL_BadCertHook( sock,
                              (SSLBadCertHandler)myBadCertHandler,
                              &errCode );
        rv = SSL_SetURL( sock, serverName );

        if (rv != SECSuccess ) {
            error = PORT_GetError();
            if( sock != NULL ) {
                PR_Close( sock );
                sock = NULL;
            }

            return NULL;
        }

        //EnableAllSSL3Ciphers( sock);
    } else {
        sock = tcpsock;
    }

  

    if ( PR_Connect(sock, addr, timeout) == PR_FAILURE ) {

        if( sock != NULL ) {
            PR_Close( sock );
            sock = NULL;
        }
        return NULL;
    }

    return (sock);
}

/**
 * Called from higher level to connect, sends a request 
 * and gets a response as an HttpResponse object
 *
 * @param request Contains the entire request url + headers etc
 * @param server Has the host, port, protocol info
 * @param timeout Time in seconds to wait for a response
 * @return The response body and headers
 */
PSHttpResponse * HttpEngine::makeRequest( PSHttpRequest &request, 
                                          const PSHttpServer& server,
                                          int timeout, PRBool expectChunked ,PRBool processStreamed) {
    PRNetAddr addr;
    PRFileDesc *sock = NULL;
    PSHttpResponse *resp = NULL;

    PRBool response_code = 0;

    server.getAddr(&addr);

    char *nickName = request.getCertNickName();

    char *serverName = (char *)server.getAddr();
    _sock = _doConnect( &addr, request.isSSL(), 0, 0,nickName, 0, serverName );

    if ( _sock != NULL) {
        PRBool status = request.send( _sock );
        if ( status ) {
            resp = new PSHttpResponse( _sock, &request, timeout, expectChunked ,this);
            response_code = resp->processResponse(processStreamed);

            if(!response_code)
            {
               
                if( resp != NULL ) {
                    delete resp;
                    resp = NULL;
                }
                if( _sock != NULL ) {
                    PR_Close( _sock );
                    _sock = NULL;
                }

                return NULL;

            }
        }
        if( _sock != NULL ) {
            PR_Close( _sock );
            _sock  = NULL;
        }
    }

    return resp;
}
