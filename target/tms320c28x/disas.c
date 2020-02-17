/*
 * TMS320C28X disassembler
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "qemu/osdep.h"
#include "disas/dis-asm.h"
#include "qemu/bitops.h"
#include "cpu.h"

typedef disassemble_info DisasContext;

static void get_loc_string(char* str, uint32_t loc, uint32_t loc_type) {
    switch((loc & 0b11000000)>>6) {
        case 0b00: //b00 III III @6bit
            sprintf(str, "@0x%x", loc & 0x3f);
            break;
        case 0b01: //b01 III III *-SP[6bit]
            sprintf(str, "*SP[0x%x]", loc & 0x3f);
            break;
        case 0b10: //b10 ... ...
            switch((loc & 0b00111000)>>3) {
                case 0b000: //b10 000 AAA *XARn++
                    sprintf(str, "*XAR%d++", loc & 0b00000111);
                    break;
                case 0b001: //b10 001 AAA *--XARn
                    sprintf(str, "*--XAR%d", loc & 0b00000111);
                    break;
                case 0b010: //b10 010 AAA *+XARn[AR0]
                    sprintf(str, "*+XAR%d[AR0]", loc & 0b00000111);
                    break;
                case 0b011: //b10 011 AAA *+XARn[AR1]
                    sprintf(str, "*+XAR%d[AR1]", loc & 0b00000111);
                    break;
                case 0b100: //@XARn @ARn
                    sprintf(str, "@XAR%d", loc & 0b00000111);
                    break;
                case 0b101: //b10 101 ...
                    switch(loc & 0b00000111) {
                        case 0b000: //@AH
                            sprintf(str, "@AH");
                            break;
                        case 0b001: //@ACC @AL
                            if (loc_type == LOC16) {
                                sprintf(str, "@AL");
                            }
                            else {
                                sprintf(str, "@ACC");
                            }
                            break;
                        case 0b010:
                            sprintf(str, "@PH");
                            break;
                        case 0b011:
                            if (loc_type == LOC16) {
                                sprintf(str, "@PL");
                            }
                            else {
                                sprintf(str, "@P");
                            }
                            break;
                        case 0b100:
                            if (loc_type == LOC16) {
                                sprintf(str, "@TH");
                            }
                            else {
                                sprintf(str, "@XT");
                            }
                            break;
                        case 0b101: //@SP
                            sprintf(str, "@SP");
                            break;
                        case 0b110: //b10 101 110 *BR0++
                            sprintf(str, "*BR0++");
                            break;
                        case 0b111: //b10 101 111 *BR0--
                            sprintf(str, "*BR0--");
                            break;
                    }
                    break;
                case 0b110: //b10 110 RRR *,ARPn
                    sprintf(str, "*,ARP%d",loc & 0b00000111);
                    break;
                case 0b111: //b10 111 ...
                    switch(loc & 0b00000111) {
                        case 0b000: //b10 111 000 *
                            sprintf(str, "*");
                            break;
                        case 0b001: //b10 111 001 *++
                            sprintf(str, "*++");
                            break;
                        case 0b010: //b10 111 010 *--
                            sprintf(str, "*--");
                            break;
                        case 0b011: //b10 111 011 *0++
                            sprintf(str, "*0++");
                            break;
                        case 0b100: //b10 111 100 *0--
                            sprintf(str, "*0--");
                            break;
                        case 0b101: //b10 111 101 *SP++
                            sprintf(str, "*SP++");
                            break;
                        case 0b110: //b10 111 110 *--SP
                            sprintf(str, "*--SP");
                            break;
                        case 0b111: //b10 111 111 *AR6%++
                            sprintf(str, "*AR6%%++");
                            break;
                    }
                    break;
            }
            break;
        case 0b11: //b11 III AAA
            sprintf(str, "*+XAR%d[0x%x]", loc & 0b00000111, (loc >> 3) & 0b00000111);
            break;
    }
}

static void get_cond_string(char *str, uint32_t cond) {
    cond = cond & 0xf;
    switch(cond) {
        case 0: //NEQ
            sprintf(str, "NEQ");
            break;
        case 1: //EQ
            sprintf(str, "EQ");
            break;
        case 2: //GT
            sprintf(str, "GT");
            break;
        case 3: //GEQ
            sprintf(str, "GEQ");
            break;
        case 4: //LT
            sprintf(str, "LT");
            break;
        case 5: //LEQ
            sprintf(str, "LEQ");
            break;
        case 6: //HI
            sprintf(str, "HI");
            break;
        case 7: //HIS,C
            sprintf(str, "HIS(C)");
            break;
        case 8: //LO,NC
            sprintf(str, "LO(NC)");
            break;
        case 9: //LOS
            sprintf(str, "LOS");
            break;
        case 10: //NOV
            sprintf(str, "NOV");
            break;
        case 11: //OV
            sprintf(str, "OV");
            break;
        case 12: //NTC
            sprintf(str, "NTC");
            break;
        case 13: //TC
            sprintf(str, "TC");
            break;
        case 14: //NBIO
            sprintf(str, "NBIO");
            break;
        case 15: //UNC
            sprintf(str, "UNC");
            break;
        default://can not reach
            break;
    }
}

int print_insn_tms320c28x(bfd_vma addr, disassemble_info *info)
{
    fprintf_function fprintf_func = info->fprintf_func;
    void *stream = info->stream;
    bfd_byte buffer[4];
    uint32_t insn,insn32;
    int status;
    int length = 2;
    status = info->read_memory_func(addr, buffer, 4, info);
    if (status != 0) {
        info->memory_error_func(status, addr, info);
        return -1;
    }
    char str[20];

    unsigned long v;
    v = (unsigned long) buffer[2];
    v |= (unsigned long) buffer[3] << 8;
    v |= (unsigned long) buffer[0] << 16;
    v |= (unsigned long) buffer[1] << 24;
    insn32 = v;
    insn = insn32 >> 16;

    // fprintf_func (stream,  ".long, 0x%04x\t", insn);

    switch ((insn & 0xf000) >> 12) {
        case 0b0000:
            switch ((insn & 0x0f00) >> 8) {
                case 0b0000: //0000 0000 .... ....
                    switch ((insn & 0x00c0) >> 6) {
                        case 0b00:
                            if(((insn >> 5) & 1) == 0) { //0000 0000 000. ....
                                if(((insn >> 4) & 1) == 0) { //0000 0000 0000 ....
                                    switch(insn & 0x000f) {
                                        case 0: //0000 0000 0000 0000
                                            break;
                                        case 1: //0000 0000 0000 0001, ABORTI P124
                                            fprintf_func(stream, "0x%04x;     ABORTI", insn);
                                            break;
                                        case 2: //0000 0000 0000 0010, POP IFR
                                            fprintf_func(stream, "0x%04x;     POP IFR", insn);
                                            break;
                                        default: //0000 0000 0000 1nnn, BANZ 16bitOffset,ARn--
                                            break;
                                    }
                                }
                                else { //0000 0000 0001 CCCC INTR INTx
                                    uint32_t n = insn & 0xf;
                                    fprintf_func(stream, "0x%04x;     INTR %s", insn, INTERRUPT_NAME[n]);
                                }
                            }
                            else {// 0000 0000 001C CCCC, TRAP #VectorNumber

                            }
                            break;
                        case 0b01: /*0000 0000 01.. .... LB 22bit */
                        {
                            uint32_t dest = insn32 & 0x3fffff;
                            fprintf_func(stream, "0x%08x; LB @0x%x", insn32, dest);
                            length = 4;
                            break;
                        }
                        case 0b10: // 0000 0000 10CC CCCC, LC 22bit
                            break;
                        case 0b11: // 0000 0000 11CC CCCC, FFC XAR7,22bit
                            break;
                    }
                    break;
                case 0b0001:
                    break;
                case 0b0010:
                    break;
                case 0b0011:
                    break;
                case 0b0100:
                    break;
                case 0b0101:
                    break;
                case 0b0110: //0000 0110 .... .... MOVL ACC,loc32
                {
                    uint32_t mode = insn & 0xff;
                    get_loc_string(str,mode,LOC32);
                    fprintf_func(stream, "0x%04x;     MOVL ACC,%s", insn, str);
                    break;
                }
                case 0b0111:
                    break;
                case 0b1000:
                    break;
                case 0b1001:
                    break;
                case 0b1010:
                    break;
                case 0b1011:
                    break;
                case 0b1100:
                    break;
                case 0b1101:
                    break;
                case 0b1110:
                    break;
                case 0b1111:
                    break;
            }
            break;
        case 0b0001:
            switch ((insn & 0x0f00) >> 8) {
                case 0b1001: //0001 1001 CCCC CCCC SUBB ACC,#8bit
                {
                    uint32_t imm = insn & 0xff;
                    fprintf_func(stream, "0x%04x;     SUBB ACC,#0x%x", insn, imm);
                    break;
                }
                case 0b1110: //0001 1110 .... .... MOVL loc32,ACC
                {
                    uint32_t mode = insn & 0xff;
                    get_loc_string(str,mode,LOC32);
                    fprintf_func(stream, "0x%04x;     MOV %s,ACC", insn, str);
                    break;
                }
            }
            break;
        case 0b0010:
            switch ((insn & 0x0f00) >> 8) {
                case 0b0000:
                    break;
                case 0b0001:
                    break;
                case 0b0010:
                    break;
                case 0b0011:
                    break;
                case 0b0100:
                    break;
                case 0b0101: /* 0010 0101 .... .... MOV ACC, loc16<<#16 */
                {
                    uint32_t mode = insn & 0xff;
                    get_loc_string(str,mode,LOC16);
                    fprintf_func(stream, "0x%04x;     MOV ACC,%s<<#16", insn, str);
                }
                    break;
                case 0b0110:
                    break;
                case 0b0111:
                    break;
                case 0b1000: /* MOV loc16, #16bit p260 */
                {
                    uint32_t imm = insn32 & 0xffff;
                    uint32_t mode = insn & 0xff;
                    get_loc_string(str,mode,LOC16);
                    fprintf_func(stream, "0x%08x; MOV %s,#0x%x", insn32, str,imm);
                    length = 4;
                    break;
                }
                case 0b1001:
                    break;
                case 0b1010:
                    break;
                case 0b1011:
                    break;
                case 0b1100:
                    break;
                case 0b1101:
                    break;
                case 0b1110:
                    break;
                case 0b1111:
                    break;
            }
            break;
        case 0b0011:
            switch ((insn & 0xf00) >> 8) {
                case 0b1010: //0011 1010 LLLL LLLL MOVL loc32, XAR0
                {
                    uint32_t mode = insn & 0xff;
                    get_loc_string(str,mode,LOC32);
                    fprintf_func(stream, "0x%04x;     MOVL %s,XAR0", insn, str);
                    break;
                }
            }
            break;
        case 0b0100:
            break;
        case 0b0101:
            switch ((insn & 0x0f00) >> 8) {
                case 0b0110://0101 0110 .... ....
                    switch ((insn & 0x00f0) >> 4) {
                        case 0b0000: //0101 0110 0000 ....
                            switch (insn & 0x000f) {
                                case 0b0011: /* 0101 0110 0000 0011  MOV ACC, loc16<<#1...15 */
                                {
                                    uint32_t shift = (insn32 & 0x0f00) >> 8;
                                    uint32_t mode = (insn32 & 0xff);
                                    get_loc_string(str,mode,LOC16);
                                    fprintf_func(stream, "0x%08x; MOV ACC,%s<<#%d", insn32, str,shift);
                                    length = 4;
                                    break;
                                }
                            }
                            break;
                        case 0b1100: //0101 0110 1100 COND  BF 16bitOffset,COND
                        {
                            int16_t imm = (uint16_t)(insn32&0xffff);
                            int16_t cond = (insn&0xf);
                            length = 4;
                            get_cond_string(str, cond);
                            fprintf_func(stream, "0x%08x; BF %d,%s", insn32, imm, str);
                            break;
                        }
                    }
                    break;
            }
            break;
        case 0b0110: //0110 COND CCCC CCCC SB 8bitOffset,COND
        {
            int16_t imm = (uint16_t)(insn&0xff);
            int16_t cond = (insn>>8)&0xf;
            get_cond_string(str, cond);
            fprintf_func(stream, "0x%04x;     SB %d,%s", insn, imm, str);
            break;
        }
        case 0b0111:
            switch((insn & 0x0f00) >> 8) {
                case 0b0110: //0111 0110 .... ....
                    if (((insn >> 7) & 0x1) == 1) { //0111 0110 1... .... MOVL XARn,#22bit 
                        uint32_t n = ((insn >> 6) & 0b11) + 6;//XAR6,XAR7
                        uint32_t imm = insn32 & 0x3fffff;
                        fprintf_func(stream, "0x%08x; MOVL XAR%d, #0x%x", insn32, n,imm);
                        length = 4;
                    }
                    else { // 0111 0110 0... ....
                        if (((insn >> 6) & 0x1) == 1) { //0111 0110 01.. .... LCR #22bit todo

                        }
                        else { //0111 0110 00.. ....
                            switch(insn & 0x3f) { 
                                case 0b011111: //0111 0110 0001 1111 MOVW DP,#16bit
                                {
                                    uint32_t imm = insn32 & 0xffff;
                                    fprintf_func(stream, "0x%08x; MOVW DP, #0x%x", insn32, imm);
                                    length = 4;
                                    break;
                                }
                            }
                        }
                    }
                    break;
            }
            break;
        case 0b1000:
            switch ((insn & 0x0f00) >> 8) {
                case 0b0101: /*1000 0101 .... .... MOV ACC, loc16<<0 */
                {                  
                    uint32_t mode = insn & 0xff;
                    get_loc_string(str,mode,LOC16);
                    fprintf_func(stream, "0x%04x;     MOV ACC,%s<<#0", insn, str);
                }
                break;
                case 0b1101: /*1000 1101 .... .... MOVL XARn,#22bit */
                {
                    uint32_t n = (insn >> 6) & 0b11;
                    uint32_t imm = insn32 & 0x3fffff;
                    fprintf_func(stream, "0x%08x; MOVL XAR%d, #0x%x", insn32, n, imm);
                    length = 4;
                }
                break;
                case 0b1111:
                    switch((insn >> 7) & 1) {
                        case 0b0: //1000 1111 0.... .... MOVL XARn,#22bit
                        {
                            uint32_t n = ((insn >> 6) & 0b11) + 4;//XAR4,XAR5
                            uint32_t imm = insn32 & 0x3fffff;
                            fprintf_func(stream, "0x%08x; MOVL XAR%d, #0x%x", insn32, n, imm);
                            length = 4;
                            break;
                        }
                        case 0b1: //1000 1111 1.... ....
                            break;
                    }
                    break;
            }
            break;
        case 0b1001:
            switch ((insn >> 9) & 0b111) {
                case 0b001: //1001 001. .... ....  MOV AX,loc16
                {
                    uint32_t mode = insn & 0xff;
                    get_loc_string(str,mode,LOC16);
                    if (((insn >> 8) & 1) == 1){ //1001 0011 .... .... ah
                        fprintf_func(stream, "0x%04x;     MOV AH,%s", insn, str);
                    }
                    else { // al
                        fprintf_func(stream, "0x%04x;     MOV AL,%s", insn, str);
                    }
                    break;
                }
                case 0b010: //1001 010. .... .... ADD AX,loc16
                {
                    uint32_t mode = insn & 0xff;
                    get_loc_string(str,mode,LOC16);
                    if (((insn >> 8) & 1) == 1){ //1001 0011 .... .... ah
                        fprintf_func(stream, "0x%04x;     ADD AH,%s", insn, str);
                    }
                    else { // al
                        fprintf_func(stream, "0x%04x;     ADD AL,%s", insn, str);
                    }
                    break;
                }
                case 0b011: //1001 011. .... .... MOV loc16,AX
                {                    
                    uint32_t mode = insn & 0xff;
                    get_loc_string(str,mode,LOC16);
                    if (((insn >> 8) & 1) == 1){ //1001 0011 .... .... ah
                        fprintf_func(stream, "0x%04x;     MOV %s,AH", insn, str);
                    }
                    else { // al
                        fprintf_func(stream, "0x%04x;     MOV %s,AL", insn, str);
                    }
                    break;
                }
            }
            break;
        case 0b1010:
            switch ((insn & 0xf00) >> 8) {
                case 0b0000: //1010 0000 LLLL LLLL MOVL loc32, XAR5
                {
                    uint32_t mode = insn & 0xff;
                    get_loc_string(str,mode,LOC32);
                    fprintf_func(stream, "0x%04x;     MOVL %s,XAR5", insn, str);
                    break;
                }
                case 0b0010: //1010 0010 LLLL LLLL MOVL loc32, XAR3
                {
                    uint32_t mode = insn & 0xff;
                    get_loc_string(str,mode,LOC32);
                    fprintf_func(stream, "0x%04x;     MOVL %s,XAR3", insn, str);
                    break;
                }
                case 0b1000: //1010 1000 LLLL LLLL MOVL loc32, XAR4
                {
                    uint32_t mode = insn & 0xff;
                    get_loc_string(str,mode,LOC32);
                    fprintf_func(stream, "0x%04x;     MOVL %s,XAR4", insn, str);
                    break;
                }
                case 0b1010: //1010 1010 LLLL LLLL MOVL loc32, XAR2
                {
                    uint32_t mode = insn & 0xff;
                    get_loc_string(str,mode,LOC32);
                    fprintf_func(stream, "0x%04x;     MOVL %s,XAR2", insn, str);
                    break;
                }
            }
            break;
        case 0b1011:            
            switch ((insn & 0xf00) >> 8) {
                case 0b0010: //1011 0010 LLLL LLLL MOVL loc32, XAR1
                {
                    uint32_t mode = insn & 0xff;
                    get_loc_string(str,mode,LOC32);
                    fprintf_func(stream, "0x%04x;     MOVL %s,XAR1", insn, str);
                    break;
                }
            }
            break;
        case 0b1100:
            switch ((insn & 0xf00) >> 8) {
                case 0b0010: //1100 0010 LLLL LLLL MOVL loc32, XAR6
                {
                    uint32_t mode = insn & 0xff;
                    get_loc_string(str,mode,LOC32);
                    fprintf_func(stream, "0x%04x;     MOVL %s,XAR6", insn, str);
                    break;
                }
                case 0b0011: //1100 0011 LLLL LLLL MOVL loc32, XAR7
                {
                    uint32_t mode = insn & 0xff;
                    get_loc_string(str,mode,LOC32);
                    fprintf_func(stream, "0x%04x;     MOVL %s,XAR7", insn, str);
                    break;
                }
            }
            break;
        case 0b1101:
            break;
        case 0b1110:
            break;
        case 0b1111:
            switch ((insn & 0x0f00) >> 8) { 
                case 0b1111://1111 1111 .... ....
                    switch ((insn & 0x00f0) >> 4) {
                        case 0b0010: /*1111 1111 0010 .... MOV ACC, #16bit<#0...15 */
                        {
                            uint32_t imm = insn32 & 0xffff;
                            uint32_t shift = insn & 0x000f;
                            fprintf_func(stream, "0x%08x; MOV ACC, #0x%x<<#%d", insn32, imm, shift);
                            length = 4;
                            break;
                        }
                    }
                    break;
            }
            break;
    }



    return length;
}