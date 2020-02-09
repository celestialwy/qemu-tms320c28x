/*
 * QEMU TMS320C28X CPU
 *
 * Copyright (c) 2019 Liwb <bolia@yeah.net>
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

#ifndef TMS320C28X_CPU_H
#define TMS320C28X_CPU_H

#include "exec/cpu-defs.h"
#include "hw/core/cpu.h"

struct Tms320c28xCPU;

#define TYPE_TMS320C28X_CPU "tms320c28x-cpu"

//用来类型转换的宏
#define TMS320C28X_CPU_CLASS(klass) \
    OBJECT_CLASS_CHECK(Tms320c28xCPUClass, (klass), TYPE_TMS320C28X_CPU)
#define TMS320C28X_CPU(obj) \
    OBJECT_CHECK(Tms320c28xCPU, (obj), TYPE_TMS320C28X_CPU)
#define TMS320C28X_CPU_GET_CLASS(obj) \
    OBJECT_GET_CLASS(Tms320c28xCPUClass, (obj), TYPE_TMS320C28X_CPU)

/**
 * Tms320c28xCPUClass:
 * @parent_realize: The parent class' realize handler.
 * @parent_reset: The parent class' reset handler.
 *
 * A Tms320c28x CPU model.
 */
typedef struct Tms320c28xCPUClass {
    /*< private >*/
    CPUClass parent_class;
    /*< public >*/

    DeviceRealize parent_realize;
    void (*parent_reset)(CPUState *cpu);
} Tms320c28xCPUClass;

// used in tcg.h
#define TARGET_INSN_START_EXTRA_WORDS 1

// mmu mode
enum {
    MMU_NOMMU_IDX = 0,
    MMU_SUPERVISOR_IDX = 1,
    MMU_USER_IDX = 2,
};

/* Interrupt */
#define NR_IRQS  32 //max irq number

enum {
    LOC16 = 1,
    LOC32 = 2,
};

typedef struct CPUTms320c28xState {
    target_ulong acc; /* Accumulator todo:ah,al */
    target_ulong xar[8]; /* Auxiliary todo:ar[8] */
    target_ulong dp; /* Data-page pointer */
    target_ulong ifr; /* Interrupt flag register */
    target_ulong ier; /* Interrupt enable register */
    target_ulong dbgier; /* Debug interrupt enable register */
    target_ulong p; /* Product register todo:ph,pl*/
    
    target_ulong pc;          /* Program counter 22bit, reset to 0x3f ffc0*/
    target_ulong rpc;         /* Return program counte 22bit*/

    target_ulong sp; /* Stack pointer, reset to 0x0400 */
    target_ulong st0; /* Status register 0 */
    target_ulong st1; /* Status register 1, reset to 0x080b */

    target_ulong xt;         /* Multiplicand register, todo:t,tl*/

    /* Fields up to this point are cleared by a CPU reset */
    struct {} end_reset_fields;

} CPUTms320c28xState;

/**
 * Tms320c28xCPU:
 * @env: #CPUTms320c28xState
 *
 * A Tms320c28x CPU.
 */
typedef struct Tms320c28xCPU {
    /*< private >*/
    CPUState parent_obj;
    /*< public >*/

    CPUNegativeOffsetState neg;
    CPUTms320c28xState env;
} Tms320c28xCPU;


void cpu_tms320c28x_list(void);
void tms320c28x_cpu_do_interrupt(CPUState *cpu); //callback when handle interrupt, used in tms320c28x_cpu_class_init
bool tms320c28x_cpu_exec_interrupt(CPUState *cpu, int int_req); // callback in cpu_exec, used in tms320c28x_cpu_class_init
void tms320c28x_cpu_dump_state(CPUState *cpu, FILE *f, int flags); //used in tms320c28x_cpu_class_init
hwaddr tms320c28x_cpu_get_phys_page_debug(CPUState *cpu, vaddr addr);  //used in tms320c28x_cpu_class_init
int tms320c28x_cpu_gdb_read_register(CPUState *cpu, uint8_t *buf, int reg);//used in tms320c28x_cpu_class_init
int tms320c28x_cpu_gdb_write_register(CPUState *cpu, uint8_t *buf, int reg);//used in tms320c28x_cpu_class_init
void tms320c28x_translate_init(void); ////used in tms320c28x_cpu_class_init
bool tms320c28x_cpu_tlb_fill(CPUState *cs, vaddr address, int size,
                           MMUAccessType access_type, int mmu_idx,
                           bool probe, uintptr_t retaddr);  //used in tms320c28x_cpu_class_init
