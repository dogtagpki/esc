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

#define FORCE_PR_LOG 1

/**
 * HTTP response handler
 */

#include <ctype.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "nspr.h"


#include "response.h"
#include "engine.h"


#ifdef WINDOWS
#include "stdafx.h"
#endif
#include "Util.h"

PRLogModuleInfo *httpRespLog = PR_NewLogModule("coolKeyHttpRes");

/**
 * Constructor. This class is used by  the HttpResponse class for reading and
 * processing data from the socket
 * @param socket The NSPR socket from which the response is expected
 * @param size The size of the internal buffer to hold data
 * @param timeout Timeout in seconds on receiving a response
 */

RecvBuf::RecvBuf( const PRFileDesc *socket, int size, int timeout ,PSHttpResponse *theClient ,PRBool theStreamMode) {
    _socket = socket;
    _allocSize = size;
    _buf = (char *)PR_Malloc(size);
    _curPos = 0;
    _curSize = 0;
    // sns can't do "chunked", but ca needs "chunked"
    _chunkedMode = PR_FALSE;
    _currentChunkSize = _currentChunkBytesRead = 0;
    _timeout = PR_TicksPerSecond() * timeout;
    _content = NULL;
    _client = theClient;
    _streamMode = PR_FALSE;

    if(!_chunkedMode)
    {
        _streamMode = theStreamMode;
    }
}

/**
 * Destructor
 */
RecvBuf::~RecvBuf() {
    if( _buf != NULL ) {
        PR_Free( _buf );
        _buf = NULL;
    }
}

/**
 * Reads the specified number of bytes from the socket and place it into the buffer
 *
 * @param socket The NSPR socket from which the response is expected
 * @param size The size of the buffer
 * @return PR_TRUE on success, otherwise PR_FALSE
 */
PRBool RecvBuf::_getBytes(int size) {
//--     DebugLogger *logger = DebugLogger::GetDebugLogger( DEBUG_MODULE );
    PRErrorCode pec;
    _curPos = 0;

    int num =1;
    int i =0;
   
    char tBuff[56]; 
    PRBool endChunk= PR_FALSE;

    // actual reading from the socket happens here
    do {
        

        num = PR_Recv( (PRFileDesc*)_socket, 
                       &_buf[_curPos],//_curSize], 
                       _allocSize,//-_curSize, 
                       0, 
                       _timeout );

        PR_LOG( httpRespLog, PR_LOG_DEBUG, 
                              ("%s RecvBuf::_getBytes:: read  %d bytes\n",GetTStamp(tBuff,56),num));

        if(num < 0)
        {
            PR_LOG( httpRespLog, PR_LOG_DEBUG, 
                              ("%s RecvBuf::_getBytes:: Conn Closed ",GetTStamp(tBuff,56)));
            return PR_FALSE;
        }

        /*
         * in chunked mode, ending chunk contains a 0 to begin
         * loop through to see if it contains just 0 (skip carriage returns
         * endChunk indicates possible end chunk.
         */
        if ((_chunkedMode == PR_TRUE) && (num < 10)) {
            endChunk = PR_FALSE;

            for (i=0; i< num; i++) {
                if (endChunk == PR_TRUE) {
                    if ((_buf[_curSize+i] == 13) || (_buf[_curSize+i] == 10))
                        continue;
                    else {
                        endChunk = PR_FALSE;
                        break; // not an endChunk
                    }
                } else { // endChunk==PR_FALSE
                    if (_buf[_curSize+i] == '0') {
                      
                        endChunk = PR_TRUE;
                    } else if ((_buf[_curSize+i] == 13) || (_buf[_curSize+i] == 10))
                        continue;
                    else {
                        endChunk = PR_FALSE;
                        break; // not an endChunk
                    }
                }
            } // for
        }

        if (num >0)
            _curSize = num; // _curSize+num;

        if (_chunkedMode == PR_FALSE) {
            if (getAllContent()) {
              //  RA::Debug( LL_PER_PDU,
                //           "RecvBuf::_getBytes: ",
                   //        "Already got all the content, no need to call PR_Recv again." );
                break;
            }
        }

        if (endChunk == PR_TRUE)
            break;
    } while (num > 0 && !_streamMode);

    if (num <0) {
        pec = PR_GetError();
   
    }

    if ( _curSize <= 0 ) {
        return PR_FALSE;
    }
    
    _buf[_curSize] = '\0';

    if(!_streamMode )
    {
        _content = (char *) PR_Malloc(_curSize+1);
        if (_content == NULL) 
             return PR_FALSE;
    
        memcpy((char*) _content, (const char *)_buf, _curSize+1);
        _contentSize = _curSize +1;

    }

    return PR_TRUE;
}

