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
#include "escd.h"
#include <stdio.h>
#include <unistd.h>
#include <prlog.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>


#include "pk11func.h"

#define SHELL "/bin/sh"
#define CORRECT_NUM_ARGS 3

static PRLogModuleInfo *escDLog = PR_NewLogModule("escDLog");

string ESC_D::keyInsertedCommand("");
string ESC_D::onSignalCommand("");
int    ESC_D::commandAlreadyLaunched = 0;

string signalCommandArg = "forceStartESC";

ESC_D *ESC_D::single = NULL;

ESC_D::ESC_D() 
{
    single = this;
    mDataLock = NULL;
}

void ESC_D::cleanup()
{
    char tBuff[56];
    int already = 0;
    PR_Lock(mDataLock);
    already = commandAlreadyLaunched;
    PR_Unlock(mDataLock);
    if(already)
    {
        PR_LOG( escDLog, PR_LOG_ERROR, ("%s Daemon: ! Command is already running. \n",GetTStamp(tBuff,56)));
    }

    PR_LOG( escDLog, PR_LOG_ERROR, ("%s Daemon: Attempting to shut down. \n",GetTStamp(tBuff,56)));

    CoolKeyShutdown();

    PR_LOG( escDLog, PR_LOG_ERROR, ("%s Daemon: Past CoolKeyShutdown \n",GetTStamp(tBuff,56)));

    PR_DestroyLock(mDataLock);

    exit(0);

}

HRESULT ESC_D::init(int argc, char **argv)
{
    char tBuff[56];
    HRESULT result = S_OK;

    PR_LOG( escDLog, PR_LOG_DEBUG, ("%s Daemon: Nmber of args! %d \n",GetTStamp(tBuff,56),argc));


    for(int i = 0; i < argc ; i++)
        PR_LOG( escDLog, PR_LOG_DEBUG, ("%s Argv[%d]: %s \n",GetTStamp(tBuff,56),i,argv[i]));

    if(argc != CORRECT_NUM_ARGS)
    {
        return E_FAIL;
    } 

     mDataLock = PR_NewLock();
     if (!mDataLock)
     {
        PR_LOG( escDLog, PR_LOG_ERROR, ("%s Cannot create mutex for ESCD! \n",GetTStamp(tBuff,56)));
        return E_FAIL;
     }

    vector<string> arg1Tokens;
    vector<string> arg2Tokens;

    string argument1 = "";
    string argument2 = "";

    string delim = "=";

    vector<string>::iterator n, v;

    argument1 = argv[1];
    argument2 = argv[2];

    TokenizeArgument(argument1,arg1Tokens,delim); 

    TokenizeArgument(argument2,arg2Tokens,delim);

    v  =  arg1Tokens.begin();
   
    if( v != arg1Tokens.end())
    { 
        n  =  v++;

        string name =  "";
        string value = "";

        if(n != arg1Tokens.end())
            name =  (*n);

    
        if(v != arg1Tokens.end())
            value = (*v);


        if(name == KEY_INSERTED_CMD && value.size())
        {
            keyInsertedCommand = value; 

            PR_LOG( escDLog, PR_LOG_DEBUG, ("%s Daemon: keyInsertedCommand: %s. \n",GetTStamp(tBuff,56),keyInsertedCommand.c_str()));

        }
    }

    vector<string>::iterator n1, v1;
    v1  =  arg2Tokens.begin();
   
    PR_LOG(escDLog, PR_LOG_ALWAYS, ("%s Daemon: got v1 \n",GetTStamp(tBuff,56))); 

    if(n1 != arg2Tokens.end())
    {
        n1  =  v1++;
        PR_LOG(escDLog, PR_LOG_ALWAYS, ("%s Daemon: got n1... \n",GetTStamp(tBuff,56)));
        string name1 =  "";
        string value1 = "";

        if(n1 != arg2Tokens.end())
            name1 =  (*n1);

        if(v1 != arg2Tokens.end())
            value1 = (*v1);

        if(name1 == ON_SIGNAL_CMD && value1.size())
        {
           onSignalCommand = value1;
           PR_LOG( escDLog, PR_LOG_DEBUG, ("%s Daemon: onSignalCommand: %s. \n",GetTStamp(tBuff,56),onSignalCommand.c_str()));
        }

    }

    CoolKeySetCallbacks(Dispatch,Reference, Release,NULL ,NULL);
    CoolKeyRegisterListener((CoolKeyListener *) this);

    struct sigaction sigESCD; 

    /* install the signal handler */

    sigemptyset(&(sigESCD.sa_mask));
    sigaddset(&(sigESCD.sa_mask),SIGUSR1);
    sigaddset(&(sigESCD.sa_mask),SIGTERM);


    pthread_sigmask(SIG_UNBLOCK,&(sigESCD.sa_mask),NULL);
    sigESCD.sa_handler = ESC_D::signalHandler;

    sigESCD.sa_flags = 0;

    sigaction(SIGUSR1,&sigESCD,NULL);
    sigaction(SIGTERM,&sigESCD,NULL);

    // Install the XWindows IO Error Handler

    XSetIOErrorHandler(ESC_D::xIOErrorHandler);

    result = S_OK; 

    return result;
}

