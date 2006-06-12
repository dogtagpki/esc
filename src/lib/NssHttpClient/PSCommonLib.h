/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 */
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
#pragma once

#ifndef _PS_COMMON_LIB_H_
#define _PS_COMMON_LIB_H_

#undef EXPORT_DECL
#ifdef _MSC_VER
#ifdef COMMON_LIB_DLL
#define EXPORT_DECL __declspec( dllexport )
#else
#define EXPORT_DECL __declspec (dllimport )
#endif // COMMON_LIB_DLL
#else
#define EXPORT_DECL
#endif // _MSC_VER

#endif // _PS_COMMON_LIB_H_
