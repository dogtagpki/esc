/* ***** BEGIN COPYRIGHT BLOCK *****
 * * Copyright (C) 2005 Red Hat, Inc.
 * * All rights reserved.
 **
 ** This library is free software; you can redistribute it and/or
 ** modify it under the terms of the GNU Lesser General Public
 ** License as published by the Free Software Foundation version
 ** 2.1 of the License.
 **
 ** This library is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 ** Lesser General Public License for more details.
 **
 ** You should have received a copy of the GNU Lesser General Public
 ** License along with this library; if not, write to the Free Software
 ** Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 ** ***** END COPYRIGHT BLOCK *****/


#define LOGFILE "inst.log"

#ifdef WIN32 
#define INITGUID 1
#include <windows.h>
#include <stdio.h>
#include <setupapi.h>
#include <direct.h>
#include <time.h>
#include <winver.h>

#define DRIVER_COUNT 4
/* This structure defines that descriptor used for each egate driver.
 * the data described in the structure is static.
 */
typedef struct _driverInfo {
    char *devClass;		/* microsoft device class */
    char *infFile;		/* original inf file escribing the device */
    char *instFile;		/* file in the installation */
    char *deviceID;		/* Typical Device ID */
    DWORD createFlags;		/* Flags for hand creating device */
    BOOL devClassUnique;	/* true only if class is unique for egate */
    BOOL doInstall;		/* this driver should be installed */
    BOOL handInstall;
} DriverInfo;

/* dynamically determined driver information */
typedef struct _dyDriverInfo {
    char *oemFile;		/* new OEM file */
    char destPath[MAX_PATH];
} DyDriverInfo;

static const DriverInfo driverInfo[DRIVER_COUNT] = {
/* Egate PCSC device. Maps to a virtual reader. These are managed by
   The Egate bus driver. */
 { "SmartCardReader", "egaterdr.inf", "egaterdr.sys", 
			"EGATEBUS", 0, FALSE, TRUE, FALSE },
/* Driver handling the actual card driver. This one is managed by the
   plug and play handler. The first card is supposed to install the
   EgateBus handler */
 { "EgateCard", "egate.inf", "egate.sys", 
			"USB\\VID_0973&PID_0001", 0, TRUE, TRUE, FALSE },
/* Egatebus is the Reader Enumerator. It starts new readers as necessary. */
 { "Egatebus", "egatebus.inf", "egatebus.sys", 
			"EGATEBUS", DICD_GENERATE_ID, TRUE, TRUE, TRUE },
/* Unknown driver is necessary for uninstall. If an egate device was inserted
   before any egate drivers were installed, an Unknown device will have been
   created. We need to remove it before we start our install */
 { "Unknown", "none.inf", "none.sys", 
			"USB\\VID_0973&PID_0001", 0, FALSE, FALSE, FALSE },
};

static DyDriverInfo dyDriverInfo[DRIVER_COUNT] = { 0 };

#define FILE_COUNT 8
static const char *deviceFileList[FILE_COUNT] = {
 "drivers\\egate.sys",
 "drivers\\egateraw.sys",
 "egdrvins1.dll",
 "egdrvins.dll",
 "slbmqp98.dll",
 "slbmgpg.dll",
 "drivers\\egatebus.sys",
 "drivers\\egaterdr.sys",
};

static char infFilePath[MAX_PATH];
static char logFilePath[MAX_PATH];

BOOL useOS = FALSE;

typedef unsigned long TIME;


void
profile(FILE *logFile, BOOL doProfile, char  *message, void *arg, TIME last, TIME *retCurrent)
{
    static TIME epoc = 0;
    TIME current;

    if (!doProfile) {
 	return;
    }
    current  = GetTickCount();

    if (epoc == 0) {
	epoc = current;
    }
    if (retCurrent) {
	*retCurrent = current;
    }
    if (!message) {
	return;
    }
    fprintf(logFile,"@TIME:");
    fprintf(logFile, message, arg); 
    fprintf(logFile," %d ms (%d ms total)\n",  current - last, current - epoc);
}

 

#include <dbt.h>

DEFINE_GUID(IDD_CLASS_EGATECARD,0x555e05a3,0x904c,0x42cf,0xae,0xf4,
                0xee,0x40,0x35,0xec,0x63,0x62);
DEFINE_GUID(IDD_CLASS_EGATERDR,0x50dd5230,0xba8a,0x11d1,0xbf,0x5d,
                0x00,0x00,0xf8,0x05,0xf5,0x30);
DEFINE_GUID(IDD_CLASS_EGATEBUS,0x9510ee5d,0x9613,0x439a,0xad,0xb9,
                0xe4,0xde,0xa5,0xa6,0xb6,0x53);

static void
usage(char *prog)
{
    fprintf(stderr,"usage: %s [-F][-q][-9][-u][-c][-p][-O][-l logfile] [egate_path]\n",prog);
    fprintf(stderr,"\tu uninstall\tc clean\n");
    fprintf(stderr,"\tp profile\tO use slow OS calls\n");
    fprintf(stderr,"\tq quiet\n");
    fprintf(stderr,"\tF and 9 reserved for future use\n");
    return;
}

/* Utility printing functions */

/* capture the window's error string */
static void
winPerror(FILE *outFile, DWORD error, const char *msgString)
{
     char buffer[256];
     char *cp;
     DWORD ret;

     fprintf(outFile,"*** %s: ",msgString);
     sprintf(buffer,"Format message problem, error = %d (0x%x)\n", error, error);

     ret=FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, error, 0, buffer, 
							sizeof(buffer), NULL);
     for (cp=buffer; *cp; cp++) {
	if (*cp == '\r') *cp = ' ';
     }
     fprintf(outFile, buffer);
}


#ifdef DEBUG
/* pretty raw dump a buffer in hex and ascii */
#define MAX_ROWS 16
void
dump(FILE *logFile, void *p, int len)
{
    unsigned char *c = (char *)p;
    char string[MAX_ROWS+1];
    int i,j;

 fprintf(logFile,"Dumping buffer %x size=%d\n",p,len);
    for (i=0, j=0; i< len ; i++,j++) {
	if (j >= MAX_ROWS) {
	   string[MAX_ROWS] = 0;
	   fprintf(logFile," %s\n",string);
	   j=0;
	}
	fprintf(logFile,"%02x ",c[i]);
	string[j] =  (char) (c[i] >= ' ' && c[i] <0x7f) ? c[i] : '.' ;
    }
    if (j && j < MAX_ROWS) {
	string[j]= 0;
	for (; j < MAX_ROWS; j++) {
	    fprintf(logFile,"   ");
	}
	fprintf(logFile," %s\n",string);
    }
}

/* dump an InfInformation buffer */
void
printInf(FILE *logFile, const PSP_INF_INFORMATION sp_inf, DWORD len)
{
    char *style = NULL;

    switch (sp_inf->InfStyle) {
    case INF_STYLE_NONE:
	style = "No Style";
	break;
    case INF_STYLE_OLDNT:
	style = "NT 3.0 Sytle";
	break;
    case INF_STYLE_WIN4:
	style = "WIN4 Sytle";
	break;
    default:
	break;
    }

    if (style) {
	fprintf(logFile,"%s: ", style);
    } else {
	fprintf(logFile,"Invalid Style %x: ", sp_inf->InfStyle);
    }

    fprintf(logFile," count=%d lcount=%d\n", sp_inf->InfCount, len);
    dump(logFile,sp_inf,len); 
}


#endif

/*
 *  INF version values are strings of ascii dates. Convert these
 *  strings to a 32-bit number so that versions can be compared.
 *  If the string appears to be invalid, then -1 is returned.
 *
 *  THIS FUNCTION is not MULTI_BYTE SAFE.
 */
#define bcd2byte(c) (*(c)-'0')
#define bcd2short(c) (bcd2byte(c)*10+bcd2byte(c+1))
#define bcd2int(c) (bcd2short(c)*100+bcd2short(c+2))
int dateStringToNumber(FILE *logFile, char *date)
{
    int day, month, year;
    time_t timet;
    struct tm time;

    /* string must be 10 bytes long. */
    if (strlen(date) != 10 ) {
	fprintf(logFile,"invalid version <%s>\n", date);
	return -1;
    }

    /* offset-> 0 1 2 3 4 5 6 7 8 9 */
    /* date  -> m m / d d / y y y y */
    /* date can be separated by '/' or '-' */
    if ( !((date[2] == '/' || date[2] == '-') &&
		(date[5] == '/'  || date[2] == '-')) ) {
	fprintf(logFile,"invalid version <%s>\n", date);
	return -1;
    }
    month=bcd2short(date);
    day=bcd2short(date+3);
    year=bcd2int(date+6);

    /* special case 00/00/0000 as the same as epoch */
    if ((month == 0) && (day == 0) && (year == 0)) {
	return 0;
    }

    /* verify we have a reasonable data */
    if ((day > 31) || ( month > 12) || (year > 2037) ||
	(day < 1) || (month < 1) || (year < 1970)) {
	fprintf(logFile,"invalid version <%s> ->", date);
	fprintf(logFile,"%02d/%02d/%04d\n", month, day, year);
	return -1;
    }

    /* fill in the time structure */
    time.tm_sec = 0;
    time.tm_min = 0;
    time.tm_hour = 0;
    time.tm_mday = day;
    time.tm_mon = month -1; /* Time takes month as '0' based, not '1' based */
    time.tm_year = year - 1900;
    time.tm_isdst = 0;

    /* convert to seconds since EPOC */
    timet = mktime(&time);
    if ((time_t)-1 == timet) {
	fprintf(logFile,"invalid version <%s>: %s\n", date,strerror(errno));
	return -1;
    }

    /* convert to days since EPOC */
    timet = timet / (3600*24);

    return (int) timet;
}

/*
 * Several of Microsoft's Setup and SetupDi functions have a two call strategy,
 * The function is called once with a NULL buffer and it returns the required
 * size of the buffer. The application then allocates the buffer and calls
 * the function again. The following wrappers accomplish this allocation 
 * automatically.
 */

