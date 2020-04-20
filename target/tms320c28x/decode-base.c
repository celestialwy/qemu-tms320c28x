static void gen_ld16u_swap(TCGv value, TCGv addr_param) 
{
    TCGv addr = tcg_temp_local_new();
    tcg_gen_shli_i32(addr, addr_param, 1);// addr*2
    tcg_gen_qemu_ld16u(value, addr, 0);

    TCGv_i32 tmp = tcg_const_i32(0);
    tcg_gen_shli_tl(tmp, value, 8);
    tcg_gen_andi_i32(tmp, tmp, 0xff00);
    tcg_gen_shri_tl(value, value, 8);
    tcg_gen_or_i32(value, value, tmp);
    tcg_temp_free_i32(tmp);
    tcg_temp_free_i32(addr);
}

static void gen_ld32u_swap(TCGv value, TCGv addr_param) 
{
    TCGv addr = tcg_temp_local_new();
    tcg_gen_shli_i32(addr, addr_param, 1);// addr*2
    tcg_gen_qemu_ld32u(value, addr, 0);

    TCGv_i32 tmp = tcg_const_i32(0);
    // ABCD -> BADC
    tcg_gen_shli_tl(tmp, value, 8);
    tcg_gen_andi_tl(tmp, tmp, 0xff00ff00);
    tcg_gen_shri_tl(value, value, 8);
    tcg_gen_andi_tl(value, value, 0x00ff00ff);
    tcg_gen_or_i32(value, value, tmp);
    // BADC -> DCBA
    tcg_gen_shli_tl(tmp, value, 16); //swap low16bit and high16bit
    tcg_gen_shri_tl(value, value, 16);
    tcg_gen_or_i32(value, value, tmp);

    tcg_temp_free_i32(tmp);
    tcg_temp_free_i32(addr);
}

static void gen_st16u_swap(TCGv value, TCGv addr_param)
{
    TCGv addr = tcg_temp_local_new();
    tcg_gen_shli_i32(addr, addr_param, 1);// addr*2

    TCGv_i32 tmp = tcg_const_i32(0);
    TCGv_i32 tmp2 = tcg_const_i32(0);

    tcg_gen_shli_tl(tmp, value, 8);
    tcg_gen_andi_i32(tmp, tmp, 0xff00);
    tcg_gen_shri_tl(tmp2, value, 8);
    tcg_gen_or_i32(tmp2, tmp2, tmp);

    tcg_gen_qemu_st16(tmp2, addr, 0);
    tcg_temp_free_i32(tmp);
    tcg_temp_free_i32(tmp2);
    tcg_temp_free_i32(addr);
}

static void gen_st32u_swap(TCGv value, TCGv addr_param) 
{
    TCGv addr = tcg_temp_local_new();
    tcg_gen_shli_i32(addr, addr_param, 1);// addr*2

    TCGv_i32 tmp = tcg_const_local_i32(0);
    TCGv_i32 tmp2 = tcg_const_local_i32(0);
    // ABCD -> CDAB
    tcg_gen_shli_tl(tmp, value, 16);
    tcg_gen_shri_tl(tmp2, value, 16);
    tcg_gen_or_i32(tmp2, tmp2, tmp);
    //CDAB -> DCBA
    tcg_gen_shli_tl(tmp, tmp2, 8);
    tcg_gen_andi_tl(tmp, tmp, 0xff00ff00);
    tcg_gen_shri_tl(tmp2, tmp2, 8);
    tcg_gen_andi_tl(tmp2, tmp2, 0x00ff00ff);
    tcg_gen_or_i32(tmp2, tmp2, tmp);

    tcg_gen_qemu_st32(tmp2, addr, 0);
    tcg_temp_free_i32(tmp);
    tcg_temp_free_i32(tmp2);
    tcg_temp_free_i32(addr);
}

