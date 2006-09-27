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

#ifndef _ITERATOR_H_
#define _ITERATOR_H_

/**
 * Base class for iterators
 */

class EXPORT_DECL Iterator {
public:
    /**
     * Returns true if there is at least one more element
     *
     * @return true if there is at least one more element
     */
    virtual bool HasMore() = 0;

   /**
     * Returns the next element, if any
     *
     * @return The next element, if any, or NULL
     */
    virtual void *Next() = 0;

    virtual ~Iterator(){}; 

};

#endif // _ITERATOR_H_