BOOL
allocSetupGetSourceFileLocation(HINF infd, PINFCONTEXT infContext, 
		        PCTSTR file, PUINT sID, PTSTR *bufp, DWORD *lenp)
{
    char *buf;
    BOOL ret;
    DWORD len;

    *lenp = 0;
    ret = SetupGetSourceFileLocation(infd, infContext, file, sID, 
							NULL, 0, &len);
    if (!ret) {
	return ret;
    }
    buf = (char *) malloc(len);
    ret = SetupGetSourceFileLocation(infd, infContext, file, sID, 
						       buf, len, lenp);
    if (ret) {
	*bufp = buf;
	return ret;
    }
    free(buf);
    *lenp = 0;
    return ret;
}

BOOL
allocSetupGetSourceInfo(HINF infd, UINT sID, 
		        UINT infoD, PTSTR *bufp, LPDWORD lenp)
{
    char *buf;
    BOOL ret;
    DWORD len;

    *lenp = 0;
    ret = SetupGetSourceInfo(infd, sID, infoD, NULL, 0, &len);
    if (!ret) {
	return ret;
    }
    buf = (char *) malloc(len);
    ret = SetupGetSourceInfo(infd, sID, infoD, buf, len, lenp);
    if (ret) {
	*bufp = buf;
	return ret;
    }
    free(buf);
    *lenp = 0;
    return ret;
}

BOOL
allocSetupQueryInfVersionInformation(PSP_INF_INFORMATION spi,
	UINT infIndex, PCTSTR key, PTSTR *bufp, PDWORD lenp)
{
    char *buf;
    BOOL ret;
    DWORD len;

    *lenp = 0;
    ret = SetupQueryInfVersionInformation(spi, infIndex, key, NULL, 0, &len);
    if (!ret) {
	return ret;
    }
    buf = (char *) malloc(len);
    ret = SetupQueryInfVersionInformation(spi, infIndex, key, buf, len, lenp);
    if (ret) {
	*bufp = buf;
	return ret;
    }
    free(buf);
    *lenp = 0;
    return ret;
}

BOOL
allocSetupQueryInfFileInformation(PSP_INF_INFORMATION spi,
				UINT infIndex, PTSTR *bufp, PDWORD lenp)
{
    char *buf;
    BOOL ret;
    DWORD len;

    *lenp = 0;
    ret = SetupQueryInfFileInformation(spi, infIndex, NULL, 0, &len);
    if (!ret) {
	return ret;
    }
    /*
     * SetupQueryInfFileInformation uses a buffer size that does not
     * include the null terminator, contrary to what its documentation
     * says.  We need to allocate len+1 bytes to work around this bug.
     */
    buf = (char *) malloc(len+1);
    ret = SetupQueryInfFileInformation(spi, infIndex, buf, len, lenp);
    if (ret) {
	*bufp = buf;
	return ret;
    }
    free(buf);
    *lenp = 0;
    return ret;
}

BOOL
allocSetupGetInfInformation(LPCVOID spec, DWORD search, 
				PSP_INF_INFORMATION *buf, PDWORD lenp)
{
    PSP_INF_INFORMATION sp_inf;
    BOOL ret;
    DWORD len;

    *lenp = 0;
    ret = SetupGetInfInformation(spec, search, NULL, 0, &len);
    if (!ret) {
	return ret;
    }
    sp_inf = (PSP_INF_INFORMATION) malloc(len);
    ret = SetupGetInfInformation(spec, search, sp_inf, len, lenp);
    if (ret) {
	*buf = sp_inf;
	return ret;
    }
    free(sp_inf);
    *lenp = 0;
    return ret;
}

#define SEARCH "oem*.inf"
#define MAX_ENV_SIZE 32767

BOOL
fastSetupGetInfFileList(PCTSTR dirPath,DWORD infStyle,PTSTR *buf,PDWORD lenp)
{
    char *searchString;
    char *files = NULL;
    WIN32_FIND_DATA fileData;
    HANDLE hDir;
    int len;
    DWORD err = NO_ERROR;

    *lenp = 0;
    *buf = 0;

    searchString = NULL;
    if (dirPath == NULL) {
	char winDir[MAX_ENV_SIZE];
	int winDirLen = 
		GetEnvironmentVariable("WINDIR", winDir, sizeof(winDir));

	if (winDirLen == 0) {
	    return FALSE;
	}
	searchString = (char *)malloc(winDirLen+sizeof("\\inf\\")
							+sizeof(SEARCH)+1);
 	if (searchString == NULL) {
	    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
	    return FALSE;
	}
	sprintf(searchString,"%s\\inf\\"SEARCH,winDir);
    } else {
	searchString = (char *)malloc(strlen(dirPath)+1+sizeof(SEARCH));
 	if (searchString == NULL) {
	    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
	    return FALSE;
	}
	sprintf(searchString,"%s\\"SEARCH,dirPath);
    }
    hDir = FindFirstFile(searchString,&fileData);
    free(searchString);
    if (hDir == INVALID_HANDLE_VALUE) {
	/* error set by FindFirst */
	return FALSE;
    }

    len=1; /* last NULL */
    do {
	/* Append */
	int strLen =  strlen(fileData.cFileName)+1; /* include string NULL */
	int newLen = len + strLen;
	char *save = files;
	files = (char *)realloc(files, newLen);
	if (files == NULL) {
	   free(save);
	   SetLastError(ERROR_NOT_ENOUGH_MEMORY);
	   break;
	}
	/* overwrite last NULL */
	memcpy(&files[len-1],fileData.cFileName,strLen);
	files[newLen-1] = 0; /* set Last NULL */
	len = newLen;
    } while (FindNextFile(hDir, &fileData));

    err = GetLastError();
    FindClose(hDir);

    if (err != ERROR_NO_MORE_FILES) {
	/* restore the previous error (in case FindClose trashes it) */
	SetLastError(err);
	free(files);
	return FALSE;
    }
    *buf = files;
    *lenp = len;
    return TRUE;
}


BOOL
slowSetupGetInfFileList(PCTSTR dirPath,DWORD infStyle,PTSTR *buf,PDWORD lenp)
{
    char *strings;
    BOOL ret;
    DWORD len;

    ret = SetupGetInfFileList(dirPath, infStyle, NULL, 0, &len);
    if (!ret) {
	return ret;
    }

    strings = (PTSTR) malloc(len);
    ret = SetupGetInfFileList(dirPath, infStyle, strings, len, lenp);
    if (ret) {
	*buf = strings;
	return ret;
    }
    free(strings);
    *lenp = 0;
    return ret;
}

BOOL
allocSetupGetInfFileList(PCTSTR dirPath,DWORD infStyle,PTSTR *buf,PDWORD lenp)
{
    return useOS ?
    	 slowSetupGetInfFileList(dirPath, infStyle, buf, lenp) :
    	 fastSetupGetInfFileList(dirPath, infStyle, buf, lenp);
	
}

allocSetupGetStringField(PINFCONTEXT pi, DWORD index, PTSTR *buf, PDWORD lenp)
{
    char *strings;
    BOOL ret;
    DWORD len;

    ret = SetupGetStringField(pi, index, NULL, 0, &len);
    if (!ret) {
	return ret;
    }

    strings = (PTSTR) malloc(len);
    ret = SetupGetStringField(pi, index, strings, len, lenp);
    if (ret) {
	*buf = strings;
	return ret;
    }
    free(strings);
    *lenp = 0;
    return ret;
}


BOOL
allocSetupDiGetClassDescription(LPGUID ClassGuid, PTSTR *buf, PDWORD lenp)
{
    char *strings;
    BOOL ret;
    DWORD len;

    ret = SetupDiGetClassDescription(ClassGuid, NULL, 0, &len);
    if (!ret) {
	len = 256;
    }

    strings = (PTSTR) malloc(len);
    ret = SetupDiGetClassDescription(ClassGuid, strings, len, lenp);
    if (ret) {
	*buf = strings;
	return ret;
    }
    free(strings);
    *lenp = 0;
    return ret;
}

/* these two functions returns errors when NULL pointers are supplied. */
BOOL
allocSetupDiGetDeviceInstanceId(HDEVINFO hdi, PSP_DEVINFO_DATA spd, 
					PTSTR *buf, PDWORD lenp)
{
    char *strings;
    BOOL ret;
    DWORD len;

    len = MAX_PATH;
    strings = (PTSTR) malloc(len);
    ret = SetupDiGetDeviceInstanceId(hdi, spd, strings, len, lenp);
    if (ret) {
	*buf = strings;
	return ret;
    }
    free(strings);
    *lenp = 0;
    return ret;
}

BOOL
allocSetupDiClassGuidsFromName(PCTSTR className, LPGUID *buf, PDWORD lenp,char *oemInfPath)
{
    LPGUID data;
    BOOL ret;
    DWORD len;

    char ClassName[256];

    ret = SetupDiClassGuidsFromName(className, NULL, 0, &len);
    if (!ret) {
	len = 256;
    }

    if(len == 0 )
    {

        ret = 0;

        len = 256;

        data = (LPGUID) malloc(len*sizeof(GUID));
        if(oemInfPath)
        {

            ret = SetupDiGetINFClass(oemInfPath,data,ClassName,256,0);

            if(ret)
                *lenp=sizeof(GUID);

        }


    } else

    {

        data = (LPGUID) malloc(len*sizeof(GUID));
        ret = SetupDiClassGuidsFromName(className, data, len, lenp);

    }

    if (ret) {
	*buf = data;
	return ret;
    }
    free(data);
    *lenp = 0;
    return ret;
}

/* find the next number in a '.' separated list of versions */
static char *nextNumber(char *string)
{
   char *next = strchr(string,'.');
   if (next) { 
	next = next+1;
   }
   return next;
}