// load loc16 value to retval
static void gen_ld_loc16(TCGv retval, uint32_t mode)
{
    TCGv loc_mode = tcg_const_i32(mode);
    gen_helper_ld_loc16(retval, cpu_env, loc_mode);
    tcg_temp_free(loc_mode);
    // switch ((mode>>3) & 0b11111) {
    //     case 0b10100:
    //     {
    //         //@ar[n]
    //         uint32_t idx = mode & 0b111;
    //         tcg_gen_andi_tl(retval, cpu_xar[idx], 0x0000ffff);
    //         break;            
    //     }
    //     default:
    //         switch(mode) {
    //             case 0b10101000: //@AH
    //                 tcg_gen_shri_i32(retval, cpu_acc, 16);
    //                 break;
    //             case 0b10101001: //@AL
    //                 tcg_gen_andi_tl(retval, cpu_acc, 0x0000ffff);
    //                 break;
    //             case 0b10101010: //@PH
    //                 tcg_gen_shri_i32(retval, cpu_p, 16);
    //                 break;
    //             case 0b10101011: //@PL
    //                 tcg_gen_andi_tl(retval, cpu_p, 0x0000ffff);
    //                 break;
    //             case 0b10101100: //@TH
    //                 tcg_gen_shri_i32(retval, cpu_xt, 16);
    //                 break;
    //             case 0b10101101: //@SP
    //                 tcg_gen_andi_tl(retval, cpu_sp, 0x0000ffff);
    //                 break;
    //             default:
    //             {
    //                 TCGv_i32 address_mode = tcg_const_i32(mode);
    //                 TCGv_i32 addr = tcg_const_i32(0);
    //                 TCGv_i32 loc_type = tcg_const_i32(LOC16);
    //                 gen_helper_addressing_mode(addr, cpu_env, address_mode, loc_type);
    //                 gen_ld16u_swap(retval, addr);
    //                 tcg_temp_free_i32(address_mode);
    //                 tcg_temp_free_i32(addr);
    //                 tcg_temp_free_i32(loc_type);
    //                 break;
    //             }
    //         }
    // }
}

// load loc16.lsb value to retval
static void gen_ld_loc16_byte_addressing(TCGv retval, uint32_t mode)
{
    TCGv loc_mode = tcg_const_i32(mode);
    gen_helper_ld_loc16_byte_addressing(retval, cpu_env, loc_mode);
    tcg_temp_free(loc_mode);
}

// load loc32 value to retval
static void gen_ld_loc32(TCGv retval, uint32_t mode)
{
    TCGv loc_mode = tcg_const_i32(mode);
    gen_helper_ld_loc32(retval, cpu_env, loc_mode);
    tcg_temp_free(loc_mode);

    // switch ((mode>>3) & 0b11111) {
    //     case 0b10100:
    //     {
    //         //@xar[n]
    //         uint32_t idx = mode & 0b111;
    //         tcg_gen_mov_tl(retval, cpu_xar[idx]);
    //         break;            
    //     }
    //     default:
    //         switch(mode) {
    //             case 0b10101001: //@ACC
    //                 tcg_gen_mov_tl(retval, cpu_acc);
    //                 break;
    //             case 0b10101011: //@P
    //                 tcg_gen_mov_tl(retval, cpu_p);
    //                 break;
    //             case 0b10101100: //@XT
    //                 tcg_gen_mov_tl(retval, cpu_xt);
    //                 break;
    //             default:
    //             {
    //                 TCGv_i32 address_mode = tcg_const_i32(mode);
    //                 TCGv_i32 addr = tcg_const_i32(0);
    //                 TCGv_i32 loc_type = tcg_const_i32(LOC32);
    //                 gen_helper_addressing_mode(addr, cpu_env, address_mode, loc_type);
    //                 gen_ld32u_swap(retval, addr);
    //                 tcg_temp_free_i32(address_mode);
    //                 tcg_temp_free_i32(addr);
    //                 tcg_temp_free_i32(loc_type);
    //                 break;
    //             }
    //         }
    // }
}

