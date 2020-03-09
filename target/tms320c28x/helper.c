#include "qemu/osdep.h"
#include "cpu.h"
#include "exec/exec-all.h"
#include "exec/helper-proto.h"
#include "sysemu/sysemu.h"
#ifndef CONFIG_USER_ONLY
#include "hw/boards.h"
#include "exception.h"
#include "hw/irq.h"
#endif
#include "ldst.h"

void HELPER(exception)(CPUTms320c28xState *env, uint32_t int_n, uint32_t pc) {

    // cs->exception_index = 13;
    // cpu_loop_exit_restore(cs, 0);
    
    Tms320c28xCPU *cpu = env_archcpu(env);

    env->pc = pc;
    raise_exception(cpu, int_n);
}


void HELPER(aborti)(CPUTms320c28xState *env) {
    //todo ??
    cpu_set_dbgm(env, 1);

    // qemu_set_irq(env->irq[12], 1);

        // CPUState *cs = env_cpu(env);


    // cs->exception_index = 13;
    // cpu_loop_exit_restore(cs, 0);
}


uint32_t HELPER(test_cond)(CPUTms320c28xState *env, uint32_t cond)
{
    cond = cond & 0xf;
    uint32_t z = (env->st0 >> 4) & 1;
    uint32_t n = (env->st0 >> 5) & 1;
    uint32_t c = (env->st0 >> 3) & 1;
    uint32_t v = (env->st0 >> 6) & 1;
    uint32_t tc = (env->st0 >> 2) & 1;
    switch(cond) {
        case 0: //NEQ
            return z == 0;
        case 1: //EQ
            return z == 1; 
        case 2: //GT
            return (z == 0) && (n == 0);
        case 3: //GEQ
            return n == 0;
        case 4: //LT
            return n == 1;
        case 5: //LEQ
            return (z == 1) || (n == 1);
        case 6: //HI
            return (c == 1) && (z == 0);
        case 7: //HIS,C
            return c == 1;
        case 8: //LO,NC
            return c == 0;
        case 9: //LOS
            return (c == 0) || (z == 1);
        case 10: //NOV
            cpu_set_v(env, 0);//clear v, v flag is tested
            return v == 0;
        case 11: //OV
            cpu_set_v(env, 0);//clear v, v flag is tested
            return v == 1;
        case 12: //NTC
            return tc == 0;
        case 13: //TC
            return tc == 1;
        case 14: //NBIO
            return false;
        case 15:
            return true;
        default://can not reach
            return false;
    }
}

void HELPER(test_N_Z_16)(CPUTms320c28xState *env, uint32_t value) {
    value = value & 0xffff;
    if ((value >> 15) == 1) {
        cpu_set_n(env, 1);
    }
    else {
        cpu_set_n(env, 0);
    }
    if (value == 0) {
        cpu_set_z(env, 1);
    }
    else {
        cpu_set_z(env, 0);
    }
}

void HELPER(test_N_Z_32)(CPUTms320c28xState *env, uint32_t value) {
    if ((value >> 31) == 1) {
        cpu_set_n(env, 1);
    }
    else {
        cpu_set_n(env, 0);
    }
    if (value == 0) {
        cpu_set_z(env, 1);
    }
    else {
        cpu_set_z(env, 0);
    }
}

void HELPER(test_C_V_16)(CPUTms320c28xState *env, uint32_t a, uint32_t b, uint32_t result) {
    uint32_t bit1 = (a >> 15) & 1;
    uint32_t bit2 = (b >> 15) & 1;
    uint32_t bit3 = (result >> 15) & 1;
    uint32_t tmp = a + b;
    if ((tmp >> 16) & 1) {
        cpu_set_c(env, 1);
    }
    else {
        cpu_set_c(env, 0);
    }
    if (bit1 == 1 && bit2 == 1 && bit3 == 0) {//neg overflow
        cpu_set_v(env, 1);
    }
    else if (bit1 == 0 && bit2 == 0 && bit3 == 1) {//pos overflow
        cpu_set_v(env, 1);
    }
    else {
        cpu_set_v(env, 0);
    }
}

void HELPER(test_C_32)(CPUTms320c28xState *env, uint32_t a, uint32_t b, uint32_t result) 
{
    uint64_t tmp = (uint64_t)a + (uint64_t)b;
    if ((tmp >> 32) & 1) {
        cpu_set_c(env, 1);
    }
    else {
        cpu_set_c(env, 0);
    }
}

