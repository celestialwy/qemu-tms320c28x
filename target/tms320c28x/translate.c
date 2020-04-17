/*
 * TMS320C28X translation
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
#include "disas/disas.h"
#include "tcg-op.h"
#include "qemu/log.h"
#include "qemu/bitops.h"
#include "qemu/qemu-print.h"
#include "exec/cpu_ldst.h"
#include "exec/translator.h"

#include "exec/helper-proto.h"
#include "exec/helper-gen.h"
#include "exec/gen-icount.h"

#include "trace-tcg.h"
#include "exec/log.h"

/* is_jmp field values */
#define DISAS_EXIT    DISAS_TARGET_0  /* force exit to main loop */
#define DISAS_JUMP    DISAS_TARGET_1  /* exit via jmp_pc/jmp_pc_imm */
#define DISAS_REPEAT    DISAS_TARGET_2  /* exit via jmp_pc/jmp_pc_imm */

typedef struct DisasContext {
    DisasContextBase base;

    bool rpt_set;
    // int rpt_counter;
    // TCGv temp[8];
} DisasContext;

static TCGv cpu_acc; /* Accumulator todo:ah,al */
static TCGv cpu_xar[8]; /* Auxiliary todo:ar[8] */
static TCGv cpu_dp; /* Data-page pointer */
static TCGv cpu_ifr; /* Interrupt flag register */
static TCGv cpu_ier; /* Interrupt enable register */
static TCGv cpu_dbgier; /* Debug interrupt enable register */
static TCGv cpu_p; /* Product register todo:ph,pl*/
static TCGv cpu_pc;          /* Program counter 22bit, reset to 0x3f ffc0*/
static TCGv cpu_rpc;         /* Return program counte 22bit*/
static TCGv cpu_sp; /* Stack pointer, reset to 0x0400 */
static TCGv cpu_st0; /* Status register 0 */
static TCGv cpu_st1; /* Status register 1, reset to 0x080b */
static TCGv cpu_xt;         /* Multiplicand register, todo:t,tl*/
static TCGv cpu_rptc;
static TCGv cpu_insn_length;
// static TCGv cpu_tmp[8]; 

void tms320c28x_translate_init(void)
{
    static const char * const regnames[] = {
        "xar0", "xar1", "xar2", "xar3", "xar4", "xar5", "xar6", "xar7",
    };
    // static const char * const regnames2[] = {
    //     "tmp0", "tmp1", "tmp2", "tmp3", "tmp4", "tmp5", "tmp6", "tmp7",
    // };
    int i;

    cpu_acc = tcg_global_mem_new(cpu_env,
                                offsetof(CPUTms320c28xState, acc), "acc");
    cpu_dp = tcg_global_mem_new(cpu_env,
                                offsetof(CPUTms320c28xState, dp), "dp");
    cpu_ifr = tcg_global_mem_new(cpu_env,
                                offsetof(CPUTms320c28xState, ifr), "ifr");
    cpu_ier = tcg_global_mem_new(cpu_env,
                                offsetof(CPUTms320c28xState, ier), "ier");
    cpu_dbgier = tcg_global_mem_new(cpu_env,
                                offsetof(CPUTms320c28xState, dbgier), "dbgier");
    cpu_p = tcg_global_mem_new(cpu_env,
                                offsetof(CPUTms320c28xState, p), "p");
    cpu_pc = tcg_global_mem_new(cpu_env,
                                offsetof(CPUTms320c28xState, pc), "pc");
    cpu_rpc = tcg_global_mem_new(cpu_env,
                                offsetof(CPUTms320c28xState, rpc), "rpc");
    cpu_sp = tcg_global_mem_new(cpu_env,
                                offsetof(CPUTms320c28xState, sp), "sp");
    cpu_st0 = tcg_global_mem_new(cpu_env,
                                offsetof(CPUTms320c28xState, st0), "st0");
    cpu_st1 = tcg_global_mem_new(cpu_env,
                                offsetof(CPUTms320c28xState, st1), "st1");
    cpu_xt = tcg_global_mem_new(cpu_env,
                                offsetof(CPUTms320c28xState, xt), "xt");
    cpu_rptc = tcg_global_mem_new(cpu_env,
                                offsetof(CPUTms320c28xState, rptc), "rptc");
                                                                
    for (i = 0; i < 8; i++) {
        cpu_xar[i] = tcg_global_mem_new(cpu_env,offsetof(CPUTms320c28xState,xar[i]),regnames[i]);
        // cpu_tmp[i] = tcg_global_mem_new(cpu_env,offsetof(CPUTms320c28xState,tmp[i]),regnames2[i]);
    }
    cpu_insn_length = tcg_global_mem_new(cpu_env,
                                offsetof(CPUTms320c28xState, insn_length), "insn_length");
}

// #include "decode-status.c"
#include "decode-base.c"

#include "decode-mov.c"
#include "decode-math-add.c"
#include "decode-math-sub.c"
#include "decode-math-other.c"
#include "decode-bitop.c"
#include "decode-branch.c"
#include "decode-interrupt.c"
#include "decode-other.c"

