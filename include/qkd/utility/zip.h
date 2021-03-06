/*
 * zip.h
 * 
 * a zip utility to quickly deflate and inflate some stuff in memory
 *
 * Author: Oliver Maurhart, <oliver.maurhart@ait.ac.at>
 *
 * Copyright (C) 2012-2016 AIT Austrian Institute of Technology
 * AIT Austrian Institute of Technology GmbH
 * Donau-City-Strasse 1 | 1220 Vienna | Austria
 * http://www.ait.ac.at
 *
 * This file is part of the AIT QKD Software Suite.
 *
 * The AIT QKD Software Suite is free software: you can redistribute 
 * it and/or modify it under the terms of the GNU General Public License 
 * as published by the Free Software Foundation, either version 3 of 
 * the License, or (at your option) any later version.
 * 
 * The AIT QKD Software Suite is distributed in the hope that it will 
 * be useful, but WITHOUT ANY WARRANTY; without even the implied warranty 
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with the AIT QKD Software Suite. 
 * If not, see <http://www.gnu.org/licenses/>.
 */

 
#ifndef __QKD_UTILITY_ZIP_H_
#define __QKD_UTILITY_ZIP_H_


// ------------------------------------------------------------
// incs

// ait
#include <qkd/utility/memory.h>


// ------------------------------------------------------------
// decls


namespace qkd {
    
namespace utility {    


/**
 * this class lets a memory be deflated and inflate by using zlib
 */
class zip {


public:
    
    
    /**
     * compress a memory area
     * 
     * @param   cMemory         the memory to compress
     * @return  the compressed memory
     */
    static qkd::utility::memory deflate(qkd::utility::memory const & cMemory);
    

    /**
     * decompress a memory area
     * 
     * @param   cMemory         the memory to decompress
     * @return  the decompressed memory
     */
    static qkd::utility::memory inflate(qkd::utility::memory const & cMemory);
    

private:


    /**
     * ctor
     */
    zip() {};

};



}

}

#endif