void HELPER(test_sub_C_32)(CPUTms320c28xState *env, uint32_t a, uint32_t b, uint32_t result) 
{
    b = ~b + 1;
    helper_test_C_32(env, a, b, result);
    if (b == 0) {
        cpu_set_c(env, 1);
    }
}

void HELPER(test_V_32)(CPUTms320c28xState *env, uint32_t a, uint32_t b, uint32_t result) 
{
    uint32_t bit1 = a >> 31;
    uint32_t bit2 = b >> 31;
    uint32_t bit3 = result >> 31;

    if (bit1 == 1 && bit2 == 1 && bit3 == 0) {//neg overflow
        cpu_set_v(env, 1);
    }
    else if (bit1 == 0 && bit2 == 0 && bit3 == 1) {//pos overflow
        cpu_set_v(env, 1);
    }
    else {
        cpu_set_v(env, 0);
    }
}

void HELPER(test_sub_V_32)(CPUTms320c28xState *env, uint32_t a, uint32_t b, uint32_t result) 
{
    b = ~b + 1;
    uint32_t bit1 = a >> 31;
    uint32_t bit2 = b >> 31;
    uint32_t bit3 = result >> 31;

    if (bit1 == 1 && bit2 == 1 && bit3 == 0) {//neg overflow
        cpu_set_v(env, 1);
    }
    else if (bit1 == 0 && bit2 == 0 && bit3 == 1) {//pos overflow
        cpu_set_v(env, 1);
    }
    else { //if not overlow, v bit is not affected
        // cpu_set_v(env, 0);
    }
}

void HELPER(test_C_V_32)(CPUTms320c28xState *env, uint32_t a, uint32_t b, uint32_t result)
{
    helper_test_C_32(env, a, b, result);
    helper_test_V_32(env, a, b, result);
}

void HELPER(test_sub_C_V_32)(CPUTms320c28xState *env, uint32_t a, uint32_t b, uint32_t result)
{
    helper_test_sub_C_32(env, a, b, result);
    helper_test_sub_V_32(env, a, b, result);
}

// affect acc value
void HELPER(test_OVC_OVM_32)(CPUTms320c28xState *env, uint32_t a, uint32_t b, uint32_t result) {
    uint32_t bit1 = a >> 31;
    uint32_t bit2 = b >> 31;
    uint32_t bit3 = result >> 31;

    if (cpu_get_ovm(env)) { //ovm = 1
        if (bit1 == 0 && bit2 == 0 && bit3 == 1)//pos overflow
        {
            env->acc = 0x7fffffff;
        }
        if (bit1 == 1 && bit2 == 1 && bit3 == 0)//neg overflow
        {
            env->acc = 0x80000000;
        }
    }
    else {
        int32_t ovc = cpu_get_ovc(env);
        if (bit1 == 0 && bit2 == 0 && bit3 == 1)//pos overflow
        {
            ovc += 1;
            cpu_set_ovc(env, ovc);
        }
        if (bit1 == 1 && bit2 == 1 && bit3 == 0)//neg overflow
        {
            ovc -= 1;
            cpu_set_ovc(env, ovc);
        }
    }    
}

void HELPER(test_sub_OVC_OVM_32)(CPUTms320c28xState *env, uint32_t a, uint32_t b, uint32_t result)
{
    b = ~b + 1;
    helper_test_OVC_OVM_32(env, a, b, result);
}

void HELPER(test2_C_V_OVC_OVM_32)(CPUTms320c28xState *env, uint32_t a, uint32_t b, uint32_t c, uint32_t result) 
{
    uint32_t bit1 = a >> 31;
    uint32_t bit2 = (b + c) >> 31;
    uint32_t bit3 = result >> 31;
    uint64_t tmp = (uint64_t)a + (uint64_t)b + (uint64_t)c;
    if ((tmp >> 32) & 1) {
        cpu_set_c(env, 1);
    }
    else {
        cpu_set_c(env, 0);
    }

    if (bit1 == 1 && bit2 == 1 && bit3 == 0) {//neg overflow
        cpu_set_v(env, 1);
        if (cpu_get_ovm(env)) { //ovm = 1
            env->acc = 0x80000000;
        }
        else {
            int32_t ovc = cpu_get_ovc(env);
            ovc -= 1;
            cpu_set_ovc(env, ovc);
        }
    }
    else if (bit1 == 0 && bit2 == 0 && bit3 == 1) {//pos overflow
        cpu_set_v(env, 1);
        if (cpu_get_ovm(env)) { //ovm = 1
            env->acc = 0x7fffffff;
        }
        else {
            int32_t ovc = cpu_get_ovc(env);
            ovc += 1;
            cpu_set_ovc(env, ovc);
        }
    }
    else {
        cpu_set_v(env, 0);
    }
}

