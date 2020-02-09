/*
 * TMS320C28x gdb server stub
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
#include "exec/gdbstub.h"

int tms320c28x_cpu_gdb_read_register(CPUState *cs, uint8_t *mem_buf, int n)
{
    Tms320c28xCPU *cpu = TMS320C28X_CPU(cs);
    CPUTms320c28xState *env = &cpu->env;

    if (n < 8) {
        return gdb_get_reg32(mem_buf, env->xar[n]);
    } else {
        switch (n) {
        case 8:    /* ACC */
            return gdb_get_reg32(mem_buf, env->acc);
        case 9:    /* DP */
            return gdb_get_reg32(mem_buf, env->dp);
        case 10:    /* IFR */
            return gdb_get_reg32(mem_buf, env->ifr);
        case 11:    /* IER */
            return gdb_get_reg32(mem_buf, env->ier);
        case 12:    /* DBGIER */
            return gdb_get_reg32(mem_buf, env->dbgier);
        case 13:    /* P */
            return gdb_get_reg32(mem_buf, env->p);
        case 14:    /* PC */
            return gdb_get_reg32(mem_buf, env->pc);
        case 15:    /* RPC */
            return gdb_get_reg32(mem_buf, env->rpc);
        case 16:    /* SP */
            return gdb_get_reg32(mem_buf, env->sp);
        case 17:    /* ST0 */
            return gdb_get_reg32(mem_buf, env->st0);
        case 18:    /* ST1 */
            return gdb_get_reg32(mem_buf, env->st1);
        case 19:    /* XT */
            return gdb_get_reg32(mem_buf, env->xt);
        default:
            break;
        }
    }
    return 0;
}

// return write register size
int tms320c28x_cpu_gdb_write_register(CPUState *cs, uint8_t *mem_buf, int n)
{
    Tms320c28xCPU *cpu = TMS320C28X_CPU(cs);
    CPUClass *cc = CPU_GET_CLASS(cs);
    CPUTms320c28xState *env = &cpu->env;
    uint32_t tmp;
    int ret_value = 4;

    if (n > cc->gdb_num_core_regs) {
        return 0;
    }

    tmp = ldl_p(mem_buf);
    if (n < 8) {
        env->xar[n] = tmp;
    } else {
        switch (n) {
        case 8:    /* ACC */
            env->acc = tmp;
            break;
        case 9:    /* DP */
            env->dp = tmp;
            break;
        case 10:    /* IFR */
            env->ifr = tmp;
            break;
        case 11:    /* IER */
            env->ier = tmp;
            break;
        case 12:    /* DBGIER */
            env->dbgier = tmp;
            break;
        case 13:    /* P */
            env->p = tmp;
            break;
        case 14:    /* PC */
            env->pc = tmp;
            break;
        case 15:    /* RPC */
            env->rpc = tmp;
            break;
        case 16:    /* SP */
            env->sp = tmp;
            break;
        case 17:    /* ST0 */
            env->st0 = tmp;
            break;
        case 18:    /* ST1 */
            env->st1 = tmp;
            break;
        case 19:    /* XT */
            env->xt = tmp;
            break;

        default:
            break;
        }
    }
    return ret_value;
}
