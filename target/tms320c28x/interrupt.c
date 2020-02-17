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

static void st32_swap(CPUTms320c28xState *env, target_ulong addr, uint32_t value)
{
    uint32_t tmp, tmp2;

    // ABCD -> CDAB
    tmp = value << 16;
    tmp2 = value >> 16;
    value = tmp | tmp2;

    //CDAB -> DCBA
    tmp = value << 8;
    tmp = tmp & 0xff00ff00;
    tmp2 = value >> 8;
    tmp2 = tmp2 & 0x00ff00ff;
    value = tmp | tmp2;

    cpu_stl_data(env, addr * 2, value);
}

static uint32_t ld32_swap(CPUTms320c28xState *env, target_ulong addr)
{
    uint32_t value = cpu_ldl_data(env, addr * 2);

    uint32_t tmp, tmp2;
    // ABCD -> BADC
    tmp = value << 8;
    tmp = tmp & 0xff00ff00;
    tmp2 = value >> 8;
    tmp2 = tmp2 & 0x00ff00ff;
    value = tmp | tmp2;
    // BADC -> DCBA
    tmp = value << 16;
    tmp2 = value >> 16;
    value = tmp | tmp2;

    return value;
}


// handle exception/interrupt, cs->exception_index
void tms320c28x_cpu_do_interrupt(CPUState *cs)
{
#ifndef CONFIG_USER_ONLY
    Tms320c28xCPU *cpu = TMS320C28X_CPU(cs);
    CPUTms320c28xState *env = &cpu->env;
    int exception = cs->exception_index;

    if (exception >= EXCP_INTERRUPT_RESET && exception <= EXCP_INTERRUPT_USER12) {
        qemu_log_mask(CPU_LOG_INT, "INTERRUPT: %s\n", INTERRUPT_NAME[exception]);

        // temp = pc + 1
        int temp = env->pc + 1; //todo: depends on insn length
        // sp = sp + 1, for odd address
        if ((env->sp & 1) == 1) {
            env->sp += 1;
        }
        // [SP] = T:ST0
        st32_swap(env, env->sp, (env->xt & 0xffff0000) | (env->st0 & 0xffff));
        env->sp += 2;
        // [SP] = AH:AL
        st32_swap(env, env->sp, env->acc);
        env->sp += 2;
        // [SP] = PH:PL
        st32_swap(env, env->sp, env->p);
        env->sp += 2;
        // [SP] = AR1:AR0
        uint32_t ar1 = env->xar[1] & 0xffff;
        uint32_t ar0 = env->xar[0] & 0xffff;
        st32_swap(env, env->sp, (ar1<<16) | ar0);
        env->sp += 2;
        // [SP] = DP:ST1
        st32_swap(env, env->sp, (env->dp<<16) | (env->st1 & 0xffff));
        env->sp += 2;
        // [SP] = DBGSTAT:IER
        st32_swap(env, env->sp, (env->dbgier<<16) | (env->ier & 0xffff));
        env->sp += 2;
        // [SP] = temp
        st32_swap(env, env->sp, temp);
        env->sp += 2;
        // sp = sp + 1, for even address
        if ((env->sp & 1) == 0) {
            env->sp += 1;
        }
        // INTM = 0
        cpu_set_intm(env, 0);
        // DBGM = 1
        cpu_set_dbgm(env, 1);
        // EALLOW = 0
        cpu_set_eallow(env, 0);
        // LOOP = 0
        cpu_set_loop(env, 0);
        // IDLESTAT = 0
        cpu_set_idlestat(env, 0);
        // PC = fetched vector
        uint32_t vector_base = 0x3fffc0;
        uint32_t addr = vector_base + exception*2;
        env->pc = ld32_swap(env, addr);

        if (exception <= EXCP_INTERRUPT_RTOSINT) {
            //clear corresponding IER bit
            uint32_t mask = 1 << (exception - 1);
            env->ier = env->ier & (~mask);
        }

    } else {
        cpu_abort(cs, "Unhandled exception 0x%x\n", exception);
    }
#endif

    cs->exception_index = -1;
}


// handle int from hw
bool tms320c28x_cpu_exec_interrupt(CPUState *cs, int interrupt_request)
{
    if (interrupt_request == CPU_INTERRUPT_INT) {
        tms320c28x_cpu_do_interrupt(cs);
        return true;
    }
    return false;
}