/* fetch the version number (rather than the version date) from an inf file */
void
getInfVersionNumber(FILE *logFile, HINF handle, unsigned short *version)
{
    INFCONTEXT context;
    BOOL ret;
    char *vString, *cString;
    DWORD len;
    int i;

    version[0] = version[1] = version[2] = version[3] = 0;

    ret = SetupFindFirstLine(handle,"Version","DriverVer",&context);

    if (!ret) { 
	winPerror(logFile, GetLastError(), 
				"SetupFindFirstLine(Version,DriverVer");
	return;
    }
    ret = allocSetupGetStringField(&context, 2, &vString,  &len);
    if (!ret) { 
	winPerror(logFile, GetLastError(), "GetVersion- GetStringFailed");
	return;
    }

    for (i=0, cString = vString; i < 4 && cString && *cString
				; i++ , cString = nextNumber(cString)) {
	version[i] = atoi(cString);
    }
    free(vString);
    return;
}

/* fetch the version number of a Driver File */
BOOL
getFileVersion(char *fileName, unsigned short *version)
{
    BOOL ret = 0;
    DWORD handle;
    int verLen;
    char * buf;

    verLen = GetFileVersionInfoSize(fileName, &handle);
    if (verLen) {
	VS_FIXEDFILEINFO *fileInfo;
	UINT ulen;

	buf = malloc(verLen);
	if (buf == NULL) {
	    /* set windows error? */
	    return 0;
	}
	ret = GetFileVersionInfo(fileName, handle, verLen, buf);
	if (!ret) {
	    free(buf);
	    return ret;
	}
	ret = VerQueryValue(buf,"\\", &fileInfo, &ulen);
	free(buf);
	if (!ret) {
	    return ret;
	}
	version[0] = (fileInfo->dwProductVersionMS >> 16) & 0xffff;
	version[1] = (fileInfo->dwProductVersionMS) & 0xffff;
	version[2] = (fileInfo->dwProductVersionLS >> 16) & 0xffff;
	version[3] = (fileInfo->dwProductVersionLS) & 0xffff;
    }
    return ret;
}

/*
 * windows returnds Driver versions as DWORDLONG. Unpack it as
 * an array of shorts so we can display them sanely.
 */
static void
unpackVersions(unsigned short *versions, DWORDLONG DriverVersion)
{
    DWORD split[2];

    split[0] = (DWORD) ((DriverVersion >> (DWORDLONG)32) & 0xffffffff);
    split[1] = (DWORD) (DriverVersion & 0xffffffff);

    versions[0] = (unsigned short)((split[0] >> 16) & 0xffff);
    versions[1] = (unsigned short)( split[0] & 0xffff);
    versions[2] = (unsigned short)((split[1] >> 16) & 0xffff);
    versions[3] = (unsigned short)( split[1] & 0xffff);
}

/*
 * Usually Windows uses the date as the driver version, but under certain
 * conditions (the driver is not signed, for instance) the date is set to 0.
 * in this case we want to use the driver version number. We use the
 * driver version number to decide how we should adjust the driver version
 * (that is date)
 */
int
adjustVersion(int driverDate, int currentDriverDate, 
  const unsigned short *versions, const unsigned short *currentVersionNumber)
{
    int i;

    if (driverDate == 0) {
	driverDate = currentDriverDate;
	for (i=0; i < 4; i++) {
	    /* installed version is greater then our current version,
	     * create a driverDate that is greater then currentDriverDate
	     */
	    if (versions[i] > currentVersionNumber[i]) {
		 driverDate = currentDriverDate + 1;
		 break;
	    }
	    /* installed version is less then our current version,
	     * create a driver version that is less then the currentDriverDate
	     */
	    if (versions[i] < currentVersionNumber[i]) {
		driverDate = currentDriverDate -1 ;
		break;
	    }
	}
    }
    return driverDate;
}

/*
 * convert a FILETIME value to the same format Windows uses to specifiy
 * versions in INF files. 
 */
char *
fileTimeToString(const FILETIME *ftime)
{
    SYSTEMTIME systime;
    char *date;
    BOOL ret;

    /* special case '0' */
    if ((ftime->dwHighDateTime == 0) && (ftime->dwLowDateTime == 0)) {
	return strdup("00/00/0000");
    }

    /* use Windows to turn a FILETIME to moth, day and year */
    ret = FileTimeToSystemTime(ftime,&systime);
    if (!ret) {
	return NULL;
    }
    /* verify system time, in case somehow systime messes up and causes us
     * to overflow the date buffer */
    if ((systime.wMonth < 1) || (systime.wMonth > 12)) {
	return NULL;
    }
    if ((systime.wDay < 1) || (systime.wDay > 31)) {
	return NULL;
    }
    /* string to version won't like the larger dates, but we are mainly
     * concerned with not overflowing the date buffer, so dates beyond
     * 2060 is fine here */
    if ((systime.wYear < 1) || (systime.wYear > 9999)) {
	return NULL;
    }
    date = malloc(11);
    sprintf(date,"%02d/%02d/%04d",systime.wMonth,systime.wDay,systime.wYear);
    return date;
}

/*
 *  lookup the full path of a infFile
 */
char *
getFullInfPath(const char *infFile)
{
    BOOL ret;
    PSP_INF_INFORMATION spi;
    char *outFile;
    DWORD len;

    ret = allocSetupGetInfInformation(infFile , INFINFO_DEFAULT_SEARCH, 
				&spi, &len);
    if (!ret) {
	return NULL;
    }
    ret = allocSetupQueryInfFileInformation(spi, 0, &outFile, &len);
    free(spi);
    if (!ret) {
	return NULL;
    }
    return outFile;
}


/*
 *  Verify Inst Dir returns true of the OEM directory is found
 * in the compiled .inf file (.pnf). This function does a brute
 * force search of the .pnf file looking for the directory string
 * (which is stored as wchar data). OEM directory is accepted as a
 * cchar. This brute force function should be replaced by a supported
 * Microsoft API once a working API to fetch this string has been found.
 *
 * The comparison is case insensitive.
 */
static int
matchCase(int a, int b)
{
    if (islower(a)) {
	a= toupper(a);
    }
    if (islower(b)) {
	b = toupper(b);
    }
    return (a==b);
}

static int 
strMatchCase(char *str1, char *str2, int n)
{
    int i;
    for (i=0; i < n; i++) {
	if (str1[i] == 0 || !matchCase(str1[i],str2[i])) {
	    return 0;
	}
    }
    return 1;
}


int
VerifyInstDir(FILE *logFile, const char *infFile, const char * OEMDirectory)
{
    int last;
    const char *oem = OEMDirectory;
    FILE *file;
    int c;
    char *pnfFile;

    pnfFile = getFullInfPath(infFile);
    if (!pnfFile) {
	return 0;
    }

    last = strlen(pnfFile);
    pnfFile[last-3] = 'p';

    file = fopen(pnfFile,"rb");
    free(pnfFile);
    if (file == NULL) {
	return 0;
    }

    while ((c = fgetc(file)) != EOF) {
	if ( matchCase(c,*oem) ) {
	
	   if (*oem == 0) {
    		fclose(file);
		return 1;
	   }
	   oem++;
	   c = fgetc(file);
	   if (c == EOF) {
		break;
	   }
	   if (c == 0 ) {
		continue;
	   }
	}
	/* current string doesn't match, start over */
	oem = OEMDirectory;
    }
    fclose(file);
    return 0;
}

/*
 * Windows XP supports a function to Uninstall an OEM INF file,
 * but other versions of windows do not have such a function.
 * Use a wrapper function to see if the supported uninstall function
 * exists, and if it does use it, otherwise use our own version with
 * looks up and deletes the .inf and .pnf file.
 */
typedef BOOL uninstallFunc(PCSTR string, DWORD flags, PVOID reserved);

static BOOL
localSetupUninstallOEMInf(PCSTR string, DWORD flags, PVOID reserved)
{
    BOOL ret;
    char *file;

    file = getFullInfPath(string);
    if (!file) {
	return 0;
    }

    /* delete it */
    ret = DeleteFile(file);
    if (ret) {
	int last = strlen(file);

	/* delete the .pnf file */
	file[last-3] = 'p';
	ret = DeleteFile(file);
    }
    free(file);
    return ret;
}

/*
 * SetupUninstallOEMInf is only available in Windows XP or later.
 * make sure eginstall can still run on other versions of Windows
 */
BOOL
generalSetupUninstallOEMInf(PCSTR string, DWORD flags, PVOID reserved)
{
    static uninstallFunc *func = NULL;

    if (func == NULL) {
	HMODULE dll;

	dll = LoadLibrary("SetupAPI.DLL");
	if (dll) {
	   func = (uninstallFunc *) GetProcAddress(dll,
						 "SetupUninstallOEMInfA");
	 }
	 /* we couldn't find the library version, must not a Windows XP.
	 * use the local version */
	 if (!func) {
	    func = localSetupUninstallOEMInf;
	}

    }
    return (*func)(string,flags, reserved);
}

static char *
mkFileName(const char *path, char *base)
{
    char *fileName;
    int len = strlen(path)+strlen(base)+3;

    fileName = malloc(len);
    if (!fileName) {
	return NULL;
    }
    sprintf(fileName,"%s\\%s",base,path);
    return fileName;
}

/*
 * clean up the files.
 *    Walk through list of egate installed files.
 *     old egate files are removed.
 *     If unsinstall is set, current egate files are also removed.
 *     If clean is set, remove all the egate files.
 *    return true if we removed any egate files.
 */