void HELPER(test_OVCU_32)(CPUTms320c28xState *env, uint32_t a, uint32_t b, uint32_t result) {
    uint64_t tmp = (uint64_t)a + (uint64_t)b;
    if ((tmp >> 32) & 1) {
        int32_t ovc = cpu_get_ovc(env);
        ovc += 1;
        cpu_set_ovc(env, ovc);
    }
}

uint32_t HELPER(extend_low_sxm)(CPUTms320c28xState *env, uint32_t value)
{
    uint32_t sxm = cpu_get_sxm(env);
    value = (value & 0xffff); //get low 16bit
    if (sxm == 1) // signed extend
    {
        return (value >> 15 == 0) ? value : (value | 0xffff0000);
    }
    else // zero extend
    {
        return value;
    }
}

void HELPER(print)(CPUTms320c28xState *env, uint32_t value) {
    qemu_log_mask(CPU_LOG_INT ,"value is: 0x%x \n",value);
}

void HELPER(print_env)(CPUTms320c28xState *env) {
    int i = 0;

    qemu_log_mask(CPU_LOG_INT, "PC =%08x RPC=%08x\n", env->pc, env->rpc);
    qemu_log_mask(CPU_LOG_INT, "ACC=%08x AH =%04x AL=%04x\n", env->acc, env->acc >> 16, env->acc & 0xffff);
    for (i = 0; i < 8; ++i) {
        qemu_log_mask(CPU_LOG_INT, "XAR%01d=%08x%c", i, env->xar[i], (i % 4) == 3 ? '\n' : ' ');
    }
    for (i = 0; i < 8; ++i) {
        qemu_log_mask(CPU_LOG_INT, "AR%01d=%04x%c", i, env->xar[i] & 0xffff, (i % 4) == 3 ? '\n' : ' ');
    }
    qemu_log_mask(CPU_LOG_INT, "DP =%04x IFR=%04x IER=%04x DBGIER=%04x\n", env->dp, env->ifr, env->ier, env->dbgier);
    qemu_log_mask(CPU_LOG_INT, "P  =%08x PH =%04x PH =%04x\n", env->p, env->p >> 16, env->p & 0xffff);
    qemu_log_mask(CPU_LOG_INT, "XT =%08x T  =%04x TL =%04x\n", env->xt, env->xt >> 16, env->xt & 0xffff);
    qemu_log_mask(CPU_LOG_INT, "SP =%04x ST0=%04x ST1=%04x\n", env->sp, env->st0, env->st1);
    qemu_log_mask(CPU_LOG_INT, "OVC=%x PM=%x V=%x N=%x Z=%x\n", cpu_get_ovc(env), cpu_get_pm(env), cpu_get_v(env), cpu_get_n(env), cpu_get_z(env));
    qemu_log_mask(CPU_LOG_INT, "C=%x TC=%x OVM=%x SXM=%x\n", cpu_get_c(env), cpu_get_tc(env), cpu_get_ovm(env), cpu_get_sxm(env));
    qemu_log_mask(CPU_LOG_INT, "ARP=%x XF=%x MOM1MAP=%x OBJMODE=%x\n", cpu_get_arp(env), cpu_get_xf(env), cpu_get_mom1map(env), cpu_get_objmode(env));
    qemu_log_mask(CPU_LOG_INT, "AMODE=%x IDLESTAT=%x EALLOW=%x LOOP=%x\n", cpu_get_amode(env), cpu_get_idlestat(env), cpu_get_eallow(env), cpu_get_loop(env));
    qemu_log_mask(CPU_LOG_INT, "SPA=%x VMAP=%x PAGE0=%x DBGM=%x INTM=%x\n", cpu_get_spa(env), cpu_get_vmap(env), cpu_get_page0(env), cpu_get_dbgm(env), cpu_get_intm(env));
}