static int decode(Tms320c28xCPU *cpu , DisasContext *ctx, uint32_t insn, uint32_t insn2) 
{
    /* Set the default instruction length.  */
    int length = 2;
    // uint32_t insn32 = insn;
    bool set_repeat_counter = false;
    // qemu_log_mask(CPU_LOG_TB_IN_ASM ,"insn is: 0x%x \n",insn);

    switch ((insn & 0xf000) >> 12) {
        case 0b0000:
            switch ((insn & 0x0f00) >> 8) {
                case 0b0000: //0000 0000 .... ....
                    switch ((insn & 0x00c0) >> 6) {
                        case 0b00:
                            if(((insn >> 5) & 1) == 0) { //0000 0000 000. ....
                                if(((insn >> 4) & 1) == 0) { //0000 0000 0000 ....
                                    switch(insn & 0x000f) {
                                        case 0: //0000 0000 0000 0000 ITRAP0
                                            //todo
                                            gen_exception(ctx, EXCP_INTERRUPT_ILLEGAL);
                                            break;
                                        case 1: //0000 0000 0000 0001, ABORTI P124
                                            gen_helper_aborti(cpu_env);
                                            break;
                                        case 2: //0000 0000 0000 0010, POP IFR
                                            gen_pop_ifr(ctx);
                                            break;
                                        case 3: //0000 0000 0000 0011 POP AR1H:AR0H
                                            gen_pop_ar1h_ar0h(ctx);
                                            break;
                                        case 4: //0000 0000 0000 0100 PUSH RPC
                                            gen_push_rpc(ctx);
                                            break;
                                        case 5: //0000 0000 0000 0101 PUSH AR1H:AR0H
                                            gen_push_ar1h_ar0h(ctx);
                                            break;
                                        case 6: //0000 0000 0000 0110 LRETR
                                            gen_lretr(ctx);
                                            break;
                                        case 7: //0000 0000 0000 0111 POP RPC
                                            gen_pop_rpc(ctx);
                                            break;
                                        default: //0000 0000 0000 1nnn CCCC CCCC CCCC CCCC BANZ 16bitOffset,ARn--
                                        {
                                            uint32_t n = insn & 0b111;
                                            int16_t offset = insn2;
                                            length = 4;
                                            gen_banz_16bitOffset_arn(ctx, offset, n);
                                            break;
                                        }
                                    }
                                }
                                else { //0000 0000 0001 CCCC INTR INTx
                                    uint32_t n = (insn & 0xf) + 1;//int1 = 0
                                    gen_intr(ctx, n);
                                }
                            }
                            else {// 0000 0000 001C CCCC, TRAP #VectorNumber

                            }
                            break;
                        case 0b01: /*0000 0000 01.. .... LB 22bit */
                        {
                            uint32_t dest = ((insn & 0x3f)<< 16) | insn2;
                            // dest = dest * 2;
                            gen_lb_22bit(ctx, dest);
                            length = 4;
                            break;
                        }
                        case 0b10: // 0000 0000 10CC CCCC CCCC CCCC CCCC CCCC, LC 22bit
                        {
                            uint32_t imm = ((insn & 0x3f)<< 16) | insn2;
                            gen_lc_22bit(ctx, imm);
                            length = 4;
                            break;
                        }
                        case 0b11: // 0000 0000 11CC CCCC CCCC CCCC CCCC CCCC, FFC XAR7,22bit
                        {
                            uint32_t imm = ((insn & 0x3f)<< 16) | insn2;
                            gen_ffc_xar7_imm(ctx, imm);
                            length = 4;
                            break;
                        }
                    }
                    break;
                case 0b0001: //0000 0001 LLLL LLLL SUBU ACC,loc16
                {
                    uint32_t mode = insn & 0xff;
                    gen_subu_acc_loc16(ctx, mode);
                    break;
                }
                case 0b0010: //0000 0010 CCCC CCCC MOVB ACC,#8bit
                {
                    uint32_t imm = insn & 0xff;
                    gen_movb_acc_8bit(ctx, imm);
                    break;
                }
                case 0b0011: //0000 0011 LLLL LLLL SUBL ACC,loc32
                {
                    uint32_t mode = insn & 0xff;
                    gen_subl_acc_loc32(ctx, mode);
                    break;
                }
                case 0b0100: //0000 0100 LLLL LLLL SUB ACC,loc16<<#16
                {
                    uint32_t mode = insn & 0xff;
                    gen_sub_acc_loc16_shift(ctx, mode, 16);
                    break;
                }
                case 0b0101: //0000 0101 LLLL LLLL ADD ACC,loc16<<#16
                {
                    uint32_t mode = insn & 0xff;
                    gen_add_acc_loc16_shift(ctx, mode, 16);
                    break;
                }
                case 0b0110: //0000 0110 .... .... MOVL ACC,loc32
                {
                    uint32_t mode = insn & 0xff;
                    gen_movl_acc_loc32(ctx, mode);
                    break;
                }
                case 0b0111: //0000 0111 LLLL LLLL ADDL ACC,loc32
                {
                    uint32_t mode = insn & 0xff;
                    gen_addl_acc_loc32(ctx, mode);
                    break;
                }
                case 0b1000: //0000 1000 LLLL LLLL 32bit ADD loc16,#16bitSigned
                {
                    uint32_t mode = insn & 0xff;
                    uint32_t imm = insn2;
                    length = 4;
                    gen_add_loc16_16bit(ctx, mode, imm);
                    break;
                }
                case 0b1001: //0000 1001 CCCC CCCC ADDB ACC,#8bit
                {
                    uint32_t imm = insn & 0xff;
                    gen_addb_acc_8bit(ctx, imm);
                    break;
                }
                case 0b1010: //0000 1010 LLLL LLLL INC loc16
                {
                    uint32_t mode = insn & 0xff;
                    gen_inc_loc16(ctx, mode);
                    break;
                }
                case 0b1011: //0000 1011 LLLL LLLL DEC loc16
                {
                    uint32_t mode = insn & 0xff;
                    gen_dec_loc16(ctx, mode);
                    break;
                }
                case 0b1100: //0000 1100 LLLL LLLL ADDCU ACC,loc16
                {
                    uint32_t mode = insn & 0xff;
                    gen_addcu_acc_loc16(ctx, mode);
                    break;
                }
                case 0b1101: //0000 1101 LLLL LLLL ADDU ACC,loc16
                {
                    uint32_t mode = insn & 0xff;
                    gen_addu_acc_loc16(ctx, mode);
                    // set_repeat_counter = true;
                    break;
                }
                case 0b1110://0000 1110 LLLL LLLL MOVU ACC,loc16
                {
                    uint32_t mode = insn & 0xff;
                    gen_movu_acc_loc16(ctx, mode);
                    break;
                }
                case 0b1111://0000 1111 LLLL LLLL CMPL ACC,loc32
                {
                    uint32_t mode = insn & 0xff;
                    gen_cmpl_acc_loc32(ctx, mode);
                    break;
                }
            }
            break;
        case 0b0001:
            switch ((insn & 0x0f00) >> 8) {
                case 0b0000: //0001 0000 LLLL LLLL MOVA T,loc16
                {
                    uint32_t mode = insn & 0xff;
                    gen_mova_t_loc16(ctx, mode);
                    break;
                }
                case 0b0001: //0001 0001 LLLL LLLL MOVS T,loc16
                {
                    uint32_t mode = insn & 0xff;
                    gen_movs_t_loc16(ctx, mode);
                    break;
                }
                case 0b0100: //0001 0100 LLLL LLLL CCCC CCCC CCCC CCCC MAC P,loc16,0:pma
                {
                    uint32_t mode = insn & 0xff;
                    uint32_t addr = insn2;
                    gen_mac_p_loc16_pma(ctx, mode, addr);
                    length = 4;
                    break;
                }
                case 0b0110: //0001 0110 LLLL LLLL MOVP T,loc16
                {

                    uint32_t mode = insn & 0xff;
                    gen_movp_t_loc16(ctx, mode);
                    break;
                }
                case 0b1000: //0001 1000 LLLL LLLL CCCC CCCC CCCC CCCC AND loc16,#16bitSigned
                {
                    uint32_t mode = insn & 0xff;
                    uint32_t imm = insn2;
                    gen_and_loc16_16bit(ctx, mode, imm);
                    length = 4;
                    break;
                }
                case 0b1001: //0001 1001 CCCC CCCC SUBB ACC,#8bit
                {
                    int32_t imm = insn & 0xff;
                    gen_subb_acc_8bit(ctx, imm);
                    break;
                }
                case 0b1011: //0001 1011 LLLL LLLL CCCC CCCC CCCC CCCC CMP loc16,#16bitSigned
                {
                    length = 4;
                    int32_t imm = insn2;
                    uint32_t mode = insn & 0xff;
                    gen_cmp_loc16_16bit(ctx, mode, imm);
                    break;
                }
                case 0b1110: //0001 1110 LLLL LLLL MOVL loc32,ACC
                {
                    uint32_t mode = insn & 0xff;
                    gen_movl_loc32_acc(ctx, mode);
                    break;
                }
                case 0b1111: //0001 1111 LLLL LLLL SUBCU ACC,loc16
                {
                    uint32_t mode = insn & 0xff;
                    gen_subcu_acc_loc16(ctx, mode);
                    break;
                }
            }
            break;
        case 0b0010:
            switch ((insn & 0x0f00) >> 8) {
                case 0b0000: //0010 0000 LLLL LLLL MOV loc16,IER
                {
                    uint32_t mode = insn & 0xff;
                    gen_mov_loc16_ier(ctx, mode);
                    break;
                }
                case 0b0001: //0010 0001 LLLL LLLL MOV loc16,T
                {
                    uint32_t mode = insn & 0xff;
                    gen_mov_loc16_t(ctx, mode);
                    break;
                }
                case 0b0010:
                    break;
                case 0b0011: //0010 0011 LLLL LLLL MOV IER,loc16
                {
                    uint32_t mode = insn & 0xff;
                    gen_mov_ier_loc16(ctx, mode);
                    break;
                }
                case 0b0100:
                    break;
                case 0b0101: /* 0010 0101 .... .... MOV ACC, loc16<<#16 */
                {
                    uint32_t mode = insn & 0xff;
                    gen_mov_acc_loc16_shift(ctx, mode, 16);
                }
                    break;
                case 0b0110:
                    break;
                case 0b0111: //0010 1111 LLLL LLLL MOV PL,loc16
                {
                    uint32_t mode = insn & 0xff;
                    gen_mov_pl_loc16(ctx, mode);
                    break;
                }
                case 0b1000: /* MOV loc16, #16bit p260 */
                {
                    uint32_t imm = insn2;
                    uint32_t mode = insn & 0xff;
                    gen_mov_loc16_16bit(ctx, mode, imm);
                    length = 4;
                    break;
                }
                case 0b1001: //0010 1001 CCCC CCCC CLRC mode
                {
                    uint32_t mode = insn & 0xff;
                    gen_clrc_mode(ctx, mode);
                    break;
                }
                case 0b1010:
                    break;
                case 0b1011: //0010 1011 LLLL LLLL MOV loc16,#0
                {
                    uint32_t mode = insn & 0xff;
                    gen_mov_loc16_0(ctx, mode);
                    break;
                }
                case 0b1100: //0010 1100 LLLL LLLL CCCC CCCC CCCC CCCC LOOPZ loc16,#16bit
                {
                    length = 4;
                    uint32_t mask = insn2;
                    uint32_t mode = insn & 0xff;
                    gen_loopz_loc16_16bit(ctx, mode, mask);
                    break;
                }
                case 0b1101: //0010 1101 LLLL LLLL MOV T,loc16
                {
                    uint32_t mode = insn & 0xff;
                    gen_mov_t_loc16(ctx, mode);
                    break;
                }
                case 0b1110: //0010 1110 LLLL LLLL CCCC CCCC CCCC CCCC LOOPNZ loc16,#16bit
                {
                    length = 4;
                    uint32_t mask = insn2;
                    uint32_t mode = insn & 0xff;
                    gen_loopnz_loc16_16bit(ctx, mode, mask);
                    break;
                }
                case 0b1111: //0010 1111 LLLL LLLL MOV PH,loc16
                {
                    uint32_t mode = insn & 0xff;
                    gen_mov_ph_loc16(ctx, mode);
                    break;
                }
            }
            break;
        case 0b0011:
            switch ((insn & 0xf00) >> 8) {
                case 0b1000: //0011 0110 LLLL LLLL MOVB AL.MSB,loc16
                {
                    uint32_t mode = insn & 0xff;
                    gen_movb_al_msb_loc16(ctx, mode);
                    break;
                }
                case 0b1001: //0011 0111 LLLL LLLL MOVB AH.MSB,loc16
                {
                    uint32_t mode = insn & 0xff;
                    gen_movb_ah_msb_loc16(ctx, mode);
                    break;
                }
                case 0b1010: //0011 1010 LLLL LLLL MOVL loc32, XAR0
                {
                    uint32_t mode = insn & 0xff;
                    gen_movl_loc32_xarn(ctx, mode, 0);
                    break;
                }
                case 0b1011: //0011 1011 CCCC CCCC SETC mode
                {
                    uint32_t mode = insn & 0xff;
                    gen_setc_mode(ctx, mode);
                    break;
                }
                case 0b1100:
                case 0b1101://0011 110a LLLL LLLL MOVB loc16,AX.LSB
                {
                    uint32_t mode = insn & 0xff;
                    uint32_t is_AH = (insn >> 8) & 1;
                    gen_movb_loc16_ax_lsb(ctx, mode, is_AH);
                    break;
                }
                case 0b1110: //0011 1110 .... ....
                {
                    switch ((insn & 0x00f0) >> 4) {
                        case 0b0000: //0011 1110 0000 SHFT CCCC CCCC CCCC CCCC AND ACC,#16bit<<0...15
                        {
                            uint32_t imm = insn2;
                            uint32_t shift = insn & 0xf;
                            gen_and_acc_16bit_shift(ctx, imm, shift);
                            length = 4;
                            break;
                        }
                        case 0b0101://0011 1110 0101 ....
                        {
                            if (((insn >> 3) & 1) == 1) {//0011 1110 0101 1nnn MOV XARn,PC
                                uint32_t n = insn & 0b111;
                                gen_mov_xarn_pc(ctx, n);
                            }
                            else {

                            }
                            break;
                        }
                        case 0b0110://0011 1110 0110 ....
                        {
                            if (((insn >> 3) & 1) == 0) //0011 1110 0110 0RRR LCR *XARn
                            {
                                uint32_t n = insn & 0b111;
                                gen_lcr_xarn(ctx, n);
                            }
                            else
                            {

                            }
                            break;
                        }
                    }
                    break;
                }
                case 0b1111: //0011 1111 LLLL LLLL MOV loc16,P
                {
                    uint32_t mode = insn & 0xff;
                    gen_mov_loc16_p(ctx, mode);
                    break;
                }
            }
            break;
        case 0b0100:
            break;
        case 0b0101:
            switch ((insn & 0x0f00) >> 8) {
                case 0b0010:
                case 0b0011://0101 001A CCCC CCCC CMPB AX,#8bit
                {
                    uint32_t imm = insn & 0xff;
                    uint32_t is_AH = (insn >> 8) & 1;
                    gen_cmp_ax_8bit(ctx, imm, is_AH);
                    break;
                }
                case 0b0100:
                case 0b0101://0101 010A LLLL LLLL CMP AX,loc16
                {
                    uint32_t mode = insn & 0xff;
                    uint32_t is_AH = (insn >> 8) & 1;
                    gen_cmp_ax_loc16(ctx, mode, is_AH);
                }
                case 0b0110://0101 0110 .... ....
                    switch ((insn & 0x00f0) >> 4) {
                        case 0b0000: //0101 0110 0000 ....
                        {
                            switch (insn & 0x000f) {
                                case 0b0000: //0101 0110 0000 0000 0000 SHFT LLLL LLLL SUB ACC,loc16<<#1...15
                                {
                                    if (((insn2 & 0xf000) >> 12) == 0) {
                                        length = 4;
                                        uint32_t mode = insn2 & 0xff;
                                        uint32_t shift = insn2 >> 8;
                                        gen_sub_acc_loc16_shift(ctx, mode, shift);
                                    }
                                    break;
                                }
                                case 0b0001: //0101 0110 0000 0001 0000 0000 LLLL LLLL ADDL loc32,ACC
                                {
                                    if (((insn2 & 0xff00) >> 8) == 0) {
                                        uint32_t mode = insn2 & 0xff;
                                        gen_addl_loc32_acc(ctx, mode);
                                        length = 4;
                                    }
                                    break;
                                }
                                case 0b0010:
                                {
                                    if (((insn2 & 0xff00) >> 8) == 0) {
                                        uint32_t mode = insn2 & 0xff;
                                        gen_mov_ovc_loc16(ctx, mode);
                                        length = 4;
                                    }
                                    break;
                                }
                                case 0b0011: /* 0101 0110 0000 0011  MOV ACC, loc16<<#1...15 */
                                {
                                    if ((insn2 & 0xf000) == 0) { //this 4bit should be 0
                                        length = 4;
                                        /* MOV ACC, loc16<<#1...15 */
                                        uint32_t shift = (insn2 & 0x0f00) >> 8;
                                        uint32_t mode = (insn2 & 0xff);
                                        gen_mov_acc_loc16_shift(ctx, mode, shift);
                                    }
                                    break;
                                }
                                case 0b0100: // 0101 0110 0000 0100 ADD ACC,loc16<<#1...15
                                {
                                    if ((insn2 & 0xf000) == 0) { //this 4bit should be 0
                                        length = 4;
                                        uint32_t shift = (insn2 & 0x0f00) >> 8;
                                        uint32_t mode = (insn2 & 0xff);
                                        gen_add_acc_loc16_shift(ctx, mode, shift);
                                    }
                                    break;
                                }
                                case 0b0101: //0101 0110 0000 0101 0000 0000 LLLL LLLL IMPYL P,XT,loc32
                                {
                                    if (((insn2 >> 8) & 0xff) == 0)
                                    {
                                        length = 4;
                                        uint32_t mode = insn2 & 0xff;
                                        gen_impyl_p_xt_loc32(ctx, mode);
                                    }
                                    break;
                                }
                                case 0b0110: //0101 0110 0000 0110 0000 0000 LLLL LLLL MOV ACC,loc16<<T
                                {
                                    if (((insn2 & 0xff00) >> 8) == 0) {
                                        uint32_t mode = insn2 & 0xff;
                                        gen_mov_acc_loc16_t(ctx, mode);
                                        length = 4;
                                    }
                                    break;
                                }
                                case 0b0111: //0101 0110 0000 0111 .... //MAC P,loc16,*XAR7/++
                                {
                                    uint32_t mode2 = insn2 >> 8;
                                    uint32_t mode1 = insn2 & 0xff;
                                    if ((mode2 == 0b11000111) || (mode2 == 0b10000111))
                                    {
                                        length = 4;
                                        gen_mac_p_loc16_xar7(ctx, mode1, mode2);
                                    }
                                    break;
                                }
                                case 0b1000: //0101 0110 0000 1000 CCCC CCCC CCCC CCCC AND ACC,#16bit<<#16
                                {
                                    uint32_t imm = insn2;
                                    gen_and_acc_16bit_shift(ctx, imm, 16);
                                    length = 4;
                                    break;
                                }
                            }
                            break;
                        }
                        case 0b0001: //0101 0110 0001 ....
                        {
                            switch (insn & 0xf) {
                                case 0b0000: //0101 0110 0001 0000 ASRL ACC,T
                                {
                                    gen_asrl_acc_t(ctx);
                                    break;
                                }
                                case 0b0110: //0101 0110 0001 0110 CLRC AMODE
                                {
                                    gen_clrc_amode(ctx);
                                    break;
                                }
                                case 0b0111: //0101 0110 0001 0111 0000 0000 LLLL LLLL SUBCUL ACC,loc32
                                {
                                    if ((insn2 >> 8) == 0) {
                                        uint32_t mode = insn2 & 0xff;
                                        gen_subcul_acc_loc32(ctx, mode);
                                        length = 4;
                                    }
                                    break;
                                }
                                case 0b1000: //0101 0110 0001 1000 CMPR 2
                                {
                                    gen_cmpr_0123(ctx, 2);
                                    break;
                                }
                                case 0b1001: //0101 0110 0001 1001 CMPR 1
                                {
                                    gen_cmpr_0123(ctx, 1);
                                    break;
                                }
                                case 0b1010: //0101 0110 0001 1010 SETC M0M1MAP
                                {
                                    gen_setc_m0m1map(ctx);
                                    break;
                                }
                                case 0b1011: //0101 0110 0001 1011 CLRC XF
                                {
                                    gen_clrc_xf(ctx);
                                    break;
                                }
                                case 0b1100: //0101 0110 0001 1100 CMPR 3
                                {
                                    gen_cmpr_0123(ctx, 3);
                                    break;
                                }
                                case 0b1101: //0101 0110 0001 1101 CMPR 0
                                {
                                    gen_cmpr_0123(ctx, 0);
                                    break;
                                }
                                case 0b1110: //0101 0110 0001 1110 LPADDR
                                {
                                    gen_lpaddr(ctx);
                                    break;
                                }
                                case 0b1111: //0101 0110 0001 1111 SETC Objmode
                                {
                                    gen_setc_objmode(ctx);
                                    break;
                                }
                            }
                            break;
                        }
                        case 0b0010: //0101 0110 0010 ....
                        {
                            switch (insn & 0xf) {
                                case 0b0001: //0101 0110 0010 0001 xxxx xxxx LLLL LLLL MOVX TL,loc16
                                {
                                    length = 4;
                                    uint32_t mode = insn2 & 0xff;
                                    gen_movx_tl_loc16(ctx, mode);
                                    break;
                                }
                                case 0b0011: //0101 0110 0010 0011 32bit ADD ACC,loc16<<T
                                {
                                    length = 4;
                                    if ((insn2 & 0xff00) == 0) { //this 8bit should be 0
                                        uint32_t loc16 = (insn2 & 0xff);
                                        gen_add_acc_loc16_t(ctx, loc16);
                                    }
                                    break;
                                }
                                case 0b0110: //0101 0110 0010 0110 SETC XF
                                {
                                    gen_setc_xf(ctx);
                                    break;
                                }
                                case 0b0111: //0101 0110 0010 0111 0000 0000 LLLL LLLL SUB ACC,loc16<<T
                                {
                                    if ((insn2 >> 8) == 0) {
                                        length = 4;
                                        uint32_t mode = insn2 & 0xff;
                                        gen_sub_acc_loc16_t(ctx, mode);
                                    }
                                    break;
                                }
                                case 0b1000: //0101 0110 0010 1000 0000 0000 LLLL LLLL MOVU loc16,OVC
                                {
                                    if ((insn2 >> 8) == 0) {
                                        length = 4;
                                        uint32_t mode = insn2 & 0xff;
                                        gen_movu_loc16_ovc(ctx, mode);
                                    }
                                    break;
                                }
                                case 0b1001: //0101 0110 0010 1001 0000 0000 LLLL LLLL MOV loc16,OVC
                                {
                                    if ((insn2 >> 8) == 0) {
                                        length = 4;
                                        uint32_t mode = insn2 & 0xff;
                                        gen_mov_loc16_ovc(ctx, mode);
                                    }
                                    break;
                                }
                                case 0b1010: //
                                case 0b1011: //0101 0110 0010 101A 0000 COND LLLL LLLL MOV loc16,AX,COND
                                {
                                    if ((insn2 >> 12) == 0) {
                                        uint32_t mode = insn2 & 0xff;
                                        uint32_t cond = (insn2 >> 8) & 0xf;
                                        uint32_t is_AH = insn & 1;
                                        gen_mov_loc16_ax_cond(ctx, mode, cond, is_AH);
                                        length = 4;
                                    }
                                    break;
                                }
                                case 0b1100: //0101 0110 0010 1100 ASR64 ACC:P,T
                                {
                                    gen_asr64_acc_p_t(ctx);
                                    break;
                                }
                                case 0b1101: //0101 0110 0010 1101 0000 0SHF LLLL LLLL MOV loc16,ACC<<2...8
                                {
                                    if ((insn2 >> 11) == 0) {
                                        uint32_t mode = insn2 & 0xff;
                                        uint32_t shift = (insn2 >> 8) & 0b111;
                                        shift += 1;
                                        gen_mov_loc16_acc_shift(ctx, mode, shift, 2);
                                        length = 4;
                                    }
                                    break;
                                }
                                case 0b1111://0101 0110 0010 1111 0000 0SHF LLLL LLLL MOVH loc16,ACC<<2...8
                                {
                                    if ((insn2 >> 11) == 0) {
                                        uint32_t mode = insn2 & 0xff;
                                        uint32_t shift = (insn2 >> 8) & 0b111;
                                        shift += 1;
                                        gen_movh_loc16_acc_shift(ctx, mode, shift, 2);
                                        length = 4;
                                    }
                                    break;
                                }
                            }
                            break;
                        }
                        case 0b0011: //0101 0110 0011 ....
                        {
                            switch (insn & 0xf) {
                                case 0b0101: //0101 0110 0011 0101 CSB ACC
                                {
                                    gen_csb_acc(ctx);
                                    break;
                                }
                                case 0b0110: //0101 0110 0011 0110 CLRC Objmode
                                {
                                    gen_clrc_objmode(ctx);
                                    break;
                                }
                                case 0b1000: //0101 0110 0011 100A MOV PM,AX
                                case 0b1001: //0101 0110 0011 100A MOV PM,AX
                                {
                                    uint32_t is_AH = insn & 1;
                                    gen_mov_pm_ax(ctx, is_AH);
                                    break;
                                }
                                case 0b1111: //0101 0110 0011 1111 CLRC M0M1MAP
                                {
                                    gen_clrc_m0m1map(ctx);
                                    break;
                                }
                            }
                            break;
                        }
                        case 0b0100: //0101 0110 0100 ....
                        {
                            switch (insn & 0xf) {
                                case 0b0000: //0101 0110 0100 0000 xxxx xxxx LLLL LLLL ADDCL ACC,loc32
                                {
                                    uint32_t mode = insn2 & 0xff;
                                    length = 4;
                                    gen_addcl_acc_loc32(ctx, mode);
                                    break;
                                }
                                case 0b0001: //0101 0110 0100 0001 0000 0000 LLLL LLLL SUBL loc32,ACC
                                {
                                    if ((insn2 >> 8) == 0)
                                    {
                                        length = 4;
                                        uint32_t mode = insn2 & 0xff;
                                        gen_subl_loc32_acc(ctx, mode);
                                    }
                                    break;
                                }
                                case 0b0011: //0101 0110 0100 0011 0000 0000 LLLL LLLL IMPYSL P,XT,loc32
                                {
                                    if ((insn2 >> 8) == 0)
                                    {
                                        length = 4;
                                        uint32_t mode = insn2 & 0xff;
                                        gen_impysl_p_xt_loc32(ctx, mode);
                                    }
                                    break;
                                }
                                case 0b0100: //0101 0110 0100 0100 0000 0000 LLLL LLLL IMPYL ACC,XT,loc32
                                {
                                    if ((insn2 >> 8) == 0)
                                    {
                                        length = 4;
                                        uint32_t mode = insn2 & 0xff;
                                        gen_impyl_acc_xt_loc32(ctx, mode);
                                    }
                                    break;
                                }
                                case 0b1000: //0101 0110 0100 1000 0000 COND LLLL LLLL MOVL loc32,ACC,COND
                                {
                                    uint32_t mode = insn2 & 0xff;
                                    uint32_t cond = (insn2 >> 8) & 0xf;
                                    if ((insn2 >> 12) == 0) {
                                        gen_movl_loc32_acc_cond(ctx, mode, cond);
                                        length = 4;
                                    }
                                    break;
                                }
                                case 0b1001: //0101 0110 0100 1001 0000 0000 LLLL LLLL SUBRL loc32,ACC
                                {
                                    uint32_t mode = insn2 & 0xff;
                                    if ((insn2 >> 8) == 0) {
                                        gen_subrl_loc32_acc(ctx, mode);
                                        length = 4;
                                    }
                                    break;
                                }
                                case 0b1011: //0101 0110 0100 1011 .... DMAC ACC:P,loc32,*XAR7/++
                                {
                                    uint32_t mode1 = insn2 & 0xff;
                                    uint32_t mode2 = (insn2 >> 8) & 0xff;
                                    if ((mode2 == 0b11000111) || (mode2 == 0b10000111))
                                    {
                                        length = 4;
                                        gen_dmac_accp_loc32_xar7(ctx, mode1, mode2);
                                    }
                                    break;
                                }
                                case 0b1100: //0101 0110 0100 1100 0000 0000 LLLL LLLL IMPYAL P,XT,loc32
                                {
                                    if ((insn2 >> 8) == 0)
                                    {
                                        uint32_t mode = insn2 & 0xff;
                                        gen_impyal_p_xt_loc32(ctx, mode);
                                        length = 4;
                                    }
                                    break;
                                }
                                case 0b1101: //0101 0110 0100 1101 .... IMACL P,loc32,*XAR7/++
                                {
                                    uint32_t mode1 = insn2 & 0xff;
                                    uint32_t mode2 = (insn2 >> 8) & 0xff;
                                    if ((mode2 == 0b11000111) || (mode2 == 0b10000111))
                                    {
                                        length = 4;
                                        gen_imacl_p_loc32_xar7(ctx, mode1, mode2);
                                    }
                                    break;
                                }
                            }
                            break;
                        }
                        case 0b0101: //0101 0110 0101 ....
                        {
                            switch (insn & 0xf) {
                                case 0b0010: //0101 0110 0101 0010 LSL64 ACC:P,T
                                {
                                    gen_lsl64_acc_p_t(ctx);
                                    break;
                                }
                                case 0b0011: //0101 0110 0101 0011 xxxx xxxx LLLL LLLL ADDUL ACC,loc32
                                {
                                    uint32_t mode = insn2 & 0xff;
                                    gen_addul_acc_loc32(ctx, mode);
                                    length = 4;
                                    break;
                                }
                                case 0b0100: //0101 0110 0101 0100 0000 0000 LLLL LLLL SUBBL ACC,loc32
                                {
                                    if (((insn2 & 0xff00) >> 8) == 0) {
                                        uint32_t mode = insn2 & 0xff;
                                        gen_subbl_acc_loc32(ctx, mode);
                                        length = 4;
                                    }
                                    break;
                                }
                                case 0b0101: //0101 0110 0101 0101 0000 0000 LLLL LLLL SUBUL ACC,loc32
                                {
                                    if (((insn2 & 0xff00) >> 8) == 0) {
                                        uint32_t mode = insn2 & 0xff;
                                        gen_subul_acc_loc32(ctx, mode);
                                        length = 4;
                                    }
                                    break;
                                }
                                case 0b0110: //0101 0110 0101 0110 MOV TL,#0
                                {
                                    gen_mov_tl_0(ctx);
                                    break;
                                }
                                case 0b0111: //0101 0110 0101 0111 0000 0000 LLLL LLLL ADDUL P,loc32
                                {
                                    if (((insn2 & 0xff00) >> 8) == 0) {
                                        uint32_t mode = insn2 & 0xff;
                                        gen_addul_p_loc32(ctx, mode);
                                        length = 4;
                                    }
                                    break;
                                }
                                case 0b1100: //0101 0110 0101 1100 CLRC OVC
                                {
                                    gen_clrc_ovc(ctx);
                                    break;
                                }
                                case 0b1101: //0101 0110 0101 1101 0000 0000 LLLL LLLL SUBUL P,loc32
                                {
                                    if (((insn2 & 0xff00) >> 8) == 0) {
                                        uint32_t mode = insn2 & 0xff;
                                        gen_subul_p_loc32(ctx, mode);
                                        length = 4;
                                    }
                                    break;
                                }
                                case 0b1110: //0101 0110 0101 1110 CMP64 ACC:P
                                {
                                    gen_cmp64_acc_p(ctx);
                                    break;
                                }
                                case 0b1111: //0101 0110 0101 1111 ABSTC  ACC
                                {
                                    gen_abstc_acc(ctx);
                                    break;
                                }
                            }
                            break;
                        }
                        case 0b0110: //0101 0110 0110 ....
                        {
                            switch (insn & 0xf) {
                                case 0b0010: //0101 0110 0110 0010 0000 0000 LLLL LLLL MOVU OVC,loc16
                                {
                                    if ((insn2 >> 8) == 0) {
                                        uint32_t mode = insn2 & 0xff;
                                        length = 4;
                                        gen_movu_ovc_loc16(ctx, mode);
                                    }
                                    break;
                                }
                                case 0b0101: //0101 0110 0110 0101 0000 0000 LLLL LLLL IMPYXUL P,XT,loc32
                                {
                                    if ((insn2 >> 8) == 0) {
                                        uint32_t mode = insn2 & 0xff;
                                        length = 4;
                                        gen_impyxul_p_xt_loc32(ctx, mode);
                                    }
                                    break;
                                }
                            }
                            break;
                        }
                        case 0b0111: //0101 0110 0111 ....
                        {
                            switch (insn & 0xf) {
                                case 0b0000: //0101 0110 0111 0000 FLIP AL
                                {
                                    gen_flip_ax(ctx, false);
                                    break;
                                }
                                case 0b0001: //0101 0110 0111 0001 FLIP AH
                                {
                                    gen_flip_ax(ctx, true);
                                    break;
                                }
                            }
                            break;
                        }
                        case 0b1000: //0101 0110 1000 SHFT ASR64 ACC:P,#1...16
                        {
                            uint32_t shift = (insn & 0xf) + 1;
                            gen_asr64_acc_p_imm(ctx, shift);
                            break;
                        }
                        case 0b1010: //0101 0110 1010 SHFT LSL64 ACC:P,#1...16
                        {
                            uint32_t shift = (insn & 0xf) + 1;
                            gen_lsl64_acc_p_imm(ctx, shift);
                            break;
                        }
                        case 0b1011: //0101 0110 1011 COND CCCC CCCC LLLL LLLL MOVB loc16,#8bit,COND
                        {
                            uint32_t cond = insn & 0xf;
                            uint32_t imm = insn2 >> 8;
                            uint32_t mode = insn2 & 0xff;
                            gen_movb_loc16_8bit_cond(ctx, mode, imm, cond);
                            length = 4;
                            break;
                        }
                        case 0b1100: //0101 0110 1100 COND  BF 16bitOffset,COND
                        {
                            int16_t offset = insn2;
                            uint32_t cond = insn & 0xf;
                            gen_bf_16bitOffset_cond(ctx, offset, cond);
                            length = 4;
                            break;
                        }
                    }
                    break;
                case 0b0111://0101 0111 LLLL LLLL MOVH loc16,P
                {
                    uint32_t mode = insn & 0xff;
                    gen_movh_loc16_p(ctx, mode);
                    break;
                }
                case 0b1000:
                case 0b1001:
                case 0b1010:
                case 0b1011:
                case 0b1100:
                case 0b1101://0101 1nnn LLLL LLLL MOVZ ARn,loc16
                {
                    uint32_t mode = insn & 0xff;
                    uint32_t n = (insn >> 8) & 0b111;
                    gen_movz_arn_loc16(ctx, mode, n);
                    break;
                }
                case 0b1110://0101 1110 LLLL LLLL MOV AR6,loc16
                {
                    uint32_t mode = insn & 0xff;
                    gen_mov_arn_loc16(ctx, mode, 6);
                    break;
                }
                case 0b1111://0101 1111 LLLL LLLL MOV AR7,loc16
                {
                    uint32_t mode = insn & 0xff;
                    gen_mov_arn_loc16(ctx, mode, 7);
                    break;
                }
            }
            break;
        case 0b0110: //0110 COND CCCC CCCC SB 8bitOffset,COND
        {
            int16_t offset = insn & 0xff;
            if ((offset >> 7) == 1) {
                offset = offset | 0xff00;
            }
            uint32_t cond = (insn >> 8) & 0xf;
            gen_sb_8bitOffset_cond(ctx, offset, cond);
            break;
        }
        case 0b0111:
            switch((insn & 0x0f00) >> 8) {
                case 0b0010: //0111 0010 LLLL LLLL ADD loc16,AL
                {
                    uint32_t mode = insn & 0xff;
                    gen_add_loc16_ax(ctx, mode, false);
                    break;
                }
                case 0b0011: //0111 0011 LLLL LLLL ADD loc16,AH
                {
                    uint32_t mode = insn & 0xff;
                    gen_add_loc16_ax(ctx, mode, true);
                    break;
                }
                case 0b0100://0111 0100 LLLL LLLL SUB loc16,AL
                {
                    uint32_t mode = insn & 0xff;
                    gen_sub_loc16_ax(ctx, mode , false);
                    break;
                }
                case 0b0101://0111 0101 LLLL LLLL SUB loc16,AH
                {
                    uint32_t mode = insn & 0xff;
                    gen_sub_loc16_ax(ctx, mode , true);
                    break;
                }
                case 0b0110: //0111 0110 .... ....
                    if (((insn >> 7) & 0x1) == 1) { //0111 0110 1... .... MOVL XARn,#22bit 
                        uint32_t n = ((insn >> 6) & 0b1) + 6;//XAR6,XAR7
                        uint32_t imm = ((insn & 0x3f)<< 16) | insn2;
                        gen_movl_xarn_22bit(ctx, n, imm);
                        length = 4;
                    }
                    else { // 0111 0110 0... ....
                        if (((insn >> 6) & 0x1) == 1) { //0111 0110 01.. .... LCR #22bit
                            length = 4;
                            uint32_t imm = ((insn & 0x3f)<< 16) | insn2;
                            gen_lcr_22bit(ctx, imm);
                        }
                        else { //0111 0110 00.. ....
                            switch(insn & 0x3f) {
                                case 0b000010: //0111 0110 0000 0010 IRET
                                {
                                    gen_iret(ctx);
                                    break;
                                }
                                case 0b000100: //0111 0110 0000 0100 LC *XAR7
                                {
                                    gen_lc_xar7(ctx);
                                    break;
                                }
                                case 0b010000: //0111 0110 0001 0000 LRETE
                                {
                                    gen_lrete(ctx);
                                    break;
                                }
                                case 0b010100: //0111 0110 0001 0100 LRET
                                {
                                    gen_lret(ctx);
                                    break;
                                }
                                case 0b011010: //0111 0110 0001 1010 EDIS
                                {
                                    gen_edis(ctx);
                                    break;
                                }
                                case 0b011011: //0111 0110 0001 1011 ASP
                                {
                                    gen_asp(ctx);
                                    break;
                                }
                                case 0b011111: //0111 0110 0001 1111 MOVW DP,#16bit
                                {
                                    uint32_t imm = insn2;
                                    gen_movw_dp_16bit(ctx, imm);
                                    length = 4;
                                    break;
                                }
                                case 0b100000: //0111 0110 0010 0000 LB *XAR7
                                {
                                    gen_lb_xar7(ctx);
                                    break;
                                }
                                case 0b100001: //0111 0110 0010 0001 IDLE
                                {
                                    gen_idle(ctx);
                                    break;
                                }
                                case 0b100010: //0111 0110 0010 0010 EALLOW
                                {
                                    gen_eallow(ctx);
                                    break;
                                }
                                case 0b100100: //0111 0110 0010 0100 ESTOP1
                                {
                                    gen_estop1(ctx);
                                    break;
                                }
                                case 0b100101: //0111 0110 0010 0101 ESTOP0
                                {
                                    gen_estop0(ctx);
                                    break;
                                }
                                case 0b100110: //0111 0110 0010 0110 CCCC CCCC CCCC CCCC AND IER,#16bit
                                {
                                    uint32_t imm = insn2;
                                    length = 4;
                                    gen_and_ier_16bit(ctx, imm);
                                    break;
                                }                                
                                case 0b101111: //0111 0110 0010 1111 CCCC CCCC CCCC CCCC AND IFR,#16bit
                                {
                                    uint32_t imm = insn2;
                                    length = 4;
                                    gen_and_ifr_16bit(ctx, imm);
                                    break;
                                }
                                case 0b111111://0111 0110 0011 1111 CCCC CCCC CCCC CCCC IACK #16bit
                                {
                                    length = 4;
                                    uint32_t imm = insn2;
                                    gen_iack_16bit(ctx, imm);
                                    break;
                                }
                            }
                        }
                    }
                    break;
                case 0b1000: 
                case 0b1001: 
                case 0b1010: 
                case 0b1011: 
                case 0b1100: 
                case 0b1101: 
                case 0b1110: 
                case 0b1111: //0111 1nnn LLLL LLLL MOV loc16,ARn
                {
                    uint32_t mode = insn & 0xff;
                    uint32_t n = (insn >> 8) & 0b111;
                    gen_mov_loc16_arn(ctx, mode, n);
                }
            }
            break;
        case 0b1000:
            switch ((insn & 0x0f00) >> 8) {
                case 0b0000: //1000 0000 LLLL LLLL MOVZ AR7,loc16
                {
                    uint32_t mode = insn & 0xff;
                    gen_movz_arn_loc16(ctx, mode, 7);
                    break;
                }
                case 0b0001: //1000 0001 LLLL LLLL ADD ACC,loc16<<#0
                {
                    uint32_t mode = insn & 0xff;
                    gen_add_acc_loc16_shift(ctx, mode, 0);
                    break;
                }
                case 0b0010: //1000 0010 LLLL LLLL MOVL XAR3,loc32
                {
                    uint32_t mode = insn & 0xff;
                    gen_movl_xarn_loc32(ctx, mode, 3);
                    break;
                }
                case 0b0011: //1000 1110 LLLL LLLL MOVL XAR5,loc32
                {
                    uint32_t mode = insn & 0xff;
                    gen_movl_xarn_loc32(ctx, mode, 5);
                    break;
                }
                case 0b0101: /*1000 0101 .... .... MOV ACC, loc16<<#0 */
                {
                    uint32_t mode = insn & 0xff;
                    gen_mov_acc_loc16_shift(ctx, mode, 0);
                    break;
                }
                case 0b0110: //1000 0110 LLLL LLLL MOVL XAR2,loc32
                {
                    uint32_t mode = insn & 0xff;
                    gen_movl_xarn_loc32(ctx, mode, 2);
                    break;
                }
                case 0b0111: //1000 0111 LLLL LLLL MOVL XT,loc32
                {
                    uint32_t mode = insn & 0xff;
                    gen_movl_xt_loc32(ctx, mode);
                    break;
                }
                case 0b1000: //1000 1000 LLLL LLLL MOVZ AR6,loc16
                {
                    uint32_t mode = insn & 0xff;
                    gen_movz_arn_loc16(ctx, mode, 6);
                    break;
                }
                case 0b1001: //1000 1001 LLLL LLLL AND ACC,loc16
                {
                    uint32_t mode = insn & 0xff;
                    gen_and_acc_loc16(ctx, mode);
                    break;
                }
                case 0b1011: //1000 1011 LLLL LLLL MOVL XAR1,loc32
                {
                    uint32_t mode = insn & 0xff;
                    gen_movl_xarn_loc32(ctx, mode, 1);
                    break;
                }
                case 0b1010: //1000 1010 LLLL LLLL MOVL XAR4,loc32
                {
                    uint32_t mode = insn & 0xff;
                    gen_movl_xarn_loc32(ctx, mode, 4);
                    break;
                }
                case 0b1101: /*1000 1101 .... .... MOVL XARn,#22bit */
                {
                    uint32_t n = (insn >> 6) & 0b11;
                    uint32_t imm = ((insn & 0x3f)<< 16) | insn2;
                    gen_movl_xarn_22bit(ctx, n, imm);
                    length = 4;
                    break;
                }
                case 0b1110: //1000 1110 LLLL LLLL MOVL XAR0,loc32
                {
                    uint32_t mode = insn & 0xff;
                    gen_movl_xarn_loc32(ctx, mode, 0);
                    break;
                }
                case 0b1111:
                    switch((insn >> 6) & 0b11) {
                        case 0b00: //1000 1111 00cc cccc cccc cccc cccc cccc MOVL XAR4,#22bit
                        {
                            uint32_t imm = ((insn & 0x3f)<< 16) | insn2;
                            gen_movl_xarn_22bit(ctx, 4, imm);
                            length = 4;
                            break;
                        }
                        case 0b01: //1000 1111 01cc cccc cccc cccc cccc cccc MOVL XAR5,#22bit
                        {
                            uint32_t imm = ((insn & 0x3f)<< 16) | insn2;
                            gen_movl_xarn_22bit(ctx, 5, imm);
                            length = 4;
                            break;
                        }
                        case 0b10: //1000 1111 10nn nmmm CCCC CCCC CCCC CCCC BAR 16bitOffset,ARn,ARm,EQ
                        {
                            uint32_t n = (insn >> 3) & 0b111;
                            uint32_t m = insn & 0b111;
                            int16_t imm = insn2;
                            gen_bar_16bitOffset_arn_arm_eq_neq(ctx, imm, n, m, TCG_COND_EQ);
                            length = 4;
                            break;
                        }
                        case 0b11: //1000 1111 11nn nmmm CCCC CCCC CCCC CCCC BAR 16bitOffset,ARn,ARm,NEQ
                        {
                            uint32_t n = (insn >> 3) & 0b111;
                            uint32_t m = insn & 0b111;
                            int16_t imm = insn2;
                            gen_bar_16bitOffset_arn_arm_eq_neq(ctx, imm, n, m, TCG_COND_NE);
                            length = 4;
                            break;
                        }
                    }
                    break;
            }
            break;
        case 0b1001:
            switch ((insn >> 9) & 0b111) {
                case 0b000: //1001 000a CCCC CCCC ANDB AX,#8bit
                {
                    uint32_t imm = insn & 0xff;
                    uint32_t is_AH = ((insn >> 8) & 1);
                    gen_andb_ax_8bit(ctx, imm, is_AH);
                    break;
                }
                case 0b001: //1001 001. .... ....  MOV AX,loc16
                {
                    uint32_t mode = insn & 0xff;
                    uint32_t is_AH = ((insn >> 8) & 1);
                    gen_mov_ax_loc16(ctx, mode, is_AH);
                    break;
                }
                case 0b010: //1001 010. .... .... ADD AX,loc16
                {
                    uint32_t mode = insn & 0xff;
                    uint32_t is_AH = ((insn >> 8) & 1);
                    gen_add_ax_loc16(ctx, mode, is_AH);
                    break;
                }
                case 0b011: //1001 011. .... .... MOV loc16,AX
                {
                    uint32_t mode = insn & 0xff;
                    uint32_t is_AH = ((insn >> 8) & 1);
                    gen_mov_loc16_ax(ctx, mode, is_AH);
                    break;
                }
                case 0b101: //1001 101A CCCC CCCC MOVB AX,#8bit
                {
                    uint32_t imm = insn & 0xff;
                    uint32_t is_AH = ((insn >> 8) & 1);
                    gen_movb_ax_8bit(ctx, imm, is_AH);
                    break;
                }
                case 0b110: //1001 110A CCCC CCCC ADDB AX,#8bitSigned
                {
                    uint32_t imm = insn & 0xff;
                    uint32_t is_AH = ((insn >> 8) & 1);
                    gen_addb_ax_8bit(ctx, imm, is_AH);
                    break;
                }
                case 0b111: //1001 111A LLLL LLLL SUB AX,loc16
                {
                    uint32_t mode = insn & 0xff;
                    uint32_t is_AH = ((insn >> 8) & 1);
                    gen_sub_ax_loc16(ctx, mode, is_AH);
                    break;
                }
            }
            break;
        case 0b1010:
            switch ((insn & 0xf00) >> 8) {
                case 0b0000: //1010 0000 LLLL LLLL MOVL loc32, XAR5
                {
                    uint32_t mode = insn & 0xff;
                    gen_movl_loc32_xarn(ctx, mode, 5);
                    break;
                }
                case 0b0010: //1010 0010 LLLL LLLL MOVL loc32, XAR3
                {
                    uint32_t mode = insn & 0xff;
                    gen_movl_loc32_xarn(ctx, mode, 3);
                    break;
                }
                case 0b0011: //1010 0011 LLLL LLLL MOVL P,loc32
                {
                    uint32_t mode = insn & 0xff;
                    gen_movl_p_loc32(ctx, mode);
                    break;
                }
                case 0b0101: //1010 0101 LLLL LLLL DMOV loc16
                {
                    uint32_t mode = insn & 0xff;
                    gen_dmov_loc16(ctx, mode);
                    break;
                }
                case 0b0110: //1010 0110 LLLL LLLL MOVDL XT,loc32
                {
                    uint32_t mode = insn & 0xff;
                    gen_movdl_xt_loc32(ctx, mode);
                    break;
                }
                case 0b0111: //1010 0111 LLLL LLLL MOVAD T,loc16
                {
                    uint32_t mode = insn & 0xff;
                    gen_movad_t_loc16(ctx, mode);
                    break;
                }
                case 0b1000: //1010 1000 LLLL LLLL MOVL loc32, XAR4
                {
                    uint32_t mode = insn & 0xff;
                    gen_movl_loc32_xarn(ctx, mode, 4);
                    break;
                }
                case 0b1001: //1010 1001 LLLL LLLL MOVL loc32,P
                {
                    uint32_t mode = insn & 0xff;
                    gen_movl_loc32_p(ctx, mode);
                    break;
                }
                case 0b1010: //1010 1010 LLLL LLLL MOVL loc32, XAR2
                {
                    uint32_t mode = insn & 0xff;
                    gen_movl_loc32_xarn(ctx, mode, 2);
                    break;
                }
                case 0b1011: //1010 1011 LLLL LLLL MOVL loc32,XT
                {
                    uint32_t mode = insn & 0xff;
                    gen_movl_loc32_xt(ctx, mode);
                    break;
                }
                case 0b1110: //1010 1110 LLLL LLLL SUB ACC,loc16<<#0
                {
                    uint32_t mode = insn & 0xff;
                    gen_sub_acc_loc16_shift(ctx, mode, 0);
                    break;
                }
            }
            break;
        case 0b1011:
            switch ((insn & 0xf00) >> 8) {
                case 0b0001: //1011 0001 LLLL LLLL MOV loc16,ACC<<1
                {
                    uint32_t mode = insn & 0xff;
                    gen_mov_loc16_acc_shift(ctx, mode, 1, 1);
                    break;
                }
                case 0b0010: //1011 0010 LLLL LLLL MOVL loc32, XAR1
                {
                    uint32_t mode = insn & 0xff;
                    gen_movl_loc32_xarn(ctx, mode, 1);
                    break;
                }
                case 0b0011: //1011 0011 LLLL LLLL MOVH loc16,ACC<<1
                {
                    uint32_t mode = insn & 0xff;
                    gen_movh_loc16_acc_shift(ctx, mode, 1, 1);
                    break;
                }
                case 0b0100: //1011 0100 LLLL LLLL CCCC CCCC CCCC CCCC IN loc16,*(PA)
                {
                    length = 4;
                    uint32_t mode = insn & 0xff;
                    uint32_t pa = insn2;
                    gen_in_loc16_pa(ctx, mode, pa);
                    break;
                }
                case 0b0110: //1011 0110 CCCC CCCC MOVB XAR7,#8bit
                {
                    uint32_t imm = insn & 0xff;
                    gen_movb_xarn_8bit(ctx, imm, 7);
                    break;
                }
                case 0b1000:
                case 0b1001:
                case 0b1010:
                case 0b1011://1011 10CC CCCC CCCC MOVZ DP,#10bit
                {
                    uint32_t imm = insn & 0x3ff;
                    gen_movz_dp_10bit(ctx, imm);
                    break;
                }
                case 0b1110: //1011 1110 CCCC CCCC MOVB XAR6,#8bit
                {
                    uint32_t imm = insn & 0xff;
                    gen_movb_xarn_8bit(ctx, imm, 6);
                    break;
                }
            }
            break;
        case 0b1100:
            switch ((insn & 0xf00) >> 8) {
                case 0b0000:
                case 0b0001: //1100 000A LLLL LLLL AND loc16,AX
                {
                    uint32_t mode = insn & 0xff;
                    uint32_t is_AH = (insn >> 8) & 1;
                    gen_and_loc16_ax(ctx, mode, is_AH);
                    break;
                }
                case 0b0010: //1100 0010 LLLL LLLL MOVL loc32, XAR6
                {
                    uint32_t mode = insn & 0xff;
                    gen_movl_loc32_xarn(ctx, mode, 6);
                    break;
                }
                case 0b0011: //1100 0011 LLLL LLLL MOVL loc32, XAR7
                {
                    uint32_t mode = insn & 0xff;
                    gen_movl_loc32_xarn(ctx, mode, 7);
                    break;
                }
                case 0b0100: //1100 0100 LLLL LLLL MOVL XAR6,loc32
                {
                    uint32_t mode = insn & 0xff;
                    gen_movl_xarn_loc32(ctx, mode, 6);
                    break;
                }
                case 0b0101: //1100 0101 LLLL LLLL MOVL XAR7,loc32
                {
                    uint32_t mode = insn & 0xff;
                    gen_movl_xarn_loc32(ctx, mode, 7);
                    break;
                }
                case 0b0110: //1100 0110 LLLL LLLL MOVB AL.LSB,loc16
                {
                    uint32_t mode = insn & 0xff;
                    gen_movb_al_lsb_loc16(ctx, mode);
                    break;
                }
                case 0b0111: //1100 0111 LLLL LLLL MOVB AH.LSB,loc16
                {
                    uint32_t mode = insn & 0xff;
                    gen_movb_ah_lsb_loc16(ctx, mode);
                    break;
                }
                case 0b1000:
                case 0b1001: //1100 100a LLLL LLLL MOVB loc16,AX.MSB
                {
                    uint32_t mode = insn & 0xff;
                    uint32_t is_AH = (insn >> 8) & 1;
                    gen_movb_loc16_ax_msb(ctx, mode, is_AH);
                    break;
                }
                case 0b1100:
                case 0b1101: //1100 110a LLLL LLLL CCCC CCCC CCCC CCCC AND AX,loc16,#16bit
                {
                    length = 4;
                    uint32_t imm = insn2;
                    uint32_t mode = insn & 0xff;
                    uint32_t is_AH = (insn >> 8) & 1;
                    gen_and_ax_loc16_16bit(ctx, mode, imm, is_AH);
                    break;
                }
                case 0b1110:
                case 0b1111: //1100 111a LLLL LLLL AND AX,loc16
                {
                    uint32_t mode = insn & 0xff;
                    uint32_t is_AH = (insn >> 8) & 1;
                    gen_and_ax_loc16(ctx, mode, is_AH);
                    break;
                }
            }
            break;
        case 0b1101:
            if (((insn >> 11) & 1) == 0) {//1101 0... .... ....
                uint32_t imm = insn & 0xff;
                uint32_t n = (insn >> 8) & 0b111;
                if (n < 6) {//1101 0nnn CCCC CCCC MOVB XAR1...5 ,#8bit
                    gen_movb_xarn_8bit(ctx, imm, n);
                }
                else {//1101 0110/1 CCCC CCCC MOVB AR6/7, #8bit
                    gen_movb_arn_8bit(ctx, imm, n);
                }
            }
            else {//1101 1... .... ....
                uint32_t imm = insn & 0x7f;
                uint32_t n = (insn >> 8) & 0b111;
                if (((insn & 0xff) >> 7) == 0) { //ADDB XARn,#7bit
                    gen_addb_xarn_7bit(ctx, n, imm);
                }
                else { //SUBB XARn,#7bit
                    gen_subb_xarn_7bit(ctx, n, imm);
                }
            }
            break;
        case 0b1110:
        {
            switch((insn & 0x0f00) >> 8) {
                case 0b1010://1110 1010 LLLL LLLL SUBR loc16,AL
                {
                    uint32_t mode = insn & 0xff;
                    gen_subr_loc16_ax(ctx, mode, false);
                    break;
                }
                case 0b1011://1110 1011 LLLL LLLL SUBR loc16,AH
                {
                    uint32_t mode = insn & 0xff;
                    gen_subr_loc16_ax(ctx, mode, true);
                    break;
                }
            }
            break;
        }
        case 0b1111:
            switch ((insn & 0x0f00) >> 8) {
                case 0b0100: //1111 0100 LLLL LLLL 32bit MOV *(0:16bit),loc16
                {
                    uint32_t imm = insn2;
                    uint32_t mode = insn & 0xff;
                    gen_16bit_loc16(ctx, mode, imm);
                    length = 4;
                    break;
                }
                case 0b0101: //1111 0101 LLLL LLLL 32bit MOV loc16,*(0:16bit)
                {
                    uint32_t imm = insn2;
                    uint32_t mode = insn & 0xff;
                    gen_loc16_16bit(ctx, mode, imm);
                    length = 4;
                    break;
                }
                case 0b0110: //1111 0110 CCCC CCCC RPT #8bit
                {
                    uint32_t value = insn & 0xff;
                    gen_rpt_8bit(ctx, value);
                    set_repeat_counter = true;
                    break;
                }
                case 0b0111: //1111 0111 CCCC CCCC RPT loc16
                {
                    uint32_t mode = insn & 0xff;
                    gen_rpt_loc16(ctx, mode);
                    set_repeat_counter = true;
                    break;
                }
                case 0b1000: //1111 10CC CCCC CCCC MOV DP,#10bit
                case 0b1001: //1111 10CC CCCC CCCC MOV DP,#10bit
                case 0b1010: //1111 10CC CCCC CCCC MOV DP,#10bit
                case 0b1011: //1111 10CC CCCC CCCC MOV DP,#10bit
                {
                    uint32_t imm = insn & 0x3ff;// 10bit
                    gen_mov_dp_10bit(ctx, imm);
                    break;
                }
                case 0b1100: //1111 1100 IIII IIII ADRK #8bit
                {
                    uint32_t imm = insn & 0xff;
                    gen_adrk_8bit(ctx, imm);
                    break;
                }
                case 0b1110: //1111 1110 .... ....
                {
                    uint32_t imm = insn & 0x7f;
                    if (((insn & 0xff) >> 7) == 0) { //ADDB SP,#7bit
                        gen_addb_sp_7bit(ctx, imm);
                    }
                    else { //SUBB SP,#7bit
                        gen_subb_sp_7bit(ctx, imm);
                    }
                    break;
                }
                case 0b1111://1111 1111 .... ....
                    switch ((insn & 0x00f0) >> 4) {
                        case 0b0000: //1111 1111 0000 SHFT CCCC CCCC CCCC CCCC SUB ACC,#16bit<<#0...15
                        {
                            uint32_t imm = insn2;
                            uint32_t shift = insn & 0x000f;
                            gen_sub_acc_16bit_shift(ctx, imm, shift);
                            length = 4;
                            break;
                        }
                        case 0b0001: //1111 1111 0001 SHFT 32bit ADD ACC, #16bit<#0...15
                        {
                            uint32_t imm = insn2;
                            uint32_t shift = insn & 0x000f;
                            gen_add_acc_16bit_shift(ctx, imm, shift);
                            length = 4;
                            break;
                        }
                        case 0b0010: //1111 1111 0010 SHFT 32bit MOV ACC, #16bit<#0...15
                        {
                            uint32_t imm = insn2;
                            uint32_t shift = insn & 0x000f;
                            gen_mov_acc_16bit_shift(ctx, imm, shift);
                            length = 4;
                            break;
                        }
                        case 0b0011: //1111 1111 0011 SHFT LSL ACC,#1...16
                        {
                            uint32_t shift = (insn & 0xf) + 1;
                            gen_lsl_acc_imm(ctx, shift);
                            break;
                        }
                        case 0b0101: //1111 1111 0101 ....
                            switch (insn & 0xf) {
                                case 0b0000: //1111 1111 0101 0000 LSL ACC,T
                                {
                                    gen_lsl_acc_t(ctx);
                                    break;
                                }
                                case 0b0110: //1111 1111 0101 0110 ABS ACC
                                {
                                    gen_abs_acc(ctx);
                                    break;
                                }
                                case 0b1001: //1111 1111 0101 1001 CMPL ACC,P<<PM
                                {
                                    gen_cmpl_acc_p_pm(ctx);
                                    break;
                                }
                                case 0b1010: //1111 1111 0101 1010 MOVL P,ACC
                                {
                                    gen_movl_p_acc(ctx);
                                    break;
                                }
                            }
                            break;
                        case 0b0110: //1111 1111 0110 ....
                        {
                            if (((insn & 0xf) >> 3) == 1) { //1111 1111 0110 1... SPM shift
                                uint32_t pm = insn & 0b111;
                                gen_spm_shift(ctx, pm);
                            }
                            else {//1111 1111 0110 0...
                                switch(insn & 0b111) {
                                    case 0b100:
                                    case 0b101: //1111 1111 0110 010a ASR AX,T
                                    {
                                        uint32_t is_AH = insn & 1;
                                        gen_asr_ax_t(ctx, is_AH);
                                        break;
                                    }
                                    case 0b110:
                                    case 0b111: //1111 1111 0110 011a LSL AX,T
                                    {
                                        uint32_t is_AH = insn & 1;
                                        gen_lsl_ax_t(ctx, is_AH);
                                        break;
                                    }
                                }
                            }
                            break;
                        }
                        case 0b1000:
                        case 0b1001://1111 1111 100A SHFT LSL AX,#1...16
                        {
                            uint32_t shift = (insn & 0xf) + 1;
                            uint32_t is_AH = (insn >> 4) & 1;
                            gen_lsl_ax_imm(ctx, shift, is_AH);
                            break;
                        }
                        case 0b1010:
                        case 0b1011://1111 1111 101A SHFT ASR AX,#1...16
                        {
                            uint32_t shift = (insn & 0xf) + 1;
                            uint32_t is_AH = (insn >> 4) & 1;
                            gen_asr_ax_imm(ctx, shift, is_AH);
                            break;
                        }
                        case 0b1110://1111 1111 1110 COND CCCC CCCC CCCC CCCC B 16bitOffset,COND
                        {
                            int16_t imm = insn2;
                            int16_t cond = (insn&0xf);
                            length = 4;
                            gen_b_16bitOffset_cond(ctx, imm, cond);
                            break;
                        }
                    }
                    break;
            }
            break;
    }

    gen_reset_rptc(ctx);
    tcg_gen_movi_i32(cpu_insn_length, length/2);//record current insn length
    if (!set_repeat_counter) {
        ctx->rpt_set = false;
    }
    else {
        ctx->rpt_set = true;
    }
    // 16bit -> 2; 32bit -> 4
    return length;
}

