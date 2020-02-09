/*
 * TMS320C28X Machine
 *
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
#include "migration/cpu.h"

// only 32bit reg allowed, use this define to fit 16bit reg
#define VMSTATE_UINT16TL(_f, _s)                                        \
    VMSTATE_UINT16_V(_f, _s, 0)

static const VMStateDescription vmstate_env = {
    .name = "env",
    .version_id = 6,
    .minimum_version_id = 6,
    .fields = (VMStateField[]) { 
        VMSTATE_UINTTL(acc, CPUTms320c28xState),
        VMSTATE_UINTTL_ARRAY(xar, CPUTms320c28xState, 8),
        VMSTATE_UINTTL(dp, CPUTms320c28xState),
        VMSTATE_UINTTL(ifr, CPUTms320c28xState),
        VMSTATE_UINTTL(ier, CPUTms320c28xState),
        VMSTATE_UINTTL(dbgier, CPUTms320c28xState),
        VMSTATE_UINTTL(p, CPUTms320c28xState),
        VMSTATE_UINTTL(pc, CPUTms320c28xState),
        VMSTATE_UINTTL(rpc, CPUTms320c28xState),
        VMSTATE_UINTTL(sp, CPUTms320c28xState),
        VMSTATE_UINTTL(st0, CPUTms320c28xState),
        VMSTATE_UINTTL(st1, CPUTms320c28xState),
        VMSTATE_UINTTL(xt, CPUTms320c28xState),
        VMSTATE_END_OF_LIST()
    }
};

static int cpu_post_load(void *opaque, int version_id)
{
    return 0;
}

const VMStateDescription vmstate_tms320c28x_cpu = {
    .name = "cpu",
    .version_id = 1,
    .minimum_version_id = 1,
    .post_load = cpu_post_load,
    .fields = (VMStateField[]) {
        VMSTATE_CPU(),
        VMSTATE_STRUCT(env, Tms320c28xCPU, 1, vmstate_env, CPUTms320c28xState),
        VMSTATE_END_OF_LIST()
    }
};