static void gen_get_loc_addr(TCGv return_addr, uint32_t mode, uint32_t loc_type)
{
    TCGv_i32 address_mode = tcg_const_i32(mode);
    TCGv_i32 loc_type_tcg = tcg_const_i32(loc_type);
    gen_helper_addressing_mode(return_addr, cpu_env, address_mode, loc_type_tcg);
    tcg_temp_free(address_mode);
    tcg_temp_free(loc_type_tcg);
}

static void gen_ld_reg_half(TCGv retval, TCGv reg, bool is_High)
{
    if(is_High) {
        tcg_gen_shri_i32(retval, reg, 16);
    }
    else
    {
        tcg_gen_andi_i32(retval, reg, 0xffff);
    }
}

static void gen_ld_reg_half_signed_extend(TCGv retval, TCGv reg, bool is_High)
{
    if(is_High) {
        tcg_gen_shri_i32(retval, reg, 16);
    }
    else
    {
        tcg_gen_andi_i32(retval, reg, 0xffff);
    }
    tcg_gen_ext16s_tl(retval, retval);
}

static void gen_st_reg_high_half(TCGv reg, TCGv_i32 oprand)
{
    TCGv_i32 tmp = tcg_const_i32(0x0000ffff);
    tcg_gen_and_tl(reg, reg, tmp);// get low 16bit
    tcg_gen_shli_tl(tmp, oprand, 16); // shift imm value
    tcg_gen_or_tl(reg, reg, tmp);// concat high 16bit
    tcg_temp_free_i32(tmp);
}

static void gen_st_reg_low_half(TCGv reg, TCGv_i32 oprand)
{
    TCGv_i32 tmp = tcg_const_i32(0xffff0000);
    tcg_gen_and_tl(reg, reg, tmp);// get high 16bit
    tcg_gen_andi_i32(tmp, oprand, 0xffff);
    tcg_gen_or_tl(reg, reg, tmp);// concat low 16bit
    tcg_temp_free_i32(tmp);
}

static void gen_st_loc16(uint32_t mode, TCGv oprand)
{
    TCGv loc_mode = tcg_const_i32(mode);
    gen_helper_st_loc16(cpu_env, loc_mode, oprand);
    tcg_temp_free(loc_mode);
    // tcg_gen_andi_i32(oprand, oprand, 0xffff);
    
    // switch ((mode>>3) & 0b11111) {
    //     case 0b10100:
    //     {
    //         //@ar[n]
    //         uint32_t idx = mode & 0b111;
    //         gen_st_reg_low_half(cpu_xar[idx], oprand);
    //         break;            
    //     }
    //     default:
    //         switch(mode) {
    //             case 0b10101000: //@AH
    //                 gen_st_reg_high_half(cpu_acc, oprand);
    //                 break;
    //             case 0b10101001: //@AL
    //                 gen_st_reg_low_half(cpu_acc, oprand);
    //                 break;
    //             case 0b10101010: //@PH
    //                 gen_st_reg_high_half(cpu_p, oprand);
    //                 break;
    //             case 0b10101011: //@PL
    //                 gen_st_reg_low_half(cpu_p, oprand);
    //                 break;
    //             case 0b10101100: //@TH
    //                 gen_st_reg_high_half(cpu_xt, oprand);
    //                 break;
    //             case 0b10101101: //@SP
    //                 tcg_gen_mov_tl(cpu_sp, oprand);
    //                 break;
    //             default:
    //             {
    //                 TCGv_i32 address_mode = tcg_const_i32(mode);
    //                 TCGv_i32 addr = tcg_const_i32(0);
    //                 TCGv_i32 loc_type = tcg_const_i32(LOC16);
    //                 gen_helper_addressing_mode(addr, cpu_env, address_mode, loc_type);
    //                 gen_st16u_swap(oprand, addr);
    //                 tcg_temp_free_i32(address_mode);
    //                 tcg_temp_free_i32(addr);
    //                 tcg_temp_free_i32(loc_type);
    //                 break;
    //             }
    //         }
    // }
}