void HELPER(abs_acc)(CPUTms320c28xState *env)
{
    if (env->acc == 0x80000000) {
        cpu_set_v(env, 1);
        if (cpu_get_ovm(env) == 1) {
            env->acc = 0x7fffffff;
        }
        else {
            env->acc = 0x80000000;
        }
    }
    else {
        int32_t tmp = env->acc;
        if (tmp < 0)
        {
            tmp = -tmp;
            env->acc = tmp;
        }
    }
}

void HELPER(abstc_acc)(CPUTms320c28xState *env)
{
    if (env->acc == 0x80000000) {
        if (cpu_get_ovm(env) == 1) {
            env->acc = 0x7fffffff;
        }
        else {
            env->acc = 0x80000000;
        }
        cpu_set_v(env, 1);
        uint32_t tc = cpu_get_tc(env);
        tc = tc ^ 1;//TC = TC XOR 1
        cpu_set_tc(env, tc);
    }
    else {
        int32_t tmp = env->acc;
        if (tmp < 0)
        {
            tmp = -tmp;
            env->acc = tmp;
        }
        uint32_t tc = cpu_get_tc(env);
        tc = tc ^ 1;//TC = TC XOR 1
        cpu_set_tc(env, tc);
    }
}

uint32_t HELPER(shift_by_pm)(CPUTms320c28xState *env, uint32_t value)
{
    int32_t pm = cpu_get_pm(env);
    pm = 1 - pm;
    if (pm > 0) {
        return value << pm;
    }
    else {
        return ((int)value) >> -pm; //signed right shift
    }
}

void HELPER(mov_16bit_loc16)(CPUTms320c28xState *env, uint32_t mode, uint32_t addr, uint32_t is_rpt)
{
    uint32_t max;
    if (is_rpt == 1)
        max = env->rptc + 1;
    else
        max = 1;
    for(int i = 0; i < max; i++) {
        uint32_t loc16_value = helper_ld_loc16(env, mode);
        
        st16_swap(env, addr, loc16_value);
        addr = addr + 1;
    }
    env->rptc = 0;//reset rptc
}

void HELPER(mov_loc16_16bit)(CPUTms320c28xState *env, uint32_t mode, uint32_t addr, uint32_t is_rpt)
{
    uint32_t max;
    if (is_rpt == 1)
        max = env->rptc + 1;
    else
        max = 1;
    for(int i = 0; i < max; i++) {
        uint32_t mem_value = ld16_swap(env, addr);
        helper_st_loc16(env, mode, mem_value);
        
        addr = addr + 1;
    }
    env->rptc = 0;//reset rptc
}

void HELPER(cmp16_N_Z_C)(CPUTms320c28xState *env, uint32_t a, uint32_t b)
{
    uint32_t result2 = a + ((~b + 1) & 0xffff);
    //test C
    if ((result2 >> 16) & 1) {
        cpu_set_c(env, 1);
    }
    else {
        cpu_set_c(env, 0);
    }

    //sign extend
    a = sign_extend_16(a);
    b = sign_extend_16(b);
    //sub
    int32_t result = a - b;
    //test N
    if (result < 0) {
        cpu_set_n(env, 1);
    }
    else {
        cpu_set_n(env, 0);
    }
    //test Z
    if (result == 0) {
        cpu_set_z(env, 1);
    }
    else {
        cpu_set_z(env, 0);
    }

}

void HELPER(cmp32_N_Z_C)(CPUTms320c28xState *env, uint32_t a, uint32_t b)
{
    uint64_t a64 = a;
    uint64_t b64 = b;
    uint64_t result2 = a64 + ((~b64 + 1) & 0xffffffff);
    //test C
    if ((result2 >> 32) & 1) {
        cpu_set_c(env, 1);
    }
    else {
        cpu_set_c(env, 0);
    }

    //sign extend
    a64 = sign_extend_32(a);
    b64 = sign_extend_32(b);
    //sub
    int64_t result = a64 - b64;
    //test N
    if (result < 0) {
        cpu_set_n(env, 1);
    }
    else {
        cpu_set_n(env, 0);
    }
    //test Z
    if (result == 0) {
        cpu_set_z(env, 1);
    }
    else {
        cpu_set_z(env, 0);
    }

}