// Initialize the target-specific portions of DisasContext struct.
// The generic DisasContextBase has already been initialized.
static void tms320c28x_tr_init_disas_context(DisasContextBase *dcb, CPUState *cs)
{
    DisasContext *dc = container_of(dcb, DisasContext, base);
    // CPUTms320c28xState *env = cs->env_ptr;
    // dc->isRPT = false;
    // for (int i = 0; i < 8; i++) {
    //     dc->temp[i] = tcg_const_local_i32(0);
    // }
    dc->rpt_set = false;
    // dc->rpt_counter = 0;
}

// Emit any code required before the start of the main loop,
// after the generic gen_tb_start().
static void tms320c28x_tr_tb_start(DisasContextBase *db, CPUState *cs)
{
    // DisasContext *dc = container_of(db, DisasContext, base);
}

// Emit the tcg_gen_insn_start opcode.
static void tms320c28x_tr_insn_start(DisasContextBase *dcbase, CPUState *cs)
{
    DisasContext *dc = container_of(dcbase, DisasContext, base);
    tcg_gen_insn_start(dc->base.pc_next, 0);
}


// When called, the breakpoint has already been checked to match the PC,
// but the target may decide the breakpoint missed the address
// (e.g., due to conditions encoded in their flags).  Return true to
// indicate that the breakpoint did hit, in which case no more breakpoints
// are checked.  If the breakpoint did hit, emit any code required to
// signal the exception, and set db->is_jmp as necessary to terminate
// the main loop.
static bool tms320c28x_tr_breakpoint_check(DisasContextBase *dcbase, CPUState *cs, const CPUBreakpoint *bp)
{
    DisasContext *dc = container_of(dcbase, DisasContext, base);

    tcg_gen_movi_tl(cpu_pc, dc->base.pc_next);
    // gen_exception(dc, EXCP_DEBUG);
    dc->base.is_jmp = DISAS_JUMP;
    /* The address covered by the breakpoint must be included in
       [tb->pc, tb->pc + tb->size) in order to for it to be
       properly cleared -- thus we increment the PC here so that
       the logic setting tb->size below does the right thing.  */
    // dc->base.pc_next += 2; // ?? 16bit op ==> 2
    return true;
}