int RecvBuf::getAllContent() {
    int result[10];
    int j=0;
    int k=0;
    int number = 0;
    for (int i=0; i<_curSize; i++) {
        if (_buf[i] == '\r') {
            if (i < (_curSize-3)) {
                if (_buf[i+1] == '\n' && _buf[i+2] == '\r' 
                  && _buf[i+3] == '\n') {
                    // find content length
                    char *clen = strstr(_buf, "Content-length:");
                    if (clen != NULL) {
                        clen = &clen[16];
                        while (1) {
                            if ((number=Util::ascii2numeric(clen[j++])) >= 0) {
                                result[k++] = number;
                            } else {
                                break;
                            }
                        }

                        number = 0;
                        for (int l=0; l<k; l++)
                            number = (int)(number + result[l]*(float)pow((float)10, (float)k-l-1));
              
                    }
                    int remainingBytes = _curSize - (i+4);
                  
                    if (remainingBytes == number) 
                        return 1;
                }
            }
        }
    }

    return 0;
}

/**
 * gets the next char from the buffer. If all the data in the buffer is read,
 * read a chunk to the buffer
 * @returns - the next char from the data
 */
char RecvBuf::_getChar() {
    if (_curPos >= _curSize) {
        if (!_getBytes(_allocSize)) {
            return -1; 
        }
    }
    
    return _buf[_curPos++];
}


/**
 * gets the next char from the buffer. If all the data in the buffer is read , 
 * read a chunk to the buffer
 * @returns - the next char from the data
 */
char RecvBuf::getChar() {
    char tBuff[56];
    if (!_chunkedMode)
        return _getChar();

    else
    {
        if (_currentChunkSize == 0)
        {
            // read the chunk header
            char ch, chunkStr[20];
            int index = 0;
          
            while (!isspace(ch = _getChar()) )
                chunkStr[index++] = ch;
            chunkStr[index] = '\0';

            sscanf((char *)chunkStr, "%x", (unsigned int *)(&_currentChunkSize));

            if (ch != '\n')
            {
                char ch2 = _getChar();
                if (ch != '\r' || ch2 != '\n')
                {
                     PR_LOG(httpRespLog, PR_LOG_DEBUG,
                          ("%s did not find chunk trailer at end of chunk .  \n",GetTStamp(tBuff,56)));
                }
            }
       
            if (_currentChunkSize == 0)
                return -1;

            //handle the freakish case where we
            // get a zero right after the chunk size \r \n

             
             char ch3 = _getChar(); 
             if(ch3 != '0')
             {
                 putBack();
             }

            _currentChunkBytesRead = 1;
            return _buf[_curPos++];
        }
        else
            if (_currentChunkBytesRead < _currentChunkSize)
            {
                // read a byte from the chunk
                _currentChunkBytesRead++;
                return _getChar();
            }
            else
            {
                // read the chunk trailer
                char ch1 = _getChar();
                char ch2 = _getChar();
                if (ch1 != '\r' || ch2 != '\n')
                {
                    PR_LOG(httpRespLog, PR_LOG_DEBUG,
                          ("%s did not find chunk trailer at the end of chunk . ch1 %c ch2 %c  \n",GetTStamp(tBuff,56),ch1,ch2));
                };

                _currentChunkSize = _currentChunkBytesRead = 0;
                if(_streamMode == PR_TRUE)
                {
                    char next;

                    if(_curPos < _curSize)
                    {
                        next = _getChar();

                        if(next == '0')
                        {
                            putBack();
                            return -1;
                        }
                    
                    }
                                    
                    return '\n';
                }

                return getChar();
            };
    };

}

char *RecvBuf::get_content() {
    return _content;
}

int RecvBuf::get_contentSize() {
    return _contentSize;
}

/**
 * Decrements the pointer to the internal buffer so that the next read would
 * retrieve the last data again
 */
void RecvBuf::putBack() {
    if (_curPos > 0) {
        _curPos--;
        if (_chunkedMode) {
            _currentChunkBytesRead--;
        }
    }
}

/**
 * Sets the chunked mode for reading data
 * Not used now..
 */