HRESULT ESC_D::Dispatch( CoolKeyListener *listener,
        unsigned long keyType, const char *keyID, unsigned long keyState,
        unsigned long data, const char *strData
    )
{
    char tBuff[56];
    HRESULT result = S_OK;

    if(keyState == eCKState_KeyInserted)
    {

        PR_LOG( escDLog, PR_LOG_ALWAYS, ("%s Daemon: Key Inserted: keyID:  %s. \n",GetTStamp(tBuff,56),keyID));

        string fullCommand="";
        string space = " " ;
        fullCommand = keyInsertedCommand + space + "keyInserted";

        if(single)
            single->launchCommand(fullCommand);
        
    }

    if(keyState == eCKState_KeyRemoved)
    {
        PR_LOG( escDLog, PR_LOG_ALWAYS, ("%s Daemon: Key Removed: keyID:  %s. \n",GetTStamp(tBuff,56),keyID));

    }

    return result;
}

HRESULT ESC_D::Reference(CoolKeyListener *listener )
{
    return S_OK;
}

HRESULT ESC_D::Release( CoolKeyListener *listener )
{
    return S_OK;
}

void ESC_D::TokenizeArgument(const string& str,
                      vector<string>& tokens,
                      const string& delimiters )
{
    string::size_type lastPos = str.find_first_not_of(delimiters, 0);

    string::size_type pos     = str.find_first_of(delimiters, lastPos);

    while (string::npos != pos || string::npos != lastPos)
    {
        // Found a token, add it to the vector.
        tokens.push_back(str.substr(lastPos, pos - lastPos));
        // Skip delimiters.  Note the "not_of"
        lastPos = str.find_first_not_of(delimiters, pos);
        // Find next "non-delimiter"
        pos = str.find_first_of(delimiters, lastPos);
    }
}

HRESULT ESC_D::launchCommand(string &command)
{
    char tBuff[56];
    const char *shell = SHELL;

    PR_LOG( escDLog, PR_LOG_ALWAYS, ("%s Daemon: About to launch command:  %s. \n",GetTStamp(tBuff,56),command.c_str()));

    int already = 0;

    PR_Lock(mDataLock);


    already = commandAlreadyLaunched;

    PR_Unlock(mDataLock);

    if(already)
    {
        PR_LOG( escDLog, PR_LOG_DEBUG, ("%s Daemon: About to launch command: Command already running.. \n",GetTStamp(tBuff,56)))  ;
        return E_FAIL;
    }

    if(!command.size())
    {
        return E_FAIL;
    }

    int status;
    pid_t pid;

    PR_Lock(mDataLock);

    commandAlreadyLaunched = 1;

    PR_Unlock(mDataLock);

    pid = fork ();
    if (pid == 0)
    {
      execl (shell, shell, "-c", command.c_str(), NULL);
      _exit (EXIT_FAILURE);
    }
    else if (pid < 0)
       status = -1;
    else
    {
        if (waitpid (pid, &status, 0) != pid)
            status = -1;
    }

    PR_Lock(mDataLock);

     
    commandAlreadyLaunched = 0;
    PR_LOG( escDLog, PR_LOG_DEBUG, ("%s Daemon: Child command has exited.. \n",GetTStamp(tBuff,56)))  ;

    PR_Unlock(mDataLock);

    if(status == -1)
        return E_FAIL;
   
    return S_OK;

}

int ESC_D::xIOErrorHandler(Display *display)
{

   char tBuff[56];
   PR_LOG( escDLog, PR_LOG_ALWAYS, ("%s Daemon: XIOErrorHandler! We are finished.\n",GetTStamp(tBuff,56)));

    if(single)
        single->cleanup();

    return 1;
}


void ESC_D::signalHandler(int signal)
{
   char tBuff[56];
   string fullCommand="";
   string space = " " ;

   PR_LOG( escDLog, PR_LOG_DEBUG, ("%s Daemon: signalHandler: signal: %d.. \n",GetTStamp(tBuff,56),signal));

   switch(signal)
   {
       case SIGUSR1:

        fullCommand = onSignalCommand + space + signalCommandArg;
        if(single)
        {
            single->launchCommand(fullCommand);
        }

       break;

       case SIGTERM:

           if(single)
               single->cleanup();

       break;
   }  
}

int main(int argc, char **argv)
{
    char tBuff[56];
    pid_t pid;

    // Fork off the parent process        
    pid = fork();
    if (pid < 0) {
        exit(1);
    }
    if (pid > 0) {
        exit(0);
    }

    PR_LOG(escDLog, PR_LOG_ALWAYS, ("%s Daemon: Initializing Daemon... \n",GetTStamp(tBuff,56)));

    umask(0);

    ESC_D daemon;

    HRESULT result = daemon.init(argc,argv);

    if(result == E_FAIL)
    {
        PR_LOG( escDLog, PR_LOG_ALWAYS, ("%s Daemon: Error initializing, exiting.. \n",GetTStamp(tBuff,56)));
        exit(1);
    }

    int hresult =  CoolKeyInit("./");

    if(hresult == E_FAIL)
    {
        PR_LOG( escDLog, PR_LOG_ALWAYS, ("%s Daemon: Error initializing CoolKey System, this will result in problems recognizing Smart Cards! \n",GetTStamp(tBuff,56)));
    }


    //Now become an XWindows program so we can die on user logout

    Display *display;
    XEvent event;
 
    display = XOpenDisplay(NULL);

    if(!display)
    {
        PR_LOG( escDLog, PR_LOG_ALWAYS, ("%s Daemon: Error Obtaining X Display! \n",GetTStamp(tBuff,56)));
    }

    PR_LOG( escDLog, PR_LOG_ALWAYS, ("%s Daemon: Attempted XOpenDisplay: %p \n",GetTStamp(tBuff,56),display)); 
    while ("looping forever") XNextEvent(display,&event);

    PR_LOG( escDLog, PR_LOG_ALWAYS, ("%s Daemon: main exiting.. \n",GetTStamp(tBuff,56))    );
 
    XCloseDisplay (display);
 
    return 0;

}
