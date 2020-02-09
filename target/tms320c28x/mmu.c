/*
 * TMS320C28X MMU.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "qemu/osdep.h"
#include "cpu.h"
#include "exec/exec-all.h"
#include "exec/gdbstub.h"
#include "qemu/host-utils.h"
#ifndef CONFIG_USER_ONLY
#include "hw/loader.h"
#endif

#ifndef CONFIG_USER_ONLY
static inline void get_phys_nommu(hwaddr *phys_addr, int *prot,
                                  target_ulong address)
{
    *phys_addr = address;
    *prot = PAGE_READ | PAGE_WRITE | PAGE_EXEC;
}
#endif

bool tms320c28x_cpu_tlb_fill(CPUState *cs, vaddr addr, int size,
                           MMUAccessType access_type, int mmu_idx,
                           bool probe, uintptr_t retaddr)
{
    // Tms320c28xCPU *cpu = TMS320C28X_CPU(cs);
    // int excp = 0;

#ifndef CONFIG_USER_ONLY
    int prot;
    hwaddr phys_addr;

    /* The mmu is disabled; lookups never fail.  */
    get_phys_nommu(&phys_addr, &prot, addr);
    tlb_set_page(cs, addr & TARGET_PAGE_MASK,
                    phys_addr & TARGET_PAGE_MASK, prot,
                    mmu_idx, TARGET_PAGE_SIZE);
    return true;
#endif
    cpu_loop_exit_restore(cs, retaddr);
}

#ifndef CONFIG_USER_ONLY
hwaddr tms320c28x_cpu_get_phys_page_debug(CPUState *cs, vaddr addr)
{
    int prot;
    hwaddr phys_addr;

    /* The mmu is definitely disabled; lookups never fail.  */
    get_phys_nommu(&phys_addr, &prot, addr);
    return phys_addr;
}
#endif