BOOL
cleanupDriverFiles(FILE *logFile, BOOL uninstall, BOOL clean, 
	int currentDriverDate, const unsigned short *currentVersionNumber)
{
    int i;
    char winDir[MAX_PATH];
    BOOL ret;
    BOOL fileRemoved = 0;
    unsigned short version[4];

    GetSystemDirectory(winDir,MAX_PATH);

    for (i = 0; i < FILE_COUNT; i++) {
	int vercmp;
	char *fileName = mkFileName(deviceFileList[i], winDir);

	if (!fileName) continue;

	ret = getFileVersion(fileName, version);
	if (!ret) {
	    int error = GetLastError();
	    /* don't be noisy about missing files */
	    if (error == ERROR_FILE_NOT_FOUND) {
		continue;
	    }
	    fprintf(logFile, "file %s ", fileName);
	    winPerror(logFile, error, "GetFileVersion Failed");
	    /* if we are cleaning up, delete it anyway */
	    if (clean) {
		fprintf(logFile," --- deleting %s: ", fileName);
		ret = DeleteFile(fileName);
		if (ret) {
		    fprintf(logFile,"OK\n");
		} else {
		    winPerror(logFile, GetLastError(), "Failed");
		}
		fileRemoved = 1;
	    }
	    continue;
	}

	/* use adjust Version to compare the version numbers. For files,
	 * we don't have the version date, only install date. We use the
	 * adjustVersion function with the following cooked 'date' version
	 * values to get the following output:
	 *   vercmp is -1 if version < currentVersionNumber.
	 *   vercmp is 0 if version == currentVersionNumber.
	 *   vercmp is 1 if version > currentVersionNUmber.
	 */
	vercmp = adjustVersion( 0, 0,  version, currentVersionNumber);
	fprintf(logFile,"file %s version %d.%d.%d.%d\n",  fileName,
		version[0], version[1], version[2], version[3]);
	if ((clean) || (vercmp < 0) || (uninstall && (vercmp == 0)) ) {
	    fprintf(logFile," --- deleting %s: ", fileName);
	    ret = DeleteFile(fileName);
	    if (ret) {
		fprintf(logFile,"OK\n");
	    } else {
	        winPerror(logFile, GetLastError(), "Failed");
	    }
	    fileRemoved = 1;
	}
	free(fileName);
    }
    return fileRemoved;
}


/*
 * clean up the existing file list
 *    Walk through looking for egate drivers.
 *     old egate drivers are removed.
 *     If unsinstall is set, also remove current egate drivers.
 *     If clean is set, remove all the egate drivers.
 *    return true if we find current versions all egate driver types. (don't
 *       count the ones we remvoed)
 *    If foundArray is passed, return which current types were found (and not
 *        removed ).
 */
BOOL
cleanupInfFileList(FILE *logFile, BOOL doProfile, BOOL uninstall, BOOL clean, 
	int currentDriverDate, const unsigned short *currentVersionNumber,
	const char *OEMDirectory, BOOL *foundArray) 
{
    BOOL ret;
    BOOL found = TRUE;
    BOOL localFoundArray[DRIVER_COUNT];
    DWORD len;
    PTSTR strings;
    char *string;
    char *buf;
    int i;

    TIME current;

    profile(logFile, doProfile, NULL, NULL, 0, &current);

    if (foundArray == NULL) {
	foundArray = localFoundArray;
    }
    memset(foundArray, 0, sizeof(localFoundArray));

    /*
     * Find all the .INF files in the system INF directory.
     */
    ret = allocSetupGetInfFileList(NULL, INF_STYLE_WIN4, &strings, &len);
    if (!ret) {
	winPerror(logFile, GetLastError(), "SetupGetInfFileList");
	return FALSE;
    }
    profile(logFile, doProfile, "Get INF file list", NULL, current, &current);

    /*
     *  Loop through the strings
     */
    for ( string = strings; *string; string += strlen(string)+1) {
	HINF infd;
	SP_ORIGINAL_FILE_INFO fileInfo;
	PSP_INF_INFORMATION sp_inf;
	char *origName = "unknown";
	char *date;
	unsigned short verNum[4];
	int driverDate;
	int isCurrent;
        TIME thisDriver = 0;
 	BOOL oemDirOK = TRUE;

	profile(logFile, doProfile, NULL, NULL, 0, &thisDriver);

        /* filter out the non-OEM files for performance reasons */
	if (!strMatchCase(string,"oem",3)) {
	    continue;
	}

 	/* see if this INF file matches any egate classes */
	for (i=0; i < DRIVER_COUNT; i++) {
	    if (!driverInfo[i].doInstall) {
		continue;
	    }
	    infd = SetupOpenInfFile(string, driverInfo[i].devClass,
				 INF_STYLE_WIN4 | INF_STYLE_OLDNT, NULL);
	    if (infd != INVALID_HANDLE_VALUE) {
		break;
	    }
	}

	/* Nope, go look at the next string */
	if (infd == INVALID_HANDLE_VALUE) {
	    profile(logFile, doProfile, "Open(%s) Failed", string, 
						thisDriver, &thisDriver);
	    continue;
	}
	profile(logFile, doProfile, "Open(%s)", string, 
						thisDriver, &thisDriver);
	getInfVersionNumber(logFile, infd, verNum);
	profile(logFile, doProfile, "GetVersion(%s)", string, 
						thisDriver, &thisDriver);

	/* get the inf information so we can extract the original filename and
	 * version.
	 */
	ret = allocSetupGetInfInformation(infd, INFINFO_INF_SPEC_IS_HINF, 
				&sp_inf, &len);
	if (!ret) {
	    SetupCloseInfFile(infd);
	    winPerror(logFile, GetLastError(), "SetupGetInfInformation");
	    profile(logFile, doProfile, "GetInfInfo(%s) - failed", string, 
						thisDriver, &thisDriver);
	    continue;
	} 
	profile(logFile, doProfile, "GetInfInfo(%s)", string, 
						thisDriver, &thisDriver);

	/* now extract the original file name and "version". Windows
         * uses the driver date as a version number  */
	memset(&fileInfo,0,sizeof(fileInfo));
	fileInfo.cbSize = sizeof(fileInfo);
	ret = SetupQueryInfOriginalFileInformation(sp_inf, 0, NULL,
			&fileInfo);
	if (!ret) {
	   winPerror(logFile, GetLastError(),
				"SetupQueryInfOriginalFileInformation");
	} else {
	    origName = fileInfo.OriginalInfName;
	}
	profile(logFile, doProfile, "QueryInfOriginalFileInfo(%s)", string, 
						thisDriver, &thisDriver);
    	ret = allocSetupQueryInfVersionInformation(sp_inf,0,
					 "DriverVer", &buf, &len);
	if (ret) {
	    date = buf;
	} else {
	    date = strdup("00/00/0000");
	}
	profile(logFile, doProfile, "QueryInfVersionInfo(%s)", string, 
						thisDriver, &thisDriver);
	free(sp_inf);

	/* now we can determine if we are dealing with an egate device or not
	 */
	SetupCloseInfFile(infd);
	if (!( driverInfo[i].devClassUnique ||
			(strcmp(origName, driverInfo[i].infFile) == 0))) {
	    /* not egate, go to the next device */
	    continue;
	}

	/* display the OEM directory status */
	oemDirOK = VerifyInstDir(logFile, string, OEMDirectory);

	fprintf(logFile,"%s: %s  <- %s version: %s, %d.%d.%d.%d %s\n", 
			driverInfo[i].devClass, string, origName,
			date, verNum[0], verNum[1], verNum[2], verNum[3],
			oemDirOK ? "OEM Directroy OK": 
					"OEM Directory out of Date");

	/* now determine if we need to keep this driver or delete it.*/
	driverDate = dateStringToNumber(logFile, date);
	isCurrent = (driverDate == currentDriverDate);


	if ((clean) || (driverDate < currentDriverDate) ||
	       (uninstall && isCurrent) || 
	       (!oemDirOK && isCurrent)) {
	    fprintf(logFile, " --- Deleting %s\n", string);
	    ret = generalSetupUninstallOEMInf(string, SUOI_FORCEDELETE, NULL);
	    if (!ret) {
	   	winPerror(logFile, GetLastError(), "Delete failed");
	    }
	     profile(logFile, doProfile, "Uninstall(%s)", string, 
						thisDriver, &thisDriver);
	} else {
	    strncpy(&dyDriverInfo[i].destPath[0], string,
			sizeof(dyDriverInfo[i].destPath));
	    dyDriverInfo[i].oemFile = &dyDriverInfo[i].destPath[0];
	    foundArray[i] = TRUE;
	}
    }
    profile(logFile, doProfile, "Interate through file list", NULL, 
							current, &current);
    free(strings);

    for (i=0; i < DRIVER_COUNT; i++) {
	found = found && (foundArray[i] || !driverInfo[i].doInstall);
   }
   if (found) {
	fprintf(logFile, "Found all up to date drivers\n");
   }
   return found;
}

#ifdef DEBUG
void
printDevInstall(FILE * logFile, const SP_DEVINSTALL_PARAMS *params)
{
   fprintf(logFile, "Flags : 0x%08x\n", params->Flags);
   fprintf(logFile, "FlagsEx : 0x%08x\n", params->FlagsEx);
   fprintf(logFile, "Driver: %s\n", params->DriverPath);
}
#endif



const char *
typeToString(DWORD type)
{
    switch (type) {
    case SPDIT_CLASSDRIVER:
	return "Class";
    case SPDIT_COMPATDRIVER:
	return "Compat";
    default:
	break;
    }
    return "UNKNOWN";
}

/*
 * print all the drivers associated with a device
 *  return the description and version of the first driver.
 */
