/*
 * tms320c28x interrupt.
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

void tms320c28x_cpu_do_interrupt(CPUState *cs)
{
#ifndef CONFIG_USER_ONLY
    // Tms320c28xCPU *cpu = TMS320C28X_CPU(cs);
    // CPUTms320c28xState *env = &cpu->env;
    // int exception = cs->exception_index;

    // env->epcr = env->pc;
    // if (exception == EXCP_SYSCALL) {
    //     env->epcr += 4;
    // }
    // /* When we have an illegal instruction the error effective address
    //    shall be set to the illegal instruction address.  */
    // if (exception == EXCP_ILLEGAL) {
    //     env->eear = env->pc;
    // }

    // /* During exceptions esr is populared with the pre-exception sr.  */
    // env->esr = cpu_get_sr(env);
    // /* In parallel sr is updated to disable mmu, interrupts, timers and
    //    set the delay slot exception flag.  */
    // env->sr &= ~SR_DME;
    // env->sr &= ~SR_IME;
    // env->sr |= SR_SM;
    // env->sr &= ~SR_IEE;
    // env->sr &= ~SR_TEE;
    // env->pmr &= ~PMR_DME;
    // env->pmr &= ~PMR_SME;
    // env->lock_addr = -1;

    // /* Set/clear dsx to indicate if we are in a delay slot exception.  */
    // if (env->dflag) {
    //     env->dflag = 0;
    //     env->sr |= SR_DSX;
    //     env->epcr -= 4;
    // } else {
    //     env->sr &= ~SR_DSX;
    // }

    // if (exception > 0 && exception < EXCP_NR) {
    //     static const char * const int_name[EXCP_NR] = {
    //         [EXCP_RESET]    = "RESET",
    //         [EXCP_BUSERR]   = "BUSERR (bus error)",
    //         [EXCP_DPF]      = "DFP (data protection fault)",
    //         [EXCP_IPF]      = "IPF (code protection fault)",
    //         [EXCP_TICK]     = "TICK (timer interrupt)",
    //         [EXCP_ALIGN]    = "ALIGN",
    //         [EXCP_ILLEGAL]  = "ILLEGAL",
    //         [EXCP_INT]      = "INT (device interrupt)",
    //         [EXCP_DTLBMISS] = "DTLBMISS (data tlb miss)",
    //         [EXCP_ITLBMISS] = "ITLBMISS (code tlb miss)",
    //         [EXCP_RANGE]    = "RANGE",
    //         [EXCP_SYSCALL]  = "SYSCALL",
    //         [EXCP_FPE]      = "FPE",
    //         [EXCP_TRAP]     = "TRAP",
    //     };

    //     qemu_log_mask(CPU_LOG_INT, "INT: %s\n", int_name[exception]);

    //     hwaddr vect_pc = exception << 8;
    //     if (env->cpucfgr & CPUCFGR_EVBARP) {
    //         vect_pc |= env->evbar;
    //     }
    //     if (env->sr & SR_EPH) {
    //         vect_pc |= 0xf0000000;
    //     }
    //     env->pc = vect_pc;
    // } else {
    //     cpu_abort(cs, "Unhandled exception 0x%x\n", exception);
    // }
#endif

    cs->exception_index = -1;
}

bool tms320c28x_cpu_exec_interrupt(CPUState *cs, int interrupt_request)
{
    // Tms320c28xCPU *cpu = TMS320C28X_CPU(cs);
    // CPUTms320c28xState *env = &cpu->env;
    // cpu->parent_obj.interrupt_request == interrupt_request ??
    // todo p59 in datasheet, standard operation for cpu maskable interrupts
    int idx = -1;

    // if ((interrupt_request & CPU_INTERRUPT_HARD) && (env->sr & SR_IEE)) {
    //     idx = EXCP_INT;
    // }
    // if ((interrupt_request & CPU_INTERRUPT_TIMER) && (env->sr & SR_TEE)) {
    //     idx = EXCP_TICK;
    // }
    if (idx >= 0) {
        cs->exception_index = idx;
        tms320c28x_cpu_do_interrupt(cs);
        return true;
    }
    return false;
}