void RecvBuf::setChunkedMode() {
    _chunkedMode = PR_TRUE;
    _currentChunkSize = _currentChunkBytesRead = 0;
}

/**
 * Gets the timeout in seconds for reading
 *
 * @return The timeout in seconds for reading
 */
int RecvBuf::getTimeout() {
    return _timeout / PR_TicksPerSecond();
}


Response::Response(const PRFileDesc *sock, NetRequest *request) {
    _socket = sock;
    _request = request;
}

/**
 * Constructor
 */

PSHttpResponse::PSHttpResponse( const PRFileDesc *sock,
                                PSHttpRequest *request,
                                int timeout , PRBool expectChunked,HttpEngine *engine):
    Response(sock, request) {
    _request = request;
    _proto = HTTPNA;
    _protocol = NULL;
     retcode =0 ;
    _statusNum = NULL;
    _statusString = NULL;
    _keepAlive = -1;
    _connectionClosed = 0;
    _bodyLength = -1;
    _content = NULL;

    _headers = new StringKeyCache("response",10*60);
    _expectChunked = expectChunked;
    _chunkedResponse = PR_FALSE;
    _timeout = timeout;
    _client = engine;
}

PSHttpResponse::~PSHttpResponse() {
    if( _protocol != NULL ) {
        PL_strfree( _protocol );
        _protocol = NULL;
    }
    if( _statusString != NULL ) {
        PL_strfree( _statusString );
        _statusString = NULL;
    }
    if( _statusNum != NULL ) {
        PL_strfree( _statusNum );
        _statusNum = NULL;
    }
    if (_headers) {
        Iterator* iterator = _headers->GetKeyIterator();
        while ( iterator->HasMore() ) {
            const char* name = (const char*)iterator->Next();
            CacheEntry* entry = _headers->Remove( name );
            if ( entry ) {
                char* value = (char*)entry->GetData();
                if( value != NULL ) {
                    PL_strfree( value );
                    value = NULL;
                }
                if( entry != NULL ) {
                    delete entry;
                    entry = NULL;
                }
            }
        }
        if( iterator != NULL ) {
            delete iterator;
            iterator = NULL;
        }
        if( _headers != NULL ) {
            delete _headers;
            _headers = NULL;
        }
    }
    _socket = 0;
}

long PSHttpResponse::getStatus() {
    return _statusNum ? atoi(_statusNum) : 0;
}

int PSHttpResponse::getReturnCode() {
    return retcode;
}

char * PSHttpResponse::getStatusString() {
    return _statusString?_statusString:(char*)"";
}

HttpProtocol PSHttpResponse::getProtocol() {
    // first check the response protocol
    if (_proto == HTTPNA) {
        if (_protocol) {
            int major, minor;

            sscanf(_protocol, "HTTP/%d.%d", &major, &minor);

            switch(major) {
            case 1:
                switch(minor) {
                case 0:
                    _proto = HTTP10;
                    break;
                case 1:
                    _proto = HTTP11;
                    break;
                }
                break;
            }
        } else {
            _proto = HTTP09;
        }
    }

    if (_proto == HTTP11) {
        // A 1.1 compliant server response shows the protocol as HTTP/1.1 even
        // for a HTTP/1.0 request,  but it promises to only use HTTP/1.0 syntax.
        if (_request->getProtocol() == HTTP10) {
            _proto = HTTP10;
        }
    }

    return _proto;
}

char * PSHttpResponse::getHeader(const char *name) {
    CacheEntry *entry = _headers->Get(name);
    return entry ? (char *)entry->GetData() : NULL;
}

int PSHttpResponse::getHeaders(char ***keys) {
    
    return _headers->GetKeys( keys );

}

long PSHttpResponse::getBodyLength() {
    return _bodyLength;
}

char * PSHttpResponse::getContent() {
    return _content;
}

void PSHttpResponse::freeContent() {
    if( _content != NULL ) {
        PR_Free( _content );
        _content = NULL;
    }
}

int PSHttpResponse::getContentSize() {

    return _contentSize;
}