BOOL
handleDrivers(FILE * logFile, BOOL doProfile, HDEVINFO hDevInfo, 
   SP_DEVINFO_DATA *sdevp, DWORD type, char *className , BOOL clean, 
   BOOL uninstall, BOOL *needClean, int currentDriverDate, 
   const unsigned short *currentVersionNumber)
{
    SP_DRVINFO_DATA sdrv_data;
    unsigned short versions[4];
    BOOL ret;
    BOOL isCurrent;
    BOOL found = 0;
    char *string = NULL;
    int driverDate;
    char *date;
    int i;
    TIME current;

    profile(logFile, doProfile, NULL, NULL, 0, &current);

    sdevp->cbSize = sizeof(*sdevp);
    ret = SetupDiBuildDriverInfoList(hDevInfo,sdevp, type);
    if (!ret) {
	int error = GetLastError();
	if (error != ERROR_NO_MORE_ITEMS) {
	    winPerror(logFile, error, "BuildDriverInfoList failed:");
	}
	return 0;
    }
    profile(logFile, doProfile, "......  BuildDriverInfoList", NULL,
					current, &current);
    for (i=0; 1; i++) {
	TIME driverTime;
	profile(logFile, doProfile, NULL, NULL, 0, &driverTime);
	sdrv_data.cbSize = sizeof(sdrv_data);
	ret = SetupDiEnumDriverInfo(hDevInfo, sdevp, type, i, &sdrv_data);
	if (!ret) {
	    int error = GetLastError();
	    if (error != ERROR_NO_MORE_ITEMS) {
		winPerror(logFile, error, "Enum Compat Driver failed:");
	    }
	    profile(logFile, doProfile, "...... enumerate DI DONE", NULL, 
				driverTime, &driverTime);
	    break;
	}
	profile(logFile, doProfile, "...... enumerate DI %d", (void *)i, 
				driverTime, &driverTime);
	unpackVersions(versions, sdrv_data.DriverVersion); 
	date = fileTimeToString(&sdrv_data.DriverDate);
	fprintf(logFile,
		"Class %s #%d (%s), \"%s\" Manufacturer:%s Provider:%s"
		" Version:%s,%d.%d.%d.%d\n",
			className, i, typeToString(sdrv_data.DriverType),  
			sdrv_data.Description, 
			sdrv_data.MfgName, sdrv_data.ProviderName, 
			date ? date : "00/00/0000",
			versions[0],versions[1],versions[2],versions[3]);
	driverDate = dateStringToNumber(logFile,
						 date ? date : "00/00/0000");
	if (date) free(date);
	/* is an Egate device, do we need to clean it out?  */

	/* driverDate == 0 is unknown, compare version numbers instead */
	driverDate = adjustVersion(driverDate, currentDriverDate, 
					versions, currentVersionNumber);
	isCurrent = (driverDate == currentDriverDate);
	if ((clean) || (driverDate < currentDriverDate) ||
						(uninstall && isCurrent)) {
	    *needClean = 1;
	} else {
	    found = 1;
	}
	profile(logFile, doProfile, "...... process DI %d", (void *)i, 
				driverTime, &driverTime);
    }
    profile(logFile, doProfile, "......  interate DriverInfoList", NULL,
					current, &current);
    return found;
}

#define PRTBOOL(x) ((x) ? "true" : "false")
void
dumpdev(FILE *logFile, HDEVINFO hdi, PSP_DEVINFO_DATA spd)
{
    char string[MAX_PATH];
    DWORD len;
    BOOL ret;

    string[0] = 0;
    ret = SetupDiGetDeviceRegistryProperty(hdi, spd, SPDRP_DEVICEDESC, NULL, string , sizeof(string), &len);
    fprintf(logFile," DEVICE_DESC: %s (%s)\n",string, PRTBOOL(ret));
    string[0] = 0;
    ret = SetupDiGetDeviceRegistryProperty(hdi, spd, SPDRP_CLASS, NULL, string , sizeof(string), &len);
    fprintf(logFile," CLASS: %s (%s)\n",string, PRTBOOL(ret));
    string[0] = 0;
    ret = SetupDiGetDeviceRegistryProperty(hdi, spd, SPDRP_COMPATIBLEIDS, NULL, string , sizeof(string), &len);
    fprintf(logFile," COMPATIBLEIDS: %s (%s)\n",string, PRTBOOL(ret));
    string[0] = 0;
    ret = SetupDiGetDeviceRegistryProperty(hdi, spd, SPDRP_DRIVER, NULL, string , sizeof(string), &len);
    fprintf(logFile," DRIVER: %s (%s)\n",string, PRTBOOL(ret));
    string[0] = 0;
    ret = SetupDiGetDeviceRegistryProperty(hdi, spd, SPDRP_ENUMERATOR_NAME, NULL, string , sizeof(string), &len);
    fprintf(logFile," ENUMERATOR_NAME: %s (%s)\n",string, PRTBOOL(ret));
    string[0] = 0;
    ret = SetupDiGetDeviceRegistryProperty(hdi, spd, SPDRP_FRIENDLYNAME, NULL, string , sizeof(string), &len);
    fprintf(logFile," FRIENDLYNAME: %s (%s)\n",string, PRTBOOL(ret));
}

/*
 * clean up the currently installed driver
 *    Walk through the existing drivers looking for running egate
 *     instances installed.
 *    Remove all the old egate drivers.
 *    If uninstall is set, remove the current egate drivers as well.
 *    If clean is set, remove all the egate drivers.
 *    returns true if we found *any* egate drivers (even ones we have
 *     removed.
 */

BOOL
cleanupInstalledDrivers(FILE *logFile, BOOL doProfile, BOOL uninstall, BOOL clean,
	 int currentDriverDate, const unsigned short *currentVersionNumber)
{
    LPGUID ClassGuid;
    HDEVINFO hDevInfo;
    SP_DEVINFO_DATA sdinfo_data;
    BOOL ret;
    BOOL isEgate = 0;
    BOOL driverWasRunning = 0;
    BOOL isCurrent = 0;
    BOOL needClean = 0;
    char *idString;
    DWORD len;
    int i, k;
    TIME current;
    BOOL devicesFound = 0;

    profile(logFile, doProfile, NULL, NULL, 0, &current);
    for (k=0; k < DRIVER_COUNT; k++) { 
	/* fetch the could for this class */
	ret = allocSetupDiClassGuidsFromName(driverInfo[k].devClass,
						 &ClassGuid, &len,NULL);
 	profile(logFile, doProfile, "Lookup GUID %s Class", 
				driverInfo[k].devClass, current, &current);
	if (!ret) {
	    int error = GetLastError();
	    fprintf(logFile, "Class <%s> -",driverInfo[k].devClass);
   	    winPerror(logFile, error, "GUID Lookup failed:");
	    continue;
	}
	
	/* Find all the devices associated with this GUID */
	hDevInfo = SetupDiGetClassDevs(ClassGuid, NULL, NULL, 0);
 	profile(logFile, doProfile, "Lookup DEVS %s Class", 
				driverInfo[k].devClass, current, &current);
	free(ClassGuid);
	if (!hDevInfo) {
   	    winPerror(logFile, GetLastError(), "Create Device Info Failed");
	    continue;;
	}

        devicesFound = 1;

	/* loop through, examining each device */
	for (i=0; 1; i++) {
	    TIME devInfoTime = 0;

	    profile(logFile, doProfile, NULL, NULL, 0, &devInfoTime);
	    sdinfo_data.cbSize = sizeof(sdinfo_data);
            ret = SetupDiEnumDeviceInfo(hDevInfo, i, &sdinfo_data);
	    profile(logFile, doProfile, "... Devinfo(%d)", (void *)i, 
						devInfoTime, &devInfoTime);
	    if (!ret) {
		int error = GetLastError();
		if (error != ERROR_NO_MORE_ITEMS) {
		    winPerror(logFile, error, "Enum device failed:");
		}
		break;
	    }
	    ret = allocSetupDiGetDeviceInstanceId(hDevInfo, &sdinfo_data,
					&idString, &len);
	    profile(logFile, doProfile, "... GetDeviceInstanceID(%d)", 
					(void *)i, devInfoTime, &devInfoTime);
	    if (!ret) {
		winPerror(logFile, GetLastError(), "Get Dev ID failed:");
		idString = strdup("Unknown");
	    }
	    isEgate = driverInfo[k].devClassUnique ||
   		(strncmp(idString,driverInfo[k].deviceID,
				strlen(driverInfo[k].deviceID)) == 0);
	    profile(logFile, doProfile, "... Returned id=%s", idString, 
						devInfoTime, &devInfoTime);
#ifdef notdef
	    if (isEgate) {
		fprintf(logFile," ID String: %s\n",idString);
		fprintf(logFile," Class    : %s\n",driverInfo[k].devClass);
	    }
#endif

	    free(idString);
	    idString = NULL; /* paranoia */

	    /* we're only interested in the egate devices. */
	    if (!isEgate) {
		continue;
	    }

	    /* clean up older drivers */
	    needClean = 1;
	    if (driverInfo[k].doInstall) {
		needClean = 0;
		/* we're just tryind to determine if the correct version
		 * of the driver exists, just look at the compat driver info
		 * for this */
#ifdef notdef
		(void )handleDrivers(logFile,doProfile,hDevInfo,&sdinfo_data, 
		  SPDIT_CLASSDRIVER, driverInfo[k].devClass, clean, uninstall,
		  &needClean, currentDriverDate, currentVersionNumber);
		profile(logFile, doProfile, "... handle Clean Class (%s)", 
			driverInfo[k].devClass, devInfoTime, &devInfoTime);
#endif
		(void )handleDrivers(logFile,doProfile,hDevInfo,&sdinfo_data, 
		  SPDIT_COMPATDRIVER, driverInfo[k].devClass, clean, uninstall,
		  &needClean, currentDriverDate, currentVersionNumber);
		profile(logFile, doProfile, "... handle Clean Drivers (%s)", 
			driverInfo[k].devClass, devInfoTime, &devInfoTime);
	    }
	    if (needClean) {
		char *desc = NULL;
		allocSetupDiGetClassDescription(ClassGuid, &desc, &len);
		if (!desc) {
		    desc = strdup(driverInfo[k].devClass);
		}
		fprintf(logFile, "**** Uninstalling %s: ", desc);
		free(desc);
		ret = SetupDiRemoveDevice(hDevInfo, &sdinfo_data);
		profile(logFile, doProfile, "... Remove Drivers (%s)", 
			driverInfo[k].devClass, devInfoTime, &devInfoTime);
		if (ret) {
		    fprintf(logFile," OK\n");
		} else {
		    winPerror(logFile, GetLastError(), " Failed:");
		}
	    }
	    driverWasRunning = 1;
	}
 	profile(logFile, doProfile, "Interate through Dev list for %s Class", 
				driverInfo[k].devClass, current, &current);
	SetupDiDestroyDeviceInfoList(hDevInfo);
 	profile(logFile, doProfile, "Destroy Dev list for %s Class", 
				driverInfo[k].devClass, current, &current);
    }
    return driverWasRunning;
}

