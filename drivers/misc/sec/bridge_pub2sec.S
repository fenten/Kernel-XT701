/*
 * Copyright (c) 2006-2008 Trusted Logic S.A.
 * All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

.text

#define SMICODEPUB_IRQ_END   0xFE
#define SMICODEPUB_FIQ_END   0xFD
#define SMICODEPUB_RPC_END   0xFC

pub2sec_bridge_entry:
   .global pub2sec_bridge_entry

      PUSH     {R4-R12, LR}
      // Copy the Secure Service ID in R12
      MOV      R6, #0xFF
      MOV      R12, R0   // not needed on ES_2_0

      MCR      p15, 0, R0, c7, c5, 4   // Prefetch Buffer flush
      MCR      p15, 0, R0, c7, c10, 4  // Data Synchro Barrier

      SMC      1
      B        service_end
      NOP
      BL       v7_flush_kern_cache_all
      MOV      R12, #SMICODEPUB_IRQ_END
      SMC      1

service_end:
      POP      {R4-R12, LR}
      BX       LR