char *PSHttpResponse::toString() {
    char *resp = (char *)"";
    char **keys;
    char *headerBuf = NULL;
    int nHeaders = getHeaders( &keys );
    if ( nHeaders > 0 ) {
        char **values = new char*[nHeaders];
        int len = 0;
        int *keyLengths = new int[nHeaders];
        int *valueLengths = new int[nHeaders];
        int i;
        for( i = 0; i < nHeaders; i++ ) {
            keyLengths[i] = (int) strlen( keys[i] );
            len += keyLengths[i] + 1;
            values[i] = getHeader(keys[i]);
            valueLengths[i] =(int) strlen( values[i] );
            len += valueLengths[i] + 1;
        }
        headerBuf = new char[len + nHeaders * 2];
        char *p = headerBuf;
        for( i = 0; i < nHeaders; i++ ) {
            strcpy( p, keys[i] );
            p += keyLengths[i];
            *p++ = ':';
            strcpy( p, values[i] );
            p += valueLengths[i];
            *p++ = ',';
        }
        *p = 0;
        for( i = 0; i < nHeaders; i++ ) {
            if( keys[i] != NULL ) {
                delete [] keys[i];
                keys[i] = NULL;
            }
        }
        if( keys != NULL ) {
            delete [] keys;
            keys = NULL;
        }
        if( values != NULL ) {
            delete [] values;
            values = NULL;
        }
        if( keyLengths != NULL ) {
            delete [] keyLengths;
            keyLengths = NULL;
        }
        if( valueLengths != NULL ) {
            delete [] valueLengths;
            valueLengths = NULL;
        }
    }

    char *s = NULL;
    if ( headerBuf ) {
        s = PR_smprintf( "PSHttpResponse [%s\nbody bytes:%d]",
                         headerBuf, _bodyLength );
    } else {
        s = PR_smprintf( "PSHttpResponse [body bytes:%d]", _bodyLength );
    }
    resp = new char[strlen(s) + 1];
    strcpy( resp, s );
    if( s != NULL ) {
        PR_smprintf_free( s );
        s = NULL;
    }
    return resp;
}

PRBool PSHttpResponse::checkKeepAlive() {
    HttpProtocol proto;
    const char *connectionHeader;

    if (_keepAlive < 0) {
        proto = getProtocol();
        if (proto == HTTP11) {
            // default is connection: keep-alive
            _keepAlive = 1;
        } else {
            // default is connection: close
            //            _keepAlive = 0;
            //CMS needs keepalive with HTTP10 (so no chunked encoding)
            _keepAlive=1;
        }

        connectionHeader = _request->getHeader("connection");
        if (connectionHeader) {
            if (!PL_strcasecmp(connectionHeader, "keep-alive")) {
                _keepAlive = 1;
            } else if (!PL_strcasecmp(connectionHeader, "close")) {
                _keepAlive = 0;
            } else {

            }
        }
    }

    return (_keepAlive == 0?PR_FALSE:PR_TRUE);
}

PRBool PSHttpResponse::checkConnection() {
    // return true if the connection is OPEN
    return (_connectionClosed == 0?PR_TRUE:PR_FALSE);
}


int PSHttpResponse::_verifyStandardBody(RecvBuf &buf,
                                        int expectedBytes,
                                        PRBool check) {
    int bytesRead = 0; 
    int curPos = 0;
    char ch;

    while(expectedBytes > 0 ) {
        ch = buf.getChar();
        if (ch < 0 ) {
            break;
        }
        // if check is true, we think we know what the content looks like
        if ( check ) {
            if (ch != (char) curPos%256) {

                check = PR_FALSE;
                break;
            }
            curPos++;
        }

        bytesRead++;

        if (expectedBytes > 0) {
            expectedBytes--;
        }
    }

    return bytesRead;
}