/* Install a single device driver. This function selects the driver without
 * UI. A lot of the calls here were determined empirically since this aspect
 * of windows is not well documented.
 */
static void
installDriver(FILE *logFile, const DriverInfo *pdi, const DyDriverInfo *pdd,
    int currentDriverDate, const unsigned short *currentVersionNumber,
							LPGUID ClassGuid)
{
    HDEVINFO hDevInfo;
    SP_DEVINFO_DATA inst_data;
    SP_DEVINFO_DATA sdinfo_data;
    SP_DRVINFO_DATA sdrv_data;
    SP_DEVINFO_DATA *spd = NULL;
    SP_DEVINSTALL_PARAMS devInst;
    BOOL ret;
    char *desc = NULL;
    DWORD len;
    int i;

    fprintf(logFile, "--- Installing driver %s (%s): Class %x ",
						pdd->oemFile, pdi->devClass, ClassGuid);

    /* first make sure the class installer is installed */
    ret = SetupDiInstallClass(NULL, pdd->oemFile, DI_FORCECOPY, NULL); 
    if (!ret) {
	winPerror(logFile, GetLastError(), "Failed:");
	return;
    }

    /* now create the driver info list */
    hDevInfo = SetupDiCreateDeviceInfoList(ClassGuid, NULL);
    if (!hDevInfo) {
	winPerror(logFile, GetLastError(), "Create Device Info Failed");
	return;
    } 

    /*
     * Create the device info structure for our device. We set the description
     * to the modified class name.
     */
    ret = allocSetupDiGetClassDescription(ClassGuid, &desc, &len);
    if (!ret) {
	winPerror(logFile, GetLastError(), "Get Class Description Failed");
    }
    if (desc) {
	desc[strlen(desc)-1] = 0;
    }
    inst_data.cbSize = sizeof(inst_data);
    ret = SetupDiCreateDeviceInfo(hDevInfo, pdi->deviceID, 
		ClassGuid, desc, NULL, pdi->createFlags, &inst_data);
    if (!ret) {
	winPerror(logFile, GetLastError(), "CreateDeviceInfo Failed");
    } else {
	spd = &inst_data;
    }
    if (desc) {
	free(desc);
    }

    /*
     * Now build all the drivers we could possible use for this driver
     */
    ret = SetupDiBuildDriverInfoList(hDevInfo, spd, SPDIT_CLASSDRIVER);
    if (!ret) {
	winPerror(logFile, GetLastError(), "BuildDeviceInfo Failed");
    }

    /*
     * select our device
     */
    ret = SetupDiSetSelectedDevice(hDevInfo, spd);
    if (!ret) {
	winPerror(logFile, GetLastError(), "Set Selected Device Failed");
    }

    /* 
     * select our driver.
     */
    sdrv_data.cbSize = sizeof(sdrv_data);
    for (i=0, ret=1 ; ret; i++) {
    	ret = SetupDiEnumDriverInfo(hDevInfo, spd, SPDIT_CLASSDRIVER, 0, 
    				&sdrv_data);
	if (ret) {
	    /* format our version data */
	    char *date;
	    int driverDate =  0;
	    unsigned short versions[4];

	    unpackVersions(versions, sdrv_data.DriverVersion); 
	    date = fileTimeToString(&sdrv_data.DriverDate);
	    if (date) {
		driverDate = dateStringToNumber(logFile, date);
		free(date);
	    }
	    driverDate = adjustVersion(driverDate, currentDriverDate, versions,
					currentVersionNumber);
	    /* found one! */
	    if (driverDate >= currentDriverDate) {
		break;
	    }
	}
    }
    if (!ret) {
	winPerror(logFile, GetLastError(), "Enum Driver Failed");
    }
    ret = SetupDiSetSelectedDriver(hDevInfo, spd, &sdrv_data);
    if (!ret) {
	winPerror(logFile, GetLastError(), "Set Selected Device Failed");
    }

    /* create an entry in the device registry */
    sdinfo_data.cbSize = sizeof(sdinfo_data);
    ret = SetupDiRegisterDeviceInfo(hDevInfo, spd, 0, NULL, NULL, &sdinfo_data);
    if (!ret) {
	winPerror(logFile, GetLastError(), "Register Device Failed");
    }

    /* exclude the OLD INET devices (Windows documentation claims we need
     * this, I don't know why. */
    devInst.cbSize = sizeof(devInst);
    ret = SetupDiGetDeviceInstallParams(hDevInfo, spd, &devInst);
    if (!ret) {
	winPerror(logFile, GetLastError(), "Get Device Install Params Failed");
    } else {
	devInst.FlagsEx |= DI_FLAGSEX_EXCLUDE_OLD_INET_DRIVERS;
	ret = SetupDiSetDeviceInstallParams(hDevInfo, spd, &devInst);
	if (!ret) {
	    winPerror(logFile, GetLastError(), 
					"Set Device Install Params Failed");
	}
    }

    /* Finally, install the device */
    ret = SetupDiInstallDevice(hDevInfo, spd);
    if (ret) {
	fprintf(logFile," OK\n");
    } else {
	winPerror(logFile, GetLastError(), "Device Install Failed");
    }
    SetupDiDestroyDeviceInfoList(hDevInfo);
}

/*
 * We detected that drivers were previously installed, now try to reinstall
 * those drivers.
 */
