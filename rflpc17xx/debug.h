/* This file is part of rflpc.                        
 *									 
 * rflpc is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by	 
 * the Free Software Foundation, either version 3 of the License, or	 
 * (at your option) any later version.					 
 * 									 
 * rflpc is distributed in the hope that it will be useful,		 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of	 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the	 
 * GNU General Public License for more details.				 
 * 									 
 * You should have received a copy of the GNU General Public License	 
 * along with rflpc.  If not, see <http://www.gnu.org/licenses/>.	 
 */

#ifndef __RFLPC_DEBUG_H__
#define __RFLPC_DEBUG_H__

#include "drivers/leds.h"

/** wait until a counter reaches c. Used to wait some time. Depends on CPU frequency */
#define RFLPC_DELAY(c) do {int i;uint32_t j; for (i=0 ; i < (c) ; ++i) j = LPC_SC->SCS; } while (0)

/** Stops execution by an infinite loop, switching between led pattern l and its opposite */
#define RFLPC_STOP(l,c) do {uint32_t leds = (l); while(1){rflpc_led_val(leds);RFLPC_DELAY((c));leds=~leds;}} while(0)

#endif
