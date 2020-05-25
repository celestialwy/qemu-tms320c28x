/*
 * QEMU monitor for m68k
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or
 * later.  See the COPYING file in the top-level directory.
 */

#include "qemu/osdep.h"
#include "cpu.h"
#include "monitor/hmp-target.h"
#include "monitor/monitor.h"

void hmp_info_tlb(Monitor *mon, const QDict *qdict)
{
    CPUArchState *env1 = mon_get_cpu_env();

    if (!env1) {
        monitor_printf(mon, "No CPU available\n");
        return;
    }

    // dump_mmu(env1);
}

static const MonitorDef monitor_defs[] = {
    { "xar0", offsetof(CPUTms320c28xState, xar[0]) },
    { "xar1", offsetof(CPUTms320c28xState, xar[1]) },
    { "xar2", offsetof(CPUTms320c28xState, xar[2]) },
    { "xar3", offsetof(CPUTms320c28xState, xar[3]) },
    { "xar4", offsetof(CPUTms320c28xState, xar[4]) },
    { "xar5", offsetof(CPUTms320c28xState, xar[5]) },
    { "xar6", offsetof(CPUTms320c28xState, xar[6]) },
    { "xar7", offsetof(CPUTms320c28xState, xar[7]) },
    { "pc", offsetof(CPUTms320c28xState, pc) },
    { "acc", offsetof(CPUTms320c28xState, acc) },
    { "dp", offsetof(CPUTms320c28xState, dp) },
    { "ifr", offsetof(CPUTms320c28xState, ifr) },
    { "ier", offsetof(CPUTms320c28xState, ier) },
    { "dbgier", offsetof(CPUTms320c28xState, dbgier) },
    { "p", offsetof(CPUTms320c28xState, p) },
    { "rpc", offsetof(CPUTms320c28xState, rpc) },
    { "sp", offsetof(CPUTms320c28xState, sp) },
    { "st0", offsetof(CPUTms320c28xState, st0) },
    { "st1", offsetof(CPUTms320c28xState, st1) },
    { "xt", offsetof(CPUTms320c28xState, xt) },
    { NULL },
};

const MonitorDef *target_monitor_defs(void)
{
    return monitor_defs;
}
