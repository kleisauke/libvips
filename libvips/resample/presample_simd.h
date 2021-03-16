/* base class for all SIMD resample operations
 *
 * 16/03/21 kleisauke
 *	- from presample.h
 */

/*

    Copyright (C) 1991-2005 The National Gallery

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301  USA

 */

/*

    These files are distributed with VIPS - http://www.vips.ecs.soton.ac.uk

 */

#ifndef VIPS_PRESAMPLE_SIMD_H
#define VIPS_PRESAMPLE_SIMD_H

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

void vips_reduce_uchar_avx2( VipsPel *pout, VipsPel *pin,
	int n, int ne, int lskip, const short *restrict k );

void vips_reduce_uchar_sse41( VipsPel *pout, VipsPel *pin,
	int n, int ne, int lskip, const short *restrict k );

#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif /*VIPS_PRESAMPLE_SIMD_H*/