int cpu_tms320c28x_signal_handler(int host_signum, void *pinfo, void *puc); //todo ??
int print_insn_tms320c28x(bfd_vma addr, disassemble_info *info); //use in tms320c28x_disas_set_info



#define cpu_list cpu_tms320c28x_list
#define cpu_signal_handler cpu_tms320c28x_signal_handler

#ifndef CONFIG_USER_ONLY
extern const VMStateDescription vmstate_tms320c28x_cpu;

/* hw/tms320c28x_pic.c */
void cpu_tms320c28x_pic_init(Tms320c28xCPU *cpu);

// /* hw/tms320c28x_timer.c */
// void cpu_tms320c28x_clock_init(Tms320c28xCPU *cpu); //todo
// uint32_t cpu_tms320c28x_count_get(Tms320c28xCPU *cpu); //todo
// void cpu_tms320c28x_count_set(Tms320c28xCPU *cpu, uint32_t val); //todo
// void cpu_tms320c28x_count_update(Tms320c28xCPU *cpu); //todo
// void cpu_tms320c28x_timer_update(Tms320c28xCPU *cpu); //todo
// void cpu_tms320c28x_count_start(Tms320c28xCPU *cpu); //todo
// void cpu_tms320c28x_count_stop(Tms320c28xCPU *cpu); //todo
#endif

#define TMS320C28X_CPU_TYPE_SUFFIX "-" TYPE_TMS320C28X_CPU
#define TMS320C28X_CPU_TYPE_NAME(model) model TMS320C28X_CPU_TYPE_SUFFIX
#define CPU_RESOLVING_TYPE TYPE_TMS320C28X_CPU

typedef CPUTms320c28xState CPUArchState;
typedef Tms320c28xCPU ArchCPU;

#include "exec/cpu-all.h"

static inline uint32_t cpu_get_amode(const CPUTms320c28xState *env)
{
    return (env->st1 >> 8) & 0x1;
}

static inline uint32_t cpu_get_arp(const CPUTms320c28xState *env)
{
    return (env->st1 >> 13);
}

static inline void cpu_set_arp(CPUTms320c28xState *env, uint32_t val)
{
    env->st1 = (env->st1 & 0x1fff) | (val << 13);
}

static inline uint32_t cpu_get_sxm(const CPUTms320c28xState *env)
{
    return (env->st0 & 0x1);
}

// static inline void cpu_set_sxm(CPUTms320c28xState *env, uint32_t val)
// {
//     env->st1 = (env->st1 & 0xfffe) | val;
// }

static inline uint32_t cpu_get_ovm(const CPUTms320c28xState *env)
{
    return (env->st0 >> 1) & 1;
}

static inline int32_t cpu_get_ovc(const CPUTms320c28xState *env)
{
    return (env->st0 >> 10) & 0b111111;
}

static inline void cpu_set_ovc(CPUTms320c28xState *env, uint32_t value)
{
    value = value & 0b111111;
    env->st0 = (env->st0 & 0x03ff) | (value << 10);
}

static inline void cpu_set_v(CPUTms320c28xState *env, uint32_t v)
{
    v = v & 1;
    env->st0 = (env->st0 & 0xffbf) | (v << 6);
}

static inline void cpu_set_c(CPUTms320c28xState *env, uint32_t c)
{
    c = c & 1;
    env->st0 = (env->st0 & 0xfff7) | (c << 3);
}

static inline void cpu_set_n(CPUTms320c28xState *env, uint32_t n)
{
    n = n & 1;
    env->st0 = (env->st0 & 0xffdf) | (n << 5);
}

static inline void cpu_set_z(CPUTms320c28xState *env, uint32_t z)
{
    z = z & 1;
    env->st0 = (env->st0 & 0xffef) | (z << 4);
}



// 获取cpu状态，在查找tb时会比较pc,cs_base,flags
static inline void cpu_get_tb_cpu_state(CPUTms320c28xState *env,
                                        target_ulong *pc,
                                        target_ulong *cs_base, uint32_t *flags)
{
    *pc = env->pc;
    // todo: cs_base,flags
    *cs_base = 0;
    *flags = 0;
}

//获取mmu状态，返回无mmu
static inline int cpu_mmu_index(CPUTms320c28xState *env, bool ifetch)
{
    int ret = MMU_NOMMU_IDX;  /* mmu is disabled */
    return ret;
}

#define CPU_INTERRUPT_TIMER   CPU_INTERRUPT_TGT_INT_0

#endif /* TMS320C28X_CPU_H */