static void tms320c28x_tr_translate_insn(DisasContextBase *dcbase, CPUState *cs)
{
    DisasContext *dc = container_of(dcbase, DisasContext, base);
    Tms320c28xCPU *cpu = TMS320C28X_CPU(cs);
    // can not load 32bit here, "translator_ldl_swap" must load from 32bit aligned
    uint32_t insn = translator_lduw_swap(&cpu->env, dc->base.pc_next, true);
    uint32_t insn2 = translator_lduw_swap(&cpu->env, dc->base.pc_next+2, true);
    int inc = decode(cpu, dc, insn, insn2);
    if (inc <= 0) {
        // gen_illegal_exception(dc);
    }
    else
        dc->base.pc_next += inc;
}

static void tms320c28x_tr_tb_stop(DisasContextBase *dcbase, CPUState *cs)
{
    DisasContext *dc = container_of(dcbase, DisasContext, base);
    // target_ulong jmp_dest;

    /* If we have already exited the TB, nothing following has effect.  */
    if (dc->base.is_jmp == DISAS_NORETURN) {
        return;
    }

    if (dc->base.singlestep_enabled) {
        gen_exception(dc, EXCP_DEBUG);
        return;
    }

    // jmp_dest = dc->base.pc_next >> 1;

    switch (dc->base.is_jmp) {
    case DISAS_REPEAT:
    case DISAS_JUMP:
        tcg_gen_lookup_and_goto_ptr();
        break;

    // case DISAS_TOO_MANY:
    //     if (unlikely(dc->base.singlestep_enabled)) {
    //         tcg_gen_movi_tl(cpu_pc, jmp_dest);
    //         // gen_exception(dc, EXCP_DEBUG);
    //     } else if ((dc->base.pc_first ^ jmp_dest) & TARGET_PAGE_MASK) {
    //         tcg_gen_movi_tl(cpu_pc, jmp_dest);
    //         tcg_gen_lookup_and_goto_ptr();
    //     } else {
    //         tcg_gen_goto_tb(0);
    //         tcg_gen_movi_tl(cpu_pc, jmp_dest);
    //         tcg_gen_exit_tb(dc->base.tb, 0);
    //     }
    //     break;

    case DISAS_EXIT:
        tcg_gen_exit_tb(NULL, 0);
        break;
    default:
        g_assert_not_reached();
    }
}

