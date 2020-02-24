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

void HELPER(exception)(CPUTms320c28xState *env, uint32_t excp)
{
    Tms320c28xCPU *cpu = env_archcpu(env);

    raise_exception(cpu, excp);
}

static inline uint32_t bit_inverse_low_half(uint32_t n) {
    n = ((n>>1) & 0x55555555) | ((n<<1) & 0xaaaaaaaa);
    n = ((n>>2) & 0x33333333) | ((n<<2) & 0xcccccccc);
    n = ((n>>4) & 0x0f0f0f0f) | ((n<<4) & 0xf0f0f0f0);
    n = ((n>>8) & 0x00ff00ff) | ((n<<8) & 0xff00ff00);
    n = n & 0x0000ffff;
    return n;
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

uint32_t HELPER(addressing_mode)(CPUTms320c28xState *env, uint32_t loc, uint32_t loc_type) {
    uint32_t amode = cpu_get_amode(env);
    uint32_t dest = -1;
    uint32_t tmp, tmp2, tmp3;
    if (amode == 0) {
        switch((loc & 0b11000000)>>6) {
            case 0b00: //b00 III III @6bit
                tmp = env->dp;
                dest = (tmp << 6) +  (loc & 0x3f);
                break;
            case 0b01: //b01 III III *-SP[6bit]
                tmp = env->sp;
                dest = tmp -  (loc & 0x3f);
                break;
            case 0b10: //b10 ... ...
                switch((loc & 0b00111000)>>3) {
                    case 0b000: //b10 000 AAA *XARn++
                        tmp = loc & 0b00000111;
                        cpu_set_arp(env, tmp);
                        dest = env->xar[tmp];
                        env->xar[tmp] = env->xar[tmp] + loc_type;
                        break;
                    case 0b001: //b10 001 AAA *--XARn
                        tmp = loc & 0b00000111;
                        cpu_set_arp(env, tmp);
                        env->xar[tmp] = env->xar[tmp] - loc_type;
                        dest = env->xar[tmp];
                        break;
                    case 0b010: //b10 010 AAA *+XARn[AR0]
                        tmp = loc & 0b00000111;
                        cpu_set_arp(env, tmp);
                        dest = env->xar[tmp] + (env->xar[0] & 0xffff);
                        break;
                    case 0b011: //b10 011 AAA *+XARn[AR1]
                        tmp = loc & 0b00000111;
                        cpu_set_arp(env, tmp);
                        dest = env->xar[tmp] + (env->xar[1] & 0xffff);
                        break;
                    // case 0b100: //@XARn @ARn
                    //     tmp = loc & 0b00000111;
                    //     if (loc_type == LOC16) {
                    //         dest = env->xar[tmp] & 0xffff;
                    //     }
                    //     else { //LOC32
                    //         dest = env->xar[tmp];
                    //     }
                    //     break;
                    case 0b101: //b10 101 ...
                        switch(loc & 0b00000111) {
                            // case 0b000: //@AH
                            //     dest = (env->acc & 0xffff0000) >> 16;
                            //     break;
                            // case 0b001: //@ACC @AL
                            //     if (loc_type == LOC16) {
                            //         dest = env->acc & 0xffff;
                            //     }
                            //     else {
                            //         dest = env->acc;
                            //     }
                            //     break;
                            // case 0b010:
                            //     dest = (env->p & 0xffff0000) >> 16;
                            //     break;
                            // case 0b011:
                            //     if (loc_type == LOC16) {
                            //         dest = env->p & 0xffff;
                            //     }
                            //     else {
                            //         dest = env->p;
                            //     }
                            //     break;
                            // case 0b100:
                            //     if (loc_type == LOC16) {
                            //         dest = (env->xt & 0xffff0000) >> 16;
                            //     }
                            //     else {
                            //         dest = env->xt;
                            //     }
                            //     break;
                            // case 0b101: //@SP
                            //     dest = env->sp;
                            //     break;
                            case 0b110: //b10 101 110 *BR0++
                                tmp = cpu_get_arp(env);
                                dest = env->xar[tmp];
                                //rcadd: XAR(ARP)(15:0)= AR(ARP)rcadd AR0
                                //XAR(ARP)(31:16) = unchanged
                                tmp2 = bit_inverse_low_half(env->xar[tmp]);
                                tmp3 = bit_inverse_low_half(env->xar[0]);
                                tmp3 = tmp2 + tmp3;
                                tmp3 = bit_inverse_low_half(tmp3);
                                env->xar[tmp] = (env->xar[tmp] & 0xffff0000) | (tmp3 & 0x0000ffff);
                                break;
                            case 0b111: //b10 101 111 *BR0--
                                tmp = cpu_get_arp(env);
                                dest = env->xar[tmp];
                                //XAR(ARP)(15:0)= AR(ARP)rbsub AR0 {see note [1]}
                                //XAR(ARP)(31:16) = unchanged
                                tmp2 = bit_inverse_low_half(env->xar[tmp]);
                                tmp3 = bit_inverse_low_half(env->xar[0]);
                                tmp3 = tmp2 - tmp3;
                                tmp3 = bit_inverse_low_half(tmp3);
                                env->xar[tmp] = (env->xar[tmp] & 0xffff0000) | (tmp3 & 0x0000ffff);
                                break;
                        }
                        break;
                    case 0b110: //b10 110 RRR *,ARPn
                        tmp = cpu_get_arp(env);
                        dest = env->xar[tmp];
                        tmp2 = loc & 0b00000111;
                        cpu_set_arp(env, tmp2);
                        break;
                    case 0b111: //b10 111 ...
                        switch(loc & 0b00000111) {
                            case 0b000: //b10 111 000 *
                                tmp = cpu_get_arp(env);
                                dest = env->xar[tmp];
                                break;
                            case 0b001: //b10 111 001 *++
                                tmp = cpu_get_arp(env);
                                dest = env->xar[tmp];
                                env->xar[tmp] = env->xar[tmp] + loc_type;
                                break;
                            case 0b010: //b10 111 010 *--
                                tmp = cpu_get_arp(env);
                                dest = env->xar[tmp];
                                env->xar[tmp] = env->xar[tmp] - loc_type;
                                break;
                            case 0b011: //b10 111 011 *0++
                                tmp = cpu_get_arp(env);
                                dest = env->xar[tmp];
                                env->xar[tmp] = env->xar[tmp] + (env->xar[0] & 0xffff);
                                break;
                            case 0b100: //b10 111 100 *0--
                                tmp = cpu_get_arp(env);
                                dest = env->xar[tmp];
                                env->xar[tmp] = env->xar[tmp] - (env->xar[0] & 0xffff);
                                break;
                            case 0b101: //b10 111 101 *SP++
                                dest = env->sp;
                                env->sp = env->sp + loc_type;
                                break;
                            case 0b110: //b10 111 110 *--SP
                                env->sp = env->sp - loc_type;
                                dest = env->sp;
                                break;
                            case 0b111: //b10 111 111 *AR6%++
                                dest = env->xar[6];
                                if ((env->xar[6] & 0xff) == (env->xar[1] & 0xff)) {// XAR6(7:0)==XAR1(7:0)
                                    env->xar[6] = env->xar[6] & 0xffffff00; //XAR6(7:0) = 0x00
                                }
                                else {
                                    tmp = env->xar[6] & 0xffff;
                                    tmp = tmp + loc_type;
                                    env->xar[6] = (env->xar[6] & 0xffff0000) | (tmp & 0xffff);
                                }
                                cpu_set_arp(env, 6);
                                break;
                        }
                        break;
                }
                break;
            case 0b11: //b11 III AAA
                tmp = loc & 0b00000111; //n AAA
                cpu_set_arp(env, tmp);
                tmp2 = (loc >> 3) & 0b00000111;//3bit III
                dest = env->xar[tmp] + tmp2;
                break;
        }
    }
    else {
        //todo amode==1
    }
    // return dest<<1;
    return dest;
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

void HELPER(test_C_V_32)(CPUTms320c28xState *env, uint32_t a, uint32_t b, uint32_t result) 
{
    uint32_t bit1 = a >> 31;
    uint32_t bit2 = b >> 31;
    uint32_t bit3 = result >> 31;
    uint64_t tmp = (uint64_t)a + (uint64_t)b;
    if ((tmp >> 32) & 1) {
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

uint32_t HELPER(ld_high_sxm)(CPUTms320c28xState *env, uint32_t value)
{
    uint32_t sxm = cpu_get_sxm(env);
    value = (value & 0xffff0000) >> 16; //get high 16bit
    if (sxm == 1) // signed extend
    {
        return (value >> 15 == 0) ? value : (value | 0xffff0000);
    }
    else // zero extend
    {
        return value;
    }
}

uint32_t HELPER(ld_low_sxm)(CPUTms320c28xState *env, uint32_t value)
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

void HELPER(aborti)(CPUTms320c28xState *env) {
    //todo ??
    cpu_set_dbgm(env, 1);

    // qemu_set_irq(env->irq[12], 1);

        // CPUState *cs = env_cpu(env);


    // cs->exception_index = 13;
    // cpu_loop_exit_restore(cs, 0);
}

void HELPER(intr)(CPUTms320c28xState *env, uint32_t int_n, uint32_t pc) {

    // cs->exception_index = 13;
    // cpu_loop_exit_restore(cs, 0);
    
    Tms320c28xCPU *cpu = env_archcpu(env);

    env->pc = pc;
    raise_exception(cpu, int_n);
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