void
installRunningDevices(FILE *logFile, BOOL doProfile, int currentDriverDate, 
					const short *currentVersionNumber)
{ 
    LPGUID ClassGuid;
    HDEVINFO hDevInfo;
    SP_DEVINFO_DATA sdinfo_data;
    BOOL ret;
    BOOL found = 0;
    DWORD len;
    int k,i;

    char infpath[512];

    fprintf(logFile,"In installRunning Devices\n");    
    for (k=0; k < DRIVER_COUNT; k++) { 
	found = 0;

	/* only egate unique drivers need to be installed directly,
	 * the general devices are installed by the co-installer.
	 */
	if (!driverInfo[k].handInstall) {
	    continue;
	}

	/*
	 * first make sure the current drivers are running
	 */

        sprintf(infpath,"./%s",driverInfo[k].infFile);

	ret = allocSetupDiClassGuidsFromName(driverInfo[k].devClass,
						 &ClassGuid, &len,infpath);
	if (!ret) {
	    int error = GetLastError();
	    fprintf(logFile, "Class <%s> -",driverInfo[k].devClass);
   	    winPerror(logFile, error, "GUID Lookup failed:");
	    continue;
	}
	
	hDevInfo = SetupDiGetClassDevs(ClassGuid, NULL, NULL, 0);
	if (!hDevInfo) {
   	    winPerror(logFile, GetLastError(), "Create Device Info Failed");
	} else {
	    for (i=0, ret=1; ret; i++) {
		BOOL needClean;
		sdinfo_data.cbSize = sizeof(sdinfo_data);
        	ret = SetupDiEnumDeviceInfo(hDevInfo, i, &sdinfo_data);
		if (!ret) {
		    break;
		}

		found = handleDrivers(logFile,doProfile,hDevInfo,&sdinfo_data, 
		  SPDIT_COMPATDRIVER, driverInfo[k].devClass, FALSE, FALSE,
			&needClean, currentDriverDate, currentVersionNumber);
		if (found) {
		    break;
		}
	   }	
	   SetupDiDestroyDeviceInfoList(hDevInfo);
	}

	/* if the driver was not already installed, install it */
	if (found) {
	    fprintf(logFile, "**** Skipping driver %s\n",
					driverInfo[k].devClass);
	    free(ClassGuid);
	    continue;
	}

	installDriver(logFile, &driverInfo[k], &dyDriverInfo[k], 
			currentDriverDate, currentVersionNumber, ClassGuid);
	free(ClassGuid);
    }
}

	
int main(int argc, char **argv)
{
    char *path = NULL;
    char *prog = *argv++;
    char *logFileName = NULL;
    FILE *logFile = NULL;
    char *cp;
    BOOL userInstall = FALSE;
    BOOL win98Install = FALSE;
    BOOL versionOk = FALSE;
    BOOL uninstall = FALSE;
    BOOL clean = FALSE;
    BOOL runningFound = FALSE;
    BOOL doProfile = FALSE;
    BOOL force = FALSE;
    OSVERSIONINFO versionInfo;
    PSP_INF_INFORMATION sp_inf;
    DWORD len;
    BOOL foundArray[DRIVER_COUNT];
    char *instDir, *end;
    char *date;
    int argCount = 0;
    int currentDriverDate;
    unsigned short currentVersionNumber[4];
    int ret;
    int i;
    TIME current;


    profile(NULL, TRUE, NULL, NULL, 0, &current);

    /* get the install directory */
    instDir = strdup(prog);
    if ((end = strrchr(instDir, '\\')) == 0) {
	free(instDir);
	instDir = getcwd(NULL,0);
    } else {
	*end = 0;
    }
    /* default path the the current directory */
    path = instDir;

    /* get the basename of prog */
    if ((end = strrchr(prog, '\\')) != 0) {
	prog = end+1;
    }

    /*
     * parse the arglist;
     */
    while ((cp = *argv++) != 0) {
	if (*cp == '-') {
	    while (*++cp) switch (*cp) {
	    case 'F':
		userInstall = TRUE;
		break;
	    case '9':
		win98Install = TRUE;
		break;
	    case 'q':
		userInstall = FALSE;
		break;

            case 'f':
               force = TRUE;
               break; 
	    case 'l':
            
		logFileName = *argv++;
		if (logFileName == NULL) {
		    usage(prog);
		    return 2;
		}
		break;
	    case 'r':
		uninstall = TRUE;
		break;
	    case 'p':
		doProfile = TRUE;
		break;
	    case 'c':
		clean = TRUE;
		useOS = TRUE;
		break;
	    /* use the OS calls. slower, but more reliable. */
	    case 'O':
		useOS = TRUE;
		break;
	    default:
		usage(prog);
		return 2;
	    }
	} else switch (argCount++) {
	case 0:
	    path = cp;
	    break;
	default:
	    usage(prog);
	    return 2;
	}
    }

    versionInfo.dwOSVersionInfoSize = sizeof(versionInfo);
    versionOk = GetVersionEx(&versionInfo);

    if (!versionOk || (versionInfo.dwMajorVersion < 5)) {
	win98Install = TRUE;
    }

    /*
     * open the log file
     */
    if (logFileName == NULL) {
	if (userInstall) {
	    logFileName = "stdout";
	} else {
	    sprintf(logFilePath,"%s\\%s",instDir,LOGFILE);
	    logFileName = logFilePath;
	}
    }
    if (strcmp(logFileName,"stdout") == 0) {
	logFile = stdout;
    } else {
	time_t now = time(NULL);
	logFile = fopen(logFileName, "a");
	if (logFile) fprintf(logFile,"%s",ctime(&now));
    }

    if (logFile == NULL) {
	if (userInstall) {
	    fprintf(stderr,"Couldn't open logfile %s\n",logFileName);
	}
	return 1;
    }

    if (win98Install) {
	fprintf(logFile,"Windows 98 install of egate not supported\n");
	fprintf(logFile,"versionOK = %s\n", versionOk ? ".true.": ".false.");
	fprintf(logFile,"version = %d.%d\n",
			versionInfo.dwMajorVersion,versionInfo.dwMinorVersion);
	return 1;
    }
    profile(logFile, doProfile, "startup", NULL, current, &current);

    sprintf(infFilePath,"%s\\%s",path,driverInfo[0].infFile);
    if (!clean) {
	HINF handle;
	TIME cleanTime;
	profile(logFile, doProfile, NULL, NULL, 0, &cleanTime);

	handle = SetupOpenInfFile(infFilePath, NULL, INF_STYLE_WIN4, NULL);
	if (handle == INVALID_HANDLE_VALUE) {
	    winPerror(logFile, GetLastError(), infFilePath);
	    return 1;
	}
	profile(logFile, doProfile, "Open INF File (%s)", 
					infFilePath, current, &current);
	getInfVersionNumber(logFile, handle, currentVersionNumber);
	ret = allocSetupGetInfInformation( handle,
		 		INFINFO_INF_SPEC_IS_HINF, &sp_inf, &len);
	if (!ret) {
	    winPerror(logFile, GetLastError(), infFilePath);
	    SetupCloseInfFile(handle);
	    return 1;
	}
	profile(logFile, doProfile, "Open allocInfo", NULL, current, &current);
	/* Windows returns the date as the version number! */
	ret = allocSetupQueryInfVersionInformation(sp_inf,0, 
						"DriverVer", &date, &len);
	free(sp_inf);
	if (!ret) {
	    winPerror(logFile, GetLastError(), "Find Version");
	    SetupCloseInfFile(handle);
	    return 1;
	}
	profile(logFile, doProfile, "Query Version", NULL, current, &current);
	fprintf(logFile,"Installing Egate version %s, %d.%d.%d.%d\n", date,
		currentVersionNumber[0],
		currentVersionNumber[1],
		currentVersionNumber[2],
		currentVersionNumber[3]);
	currentDriverDate=dateStringToNumber(logFile,date);
	free(date);
	SetupCloseInfFile(handle);
	if (currentDriverDate < 1) {
	    return 1;
	}
	profile(logFile, doProfile, "Get Version Total time", NULL, 
							cleanTime, &current);
    }
    /* cleanup running Files */
    runningFound = cleanupInstalledDrivers(logFile, doProfile, uninstall, 
			  clean, currentDriverDate, currentVersionNumber);
    profile(logFile, doProfile, "Cleanup installed drivers", NULL, 
							current, &current);

    /* cleanup old installed driver files */
    cleanupDriverFiles(logFile, uninstall, clean, 
				currentDriverDate, currentVersionNumber);

    profile(logFile, doProfile, "Cleanup driver files", NULL, 
							current, &current);
   

    /* update INF FILES */
    ret = cleanupInfFileList(logFile, doProfile, uninstall, clean, 
		currentDriverDate, currentVersionNumber, path, foundArray);
    profile(logFile, doProfile, "Cleanup INF file lists", NULL, 
							current, &current);
    if (!(ret || clean || uninstall)) {
	for (i=0; i < DRIVER_COUNT; i++) {
	   if (!driverInfo[i].doInstall) {
		continue;
	   }
	   if (foundArray[i]) {
		fprintf(logFile,"%s already installed\n",driverInfo[i].infFile);
	   } else {
    		sprintf(infFilePath,"%s\\%s",path,driverInfo[i].infFile);
		fprintf(logFile, "Installing %s: ",infFilePath);
		ret = SetupCopyOEMInf( infFilePath, path, SPOST_PATH, 
			SP_COPY_NOOVERWRITE, dyDriverInfo[i].destPath, 
			sizeof(dyDriverInfo[i].destPath), NULL, 
			&dyDriverInfo[i].oemFile);
		if (ret) {
		    fprintf(logFile,"OK\n");
		} else {
		    winPerror(logFile, GetLastError(), "Failed");
		}
	    }
	}
    }
    profile(logFile, doProfile, "CopyOEMInf", NULL, current, &current);

    /* if drivers were running at install time, restart them all */
    if ((force || runningFound) && !uninstall && !clean) {
	installRunningDevices(logFile, doProfile, currentDriverDate, 
						currentVersionNumber);
        profile(logFile, doProfile, "install running devices", NULL, 
							current, &current);
    }

    /* print final profile message *BEFORE* we close the logfile */
    profile(logFile, doProfile, "DONE", NULL, current, &current);
    if (logFile != stdout) {
	fclose(logFile);
    }

    return 0;
}

#else
/* MAC & Linux */
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <fts.h>
#ifdef MAC
#include <Security/Authorization.h>
#include <Security/AuthorizationTags.h>
#endif

#include "instlib.h"

static void
usage(char *prog)
{
    fprintf(stderr,"usage: %s [-F][-q][-l logfile] [egate_path]\n",prog);
    return;
}

//Are we Mac OS X Tiger OS

int IsTiger = 0;

#ifdef MAC
const char readerConf[] = {
"FRIENDLYNAME	\"SchlumbergerSema eGate\"\n"
"DEVICENAME	MY_DEVICE\n"
"LIBPATH 	/usr/libexec/SmartCardServices/drivers/slbEGate.bundle\n"
"CHANNELID	0x00000000\n"
};

const char *TigerLibPath  = "/usr/libexec/SmartCardServices/drivers/ifdGate.bundle";

#else
const char readerConf[] = {
"FRIENDLYNAME	\"SchlumbergerSema eGate\"\n"
"DEVICENAME	MY_DEVICE\n"
"LIBPATH 	/etc/reader/slbEGate.bundle/Contents/Linux/slbEGate\n"
"CHANNELID	0x00000000\n"
};
#endif
	