// store value to loc16
static void gen_st_loc16_byte_addressing(uint32_t mode, TCGv oprand)
{
    TCGv loc_mode = tcg_const_i32(mode);
    gen_helper_st_loc16_byte_addressing(cpu_env, loc_mode, oprand);
    tcg_temp_free(loc_mode);
}

static void gen_st_loc32(uint32_t mode, TCGv_i32 oprand)
{
    TCGv loc_mode = tcg_const_i32(mode);
    gen_helper_st_loc32(cpu_env, loc_mode, oprand);
    tcg_temp_free(loc_mode);
    // switch ((mode>>3) & 0b11111) {
    //     case 0b10100:
    //     {
    //         //xarn
    //         uint32_t idx = mode & 0b111;
    //         tcg_gen_mov_tl(cpu_xar[idx], oprand);
    //         break;
    //     }
    //     default:
    //         switch(mode) {
    //             case 0b10101001: //@ACC
    //                 tcg_gen_mov_tl(cpu_acc, oprand);
    //                 break;
    //             case 0b10101011: //@P
    //                 tcg_gen_mov_tl(cpu_p, oprand);
    //                 break;
    //             case 0b10101100: //@XT
    //                 tcg_gen_mov_tl(cpu_xt, oprand);
    //                 break;
    //             default:
    //             {
    //                 TCGv_i32 address_mode = tcg_const_i32(mode);
    //                 TCGv_i32 addr = tcg_const_i32(0);
    //                 TCGv_i32 loc_type = tcg_const_i32(LOC32);

    //                 gen_helper_addressing_mode(addr, cpu_env, address_mode, loc_type);                
    //                 gen_st32u_swap(oprand, addr);

    //                 tcg_temp_free_i32(address_mode);
    //                 tcg_temp_free_i32(addr);
    //                 break;
    //             }
    //         }
    // }
}

static void gen_reset_rptc(DisasContext *ctx)
{
    if (ctx->rpt_set) {
        tcg_gen_movi_i32(cpu_rptc, 0);
        // gen_helper_print(cpu_env, cpu_rptc);
    }
}

static void gen_test_ax_N_Z(uint32_t mode)
{
    if (mode == 0b10101000) { //loc16 == @AH
        TCGv ah = tcg_temp_local_new();
        tcg_gen_shri_i32(ah, cpu_acc, 16);
        gen_helper_test_N_Z_16(cpu_env, ah);
        tcg_temp_free(ah);
    }
    if (mode == 0b10101001) { //loc16 == @AL
        TCGv al = tcg_temp_local_new();
        tcg_gen_andi_i32(al, cpu_acc, 0xffff);
        gen_helper_test_N_Z_16(cpu_env, cpu_acc);
        tcg_temp_free(al);
    }
}

static void gen_test_acc_N_Z(uint32_t mode)
{
    if (mode == 0b10101001) { //if loc32 == @ACC, test N,Z with ACC
        gen_helper_test_N_Z_32(cpu_env, cpu_acc);
    }
}

static void gen_exception(DisasContext *ctx, unsigned int excp)
{
    TCGv_i32 tmp = tcg_const_i32(excp);
    TCGv pc = tcg_const_i32(ctx->base.pc_next >> 1);// current pc
    gen_helper_exception(cpu_env, tmp, pc);
    tcg_temp_free_i32(tmp);
    tcg_temp_free_i32(pc);
}