PRBool    PSHttpResponse::_handleChunkedConversation(RecvBuf &buf)
{
    char ch = 0;
    char tBuff[56];
    char chunk[4096];

    int i = 0;

    PSChunkedResponseCallback cb = _request->getChunkedCallback();
    void *uw = _request->getChunkedCallbackUserWord();

    if(!cb)
    {
        return PR_FALSE;
    }

    PR_LOG(httpRespLog, PR_LOG_DEBUG, 
                          ("%s PSHttpResponse::_handleChunkedConversation  \n",GetTStamp(tBuff,56)));

    while(1)
    {
        if(_client && _client->isConnectionClosed())
        {
            PR_LOG(httpRespLog, PR_LOG_DEBUG, 
                   ("%s PSHttpResponse::_handleChunkedConversation  client"
                    " claims conn closed!\n",GetTStamp(tBuff,56)));

            chunk[0] = 0;
            i = 0;

            return PR_TRUE;
        }
        else
        {
           ch = buf.getChar();
        }

        if(ch == -1)
        {
             PR_LOG(httpRespLog, PR_LOG_DEBUG, 
                    ("%s PSHttpResponse::_handleChunkedConversation getChar"
                     " returned -1 ! \n",GetTStamp(tBuff,56)));
            chunk[i] = 0;
            if(i >= 0)
            {
                PR_LOG(httpRespLog, PR_LOG_DEBUG, 
                       ("%s PSHttpResponse::_handleChunkedConversation"
                        "  chunk complete EOF condition. chunk: %s\n",GetTStamp(tBuff,56),(char *) chunk));
                (*cb)((unsigned char *) chunk,i,uw,HTTP_CHUNKED_EOF);

                chunk[0] = 0;
                i = 0;
            }

            return PR_TRUE;
        }

        if (ch == '\n' )
        {
            chunk[i] = 0;

            if(i > 0)
            {
                PR_LOG(httpRespLog, PR_LOG_DEBUG,
                       ("%s PSHttpResponse::_handleChunkedConversation"
                        "  chunk complete normal condition. chunk: %s\n",GetTStamp(tBuff,56),(char *) chunk));
                (*cb)((unsigned char *) chunk,i,uw,HTTP_CHUNK_COMPLETE);
            }
            
            chunk[0] = 0;
            i = 0;
        }
        else
        {

            chunk[i++] = ch;
        }
    
    }
    return PR_TRUE;
}

PRBool PSHttpResponse::_handleBody( RecvBuf &buf ) {
    char *clHeader;      // content length header
    char *teHeader;      // transfer-encoding header
    int expected_cl=-1;  // expected content length

    teHeader = getHeader("transfer-encoding");
    if(!teHeader)
    {
        teHeader = getHeader("Transfer-Encoding");
    }

    if (teHeader && !PL_strcasecmp(teHeader, "chunked")) {
        _chunkedResponse = PR_TRUE;
        buf.setChunkedMode();

        PSChunkedResponseCallback cb = _request->getChunkedCallback();

        if(cb != NULL)
        {// We need to process the chunked encoded conversation
             _handleChunkedConversation(buf);

        }
    } else {
        _chunkedResponse = PR_FALSE;
        clHeader = getHeader("Content-length");
        if (clHeader) {
             expected_cl =  atoi(clHeader);
        }
    }

    if (_request->getExpectStandardBody()) {
        _bodyLength = _verifyStandardBody(buf, expected_cl, PR_TRUE);

    } else {
        _bodyLength = _verifyStandardBody(buf, expected_cl, PR_FALSE);
    }

    if (expected_cl >= 0) {
        if (_bodyLength != expected_cl) {

        }
    }

    return PR_TRUE;
}

/**
 * Reads until the first space character
 *
 * @param buf Receive buffer to read from
 * @param headerBuf Array to read header into
 * @param len Size of headerBuf
 * @return Number of characters read, or -1 if too many
 */
static int readHeader( RecvBuf& buf, char* headerBuf, int len ) {
    int index = 0;

    do {
        char ch = buf.getChar();

        if ( ch != -1 && !isspace(ch) ) { 
            headerBuf[index++] = ch;
            if ( index >= (len-1) ) {
                return -1;
            }            
        } else {
            headerBuf[index] = '\0';
            break;    
        }
    } while( true );
    
    return index;
}