static void tms320c28x_tr_disas_log(const DisasContextBase *dcbase, CPUState *cs)
{
    DisasContext *s = container_of(dcbase, DisasContext, base);

    qemu_log("IN: %s\n", lookup_symbol(s->base.pc_first));
    log_target_disas(cs, s->base.pc_first, s->base.tb->size);
}

static const TranslatorOps tms320c28x_tr_ops = {
    .init_disas_context = tms320c28x_tr_init_disas_context,
    .tb_start           = tms320c28x_tr_tb_start,
    .insn_start         = tms320c28x_tr_insn_start,
    .breakpoint_check   = tms320c28x_tr_breakpoint_check,
    .translate_insn     = tms320c28x_tr_translate_insn,
    .tb_stop            = tms320c28x_tr_tb_stop,
    .disas_log          = tms320c28x_tr_disas_log,
};

void gen_intermediate_code(CPUState *cs, TranslationBlock *tb, int max_insns)
{
    DisasContext ctx;

    translator_loop(&tms320c28x_tr_ops, &ctx.base, cs, tb, max_insns);
}

void tms320c28x_cpu_dump_state(CPUState *cs, FILE *f, int flags)
{
    Tms320c28xCPU *cpu = TMS320C28X_CPU(cs);
    CPUTms320c28xState *env = &cpu->env;
    int i;

    qemu_fprintf(f, "PC =%08x RPC=%08x\n", env->pc, env->rpc);
    qemu_fprintf(f, "ACC=%08x AH =%04x AL=%04x\n", env->acc, env->acc >> 16, env->acc & 0xffff);
    qemu_fprintf(f, "P  =%08x PH =%04x PL =%04x\n", env->p, env->p >> 16, env->p & 0xffff);
    qemu_fprintf(f, "XT =%08x T  =%04x TL =%04x\n", env->xt, env->xt >> 16, env->xt & 0xffff);
    for (i = 0; i < 8; ++i) {
        qemu_fprintf(f, "XAR%01d=%08x AR%01dH=%04x AR%01d=%04x\n", i, env->xar[i], i, env->xar[i] >> 16, i, env->xar[i] & 0xffff);
    }
    qemu_fprintf(f, "ST0=%04x\n", env->st0);
    qemu_fprintf(f, "OVC=%x PM=%x V=%x N=%x Z=%x\n", CPU_GET_STATUS(st0, OVC), CPU_GET_STATUS(st0, PM), CPU_GET_STATUS(st0, V), CPU_GET_STATUS(st0, N), CPU_GET_STATUS(st0, Z));
    qemu_fprintf(f, "C=%x TC=%x OVM=%x SXM=%x\n", CPU_GET_STATUS(st0, C), CPU_GET_STATUS(st0, TC), CPU_GET_STATUS(st0, OVM), CPU_GET_STATUS(st0, SXM));
    qemu_fprintf(f, "ST1=%04x\n", env->st1);
    qemu_fprintf(f, "ARP=%x XF=%x MOM1MAP=%x OBJMODE=%x\n", CPU_GET_STATUS(st1, ARP), CPU_GET_STATUS(st1, XF), CPU_GET_STATUS(st1, M0M1MAP), CPU_GET_STATUS(st1, OBJMODE));
    qemu_fprintf(f, "AMODE=%x IDLESTAT=%x EALLOW=%x LOOP=%x\n", CPU_GET_STATUS(st1, AMODE), CPU_GET_STATUS(st1, IDLESTAT), CPU_GET_STATUS(st1, EALLOW), CPU_GET_STATUS(st1, LOOP));
    qemu_fprintf(f, "SPA=%x VMAP=%x PAGE0=%x DBGM=%x INTM=%x\n", CPU_GET_STATUS(st1, SPA), CPU_GET_STATUS(st1, VMAP), CPU_GET_STATUS(st1, PAGE0), CPU_GET_STATUS(st1, DBGM), CPU_GET_STATUS(st1, INTM));
    qemu_fprintf(f, "DP =%04x SP =%04x IFR=%04x IER=%04x\n", env->dp, env->sp, env->ifr, env->ier);
    qemu_fprintf(f, "DBGIER=%04x\n", env->dbgier);
    qemu_fprintf(f, "RPTC=%x\n",env->rptc);
}

void restore_state_to_opc(CPUTms320c28xState *env, TranslationBlock *tb,
                          target_ulong *data)
{
    env->pc = data[0];
    // env->dflag = data[1] & 1;
    // if (data[1] & 2) {
    //     env->ppc = env->pc - 4;
    // }
}
