// ****************************************************************************
//
// Copyright (c) 2003 America Online, Inc.  All rights reserved.
// This software contains valuable confidential and proprietary information
// of America Online, Inc. and is subject to applicable licensing agreements.
// Unauthorized reproduction, transmission or distribution of this file and
// its contents is a violation of applicable laws.
//
//           A M E R I C A   O N L I N E   C O N F I D E N T I A L
//
// ****************************************************************************
#ifndef AOL_NKEY_MYPKCS11_H
#define AOL_NKEY_MYPKCS11_H

#if defined(_WIN32)
#define CK_PTR *
#define CK_DECLARE_FUNCTION(rv,func) rv __declspec(dllexport) func
#define CK_DECLARE_FUNCTION_POINTER(rv,func) rv (* func)
#define CK_CALLBACK_FUNCTION(rv,func) rv (* func)
#define CK_NULL_PTR 0
#else
#define CK_PTR *
#define CK_DECLARE_FUNCTION(rv,func) rv func
#define CK_DECLARE_FUNCTION_POINTER(rv,func) rv (* func)
#define CK_CALLBACK_FUNCTION(rv,func) rv (* func)
#define CK_NULL_PTR 0
#endif

#if defined(_WIN32)
#pragma warning(disable:4103)
#pragma pack(push, cryptoki, 1)
#endif

#include "pkcs11.h"

//#include "pkcs11n.h"

#if defined (_WIN32)
#pragma warning(disable:4103)
#pragma pack(pop, cryptoki)
#endif


#endif