PRBool PSHttpResponse::processResponse(PRBool processStreamed) {
    RecvBuf buf( _socket, 8192, _timeout , this,processStreamed);

    if (_expectChunked) {
        buf.setChunkedMode();
    }

    try {
        char tmp[2048];
        int tmpLen = sizeof(tmp);

        // Get protocol string
        int nRead = readHeader( buf, tmp, tmpLen );

        if ( nRead <= 0 ) {

            return PR_FALSE;    
        }

        _protocol = PL_strdup(tmp);


        // Get status num
        nRead = readHeader( buf, tmp, tmpLen );
        if ( nRead < 0 ) {

            return PR_FALSE;    
        }

        _statusNum = PL_strdup( tmp );


        retcode = atoi( tmp );

        // Get status string
        int index = 0;
        do {
            char ch = buf.getChar();
            if ( ch != -1 && ch != '\r' ) {
                tmp[index++] = ch;
                if ( index >= (tmpLen-2) ) {
                    tmp[index] = 0;

                    return PR_FALSE;
                }            
            } else {
                break;
            }
        } while (true);
        tmp[index] = '\0';
        _statusString = PL_strdup( tmp );

        // Skip CRLF
        (void)buf.getChar();

        // loop over response headers
        index = 0;
//#ifdef CHECK
        PRBool doneParsing = PR_FALSE;
        PRBool atEOL = PR_FALSE;
        PRBool inName = PR_TRUE;
        char name[2048];
        int nameLen = sizeof(name);

        while ( !doneParsing ) {
            char value[2048];
            int valueLen = sizeof(value);
            char ch = buf.getChar();

            switch( ch ) {
            case ':':
                if ( inName ) {
                    name[index] = '\0';
                    index = 0;
                    inName = PR_FALSE;

                    nRead = readHeader( buf, value, valueLen );
                    if ( nRead < 0 ) {

                        //                        return PR_FALSE;    
                    } else {
                    //    value[index++] = ch;
                        if ( index >= (int)(sizeof(value) - 1 ) ) {

                            //                            return PR_FALSE;            
                        }
                    }
                    break;
                case '\r':
                    if ( inName && !atEOL ) {
                        name[index] = '\0';

                        //                        return PR_FALSE;
                    }
                    break;
                case '\n':
                    if ( atEOL ) {
                        doneParsing = PR_TRUE;
                        break;
                    }
                    if ( inName ) {
                        name[index] = '\0';

                        //                        return PR_FALSE;
                    }
                    value[index] = '\0';
                    index = 0;
                    inName = PR_TRUE;
                    _headers->Put(name, PL_strdup(value));
                    atEOL = PR_TRUE;
                    break;
                default:
                    atEOL = PR_FALSE;
                    if (inName) {
                         name[index++] = ch;
                    } else {
                         value[index++] = ch;
                    }
                    if ( inName && (index >= (nameLen-2)) ) {
                        name[index] = '\0';

                        //                        return PR_FALSE;            
                    } else if ( !inName && (index >= (valueLen-1)) ) {

                        //                        return PR_FALSE;            
                    }    
                    break;
                }
            }

        } //while
//#endif //CHECK
    } catch ( RecvBuf::EndOfFile & ) {
        if ( !_request->isHangupOk() ) {

            int errCode = PR_GetError();
            if ( PR_IO_TIMEOUT_ERROR == errCode ) {

            } else {

            }
        }
        return PR_FALSE;
    }

    // Read the body (HEAD requests don't have bodies)
    // jpierre 1xx, 204 and 304 don't have bodies either
    if ( PL_strcmp(_request->getMethod(), "HEAD") &&
         (!((retcode>=100) && (retcode<200))) &&
         (retcode!=204) &&
         (retcode!=304) ) {
        if ( _handleBody(buf) == PR_FALSE ) {
            return PR_FALSE;
        }
    }

    if ( checkConnection() && !checkKeepAlive() ) {
        // if connection is still open, and we didn't expect a keepalive,
        // read another byte to see if the connection has closed.
        try {
            char ch;
            ch = buf.getChar();
            buf.putBack();
            // conflict!

        } catch (RecvBuf::EndOfFile &) {
            _connectionClosed = 1;
        }
    }

    _checkResponseSanity();
    _content = (char *)buf.get_content();
    _contentSize = buf.get_contentSize();
  
    return PR_TRUE;
}

void PSHttpResponse::_checkResponseSanity() {
    char *clHeader = getHeader("Content-length");
    char *teHeader = getHeader("Transfer-Encoding");


    ///////////////////////////////////////////////////
    // Check items relevant to HTTP/1.0 and HTTP/1.1 //
    ///////////////////////////////////////////////////

    // check for both content-length and chunked
    if ( clHeader && teHeader ) {

    }

    // check for basic headers
    if ( !getHeader("Date") ) {

    }
    if ( !getHeader("Server") ) {

    }

    int expectedLength;
    if ((expectedLength = _request->getExpectedResponseLength()) > 0) {
        if (expectedLength != _bodyLength) {

        }
    }

    ///////////////////////////////////////
    // Check items relevant to HTTP/1.0  //
    ///////////////////////////////////////
    if ( getProtocol() == HTTP10 ) {
        if ( _chunkedResponse ) {

        }
    }

    ///////////////////////////////////////
    // Check items relevant to HTTP/1.1  //
    ///////////////////////////////////////
    if ( getProtocol() == HTTP11 ) {
        if ( (!clHeader && !_chunkedResponse) &&
             (!((retcode>=100) && (retcode<200))) &&
             (retcode!=204) &&
             (retcode!=304) ) {

        }
    }
}