static bool is_reg_addressing_mode(uint32_t mode, uint32_t loc_type)
{
    if (loc_type == LOC16) 
    {
        if (mode >= 160 && mode <=173) {
            return true;
        }
        else {
            return false;
        }
    }
    if (loc_type == LOC32)
    {
        if ((mode >= 160 && mode <=167) || mode == 169 || mode ==171 || mode == 172) {
            return true;
        }
        else {
            return false;
        }
    }
    return false;
}

static void gen_set_bit(TCGv reg, uint32_t bit, uint32_t mask, TCGv value)
{
    TCGv tmp = tcg_temp_new();
    tcg_gen_andi_i32(reg, reg, ~mask);//clear bit
    tcg_gen_shli_i32(tmp, value, bit);
    tcg_gen_andi_i32(tmp, tmp, mask);
    tcg_gen_or_i32(reg, reg, tmp);
    tcg_temp_free(tmp);
}

static void gen_seti_bit(TCGv reg, uint32_t bit, uint32_t mask, uint32_t value)
{
    tcg_gen_andi_i32(reg, reg, ~mask);//clear bit
    tcg_gen_ori_i32(reg, reg, (value << bit) & mask);
}

static void gen_get_bit(TCGv retval, TCGv reg, uint32_t bit, uint32_t mask)
{
    tcg_gen_andi_i32(retval, reg, mask);
    tcg_gen_shri_i32(retval, retval, bit);
}

static void gen_goto_tb(DisasContext *dc, int n, target_ulong dest)
{
    tcg_gen_goto_tb(n);
    tcg_gen_movi_tl(cpu_pc, dest);
    tcg_gen_exit_tb(dc->base.tb, n);
}

static void gen_shift_by_pm(TCGv retval, TCGv oprand)
{
    TCGLabel *leftshift = gen_new_label();
    TCGLabel *done = gen_new_label();
    TCGv pm = tcg_temp_local_new();
    gen_get_bit(pm ,cpu_st0, PM_BIT, PM_MASK);
    tcg_gen_not_i32(pm, pm);
    tcg_gen_addi_i32(pm, pm, 2);

    tcg_gen_brcondi_i32(TCG_COND_GT, pm, 0, leftshift);
    //signed right shift
    tcg_gen_not_i32(pm, pm);
    tcg_gen_addi_i32(pm, pm, 1);
    tcg_gen_andi_i32(pm, pm, 0b111);
    tcg_gen_sar_i32(retval, oprand, pm);
    tcg_gen_br(done);
    gen_set_label(leftshift);
    tcg_gen_shl_i32(retval, oprand, pm);
    gen_set_label(done);

    tcg_temp_free(pm);
}

//shift mpy result by pm, used in signed 32*32 mpy,
static void gen_shift_by_pm2(TCGv retval, TCGv oprand_low, TCGv oprand_high)
{
    TCGLabel *leftshift = gen_new_label();
    TCGLabel *done = gen_new_label();
    TCGv pm = tcg_temp_local_new();
    TCGv tmp = tcg_temp_local_new();
    gen_get_bit(pm ,cpu_st0, PM_BIT, PM_MASK);
    tcg_gen_not_i32(pm, pm);
    tcg_gen_addi_i32(pm, pm, 2);

    tcg_gen_brcondi_i32(TCG_COND_GT, pm, 0, leftshift);
    //signed right shift
    tcg_gen_not_i32(pm, pm);
    tcg_gen_addi_i32(pm, pm, 1);
    tcg_gen_andi_i32(pm, pm, 0b111);
    tcg_gen_shr_i32(retval, oprand_low, pm);

    tcg_gen_movi_i32(tmp, 32);
    tcg_gen_sub_i32(pm, tmp, pm);
    tcg_gen_shl_i32(tmp, oprand_high, pm);
    tcg_gen_or_i32(retval, retval, tmp);
    tcg_gen_br(done);
    gen_set_label(leftshift);
    tcg_gen_shl_i32(retval, oprand_low, pm);
    gen_set_label(done);

    tcg_temp_free(pm);
    tcg_temp_free(tmp);
}