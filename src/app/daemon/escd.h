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

#ifndef ESC_D_H
#define ESC_D_H
#include <string>
#include <vector>
#include <signal.h>
#include "prlock.h"

#include <X11/Xlib.h>

using namespace std;

#include "CoolKey.h"

#define KEY_INSERTED_CMD    "--key_Inserted"
#define ON_SIGNAL_CMD       "--on_Signal"
class ESC_D
{
public:

    ESC_D();

    HRESULT init(int argc, char **argv);

    void    cleanup();
    static string keyInsertedCommand;
    static string onSignalCommand;

    static void signalHandler(int signal);

    static int xIOErrorHandler(Display *display);

    static ESC_D *single;

    PRLock    *mDataLock;
    
    HRESULT launchCommand(string &command);

    private:

    void TokenizeArgument(const string& str,
                      vector<string>& tokens,
                      const string& delimiters = " ");



    static HRESULT Dispatch( CoolKeyListener *listener,
        unsigned long keyType, const char *keyID, unsigned long keyState,
        unsigned long data, const char *strData
    );

    static HRESULT  Reference(CoolKeyListener *listener );
    static HRESULT  Release(CoolKeyListener *listener );


    static int  commandAlreadyLaunched;

};






#endif
