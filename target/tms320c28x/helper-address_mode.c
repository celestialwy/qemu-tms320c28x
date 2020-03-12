#include "qemu/osdep.h"
#include "cpu.h"
#include "exec/exec-all.h"
#include "exec/helper-proto.h"
#include "ldst.h"

typedef void (*op_store)(CPUTms320c28xState *env, target_ulong addr, uint32_t value);


uint32_t HELPER(ld_loc16)(CPUTms320c28xState *env, uint32_t loc)
{
    uint32_t amode = CPU_GET_STATUS(st1, AMODE);
    uint32_t loc_type = 1;
    uint32_t dest_addr = -1;
    uint32_t dest_value = 0;
    uint32_t tmp, tmp2, tmp3;

    if (amode == 0) {
        switch((loc & 0b11000000)>>6) {
            case 0b00: //b00 III III @6bit
                tmp = env->dp;
                dest_addr = (tmp << 6) +  (loc & 0x3f);
                dest_value = ld16_swap(env, dest_addr);
                break;
            case 0b01: //b01 III III *-SP[6bit]
                tmp = env->sp;
                dest_addr = tmp -  (loc & 0x3f);
                dest_value = ld16_swap(env, dest_addr);
                break;
            case 0b10: //b10 ... ...
                switch((loc & 0b00111000)>>3) {
                    case 0b000: //b10 000 AAA *XARn++
                        tmp = loc & 0b00000111;
                        CPU_SET_STATUS(st1, ARP, tmp);
                        dest_addr = env->xar[tmp];
                        dest_value = ld16_swap(env, dest_addr);
                        env->xar[tmp] = env->xar[tmp] + loc_type;
                        break;
                    case 0b001: //b10 001 AAA *--XARn
                        tmp = loc & 0b00000111;
                        CPU_SET_STATUS(st1, ARP, tmp);
                        env->xar[tmp] = env->xar[tmp] - loc_type;
                        dest_addr = env->xar[tmp];
                        dest_value = ld16_swap(env, dest_addr);
                        break;
                    case 0b010: //b10 010 AAA *+XARn[AR0]
                        tmp = loc & 0b00000111;
                        CPU_SET_STATUS(st1, ARP, tmp);
                        dest_addr = env->xar[tmp] + (env->xar[0] & 0xffff);
                        dest_value = ld16_swap(env, dest_addr);
                        break;
                    case 0b011: //b10 011 AAA *+XARn[AR1]
                        tmp = loc & 0b00000111;
                        CPU_SET_STATUS(st1, ARP, tmp);
                        dest_addr = env->xar[tmp] + (env->xar[1] & 0xffff);
                        dest_value = ld16_swap(env, dest_addr);
                        break;
                    case 0b100: //@ARn
                        tmp = loc & 0b00000111;
                        dest_value = env->xar[tmp] & 0xffff;
                        break;
                    case 0b101: //b10 101 ...
                        switch(loc & 0b00000111) {
                            case 0b000: //@AH
                                dest_value = (env->acc & 0xffff0000) >> 16;
                                break;
                            case 0b001: //@ACC @AL
                                dest_value = env->acc & 0xffff;
                                break;
                            case 0b010: //@PH
                                dest_value = (env->p & 0xffff0000) >> 16;
                                break;
                            case 0b011: //@PL
                                dest_value = env->p & 0xffff;
                                break;
                            case 0b100:
                                dest_value = (env->xt & 0xffff0000) >> 16;
                                break;
                            case 0b101: //@SP
                                dest_value = env->sp;
                                break;
                            case 0b110: //b10 101 110 *BR0++
                                tmp = CPU_GET_STATUS(st1, ARP);
                                dest_addr = env->xar[tmp];
                                dest_value = ld16_swap(env, dest_addr);
                                //rcadd: XAR(ARP)(15:0)= AR(ARP)rcadd AR0
                                //XAR(ARP)(31:16) = unchanged
                                tmp2 = bit_inverse_low_half(env->xar[tmp]);
                                tmp3 = bit_inverse_low_half(env->xar[0]);
                                tmp3 = tmp2 + tmp3;
                                tmp3 = bit_inverse_low_half(tmp3);
                                env->xar[tmp] = (env->xar[tmp] & 0xffff0000) | (tmp3 & 0x0000ffff);
                                break;
                            case 0b111: //b10 101 111 *BR0--
                                tmp = CPU_GET_STATUS(st1, ARP);
                                dest_addr = env->xar[tmp];
                                dest_value = ld16_swap(env, dest_addr);
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
                        tmp = CPU_GET_STATUS(st1, ARP);
                        dest_addr = env->xar[tmp];
                        dest_value = ld16_swap(env, dest_addr);
                        tmp2 = loc & 0b00000111;
                        CPU_SET_STATUS(st1, ARP, tmp2);
                        break;
                    case 0b111: //b10 111 ...
                        switch(loc & 0b00000111) {
                            case 0b000: //b10 111 000 *
                                tmp = CPU_GET_STATUS(st1, ARP);
                                dest_addr = env->xar[tmp];
                                dest_value = ld16_swap(env, dest_addr);
                                break;
                            case 0b001: //b10 111 001 *++
                                tmp = CPU_GET_STATUS(st1, ARP);
                                dest_addr = env->xar[tmp];
                                dest_value = ld16_swap(env, dest_addr);
                                env->xar[tmp] = env->xar[tmp] + loc_type;
                                break;
                            case 0b010: //b10 111 010 *--
                                tmp = CPU_GET_STATUS(st1, ARP);
                                dest_addr = env->xar[tmp];
                                dest_value = ld16_swap(env, dest_addr);
                                env->xar[tmp] = env->xar[tmp] - loc_type;
                                break;
                            case 0b011: //b10 111 011 *0++
                                tmp = CPU_GET_STATUS(st1, ARP);
                                dest_addr = env->xar[tmp];
                                dest_value = ld16_swap(env, dest_addr);
                                env->xar[tmp] = env->xar[tmp] + (env->xar[0] & 0xffff);
                                break;
                            case 0b100: //b10 111 100 *0--
                                tmp = CPU_GET_STATUS(st1, ARP);
                                dest_addr = env->xar[tmp];
                                dest_value = ld16_swap(env, dest_addr);
                                env->xar[tmp] = env->xar[tmp] - (env->xar[0] & 0xffff);
                                break;
                            case 0b101: //b10 111 101 *SP++
                                dest_addr = env->sp;
                                dest_value = ld16_swap(env, dest_addr);
                                env->sp = env->sp + loc_type;
                                break;
                            case 0b110: //b10 111 110 *--SP
                                env->sp = env->sp - loc_type;
                                dest_addr = env->sp;
                                dest_value = ld16_swap(env, dest_addr);
                                break;
                            case 0b111: //b10 111 111 *AR6%++
                                dest_addr = env->xar[6];
                                dest_value = ld16_swap(env, dest_addr);
                                if ((env->xar[6] & 0xff) == (env->xar[1] & 0xff)) {// XAR6(7:0)==XAR1(7:0)
                                    env->xar[6] = env->xar[6] & 0xffffff00; //XAR6(7:0) = 0x00
                                }
                                else {
                                    tmp = env->xar[6] & 0xffff;
                                    tmp = tmp + loc_type;
                                    env->xar[6] = (env->xar[6] & 0xffff0000) | (tmp & 0xffff);
                                }
                                CPU_SET_STATUS(st1, ARP, 6);
                                break;
                        }
                        break;
                }
                break;
            case 0b11: //b11 III AAA
                tmp = loc & 0b00000111; //n AAA
                CPU_SET_STATUS(st1, ARP, tmp);
                tmp2 = (loc >> 3) & 0b00000111;//3bit III
                dest_addr = env->xar[tmp] + tmp2;
                dest_value = ld16_swap(env, dest_addr);
                break;
        }
    }
    else {
        //todo amode==1
    }
    return dest_value;
}

uint32_t HELPER(ld_loc16_byte_addressing)(CPUTms320c28xState *env, uint32_t loc)
{
    bool is_byte_addressing = false;
    uint32_t offset = 0;
    uint32_t n = 0;
    uint32_t value = -1;
    if ((loc >> 6) == 0b11)
    {
        is_byte_addressing = true;
        n = loc & 0b111;
        offset = (loc >> 3) & 0b00000111;
        CPU_SET_STATUS(st1, ARP, n);
    }
    else if ((loc >> 3) == 0b10010)
    {
        is_byte_addressing = true;
        n = loc & 0b111;
        offset = env->xar[0] & 0xffff;
        CPU_SET_STATUS(st1, ARP, n);
    }
    else if ((loc >> 3) == 0b10011)
    {
        is_byte_addressing = true;
        n = loc & 0b111;
        offset = env->xar[1] & 0xffff;
        CPU_SET_STATUS(st1, ARP, n);
    }
    if (is_byte_addressing) {
        value = cpu_ldub_data(env, env->xar[n] * 2 + offset);
    }
    else {
        value = helper_ld_loc16(env, loc);
        value = value & 0xff;
    }
    return value;
}

uint32_t HELPER(ld_loc32)(CPUTms320c28xState *env, uint32_t loc)
{
    uint32_t amode = CPU_GET_STATUS(st1, AMODE);
    uint32_t loc_type = 2;
    uint32_t dest_addr = -1;
    uint32_t dest_value = 0;
    uint32_t tmp, tmp2, tmp3;

    if (amode == 0) {
        switch((loc & 0b11000000)>>6) {
            case 0b00: //b00 III III @6bit
                tmp = env->dp;
                dest_addr = (tmp << 6) +  (loc & 0x3f);
                dest_value = ld32_swap(env, dest_addr);
                break;
            case 0b01: //b01 III III *-SP[6bit]
                tmp = env->sp;
                dest_addr = tmp -  (loc & 0x3f);
                dest_value = ld32_swap(env, dest_addr);
                break;
            case 0b10: //b10 ... ...
                switch((loc & 0b00111000)>>3) {
                    case 0b000: //b10 000 AAA *XARn++
                        tmp = loc & 0b00000111;
                        CPU_SET_STATUS(st1, ARP, tmp);
                        dest_addr = env->xar[tmp];
                        dest_value = ld32_swap(env, dest_addr);
                        env->xar[tmp] = env->xar[tmp] + loc_type;
                        break;
                    case 0b001: //b10 001 AAA *--XARn
                        tmp = loc & 0b00000111;
                        CPU_SET_STATUS(st1, ARP, tmp);
                        env->xar[tmp] = env->xar[tmp] - loc_type;
                        dest_addr = env->xar[tmp];
                        dest_value = ld32_swap(env, dest_addr);
                        break;
                    case 0b010: //b10 010 AAA *+XARn[AR0]
                        tmp = loc & 0b00000111;
                        CPU_SET_STATUS(st1, ARP, tmp);
                        dest_addr = env->xar[tmp] + (env->xar[0] & 0xffff);
                        dest_value = ld32_swap(env, dest_addr);
                        break;
                    case 0b011: //b10 011 AAA *+XARn[AR1]
                        tmp = loc & 0b00000111;
                        CPU_SET_STATUS(st1, ARP, tmp);
                        dest_addr = env->xar[tmp] + (env->xar[1] & 0xffff);
                        dest_value = ld32_swap(env, dest_addr);
                        break;
                    case 0b100: //@XARn
                        tmp = loc & 0b00000111;
                        dest_value = env->xar[tmp];
                        break;
                    case 0b101: //b10 101 ...
                        switch(loc & 0b00000111) {
                            case 0b001: //@ACC
                                dest_value = env->acc;
                                break;
                            case 0b011:
                                dest_value = env->p;
                                break;
                            case 0b100:
                                dest_value = env->xt;
                                break;
                            case 0b110: //b10 101 110 *BR0++
                                tmp = CPU_GET_STATUS(st1, ARP);
                                dest_addr = env->xar[tmp];
                                dest_value = ld32_swap(env, dest_addr);
                                //rcadd: XAR(ARP)(15:0)= AR(ARP)rcadd AR0
                                //XAR(ARP)(31:16) = unchanged
                                tmp2 = bit_inverse_low_half(env->xar[tmp]);
                                tmp3 = bit_inverse_low_half(env->xar[0]);
                                tmp3 = tmp2 + tmp3;
                                tmp3 = bit_inverse_low_half(tmp3);
                                env->xar[tmp] = (env->xar[tmp] & 0xffff0000) | (tmp3 & 0x0000ffff);
                                break;
                            case 0b111: //b10 101 111 *BR0--
                                tmp = CPU_GET_STATUS(st1, ARP);
                                dest_addr = env->xar[tmp];
                                dest_value = ld32_swap(env, dest_addr);
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
                        tmp = CPU_GET_STATUS(st1, ARP);
                        dest_addr = env->xar[tmp];
                        dest_value = ld32_swap(env, dest_addr);
                        tmp2 = loc & 0b00000111;
                        CPU_SET_STATUS(st1, ARP, tmp2);
                        break;
                    case 0b111: //b10 111 ...
                        switch(loc & 0b00000111) {
                            case 0b000: //b10 111 000 *
                                tmp = CPU_GET_STATUS(st1, ARP);
                                dest_addr = env->xar[tmp];
                                dest_value = ld32_swap(env, dest_addr);
                                break;
                            case 0b001: //b10 111 001 *++
                                tmp = CPU_GET_STATUS(st1, ARP);
                                dest_addr = env->xar[tmp];
                                dest_value = ld32_swap(env, dest_addr);
                                env->xar[tmp] = env->xar[tmp] + loc_type;
                                break;
                            case 0b010: //b10 111 010 *--
                                tmp = CPU_GET_STATUS(st1, ARP);
                                dest_addr = env->xar[tmp];
                                dest_value = ld32_swap(env, dest_addr);
                                env->xar[tmp] = env->xar[tmp] - loc_type;
                                break;
                            case 0b011: //b10 111 011 *0++
                                tmp = CPU_GET_STATUS(st1, ARP);
                                dest_addr = env->xar[tmp];
                                dest_value = ld32_swap(env, dest_addr);
                                env->xar[tmp] = env->xar[tmp] + (env->xar[0] & 0xffff);
                                break;
                            case 0b100: //b10 111 100 *0--
                                tmp = CPU_GET_STATUS(st1, ARP);
                                dest_addr = env->xar[tmp];
                                dest_value = ld32_swap(env, dest_addr);
                                env->xar[tmp] = env->xar[tmp] - (env->xar[0] & 0xffff);
                                break;
                            case 0b101: //b10 111 101 *SP++
                                dest_addr = env->sp;
                                dest_value = ld32_swap(env, dest_addr);
                                env->sp = env->sp + loc_type;
                                break;
                            case 0b110: //b10 111 110 *--SP
                                env->sp = env->sp - loc_type;
                                dest_addr = env->sp;
                                dest_value = ld32_swap(env, dest_addr);
                                break;
                            case 0b111: //b10 111 111 *AR6%++
                                dest_addr = env->xar[6];
                                dest_value = ld32_swap(env, dest_addr);
                                if ((env->xar[6] & 0xff) == (env->xar[1] & 0xff)) {// XAR6(7:0)==XAR1(7:0)
                                    env->xar[6] = env->xar[6] & 0xffffff00; //XAR6(7:0) = 0x00
                                }
                                else {
                                    tmp = env->xar[6] & 0xffff;
                                    tmp = tmp + loc_type;
                                    env->xar[6] = (env->xar[6] & 0xffff0000) | (tmp & 0xffff);
                                }
                                CPU_SET_STATUS(st1, ARP, 6);
                                break;
                        }
                        break;
                }
                break;
            case 0b11: //b11 III AAA
                tmp = loc & 0b00000111; //n AAA
                CPU_SET_STATUS(st1, ARP, tmp);
                tmp2 = (loc >> 3) & 0b00000111;//3bit III
                dest_addr = env->xar[tmp] + tmp2;
                dest_value = ld32_swap(env, dest_addr);
                break;
        }
    }
    else {
        //todo amode==1
    }
    return dest_value;
}

static void do_st_loc16(CPUTms320c28xState *env, uint32_t loc, uint32_t value, op_store st16_swap)
{
    value = value & 0xffff;

    uint32_t amode = CPU_GET_STATUS(st1, AMODE);
    uint32_t loc_type = 1;
    uint32_t dest_addr = -1;
    uint32_t tmp, tmp2, tmp3;

    if (amode == 0) {
        switch((loc & 0b11000000)>>6) {
            case 0b00: //b00 III III @6bit
                tmp = env->dp;
                dest_addr = (tmp << 6) +  (loc & 0x3f);
                st16_swap(env, dest_addr, value);
                break;
            case 0b01: //b01 III III *-SP[6bit]
                tmp = env->sp;
                dest_addr = tmp -  (loc & 0x3f);
                st16_swap(env, dest_addr, value);
                break;
            case 0b10: //b10 ... ...
                switch((loc & 0b00111000)>>3) {
                    case 0b000: //b10 000 AAA *XARn++
                        tmp = loc & 0b00000111;
                        CPU_SET_STATUS(st1, ARP, tmp);
                        dest_addr = env->xar[tmp];
                        st16_swap(env, dest_addr, value);
                        env->xar[tmp] = env->xar[tmp] + loc_type;
                        break;
                    case 0b001: //b10 001 AAA *--XARn
                        tmp = loc & 0b00000111;
                        CPU_SET_STATUS(st1, ARP, tmp);
                        env->xar[tmp] = env->xar[tmp] - loc_type;
                        dest_addr = env->xar[tmp];
                        st16_swap(env, dest_addr, value);
                        break;
                    case 0b010: //b10 010 AAA *+XARn[AR0]
                        tmp = loc & 0b00000111;
                        CPU_SET_STATUS(st1, ARP, tmp);
                        dest_addr = env->xar[tmp] + (env->xar[0] & 0xffff);
                        st16_swap(env, dest_addr, value);
                        break;
                    case 0b011: //b10 011 AAA *+XARn[AR1]
                        tmp = loc & 0b00000111;
                        CPU_SET_STATUS(st1, ARP, tmp);
                        dest_addr = env->xar[tmp] + (env->xar[1] & 0xffff);
                        st16_swap(env, dest_addr, value);
                        break;
                    case 0b100: //@ARn
                        tmp = loc & 0b00000111;
                        env->xar[tmp] = st_low_half(env->xar[tmp], value);
                        break;
                    case 0b101: //b10 101 ...
                        switch(loc & 0b00000111) {
                            case 0b000: //@AH
                                env->acc = st_high_half(env->acc, value);
                                break;
                            case 0b001: //@AL
                                env->acc = st_low_half(env->acc, value);
                                break;
                            case 0b010: //@PH
                                env->p = st_high_half(env->p, value);
                                break;
                            case 0b011: //@PL
                                env->p = st_low_half(env->p, value);
                                break;
                            case 0b100: //@TH
                                env->xt = st_high_half(env->xt, value);
                                break;
                            case 0b101: //@SP
                                env->sp = st_low_half(env->sp, value);
                                break;
                            case 0b110: //b10 101 110 *BR0++
                                tmp = CPU_GET_STATUS(st1, ARP);
                                dest_addr = env->xar[tmp];
                                st16_swap(env, dest_addr, value);
                                //rcadd: XAR(ARP)(15:0)= AR(ARP)rcadd AR0
                                //XAR(ARP)(31:16) = unchanged
                                tmp2 = bit_inverse_low_half(env->xar[tmp]);
                                tmp3 = bit_inverse_low_half(env->xar[0]);
                                tmp3 = tmp2 + tmp3;
                                tmp3 = bit_inverse_low_half(tmp3);
                                env->xar[tmp] = (env->xar[tmp] & 0xffff0000) | (tmp3 & 0x0000ffff);
                                break;
                            case 0b111: //b10 101 111 *BR0--
                                tmp = CPU_GET_STATUS(st1, ARP);
                                dest_addr = env->xar[tmp];
                                st16_swap(env, dest_addr, value);
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
                        tmp = CPU_GET_STATUS(st1, ARP);
                        dest_addr = env->xar[tmp];
                        st16_swap(env, dest_addr, value);
                        tmp2 = loc & 0b00000111;
                        CPU_SET_STATUS(st1, ARP, tmp2);
                        break;
                    case 0b111: //b10 111 ...
                        switch(loc & 0b00000111) {
                            case 0b000: //b10 111 000 *
                                tmp = CPU_GET_STATUS(st1, ARP);
                                dest_addr = env->xar[tmp];
                                st16_swap(env, dest_addr, value);
                                break;
                            case 0b001: //b10 111 001 *++
                                tmp = CPU_GET_STATUS(st1, ARP);
                                dest_addr = env->xar[tmp];
                                st16_swap(env, dest_addr, value);
                                env->xar[tmp] = env->xar[tmp] + loc_type;
                                break;
                            case 0b010: //b10 111 010 *--
                                tmp = CPU_GET_STATUS(st1, ARP);
                                dest_addr = env->xar[tmp];
                                st16_swap(env, dest_addr, value);
                                env->xar[tmp] = env->xar[tmp] - loc_type;
                                break;
                            case 0b011: //b10 111 011 *0++
                                tmp = CPU_GET_STATUS(st1, ARP);
                                dest_addr = env->xar[tmp];
                                st16_swap(env, dest_addr, value);
                                env->xar[tmp] = env->xar[tmp] + (env->xar[0] & 0xffff);
                                break;
                            case 0b100: //b10 111 100 *0--
                                tmp = CPU_GET_STATUS(st1, ARP);
                                dest_addr = env->xar[tmp];
                                st16_swap(env, dest_addr, value);
                                env->xar[tmp] = env->xar[tmp] - (env->xar[0] & 0xffff);
                                break;
                            case 0b101: //b10 111 101 *SP++
                                dest_addr = env->sp;
                                st16_swap(env, dest_addr, value);
                                env->sp = env->sp + loc_type;
                                break;
                            case 0b110: //b10 111 110 *--SP
                                env->sp = env->sp - loc_type;
                                dest_addr = env->sp;
                                st16_swap(env, dest_addr, value);
                                break;
                            case 0b111: //b10 111 111 *AR6%++
                                dest_addr = env->xar[6];
                                st16_swap(env, dest_addr, value);
                                if ((env->xar[6] & 0xff) == (env->xar[1] & 0xff)) {// XAR6(7:0)==XAR1(7:0)
                                    env->xar[6] = env->xar[6] & 0xffffff00; //XAR6(7:0) = 0x00
                                }
                                else {
                                    tmp = env->xar[6] & 0xffff;
                                    tmp = tmp + loc_type;
                                    env->xar[6] = (env->xar[6] & 0xffff0000) | (tmp & 0xffff);
                                }
                                CPU_SET_STATUS(st1, ARP, 6);
                                break;
                        }
                        break;
                }
                break;
            case 0b11: //b11 III AAA
                tmp = loc & 0b00000111; //n AAA
                CPU_SET_STATUS(st1, ARP, tmp);
                tmp2 = (loc >> 3) & 0b00000111;//3bit III
                dest_addr = env->xar[tmp] + tmp2;
                st16_swap(env, dest_addr, value);
                break;
        }
    }
    else {
        //todo amode==1
    }
}


void HELPER(st_loc16)(CPUTms320c28xState *env, uint32_t loc, uint32_t value)
{
    do_st_loc16(env, loc, value, st16_swap);
}


void HELPER(st_loc16_byte_addressing)(CPUTms320c28xState *env, uint32_t loc, uint32_t value)
{
    bool is_byte_addressing = false;
    uint32_t offset = 0;
    uint32_t n = 0;
    if ((loc >> 6) == 0b11)
    {
        is_byte_addressing = true;
        n = loc & 0b111;
        offset = (loc >> 3) & 0b00000111;
        CPU_SET_STATUS(st1, ARP, n);
    }
    else if ((loc >> 3) == 0b10010)
    {
        is_byte_addressing = true;
        n = loc & 0b111;
        offset = env->xar[0] & 0xffff;
        CPU_SET_STATUS(st1, ARP, n);
    }
    else if ((loc >> 3) == 0b10011)
    {
        is_byte_addressing = true;
        n = loc & 0b111;
        offset = env->xar[1] & 0xffff;
        CPU_SET_STATUS(st1, ARP, n);
    }

    if (is_byte_addressing) {
        value = value & 0xff;
        cpu_stb_data(env, env->xar[n] * 2 + offset, value);
    }
    else {
        do_st_loc16(env, loc, value, st16_lsb);
    }
}

void HELPER(st_loc32)(CPUTms320c28xState *env, uint32_t loc, uint32_t value)
{
    uint32_t amode = CPU_GET_STATUS(st1, AMODE);
    uint32_t loc_type = 2;
    uint32_t dest_addr = -1;
    uint32_t tmp, tmp2, tmp3;

    if (amode == 0) {
        switch((loc & 0b11000000)>>6) {
            case 0b00: //b00 III III @6bit
                tmp = env->dp;
                dest_addr = (tmp << 6) +  (loc & 0x3f);
                st32_swap(env, dest_addr, value);
                break;
            case 0b01: //b01 III III *-SP[6bit]
                tmp = env->sp;
                dest_addr = tmp -  (loc & 0x3f);
                st32_swap(env, dest_addr, value);
                break;
            case 0b10: //b10 ... ...
                switch((loc & 0b00111000)>>3) {
                    case 0b000: //b10 000 AAA *XARn++
                        tmp = loc & 0b00000111;
                        CPU_SET_STATUS(st1, ARP, tmp);
                        dest_addr = env->xar[tmp];
                        st32_swap(env, dest_addr, value);
                        env->xar[tmp] = env->xar[tmp] + loc_type;
                        break;
                    case 0b001: //b10 001 AAA *--XARn
                        tmp = loc & 0b00000111;
                        CPU_SET_STATUS(st1, ARP, tmp);
                        env->xar[tmp] = env->xar[tmp] - loc_type;
                        dest_addr = env->xar[tmp];
                        st32_swap(env, dest_addr, value);
                        break;
                    case 0b010: //b10 010 AAA *+XARn[AR0]
                        tmp = loc & 0b00000111;
                        CPU_SET_STATUS(st1, ARP, tmp);
                        dest_addr = env->xar[tmp] + (env->xar[0] & 0xffff);
                        st32_swap(env, dest_addr, value);
                        break;
                    case 0b011: //b10 011 AAA *+XARn[AR1]
                        tmp = loc & 0b00000111;
                        CPU_SET_STATUS(st1, ARP, tmp);
                        dest_addr = env->xar[tmp] + (env->xar[1] & 0xffff);
                        st32_swap(env, dest_addr, value);
                        break;
                    case 0b100: //@XARn
                        tmp = loc & 0b00000111;
                        env->xar[tmp] = value;
                        break;
                    case 0b101: //b10 101 ...
                        switch(loc & 0b00000111) {
                            case 0b001: //@ACC
                                env->acc = value;
                                break;
                            case 0b011: //@P
                                env->p = value;
                                break;
                            case 0b100: //@XT
                                env->xt = value;
                                break;
                            case 0b110: //b10 101 110 *BR0++
                                tmp = CPU_GET_STATUS(st1, ARP);
                                dest_addr = env->xar[tmp];
                                st32_swap(env, dest_addr, value);
                                //rcadd: XAR(ARP)(15:0)= AR(ARP)rcadd AR0
                                //XAR(ARP)(31:16) = unchanged
                                tmp2 = bit_inverse_low_half(env->xar[tmp]);
                                tmp3 = bit_inverse_low_half(env->xar[0]);
                                tmp3 = tmp2 + tmp3;
                                tmp3 = bit_inverse_low_half(tmp3);
                                env->xar[tmp] = (env->xar[tmp] & 0xffff0000) | (tmp3 & 0x0000ffff);
                                break;
                            case 0b111: //b10 101 111 *BR0--
                                tmp = CPU_GET_STATUS(st1, ARP);
                                dest_addr = env->xar[tmp];
                                st32_swap(env, dest_addr, value);
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
                        tmp = CPU_GET_STATUS(st1, ARP);
                        dest_addr = env->xar[tmp];
                        st32_swap(env, dest_addr, value);
                        tmp2 = loc & 0b00000111;
                        CPU_SET_STATUS(st1, ARP, tmp2);
                        break;
                    case 0b111: //b10 111 ...
                        switch(loc & 0b00000111) {
                            case 0b000: //b10 111 000 *
                                tmp = CPU_GET_STATUS(st1, ARP);
                                dest_addr = env->xar[tmp];
                                st32_swap(env, dest_addr, value);
                                break;
                            case 0b001: //b10 111 001 *++
                                tmp = CPU_GET_STATUS(st1, ARP);
                                dest_addr = env->xar[tmp];
                                st32_swap(env, dest_addr, value);
                                env->xar[tmp] = env->xar[tmp] + loc_type;
                                break;
                            case 0b010: //b10 111 010 *--
                                tmp = CPU_GET_STATUS(st1, ARP);
                                dest_addr = env->xar[tmp];
                                st32_swap(env, dest_addr, value);
                                env->xar[tmp] = env->xar[tmp] - loc_type;
                                break;
                            case 0b011: //b10 111 011 *0++
                                tmp = CPU_GET_STATUS(st1, ARP);
                                dest_addr = env->xar[tmp];
                                st32_swap(env, dest_addr, value);
                                env->xar[tmp] = env->xar[tmp] + (env->xar[0] & 0xffff);
                                break;
                            case 0b100: //b10 111 100 *0--
                                tmp = CPU_GET_STATUS(st1, ARP);
                                dest_addr = env->xar[tmp];
                                st32_swap(env, dest_addr, value);
                                env->xar[tmp] = env->xar[tmp] - (env->xar[0] & 0xffff);
                                break;
                            case 0b101: //b10 111 101 *SP++
                                dest_addr = env->sp;
                                st32_swap(env, dest_addr, value);
                                env->sp = env->sp + loc_type;
                                break;
                            case 0b110: //b10 111 110 *--SP
                                env->sp = env->sp - loc_type;
                                dest_addr = env->sp;
                                st32_swap(env, dest_addr, value);
                                break;
                            case 0b111: //b10 111 111 *AR6%++
                                dest_addr = env->xar[6];
                                st32_swap(env, dest_addr, value);
                                if ((env->xar[6] & 0xff) == (env->xar[1] & 0xff)) {// XAR6(7:0)==XAR1(7:0)
                                    env->xar[6] = env->xar[6] & 0xffffff00; //XAR6(7:0) = 0x00
                                }
                                else {
                                    tmp = env->xar[6] & 0xffff;
                                    tmp = tmp + loc_type;
                                    env->xar[6] = (env->xar[6] & 0xffff0000) | (tmp & 0xffff);
                                }
                                CPU_SET_STATUS(st1, ARP, 6);
                                break;
                        }
                        break;
                }
                break;
            case 0b11: //b11 III AAA
                tmp = loc & 0b00000111; //n AAA
                CPU_SET_STATUS(st1, ARP, tmp);
                tmp2 = (loc >> 3) & 0b00000111;//3bit III
                dest_addr = env->xar[tmp] + tmp2;
                st32_swap(env, dest_addr, value);
                break;
        }
    }
    else {
        //todo amode==1
    }
}

uint32_t HELPER(addressing_mode)(CPUTms320c28xState *env, uint32_t loc, uint32_t loc_type) {
    uint32_t amode = CPU_GET_STATUS(st1, AMODE);
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
                        CPU_SET_STATUS(st1, ARP, tmp);
                        dest = env->xar[tmp];
                        env->xar[tmp] = env->xar[tmp] + loc_type;
                        break;
                    case 0b001: //b10 001 AAA *--XARn
                        tmp = loc & 0b00000111;
                        CPU_SET_STATUS(st1, ARP, tmp);
                        env->xar[tmp] = env->xar[tmp] - loc_type;
                        dest = env->xar[tmp];
                        break;
                    case 0b010: //b10 010 AAA *+XARn[AR0]
                        tmp = loc & 0b00000111;
                        CPU_SET_STATUS(st1, ARP, tmp);
                        dest = env->xar[tmp] + (env->xar[0] & 0xffff);
                        break;
                    case 0b011: //b10 011 AAA *+XARn[AR1]
                        tmp = loc & 0b00000111;
                        CPU_SET_STATUS(st1, ARP, tmp);
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
                                tmp = CPU_GET_STATUS(st1, ARP);
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
                                tmp = CPU_GET_STATUS(st1, ARP);
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
                        tmp = CPU_GET_STATUS(st1, ARP);
                        dest = env->xar[tmp];
                        tmp2 = loc & 0b00000111;
                        CPU_SET_STATUS(st1, ARP, tmp2);
                        break;
                    case 0b111: //b10 111 ...
                        switch(loc & 0b00000111) {
                            case 0b000: //b10 111 000 *
                                tmp = CPU_GET_STATUS(st1, ARP);
                                dest = env->xar[tmp];
                                break;
                            case 0b001: //b10 111 001 *++
                                tmp = CPU_GET_STATUS(st1, ARP);
                                dest = env->xar[tmp];
                                env->xar[tmp] = env->xar[tmp] + loc_type;
                                break;
                            case 0b010: //b10 111 010 *--
                                tmp = CPU_GET_STATUS(st1, ARP);
                                dest = env->xar[tmp];
                                env->xar[tmp] = env->xar[tmp] - loc_type;
                                break;
                            case 0b011: //b10 111 011 *0++
                                tmp = CPU_GET_STATUS(st1, ARP);
                                dest = env->xar[tmp];
                                env->xar[tmp] = env->xar[tmp] + (env->xar[0] & 0xffff);
                                break;
                            case 0b100: //b10 111 100 *0--
                                tmp = CPU_GET_STATUS(st1, ARP);
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
                                CPU_SET_STATUS(st1, ARP, 6);
                                break;
                        }
                        break;
                }
                break;
            case 0b11: //b11 III AAA
                tmp = loc & 0b00000111; //n AAA
                CPU_SET_STATUS(st1, ARP, tmp);
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