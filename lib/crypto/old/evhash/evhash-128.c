/*
 * evhash-128.c
 *
 * 128bit evaluation hash algorithm for the q3p crypto engine
 * 
 * Author: Thomas Themel - thomas.themel@ait.ac.at
 *
 * Copyright (C) 2010-2015 Austrian Institute of Technology
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

#define GF_BITS 128
#define TWO_STEP_PRECALC

/* GF(2^128) as GF(2)[x] mod x^128+x^7+x^2+x+1 */
/* field element congruent with irreducible polynomial: 135 */
static unsigned int gf_modulus = 0x87 ;


#include "evhash-impl.c"