int main(int argc, char **argv)
{
    char *path = NULL;
    char *prog = *argv++;
    char *logFileName = NULL;
    char * muscleDir;
    char * restart_pcscd;
    char *cp;
    char *base;
    int versionOk = 0;
    int userInstall = 1;
    int force = 0;
    int skip_reader_conf = 0;
    FILE *reader_conf = NULL;
    int skip_plugin = 0;
    int egate_found = 0;
    int restart = 0;
    int argCount = 0;
    int i;
    char *egate_path = 0;
    char *install_path = 0;
    int return_code = 0; /* return code for this function */
    int rc = 0; /* return code from functions called from here */
    char bufp[MAXPATHLEN];
    char *buf;
    int bufSize;
    char *fullProg;
    int pid;

    /* get the install directory */
    path = GetFullPath(prog,0);
    fullProg = GetFullPath(prog,1);
    buf = bufp;
    bufSize = sizeof(bufp);

    /* make sure we don't over flow the buffer in the sprintf */
    if (strlen(path)+20 > MAXPATHLEN) {
	bufSize=strlen(path)+20;
	buf = (char *)malloc(bufSize);
	if (buf == 0) {
	   return ARG_ERR(1);
	}
    }

    /* get the basename of prog */
    if ((cp = strrchr(prog, '/')) != 0) {
	prog = cp+1;
    }

    /*
     * parse the arglist;
     */
    while (cp = *argv++) {
printf("argv=[%s]\n",cp);
	if (*cp == '-') {
	    while (*++cp) switch (*cp) {
	    case 'l':
		logFileName = *argv++;
		if (logFileName == NULL) {
		    usage(prog);
		    return ARG_ERR(2);
		}
		break;
	    case 'F':
		userInstall = 1;
		break;
	    case 'f':
		force = 1;
		break;
	    case 'q':
		userInstall = 0;
		break;
	    case '#':
		restart = 1;
		break;
	    default:
		usage(prog);
		return ARG_ERR(2);
	    }
	} else switch (argCount++) {
	case 0:
	    path = cp;
	    break;
	default:
	    usage(prog);
	    return ARG_ERR(2);
	}
    }

#ifdef MAC

    SInt32 MacVersion;

    Gestalt('sysv', &MacVersion);

    char temp[100];

    sprintf(temp,"%x",MacVersion);

    syslog(LOG_INFO, "OS X kernel version hex. %s \n",temp);

    int isTigerDotTwo  = 0;

    if( MacVersion >=  0x1042)
    {
        isTigerDotTwo = 1;
    }
    else
    {
        syslog(LOG_INFO, "OS X kernel not 10.4.2. \n");
    }

    if(MacVersion >= 0x1030 && MacVersion <= 0x1039)
    {
        syslog(LOG_INFO,"OS X kernel is Panther. \n");
    }

    if(MacVersion >= 0x1040 && MacVersion <= 0x1049)
    {
        IsTiger = 1;
        syslog(LOG_INFO,"OS X kernel is Tiger. \n");
    }
#endif

    if (geteuid() != 0) {
#ifdef MAC
	/* we aren't root, get privs we need */
	AuthorizationRef auth;
	int status;
	int pid;
	char *args[4]; 
	char arglist[7]; 
	int nextArg = 0;
	int nextArgList = 0;

	if (restart == 1) {
	   /* log error ? */
	    syslog(LOG_ERR,"Restart without root!\n");
	   return ARG_ERR(3);
	}

	args[nextArg++] = arglist;
	arglist[nextArgList++] = '-';
	arglist[nextArgList++] = '#';
	arglist[nextArgList++] = userInstall ? 'F': 'q';
	if (force) {
	    arglist[nextArgList++] = 'f';
	}
	if (logFileName) {
	    arglist[nextArgList++] = 'l';
	    args[nextArg++] = logFileName;
	}
	arglist[nextArgList++] = 0;
	args[nextArg++] = NULL;

	rc = AuthorizationCreate(NULL,kAuthorizationEmptyEnvironment,
			kAuthorizationFlagPreAuthorize, &auth);
	if (rc != 0) {
	    syslog(LOG_ERR,"AuthorizationCreate failed rc=%d\n",rc);
	    return AUTH_ERR(rc);
	}
	rc = AuthorizationExecuteWithPrivileges(auth,fullProg,
		kAuthorizationFlagDefaults, args, NULL);
	if (rc != 0) {
	    syslog(LOG_ERR,
		"Couldn't get admin privilege to install aolkey. (%d)\n", rc);
	    return AUTH_ERR(rc);
	}
	pid = wait(&status);
	if (pid == -1 || !WIFEXITED(status)) {
	    syslog(LOG_ERR,"Couldn't get return status pid=%d status=0x%x\n",
				pid, status);
	    return PID_ERR(status);
	}
	return(WEXITSTATUS(status));
#else
	fprintf(stderr,"eginstall must be run as root\n");
#endif
    }

    /* first, install reader.conf */
    /* verify that egate is not yet installed */
    reader_conf = fopen("/etc/reader.conf","r");
    if (reader_conf && !IsTiger) {
	char *str;

	while ((str=fgets(buf, bufSize, reader_conf)) != NULL) {
	    if (strncmp(str, "FRIENDLYNAME", sizeof("FRIENDLYNAME")-1) == 0) {
	        if (HasString(str,"egate")) {
		    egate_found =1;
		    continue;
		}
	    }
	    if ((egate_found) &&  
			(strncmp(str,"LIBPATH",sizeof("LIBPATH")-1) == 0)) {
		str += sizeof("LIBPATH") -1;

		/* strip the white space ... */
		/* .. from the beginning */
		while (*str && isblank(*str)) {
		    str++;
		}
		/* .. and from the end */
		if (*str) {
		    int len = strlen(str)-1;
		    while (len && (isblank(str[len]) || (str[len] == '\n'))) {
			len --;
		    }
		    str[len+1] = 0;
		}

		if (*str) {
		    egate_path = strdup(str);
		}
	    }
	}
	fclose(reader_conf);
	if (!egate_path) {
	   RmFile("/etc/_reader.conf");
	   Copy("/etc/reader.conf","/etc/_reader.conf", 0644);
	}
    } else {
	(void)RmFile("/etc/_reader.conf");
    }
    sprintf(buf,"%s/slbEGate.bundle",path);
    install_path = strdup(buf);
    if (egate_path) {
	unsigned long current_version, install_version;
	current_version = GetBundleVersion(egate_path);
	install_version = GetBundleVersion(install_path);
	if (current_version <= install_version) {
	    RmFile("/etc/reader.conf");
	    syslog(LOG_INFO,"Replacing version %d with version %d of %s\n",
				current_version, install_version, "slbEGate.bundle");
	} else {
	    syslog(LOG_INFO,
	    	    "Newer version of E-Gate already installed"
	    	    "(oldversion = %d, newversion = %d)\n",
			current_version, install_version);
	    skip_reader_conf = 1;
	}
	free(egate_path);
    }

    if (!skip_reader_conf) {
	/* 
	 * we use _reader.conf so that we don't wind up with a partially
	 * updated file that we can't later recover from.
	 */

    if(!IsTiger)
    {    
        int readerfd;
        readerfd = open("/etc/_reader.conf", O_CREAT|O_APPEND|O_WRONLY, 0644);
        if (readerfd < 0) {
	    syslog(LOG_ERR,
		"Couldn't open /etc/_reader.conf for update err=%d (%s)\n", 
				errno, strerror(errno));
	    return RDRCNF_ERR(errno);
        }
	rc = write(readerfd, readerConf, sizeof(readerConf)-1);
	if (rc < 0) {
	    syslog(LOG_ERR,
		"Write Failure on /etc/_reader.conf err=%d (%s) fd=%d\n", 
				errno, strerror(errno), readerfd);
	    return RDRCNF_ERR(errno);
        } else if (rc < sizeof(readerConf)-1) {
	    syslog(LOG_ERR,"only wrote %d bytes to /etc/_reader.conf\n", rc);
	    return RDRCNF_ERR(errno);
	}
	close(readerfd);

        
    }

     #ifdef MAC

        // Install in same place whether Panther or Tiger
        rc = Cpdir(install_path,TigerLibPath);

     #else

	rc = Cpdir(install_path,"/etc/reader/slbEGate.bundle");

     #endif

	if (rc < 0) {
	    syslog(LOG_ERR, "Failed to copy egate driver %s"
		" to slbEGate.bundle err=%d (%s)\n", install_path,
				errno, strerror(errno));
	    return RDRCNF_ERR(errno);
	}

        if(!IsTiger)
        {
	    (void)RmFile("/etc/reader.conf");
	    rc = rename("/etc/_reader.conf","/etc/reader.conf");
	    if (rc < 0) {
	        syslog(LOG_ERR, "Failed to rename /etc/_reader.conf"
	        	" to /etc/reader.conf err=%d\n", errno, strerror(errno));
	     return RDRCNF_ERR(errno);
            }

        }

	syslog(LOG_INFO, "Installed egate. ");
    } 

    pid = FindProcessByName("pcscd");

    /* If nothing to install, don't restart pcscd */
    if (skip_reader_conf && pid != -1) {
	syslog(LOG_INFO, 
	   "Nothing new to install, pcscd running: skip restarting pcscd\n");
	return return_code;
    }


    if (pid != -1) {
	syslog(LOG_INFO,"Killing PCSCD process id %d\n", pid);

	kill( pid, SIGTERM );
	/* wait 10 seconds for the process to die */
	rc = WaitForProcessDeath(pid,10);
	if (rc == PROCESS_ALIVE) {
	   kill(pid, SIGKILL);
	   rc = WaitForProcessDeath(pid,10);
	}
	if (rc != PROCESS_DEAD) {
	   syslog(LOG_ERR,"PCSCD still running!!!");
	   return RESTART_ERR(errno);
	}
    }

    if (FileExists("/tmp/pcsc")) {
	/* clean up old pcsc files */
	syslog(LOG_INFO,"Cleaning up Old PCSC Files\n");
	rc = RmDir("/tmp/pcsc");
	if (rc != 0) {
	   syslog(LOG_ERR,"Failed to clean up /tmp/pcsc\n");
	   return_code =  CLEANUP_ERR(errno);
	}
    }

    /* now start pcscd */
#ifdef MAC
    if(!IsTiger)
    {
        rc = system("/usr/sbin/pcscd");
    }
    else
    {
        rc = 0;
        syslog(LOG_INFO,"Tiger 10.4.2 and later, does not  start /usr/sbin/pcscd \n");

    }
#else
    rc = system("/usr/local/sbin/pcscd");
#endif
    if (rc < 0) {
	syslog(LOG_ERR,"Could't restart PCSCD err=%d (%s)\n",
						errno, strerror(errno));
	return_code =  EXEC_ERR(errno);
   }

#ifdef MAC
    /* TODO this needs to be done for linux */
    /* copy the restart command to /usr/sbin */
    sprintf(buf,"%s/pcscd_restart",path);

    /* do we need to verify a hash to make sure we are installing
     * the correct pcscd_restart program? */

    /* copy restart to /usr/sbin. We do this because once we install it,
     * it cannot be removed unless the user becomes root. This allows the
     * user to move around the photon installation without loosing the
     * restart program. */
    rc = Copy(buf,"/usr/sbin/pcscd_restart",0755);
    if (rc < 0) {
	/* not fatal.. */
        syslog(LOG_INFO,"Couldn't copy pcscd_restart to /usr/sbin err=%d\n",
									errno);
    } else {
	rc = Setuid("/usr/sbin/pcscd_restart");
	if (rc < 0) {
	    /* not fatal... */
            syslog(LOG_INFO,"Couldn't setuid on pcscd_restart err=%d\n", errno);
	}
    }


    /* make sure the restart code works */

    if(!isTigerDotTwo)
    {
        rc = SetupSystemBoot();
        if (rc < 0) {
	/* not fatal.. */
            syslog(LOG_INFO,"Couldn't setup restart on boot err=%d\n",errno);
        }
    }
    else
    {
        syslog(LOG_INFO,"Tiger 10.4.2 and later, don't do SetupSystemBoot to setup restart on boot.\n");

        rc = RemoveSystemBoot();

        if( rc < 0)
        {
            syslog(LOG_INFO,"Couldn't remove restart on boot err=%d\n",errno);
        }

    }
#endif

    syslog(LOG_INFO,"Installation complete. return_code=%d\n",return_code);
    return return_code;
}
#endif
