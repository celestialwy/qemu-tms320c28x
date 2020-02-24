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

#include "qemu/osdep.h"
#include "qapi/error.h"
#include "qemu/qemu-print.h"
#include "cpu.h"

//设置pc
static void tms320c28x_cpu_set_pc(CPUState *cs, vaddr value)
{
    Tms320c28xCPU *cpu = TMS320C28X_CPU(cs);

    cpu->env.pc = (value  & 0x3fffff) >> 1;
    //todo: is there other work?
}

//是否在工作 ？?
static bool tms320c28x_cpu_has_work(CPUState *cs)
{
    return cs->interrupt_request & (CPU_INTERRUPT_INT);
}

// 打印指令反汇编码
static void tms320c28x_disas_set_info(CPUState *cpu, disassemble_info *info)
{
    info->print_insn = print_insn_tms320c28x;
}

/* CPUClass::reset() */
static void tms320c28x_cpu_reset(CPUState *s)
{
    Tms320c28xCPU *cpu = TMS320C28X_CPU(s);
    Tms320c28xCPUClass *occ = TMS320C28X_CPU_GET_CLASS(cpu);

    occ->parent_reset(s);

    memset(&cpu->env, 0, offsetof(CPUTms320c28xState, end_reset_fields));

    cpu->env.pc = 0x3fffc0;
    cpu->env.sp = 0x0400;
    cpu->env.st1 = 0x0a0b;

    s->exception_index = -1;// reset exception/interrupt
}

// cpu实现？？
static void tms320c28x_cpu_realizefn(DeviceState *dev, Error **errp)
{
    CPUState *cs = CPU(dev);
    Tms320c28xCPUClass *occ = TMS320C28X_CPU_GET_CLASS(dev);
    Error *local_err = NULL;

    cpu_exec_realizefn(cs, &local_err);
    if (local_err != NULL) {
        error_propagate(errp, local_err);
        return;
    }

    qemu_init_vcpu(cs);
    cpu_reset(cs);

    occ->parent_realize(dev, errp);
}

// cpu init function
static void tms320c28x_cpu_initfn(Object *obj)
{
    Tms320c28xCPU *cpu = TMS320C28X_CPU(obj);

    cpu_set_cpustate_pointers(cpu);
}

/* CPU models */

static ObjectClass *tms320c28x_cpu_class_by_name(const char *cpu_model)
{
    ObjectClass *oc;
    char *typename;

    typename = g_strdup_printf(TMS320C28X_CPU_TYPE_NAME("%s"), cpu_model);
    oc = object_class_by_name(typename);
    g_free(typename);
    if (oc != NULL && (!object_class_dynamic_cast(oc, TYPE_TMS320C28X_CPU) ||
                       object_class_is_abstract(oc))) {
        return NULL;
    }
    return oc;
}

static void tms320c28x_any_initfn(Object *obj)
{
    Tms320c28xCPU *cpu = TMS320C28X_CPU(obj);

    // cpu->env.pc = 0x3fffc0;
    // cpu->env.sp = 0x0400;
    // cpu->env.st1 = 0x080b;
    cpu->env.st1 = 0x0a0b; //objmode=1
}

static void tms320c28x_cpu_class_init(ObjectClass *oc, void *data)
{
    Tms320c28xCPUClass *occ = TMS320C28X_CPU_CLASS(oc);
    CPUClass *cc = CPU_CLASS(occ);
    DeviceClass *dc = DEVICE_CLASS(oc);

    device_class_set_parent_realize(dc, tms320c28x_cpu_realizefn,
                                    &occ->parent_realize);
    occ->parent_reset = cc->reset;
    cc->reset = tms320c28x_cpu_reset;

    cc->class_by_name = tms320c28x_cpu_class_by_name;
    cc->has_work = tms320c28x_cpu_has_work;
    cc->do_interrupt = tms320c28x_cpu_do_interrupt;
    cc->cpu_exec_interrupt = tms320c28x_cpu_exec_interrupt;
    cc->dump_state = tms320c28x_cpu_dump_state;
    cc->set_pc = tms320c28x_cpu_set_pc;
    cc->gdb_read_register = tms320c28x_cpu_gdb_read_register;
    cc->gdb_write_register = tms320c28x_cpu_gdb_write_register;
    cc->tlb_fill = tms320c28x_cpu_tlb_fill;
#ifndef CONFIG_USER_ONLY
    cc->get_phys_page_debug = tms320c28x_cpu_get_phys_page_debug;
    dc->vmsd = &vmstate_tms320c28x_cpu;
#endif
    cc->gdb_num_core_regs = 20; //Number of core registers accessible to GDB.
    cc->tcg_initialize = tms320c28x_translate_init;
    cc->disas_set_info = tms320c28x_disas_set_info;
}

/* Sort alphabetically by type name, except for "any". */
static gint tms320c28x_cpu_list_compare(gconstpointer a, gconstpointer b)
{
    ObjectClass *class_a = (ObjectClass *)a;
    ObjectClass *class_b = (ObjectClass *)b;
    const char *name_a, *name_b;

    name_a = object_class_get_name(class_a);
    name_b = object_class_get_name(class_b);
    if (strcmp(name_a, "any-" TYPE_TMS320C28X_CPU) == 0) {
        return 1;
    } else if (strcmp(name_b, "any-" TYPE_TMS320C28X_CPU) == 0) {
        return -1;
    } else {
        return strcmp(name_a, name_b);
    }
}

static void tms320c28x_cpu_list_entry(gpointer data, gpointer user_data)
{
    ObjectClass *oc = data;
    const char *typename;
    char *name;

    typename = object_class_get_name(oc);
    name = g_strndup(typename,
                     strlen(typename) - strlen("-" TYPE_TMS320C28X_CPU));
    qemu_printf("  %s\n", name);
    g_free(name);
}

void cpu_tms320c28x_list(void)
{
    GSList *list;

    list = object_class_get_list(TYPE_TMS320C28X_CPU, false);
    list = g_slist_sort(list, tms320c28x_cpu_list_compare);
    qemu_printf("Available CPUs:\n");
    g_slist_foreach(list, tms320c28x_cpu_list_entry, NULL);
    g_slist_free(list);
}

#define DEFINE_TMS320C28X_CPU_TYPE(cpu_model, initfn) \
    {                                               \
        .parent = TYPE_TMS320C28X_CPU,                \
        .instance_init = initfn,                    \
        .name = TMS320C28X_CPU_TYPE_NAME(cpu_model),  \
    }

static const TypeInfo tms320c28x_cpus_type_infos[] = {
    { /* base class should be registered first */
        .name = TYPE_TMS320C28X_CPU,
        .parent = TYPE_CPU,
        .instance_size = sizeof(Tms320c28xCPU),
        .instance_init = tms320c28x_cpu_initfn,
        .abstract = true,
        .class_size = sizeof(Tms320c28xCPUClass),
        .class_init = tms320c28x_cpu_class_init,
    },
    DEFINE_TMS320C28X_CPU_TYPE("any", tms320c28x_any_initfn),
};

DEFINE_TYPES(tms320c28x_cpus_type_infos)
