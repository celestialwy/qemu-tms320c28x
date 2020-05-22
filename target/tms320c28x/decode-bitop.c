// AND ACC,#16bit<<#0...16
static void gen_and_acc_16bit_shift(DisasContext *ctx, uint32_t imm, uint32_t shift)
{
    tcg_gen_andi_i32(cpu_acc, cpu_acc, imm<<shift);
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
}

// AND ACC,loc16
static void gen_and_acc_loc16(DisasContext *ctx, uint32_t mode)
{
    TCGLabel *begin = gen_new_label();
    TCGLabel *end = gen_new_label();
    TCGv a = tcg_temp_local_new();

    gen_set_label(begin);
    gen_ld_loc16(a, mode);
    tcg_gen_and_i32(cpu_acc, cpu_acc, a);

    tcg_gen_brcondi_i32(TCG_COND_EQ, cpu_rptc, 0, end);
    tcg_gen_subi_i32(cpu_rptc, cpu_rptc, 1);
    tcg_gen_br(begin);

    gen_set_label(end);
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
    tcg_temp_free(a);
}

// AND AX,loc16,#16bit
static void gen_and_ax_loc16_16bit(DisasContext *ctx, uint32_t mode, uint32_t imm, bool is_AH)
{
    TCGv a = tcg_temp_new();
    gen_ld_loc16(a, mode);
    tcg_gen_andi_i32(a, a, imm);
    if (is_AH) {
        gen_st_reg_high_half(cpu_acc, a);
    }
    else {
        gen_st_reg_low_half(cpu_acc, a);
    }
    gen_helper_test_N_Z_16(cpu_env, a);

    tcg_temp_free(a);
}

// AND IER,#16bit
static void gen_and_ier_16bit(DisasContext *ctx, uint32_t imm)
{
    tcg_gen_andi_i32(cpu_ier, cpu_ier, imm);
}

// AND IFR,#16bit
static void gen_and_ifr_16bit(DisasContext *ctx, uint32_t imm)
{
    tcg_gen_andi_i32(cpu_ifr, cpu_ifr, imm);
}

// AND loc16,ax
static void gen_and_loc16_ax(DisasContext *ctx, uint32_t mode, bool is_AH)
{
    TCGv a = tcg_temp_new();
    TCGv b = tcg_temp_new();
    gen_ld_reg_half(b, cpu_acc, is_AH);

    if (is_reg_addressing_mode(mode, LOC16))
    {
        gen_ld_loc16(a, mode);
        tcg_gen_and_i32(a, a, b);
        gen_helper_test_N_Z_16(cpu_env, a);
        gen_st_loc16(mode, a);
    }
    else 
    {
        TCGv addr = tcg_temp_new();
        gen_get_loc_addr(addr, mode, LOC16);
        gen_ld16u_swap(a, addr); //load
        tcg_gen_and_i32(a, a, b);
        gen_helper_test_N_Z_16(cpu_env, a);
        gen_st16u_swap(a, addr); //store
        tcg_temp_free(addr);
    }

    tcg_temp_free(a);
    tcg_temp_free(b);
}

// AND AX,loc16
static void gen_and_ax_loc16(DisasContext *ctx, uint32_t mode, bool is_AH)
{
    TCGv a = tcg_temp_new();
    TCGv b = tcg_temp_new();
    gen_ld_reg_half(a, cpu_acc, is_AH);
    gen_ld_loc16(b, mode);
    tcg_gen_and_i32(a, a, b);
    gen_helper_test_N_Z_16(cpu_env, a);
    if (is_AH)
        gen_st_reg_high_half(cpu_acc, a);
    else
        gen_st_reg_low_half(cpu_acc, a);
    tcg_temp_free(a);
    tcg_temp_free(b);
}

// AND loc16,#16bitSigned
static void gen_and_loc16_16bit(DisasContext *ctx, uint32_t mode, uint32_t imm)
{
    TCGv a = tcg_temp_new();
    TCGv b = tcg_const_i32(imm);

    if (is_reg_addressing_mode(mode, LOC16))
    {
        gen_ld_loc16(a, mode);
        tcg_gen_and_i32(a, a, b);
        gen_helper_test_N_Z_16(cpu_env, a);
        gen_st_loc16(mode, a);
    }
    else 
    {
        TCGv addr = tcg_temp_new();
        gen_get_loc_addr(addr, mode, LOC16);
        gen_ld16u_swap(a, addr); //load
        tcg_gen_and_i32(a, a, b);
        gen_helper_test_N_Z_16(cpu_env, a);
        gen_st16u_swap(a, addr); //store
        tcg_temp_free(addr);
    }
    
    tcg_temp_free(a);
    tcg_temp_free(b);
}

//ANDB AX,#8bit
static void gen_andb_ax_8bit(DisasContext *ctx, uint32_t imm, bool is_AH)
{
    TCGv a = tcg_temp_new();
    gen_ld_reg_half(a, cpu_acc, is_AH);
    tcg_gen_andi_i32(a, a, imm);
    gen_helper_test_N_Z_16(cpu_env, a);
    if (is_AH)
        gen_st_reg_high_half(cpu_acc, a);
    else
        gen_st_reg_low_half(cpu_acc, a);
    tcg_temp_free(a);
}

// ASP
static void gen_asp(DisasContext *ctx)
{
    TCGv lsb = tcg_temp_new();
    tcg_gen_andi_i32(lsb, cpu_sp, 1);

    tcg_gen_add_i32(cpu_sp, cpu_sp, lsb);//sp = sp + 1
    gen_set_bit(cpu_st1, SPA_BIT, SPA_MASK, lsb);

    tcg_temp_free(lsb);
}

// ASR AX,#1...16
static void gen_asr_ax_imm(DisasContext *ctx, uint32_t shift, bool is_AH)
{
    TCGv a = tcg_temp_new();
    TCGv c = tcg_temp_new();
    gen_ld_reg_half_signed_extend(a, cpu_acc, is_AH);//a, sigend extend
    if (shift != 1)
        tcg_gen_sari_i32(a, a, shift - 1);
    tcg_gen_andi_i32(c, a, 1);//C = last bit out
    tcg_gen_sari_i32(a, a, 1);
    if (is_AH)
        gen_st_reg_high_half(cpu_acc, a);
    else
        gen_st_reg_low_half(cpu_acc, a);
    gen_helper_test_N_Z_16(cpu_env, a);
    gen_set_bit(cpu_st0, C_BIT, C_MASK, c);

    tcg_temp_free(a);
    tcg_temp_free(c);
}

// ASR AX,T
static void gen_asr_ax_t(DisasContext *ctx, bool is_AH)
{
    TCGLabel *shift_is_zero = gen_new_label();
    TCGLabel *done = gen_new_label();

    TCGv a = tcg_temp_local_new();
    TCGv shift = tcg_temp_local_new();
    TCGv c = tcg_temp_local_new();
    gen_ld_reg_half_signed_extend(a, cpu_acc, is_AH);//a = ax, signed extend

    gen_ld_reg_half(shift, cpu_xt, 1);
    tcg_gen_andi_i32(shift, shift, 0b1111);//shift = t[3:0]
    //branch
    tcg_gen_brcondi_i32(TCG_COND_EQ, shift, 0, shift_is_zero);
    //shift != 0
    tcg_gen_subi_i32(shift, shift, 1);
    tcg_gen_sar_i32(a, a, shift);
    tcg_gen_andi_i32(c, a, 1);//C = last bit out
    tcg_gen_sari_i32(a, a, 1);
    tcg_gen_br(done);
    //shift == 0
    gen_set_label(shift_is_zero);
    tcg_gen_movi_i32(c, 0);
    //done
    gen_set_label(done);
    if (is_AH)
        gen_st_reg_high_half(cpu_acc, a);
    else
        gen_st_reg_low_half(cpu_acc, a);
    gen_helper_test_N_Z_16(cpu_env, a);
    gen_set_bit(cpu_st0, C_BIT, C_MASK, c);

    tcg_temp_free(a);
    tcg_temp_free(c);
    tcg_temp_free(shift);
}

// ASR64 ACC:P,#1...16
static void gen_asr64_acc_p_imm(DisasContext *ctx, uint32_t shift)
{
    TCGv a = tcg_temp_new();
    TCGv c = tcg_temp_new();
    //shift acc
    tcg_gen_shli_i32(a, cpu_acc, 32 - shift);
    tcg_gen_sari_i32(cpu_acc, cpu_acc, shift);

    //shift p
    tcg_gen_shri_i32(cpu_p, cpu_p, shift - 1);
    tcg_gen_andi_i32(c, cpu_p, 1);//last bit out
    tcg_gen_shri_i32(cpu_p, cpu_p, 1);
    //set high bit of p
    tcg_gen_or_i32(cpu_p, cpu_p, a);

    gen_helper_test_N_Z_64(cpu_env, cpu_acc, cpu_p);
    gen_set_bit(cpu_st0, C_BIT, C_MASK, c);

    tcg_temp_free(a);
    tcg_temp_free(c);
}

// ASR64 ACC:P,T
static void gen_asr64_acc_p_t(DisasContext *ctx)
{
    TCGLabel *shift_is_zero = gen_new_label();
    TCGLabel *done = gen_new_label();

    TCGv shift = tcg_temp_local_new();
    TCGv c = tcg_temp_local_new();

    gen_ld_reg_half(shift, cpu_xt, 1);//get t
    tcg_gen_andi_i32(shift, shift, 0b111111);//shift = t[5:0]
    //branch
    tcg_gen_brcondi_i32(TCG_COND_EQ, shift, 0, shift_is_zero);
    //shift != 0
    TCGv a = tcg_temp_new();
    TCGv tmp = tcg_const_i32(32);
    //shift acc
    tcg_gen_sub_i32(tmp, tmp, shift);//32-shift, 
    tcg_gen_shl_i32(a, cpu_acc, tmp);//save acc shift out value
    tcg_gen_sar_i32(cpu_acc, cpu_acc, shift);
    //shift p
    tcg_gen_subi_i32(tmp, shift, 1);//tmp = shift -1
    tcg_gen_shr_i32(cpu_p, cpu_p, tmp);
    tcg_gen_andi_i32(c, cpu_p, 1);//last bit out
    tcg_gen_shri_i32(cpu_p, cpu_p, 1);
    //set high bit of p
    tcg_gen_or_i32(cpu_p, cpu_p, a);
    tcg_temp_free(a);
    tcg_temp_free(tmp);
    tcg_gen_br(done);
    //shift == 0
    gen_set_label(shift_is_zero);
    tcg_gen_movi_i32(c, 0);
    //done
    gen_set_label(done);
    gen_helper_test_N_Z_64(cpu_env, cpu_acc, cpu_p);
    gen_set_bit(cpu_st0, C_BIT, C_MASK, c);

    tcg_temp_free(c);
    tcg_temp_free(shift);
}

// ASRL ACC,T
static void gen_asrl_acc_t(DisasContext *ctx)
{
    TCGLabel *shift_is_zero = gen_new_label();
    TCGLabel *done = gen_new_label();

    TCGv shift = tcg_temp_local_new();
    TCGv c = tcg_temp_local_new();

    gen_ld_reg_half(shift, cpu_xt, 1);//get t
    tcg_gen_andi_i32(shift, shift, 0b11111);//shift = t[4:0]
    //branch
    tcg_gen_brcondi_i32(TCG_COND_EQ, shift, 0, shift_is_zero);
    //shift != 0
    TCGv a = tcg_temp_new();
    //shift acc
    tcg_gen_subi_i32(shift, shift, 1);//shift = shift -1
    tcg_gen_sar_i32(cpu_acc, cpu_acc, shift);
    tcg_gen_andi_i32(c, cpu_acc, 1);//last bit out
    tcg_gen_sari_i32(cpu_acc, cpu_acc, 1);

    tcg_temp_free(a);
    tcg_gen_br(done);
    //shift == 0
    gen_set_label(shift_is_zero);
    tcg_gen_movi_i32(c, 0);
    //done
    gen_set_label(done);
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
    gen_set_bit(cpu_st0, C_BIT, C_MASK, c);

    tcg_temp_free(c);
    tcg_temp_free(shift);
}

// CLRC AMODE
static void gen_clrc_amode(DisasContext *ctx)
{
    gen_seti_bit(cpu_st1, AMODE_BIT, AMODE_MASK, 0);
}

// CLRC M0M1MAP
static void gen_clrc_m0m1map(DisasContext *ctx)
{
    gen_seti_bit(cpu_st1, M0M1MAP_BIT, M0M1MAP_MASK, 0);
}

// CLRC Objmode
static void gen_clrc_objmode(DisasContext *ctx)
{
    gen_seti_bit(cpu_st1, OBJMODE_BIT, OBJMODE_MASK, 0);
}

// CLRC OVC
static void gen_clrc_ovc(DisasContext *ctx)
{
    gen_seti_bit(cpu_st0, OVC_BIT, OVC_MASK, 0);
}

// CLRC XF
static void gen_clrc_xf(DisasContext *ctx)
{
    gen_seti_bit(cpu_st1, XF_BIT, XF_MASK, 0);
}

// CLRC Mode
static void gen_clrc_mode(DisasContext *ctx, uint32_t mode)
{
    uint32_t st0_mask = ~(mode & 0xf);
    uint32_t st1_mask = ~((mode >> 4) & 0xf);
    tcg_gen_andi_i32(cpu_st0, cpu_st0, st0_mask);
    tcg_gen_andi_i32(cpu_st1, cpu_st1, st1_mask);
}

// EALLOW
static void gen_eallow(DisasContext *ctx)
{
    gen_seti_bit(cpu_st1, EALLOW_BIT, EALLOW_MASK, 1);
}

// EDIS
static void gen_edis(DisasContext *ctx)
{
    gen_seti_bit(cpu_st1, EALLOW_BIT, EALLOW_MASK, 0);
}

// FLIP AX
static void gen_flip_ax(DisasContext *ctx, bool is_AH)
{
    TCGv a = tcg_temp_new_i32();
    TCGv tmp = tcg_temp_new_i32();
    TCGv tmp2 = tcg_temp_new_i32();
    gen_ld_reg_half(a, cpu_acc, is_AH);
    // n = ((n>>1) & 0x55555555) | ((n<<1) & 0xaaaaaaaa);
    tcg_gen_shri_i32(tmp, a, 1);
    tcg_gen_andi_i32(tmp, tmp, 0x55555555);
    tcg_gen_shli_i32(tmp2, a, 1);
    tcg_gen_andi_i32(tmp2, tmp2, 0xaaaaaaaa);
    tcg_gen_or_i32(a, tmp, tmp2);
    // n = ((n>>2) & 0x33333333) | ((n<<2) & 0xcccccccc);
    tcg_gen_shri_i32(tmp, a, 2);
    tcg_gen_andi_i32(tmp, tmp, 0x33333333);
    tcg_gen_shli_i32(tmp2, a, 2);
    tcg_gen_andi_i32(tmp2, tmp2, 0xcccccccc);
    tcg_gen_or_i32(a, tmp, tmp2);
    // n = ((n>>4) & 0x0f0f0f0f) | ((n<<4) & 0xf0f0f0f0);
    tcg_gen_shri_i32(tmp, a, 4);
    tcg_gen_andi_i32(tmp, tmp, 0x0f0f0f0f);
    tcg_gen_shli_i32(tmp2, a, 4);
    tcg_gen_andi_i32(tmp2, tmp2, 0xf0f0f0f0);
    tcg_gen_or_i32(a, tmp, tmp2);
    // n = ((n>>8) & 0x00ff00ff) | ((n<<8) & 0xff00ff00);
    tcg_gen_shri_i32(tmp, a, 8);
    tcg_gen_andi_i32(tmp, tmp, 0x00ff00ff);
    tcg_gen_shli_i32(tmp2, a, 8);
    tcg_gen_andi_i32(tmp2, tmp2, 0xff00ff00);
    tcg_gen_or_i32(a, tmp, tmp2);
    // n = n & 0x0000ffff;
    tcg_gen_andi_i32(a, a, 0x0000ffff);

    gen_helper_test_N_Z_16(cpu_env, a);
    
    if (is_AH)
        gen_st_reg_high_half(cpu_acc, a);
    else
        gen_st_reg_low_half(cpu_acc, a);

}

// LPADDR
static void gen_lpaddr(DisasContext *ctx)
{
    //p226, todo putting the device in C2xLP compatible addressing mode
    gen_seti_bit(cpu_st1, AMODE_BIT, AMODE_MASK, 1);
}

// LSL ACC,#1...16
static void gen_lsl_acc_imm(DisasContext *ctx, uint32_t shift)
{
    TCGv tmp = tcg_temp_new_i32();

    TCGLabel *begin = gen_new_label();
    TCGLabel *end = gen_new_label();
    gen_set_label(begin);

    tcg_gen_shri_i32(tmp, cpu_acc, 32 - shift);
    tcg_gen_andi_i32(tmp, tmp, 1);
    gen_set_bit(cpu_st0, C_BIT, C_MASK, tmp);
    tcg_gen_shli_i32(cpu_acc, cpu_acc, shift);

    tcg_gen_brcondi_i32(TCG_COND_EQ, cpu_rptc, 0, end);
    tcg_gen_subi_i32(cpu_rptc, cpu_rptc, 1);
    tcg_gen_br(begin);
    gen_set_label(end);

    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
    tcg_temp_free(tmp);
}

// LSL ACC,T
static void gen_lsl_acc_t(DisasContext *ctx)
{
    TCGv tmp = tcg_const_i32(16);
    TCGv shift = tcg_temp_new_i32();
    gen_ld_reg_half(shift, cpu_xt, true);
    tcg_gen_andi_i32(shift, shift, 0b1111);//shift = T[3:0]

    tcg_gen_sub_i32(tmp, tmp, shift);
    tcg_gen_shr_i32(tmp, cpu_acc, tmp);
    tcg_gen_andi_i32(tmp, tmp, 1);
    gen_set_bit(cpu_st0, C_BIT, C_MASK, tmp);

    tcg_gen_shl_i32(cpu_acc, cpu_acc, shift);
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);

    tcg_temp_free(tmp);
    tcg_temp_free(shift);
}

// LSL AX,#1...16
static void gen_lsl_ax_imm(DisasContext *ctx, uint32_t shift, bool is_AH)
{
    TCGv tmp = tcg_temp_new_i32();
    TCGv ax = tcg_temp_new_i32();
    gen_ld_reg_half(ax, cpu_acc, is_AH);

    tcg_gen_shri_i32(tmp, ax, 16 - shift);
    tcg_gen_andi_i32(tmp, tmp, 1);
    gen_set_bit(cpu_st0, C_BIT, C_MASK, tmp);
    tcg_gen_shli_i32(ax, ax, shift);
    gen_helper_test_N_Z_16(cpu_env, ax);
    if (is_AH)
        gen_st_reg_high_half(cpu_acc, ax);
    else
        gen_st_reg_low_half(cpu_acc, ax);
    tcg_temp_free(tmp);
    tcg_temp_free(ax);
}

// LSL AX,T
static void gen_lsl_ax_t(DisasContext *ctx, bool is_AH)
{
    TCGv tmp = tcg_const_i32(32);
    TCGv shift = tcg_temp_new_i32();
    TCGv ax = tcg_temp_new_i32();
    gen_ld_reg_half(ax, cpu_acc, is_AH);

    gen_ld_reg_half(shift, cpu_xt, true);
    tcg_gen_andi_i32(shift, shift, 0b1111);//shift = T[3:0]

    tcg_gen_sub_i32(tmp, tmp, shift);
    tcg_gen_shr_i32(tmp, ax, tmp);
    tcg_gen_andi_i32(tmp, tmp, 1);
    gen_set_bit(cpu_st0, C_BIT, C_MASK, tmp);

    tcg_gen_shl_i32(ax, ax, shift);
    gen_helper_test_N_Z_16(cpu_env, ax);
    if (is_AH)
        gen_st_reg_high_half(cpu_acc, ax);
    else
        gen_st_reg_low_half(cpu_acc, ax);

    tcg_temp_free(tmp);
    tcg_temp_free(ax);
    tcg_temp_free(shift);
}

// LSL64 ACC:P,#1...16
static void gen_lsl64_acc_p_imm(DisasContext *ctx, uint32_t shift)
{
    TCGv tmp = tcg_temp_new_i32();
    tcg_gen_shri_i32(tmp, cpu_acc, 32 - shift);
    tcg_gen_andi_i32(tmp, tmp, 1);
    gen_set_bit(cpu_st0, C_BIT, C_MASK, tmp);

    tcg_gen_shli_i32(cpu_acc, cpu_acc, shift);
    tcg_gen_shri_i32(tmp, cpu_p, 32 - shift);
    tcg_gen_or_i32(cpu_acc, cpu_acc, tmp);
    tcg_gen_shli_i32(cpu_p, cpu_p, shift);

    gen_helper_test_N_Z_64(cpu_env, cpu_acc, cpu_p);
    tcg_temp_free(tmp);
}

// LSL64 ACC:P,T
static void gen_lsl64_acc_p_t(DisasContext *ctx)
{
    TCGv tmp = tcg_temp_new_i32();
    TCGv shift = tcg_temp_new_i32();
    TCGv shift2 = tcg_const_i32(32);
    gen_ld_reg_half(shift, cpu_xt, true);
    tcg_gen_andi_i32(shift, shift, 0b111111);//shift = T[5:0]
    tcg_gen_sub_i32(shift2, shift2, shift);

    tcg_gen_shr_i32(tmp, cpu_acc, shift2);
    tcg_gen_andi_i32(tmp, tmp, 1);
    gen_set_bit(cpu_st0, C_BIT, C_MASK, tmp);

    tcg_gen_shl_i32(cpu_acc, cpu_acc, shift);
    tcg_gen_shr_i32(tmp, cpu_p, shift2);
    tcg_gen_or_i32(cpu_acc, cpu_acc, tmp);
    tcg_gen_shl_i32(cpu_p, cpu_p, shift);

    gen_helper_test_N_Z_64(cpu_env, cpu_acc, cpu_p);
    tcg_temp_free(tmp);
    tcg_temp_free(shift);
    tcg_temp_free(shift2);
}

// LSLL ACC,T
static void gen_lsll_acc_t(DisasContext *ctx)
{
    TCGv tmp = tcg_const_i32(16);
    TCGv shift = tcg_temp_new_i32();
    gen_ld_reg_half(shift, cpu_xt, true);
    tcg_gen_andi_i32(shift, shift, 0b11111);//shift = T[4:0]

    tcg_gen_sub_i32(tmp, tmp, shift);
    tcg_gen_shr_i32(tmp, cpu_acc, tmp);
    tcg_gen_andi_i32(tmp, tmp, 1);
    gen_set_bit(cpu_st0, C_BIT, C_MASK, tmp);

    tcg_gen_shl_i32(cpu_acc, cpu_acc, shift);
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);

    tcg_temp_free(tmp);
    tcg_temp_free(shift);
}

// LSR AX,#1...16
static void gen_lsr_ax_imm(DisasContext *ctx, uint32_t shift, bool is_AH)
{
    TCGv a = tcg_temp_new();
    TCGv c = tcg_temp_new();
    gen_ld_reg_half(a, cpu_acc, is_AH);//ax
    if (shift != 1)
        tcg_gen_shri_i32(a, a, shift - 1);
    tcg_gen_andi_i32(c, a, 1);//C = last bit out
    tcg_gen_shri_i32(a, a, 1);
    if (is_AH)
        gen_st_reg_high_half(cpu_acc, a);
    else
        gen_st_reg_low_half(cpu_acc, a);
    gen_helper_test_N_Z_16(cpu_env, a);
    gen_set_bit(cpu_st0, C_BIT, C_MASK, c);

    tcg_temp_free(a);
    tcg_temp_free(c);
}

// LSR AX,T
static void gen_lsr_ax_t(DisasContext *ctx, bool is_AH)
{
    TCGLabel *shift_is_zero = gen_new_label();
    TCGLabel *done = gen_new_label();

    TCGv a = tcg_temp_local_new();
    TCGv shift = tcg_temp_local_new();
    TCGv c = tcg_temp_local_new();
    gen_ld_reg_half(a, cpu_acc, is_AH);//a = ax

    gen_ld_reg_half(shift, cpu_xt, 1);
    tcg_gen_andi_i32(shift, shift, 0b1111);//shift = t[3:0]
    //branch
    tcg_gen_brcondi_i32(TCG_COND_EQ, shift, 0, shift_is_zero);
    //shift != 0
    tcg_gen_subi_i32(shift, shift, 1);
    tcg_gen_shr_i32(a, a, shift);
    tcg_gen_andi_i32(c, a, 1);//C = last bit out
    tcg_gen_shri_i32(a, a, 1);
    tcg_gen_br(done);
    //shift == 0
    gen_set_label(shift_is_zero);
    tcg_gen_movi_i32(c, 0);
    //done
    gen_set_label(done);
    if (is_AH)
        gen_st_reg_high_half(cpu_acc, a);
    else
        gen_st_reg_low_half(cpu_acc, a);
    gen_helper_test_N_Z_16(cpu_env, a);
    gen_set_bit(cpu_st0, C_BIT, C_MASK, c);

    tcg_temp_free(a);
    tcg_temp_free(c);
    tcg_temp_free(shift);
}

// LSR64 ACC:P,#1...16
static void gen_lsr64_acc_p_imm(DisasContext *ctx, uint32_t shift)
{
    TCGv a = tcg_temp_new();
    TCGv c = tcg_temp_new();
    //shift acc
    tcg_gen_shli_i32(a, cpu_acc, 32 - shift);
    tcg_gen_shri_i32(cpu_acc, cpu_acc, shift);

    //shift p
    tcg_gen_shri_i32(cpu_p, cpu_p, shift - 1);
    tcg_gen_andi_i32(c, cpu_p, 1);//last bit out
    tcg_gen_shri_i32(cpu_p, cpu_p, 1);
    //set high bit of p
    tcg_gen_or_i32(cpu_p, cpu_p, a);

    gen_helper_test_N_Z_64(cpu_env, cpu_acc, cpu_p);
    gen_set_bit(cpu_st0, C_BIT, C_MASK, c);

    tcg_temp_free(a);
    tcg_temp_free(c);
}

// LSR64 ACC:P,T
static void gen_lsr64_acc_p_t(DisasContext *ctx)
{
    TCGLabel *shift_is_zero = gen_new_label();
    TCGLabel *done = gen_new_label();

    TCGv shift = tcg_temp_local_new();
    TCGv c = tcg_temp_local_new();

    gen_ld_reg_half(shift, cpu_xt, 1);//get t
    tcg_gen_andi_i32(shift, shift, 0b111111);//shift = t[5:0]
    //branch
    tcg_gen_brcondi_i32(TCG_COND_EQ, shift, 0, shift_is_zero);
    //shift != 0
    TCGv a = tcg_temp_new();
    TCGv tmp = tcg_const_i32(32);
    //shift acc
    tcg_gen_sub_i32(tmp, tmp, shift);//32-shift, 
    tcg_gen_shl_i32(a, cpu_acc, tmp);//save acc shift out value
    tcg_gen_shr_i32(cpu_acc, cpu_acc, shift);
    //shift p
    tcg_gen_subi_i32(tmp, shift, 1);//tmp = shift -1
    tcg_gen_shr_i32(cpu_p, cpu_p, tmp);
    tcg_gen_andi_i32(c, cpu_p, 1);//last bit out
    tcg_gen_shri_i32(cpu_p, cpu_p, 1);
    //set high bit of p
    tcg_gen_or_i32(cpu_p, cpu_p, a);
    tcg_temp_free(a);
    tcg_temp_free(tmp);
    tcg_gen_br(done);
    //shift == 0
    gen_set_label(shift_is_zero);
    tcg_gen_movi_i32(c, 0);
    //done
    gen_set_label(done);
    gen_helper_test_N_Z_64(cpu_env, cpu_acc, cpu_p);
    gen_set_bit(cpu_st0, C_BIT, C_MASK, c);

    tcg_temp_free(c);
    tcg_temp_free(shift);
}

// LSRL ACC,T
static void gen_lsrl_acc_t(DisasContext *ctx)
{
    TCGLabel *shift_is_zero = gen_new_label();
    TCGLabel *done = gen_new_label();

    TCGv shift = tcg_temp_local_new();
    TCGv c = tcg_temp_local_new();

    gen_ld_reg_half(shift, cpu_xt, 1);//get t
    tcg_gen_andi_i32(shift, shift, 0b11111);//shift = t[4:0]
    //branch
    tcg_gen_brcondi_i32(TCG_COND_EQ, shift, 0, shift_is_zero);
    //shift != 0
    TCGv a = tcg_temp_new();
    //shift acc
    tcg_gen_subi_i32(shift, shift, 1);//shift = shift -1
    tcg_gen_shr_i32(cpu_acc, cpu_acc, shift);
    tcg_gen_andi_i32(c, cpu_acc, 1);//last bit out
    tcg_gen_shri_i32(cpu_acc, cpu_acc, 1);

    tcg_temp_free(a);
    tcg_gen_br(done);
    //shift == 0
    gen_set_label(shift_is_zero);
    tcg_gen_movi_i32(c, 0);
    //done
    gen_set_label(done);
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
    gen_set_bit(cpu_st0, C_BIT, C_MASK, c);

    tcg_temp_free(c);
    tcg_temp_free(shift);
}

//NASP
static void gen_nasp(DisasContext *ctx)
{
    TCGLabel *done = gen_new_label();
    TCGv spa = tcg_temp_new_i32();
    gen_get_bit(spa, cpu_st1, SPA_BIT, SPA_MASK);

    tcg_gen_brcondi_i32(TCG_COND_NE, spa, 1, done);
    tcg_gen_subi_i32(cpu_sp, cpu_sp, 1);
    gen_seti_bit(cpu_st1, SPA_BIT, SPA_MASK, 0);
    gen_set_label(done);
    tcg_temp_free(spa);
}

//NEG ACC
static void gen_neg_acc(DisasContext *ctx)
{
    TCGv tmp = tcg_temp_local_new_i32();
    TCGLabel *acc_eq_8000 = gen_new_label();
    TCGLabel *acc_eq_0 = gen_new_label();
    TCGLabel *ovm_eq_1 = gen_new_label();
    TCGLabel *part2 = gen_new_label();
    TCGLabel *part3 = gen_new_label();
    //if acc == 0x80000000
    tcg_gen_brcondi_i32(TCG_COND_EQ, cpu_acc, 0x80000000, acc_eq_8000);
    //false:
    //acc = -acc
    tcg_gen_neg_i32(cpu_acc, cpu_acc);
    tcg_gen_br(part2);
    gen_set_label(acc_eq_8000);
    //true:
    //v = 1
    gen_seti_bit(cpu_st0, V_BIT, V_MASK, 1);
    //if ovm == 1
    gen_get_bit(tmp, cpu_st0, OVM_BIT, OVM_MASK);//ovm bit
    tcg_gen_brcondi_i32(TCG_COND_EQ, tmp, 1, ovm_eq_1);
    //false
    tcg_gen_movi_i32(cpu_acc, 0x80000000);
    tcg_gen_br(part2);
    gen_set_label(ovm_eq_1);
    //true
    tcg_gen_movi_i32(cpu_acc, 0x7fffffff);
    gen_set_label(part2);
    //if acc == 0
    tcg_gen_brcondi_i32(TCG_COND_EQ, cpu_acc, 0, acc_eq_0);
    //false
    gen_seti_bit(cpu_st0, C_BIT, C_MASK, 0);//c = 0
    tcg_gen_br(part3);
    gen_set_label(acc_eq_0);
    //true
    gen_seti_bit(cpu_st0, C_BIT, C_MASK, 1);//c = 1
    gen_set_label(part3);
    //done
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
    tcg_temp_free(tmp);
}

//NEG AX
static void gen_neg_ax(DisasContext *ctx, bool is_AH)
{
    TCGv ax = tcg_temp_local_new_i32();
    gen_ld_reg_half(ax, cpu_acc, is_AH);

    TCGv tmp = tcg_temp_local_new_i32();
    TCGLabel *ax_eq_8000 = gen_new_label();
    TCGLabel *ax_eq_0 = gen_new_label();
    TCGLabel *part2 = gen_new_label();
    TCGLabel *part3 = gen_new_label();
    //if ax == 0x8000
    tcg_gen_brcondi_i32(TCG_COND_EQ, ax, 0x8000, ax_eq_8000);
    //false:
    //ax = -ax
    tcg_gen_neg_i32(ax, ax);
    tcg_gen_andi_i32(ax, ax, 0xffff);
    tcg_gen_br(part2);
    gen_set_label(ax_eq_8000);
    //true:
    //v = 1
    gen_seti_bit(cpu_st0, V_BIT, V_MASK, 1);
    //ax = 8000
    tcg_gen_movi_i32(ax, 0x8000);
    gen_set_label(part2);
    //if ax == 0
    tcg_gen_brcondi_i32(TCG_COND_EQ, ax, 0, ax_eq_0);
    //false
    gen_seti_bit(cpu_st0, C_BIT, C_MASK, 0);//c = 0
    tcg_gen_br(part3);
    gen_set_label(ax_eq_0);
    //true
    gen_seti_bit(cpu_st0, C_BIT, C_MASK, 1);//c = 1
    gen_set_label(part3);
    gen_helper_test_N_Z_16(cpu_env, ax);
    if (is_AH)
        gen_st_reg_high_half(cpu_acc, ax);
    else
        gen_st_reg_low_half(cpu_acc, ax);
    tcg_temp_free(ax);
    tcg_temp_free(tmp);
}

//NEG64 ACC:P
static void gen_neg64_acc_p(DisasContext *ctx)
{
    TCGv_i64 oprand = tcg_temp_local_new_i64();
    TCGv_i64 tmp64 = tcg_temp_local_new_i64();
    tcg_gen_ext_i32_i64(oprand, cpu_acc);
    tcg_gen_shli_i64(oprand, oprand, 32);
    tcg_gen_ext_i32_i64(tmp64, cpu_p);
    tcg_gen_or_i64(oprand, oprand, tmp64);

    TCGv tmp = tcg_temp_local_new_i32();
    TCGLabel *oprand_eq_8000 = gen_new_label();
    TCGLabel *oprand_eq_0 = gen_new_label();
    TCGLabel *ovm_eq_1 = gen_new_label();
    TCGLabel *part2 = gen_new_label();
    TCGLabel *part3 = gen_new_label();
    //if acc:p == 0x80000000,00000000
    tcg_gen_brcondi_i64(TCG_COND_EQ, oprand, 0x8000000000000000, oprand_eq_8000);
    //false:
    //acc:p = -acc:p
    tcg_gen_neg_i64(oprand, oprand);
    tcg_gen_br(part2);
    gen_set_label(oprand_eq_8000);
    //true:
    //v = 1
    gen_seti_bit(cpu_st0, V_BIT, V_MASK, 1);
    //if ovm == 1
    gen_get_bit(tmp, cpu_st0, OVM_BIT, OVM_MASK);//ovm bit
    tcg_gen_brcondi_i32(TCG_COND_EQ, tmp, 1, ovm_eq_1);
    //false
    tcg_gen_movi_i64(oprand, 0x8000000000000000);
    tcg_gen_br(part2);
    gen_set_label(ovm_eq_1);
    //true
    tcg_gen_movi_i64(oprand, 0x7fffffffffffffff);
    gen_set_label(part2);
    //if acc == 0
    tcg_gen_brcondi_i64(TCG_COND_EQ, oprand, 0, oprand_eq_0);
    //false
    gen_seti_bit(cpu_st0, C_BIT, C_MASK, 0);//c = 0
    tcg_gen_br(part3);
    gen_set_label(oprand_eq_0);
    //true
    gen_seti_bit(cpu_st0, C_BIT, C_MASK, 1);//c = 1
    gen_set_label(part3);
    //done
    tcg_gen_extrh_i64_i32(cpu_acc, oprand);
    tcg_gen_extrl_i64_i32(cpu_p, oprand);
    gen_helper_test_N_Z_64(cpu_env, cpu_acc, cpu_p);
    tcg_temp_free(tmp);
    tcg_temp_free_i64(tmp64);
    tcg_temp_free_i64(oprand);

}

//NEGTC ACC
static void gen_negtc_acc(DisasContext *ctx)
{
    TCGv tc = tcg_temp_new_i32();
    TCGLabel *done = gen_new_label();

    gen_get_bit(tc, cpu_st0, TC_BIT, TC_MASK);
    tcg_gen_brcondi_i32(TCG_COND_NE, tc, 1, done);
    gen_neg_acc(ctx);
    gen_set_label(done);
    tcg_temp_free_i32(tc);
}

//NOT ACC
static void gen_not_acc(DisasContext *ctx)
{
    tcg_gen_not_i32(cpu_acc, cpu_acc);
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
}

//NOT AX
static void gen_not_ax(DisasContext *ctx, bool is_AH)
{
    TCGv ax = tcg_temp_new_i32();
    gen_ld_reg_half(ax, cpu_acc, is_AH);
    gen_helper_test_N_Z_16(cpu_env, ax);
    tcg_gen_not_i32(ax, ax);
    if (is_AH)
        gen_st_reg_high_half(cpu_acc, ax);
    else
        gen_st_reg_low_half(cpu_acc, ax);

    tcg_temp_free(ax);
}

//OR ACC,loc16
static void gen_or_acc_loc16(DisasContext *ctx, uint32_t mode)
{
    TCGv tmp = tcg_temp_local_new_i32();

    TCGLabel *begin = gen_new_label();
    TCGLabel *end = gen_new_label();
    gen_set_label(begin);

    gen_ld_loc16(tmp, mode);
    tcg_gen_or_i32(cpu_acc, cpu_acc, tmp);

    tcg_gen_brcondi_i32(TCG_COND_EQ, cpu_rptc, 0, end);
    tcg_gen_subi_i32(cpu_rptc, cpu_rptc, 1);
    tcg_gen_br(begin);
    gen_set_label(end);
    
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
    tcg_temp_free(tmp);
}

//OR ACC,#16bit<<#0...16
static void gen_or_acc_16bit_shift(DisasContext *ctx, uint32_t imm, uint32_t shift)
{
    TCGv tmp = tcg_const_i32(imm);
    tcg_gen_shli_i32(tmp, tmp, shift);
    tcg_gen_or_i32(cpu_acc, cpu_acc, tmp);
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
    tcg_temp_free(tmp);
}

//OR AX,loc16
static void gen_or_ax_loc16(DisasContext *ctx, uint32_t mode, bool is_AH)
{
    TCGv ax = tcg_temp_new_i32();
    TCGv tmp = tcg_temp_new_i32();
    gen_ld_reg_half(ax, cpu_acc, is_AH);
    gen_ld_loc16(tmp, mode);
    tcg_gen_or_i32(ax, ax, tmp);
    gen_helper_test_N_Z_16(cpu_env, ax);
    if (is_AH)
        gen_st_reg_high_half(cpu_acc, ax);
    else
        gen_st_reg_low_half(cpu_acc, ax);
    tcg_temp_free(ax);
    tcg_temp_free(tmp);
}

//OR IER,#16bit
static void gen_or_ier_16bit(DisasContext *ctx, uint32_t imm)
{
    tcg_gen_ori_i32(cpu_ier, cpu_ier, imm);
}

//OR IFR,#16bit
static void gen_or_ifr_16bit(DisasContext *ctx, uint32_t imm)
{
    tcg_gen_ori_i32(cpu_ifr, cpu_ifr, imm);
}

//OR loc16,#16bit
static void gen_or_loc16_16bit(DisasContext *ctx,uint32_t mode, uint32_t imm)
{
    TCGv tmp = tcg_temp_new_i32();
    if (is_reg_addressing_mode(mode, LOC16))
    {
        gen_ld_loc16(tmp, mode);
        tcg_gen_ori_i32(tmp, tmp, imm);
        gen_helper_test_N_Z_16(cpu_env, tmp);
        gen_st_loc16(mode, tmp);
    }
    else 
    {
        TCGv addr = tcg_temp_new();
        gen_get_loc_addr(addr, mode, LOC16);
        gen_ld16u_swap(tmp, addr); //load
        tcg_gen_ori_i32(tmp, tmp, imm);
        gen_helper_test_N_Z_16(cpu_env, tmp);
        gen_st16u_swap(tmp, addr); //store
        tcg_temp_free(addr);
    }

    tcg_temp_free(tmp);
}

//OR loc16,AX
static void gen_or_loc16_ax(DisasContext *ctx,uint32_t mode, bool is_AH)
{
    TCGv tmp = tcg_temp_new_i32();
    TCGv ax = tcg_temp_new_i32();
    gen_ld_reg_half(ax, cpu_acc, is_AH);
    if (is_reg_addressing_mode(mode, LOC16))
    {
        gen_ld_loc16(tmp, mode);
        tcg_gen_or_i32(tmp, tmp, ax);
        gen_helper_test_N_Z_16(cpu_env, tmp);
        gen_st_loc16(mode, tmp);
    }
    else 
    {
        TCGv addr = tcg_temp_new();
        gen_get_loc_addr(addr, mode, LOC16);
        gen_ld16u_swap(tmp, addr); //load
        tcg_gen_or_i32(tmp, tmp, ax);
        gen_helper_test_N_Z_16(cpu_env, tmp);
        gen_st16u_swap(tmp, addr); //store
        tcg_temp_free(addr);
    }

    tcg_temp_free(tmp);
}

// ORB AX,#8bit
static void gen_orb_ax_8bit(DisasContext *ctx, uint32_t imm, bool is_AH)
{
    TCGv ax = tcg_temp_new_i32();
    gen_ld_reg_half(ax, cpu_acc, is_AH);
    tcg_gen_ori_i32(ax, ax, imm);
    gen_helper_test_N_Z_16(cpu_env, ax);
    if (is_AH)
        gen_st_reg_high_half(cpu_acc, ax);
    else
        gen_st_reg_low_half(cpu_acc, ax);
    tcg_temp_free(ax);
}

//ROL ACC
static void gen_rol_acc(DisasContext *ctx)
{
    TCGv tmp = cpu_shadow[0];
    TCGv tmp2 = cpu_shadow[1];

    TCGLabel *begin = gen_new_label();
    TCGLabel *end = gen_new_label();
    gen_set_label(begin);

    tcg_gen_shri_i32(tmp, cpu_acc, 31);
    gen_get_bit(tmp2, cpu_st0, C_BIT, C_MASK);
    tcg_gen_shli_i32(cpu_acc, cpu_acc, 1);
    tcg_gen_or_i32(cpu_acc, cpu_acc, tmp2);
    gen_set_bit(cpu_st0, C_BIT, C_MASK, tmp);
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);

    tcg_gen_brcondi_i32(TCG_COND_EQ, cpu_rptc, 0, end);
    tcg_gen_subi_i32(cpu_rptc, cpu_rptc, 1);
    tcg_gen_br(begin);
    gen_set_label(end);

}

//ROR ACC
static void gen_ror_acc(DisasContext *ctx)
{
    TCGv tmp = cpu_shadow[0];
    TCGv tmp2 = cpu_shadow[1];

    TCGLabel *begin = gen_new_label();
    TCGLabel *end = gen_new_label();
    gen_set_label(begin);

    tcg_gen_andi_i32(tmp, cpu_acc, 1);
    gen_get_bit(tmp2, cpu_st0, C_BIT, C_MASK);
    tcg_gen_shli_i32(tmp2, tmp2, 31);
    tcg_gen_shri_i32(cpu_acc, cpu_acc, 1);
    tcg_gen_or_i32(cpu_acc, cpu_acc, tmp2);
    gen_set_bit(cpu_st0, C_BIT, C_MASK, tmp);
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);

    tcg_gen_brcondi_i32(TCG_COND_EQ, cpu_rptc, 0, end);
    tcg_gen_subi_i32(cpu_rptc, cpu_rptc, 1);
    tcg_gen_br(begin);
    gen_set_label(end);

}

// SETC Mode
static void gen_setc_mode(DisasContext *ctx, uint32_t mode)
{
    uint32_t st0_mask = mode & 0xf;
    uint32_t st1_mask = (mode >> 4) & 0xf;
    tcg_gen_ori_i32(cpu_st0, cpu_st0, st0_mask);
    tcg_gen_ori_i32(cpu_st1, cpu_st1, st1_mask);
}

// SETC M0M1MAP
static void gen_setc_m0m1map(DisasContext *ctx)
{
    gen_seti_bit(cpu_st1, M0M1MAP_BIT, M0M1MAP_MASK, 1);
}

// SETC Objmode
static void gen_setc_objmode(DisasContext *ctx)
{
    gen_seti_bit(cpu_st1, OBJMODE_BIT, OBJMODE_MASK, 1);
}

// SETC XF
static void gen_setc_xf(DisasContext *ctx)
{
    gen_seti_bit(cpu_st1, XF_BIT, XF_MASK, 1);
}

// SFR ACC,#1...16
static void gen_sfr_acc_shift(DisasContext *ctx, uint32_t shift)
{
    TCGv sxm = cpu_shadow[0];
    TCGv last_bit_out = cpu_shadow[1];
    TCGLabel *sxm_1 = gen_new_label();
    TCGLabel *done = gen_new_label();
    TCGLabel *begin = gen_new_label();
    TCGLabel *end = gen_new_label();
    gen_set_label(begin);
    
    
    gen_get_bit(sxm, cpu_st0, SXM_BIT, SXM_MASK);
    tcg_gen_shri_i32(last_bit_out, cpu_acc, shift - 1);
    tcg_gen_andi_i32(last_bit_out, last_bit_out, 1);
    gen_set_bit(cpu_st0, C_BIT, C_MASK, last_bit_out);
    tcg_gen_brcondi_i32(TCG_COND_EQ, sxm , 1, sxm_1);
    //sxm != 1
    tcg_gen_shri_i32(cpu_acc, cpu_acc, shift);
    tcg_gen_br(done);
    gen_set_label(sxm_1);
    //sxm = 1
    tcg_gen_sari_i32(cpu_acc, cpu_acc, shift);
    gen_set_label(done);
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);

    tcg_gen_brcondi_i32(TCG_COND_EQ, cpu_rptc, 0, end);
    tcg_gen_subi_i32(cpu_rptc, cpu_rptc, 1);
    tcg_gen_br(begin);
    gen_set_label(end);
}

// SFR ACC,T
static void gen_sfr_acc_t(DisasContext *ctx)
{
    TCGv sxm = cpu_shadow[0];
    TCGv last_bit_out = cpu_shadow[1];
    TCGv shift = cpu_shadow[2];
    TCGLabel *sxm_1 = gen_new_label();
    TCGLabel *shift_not_zero = gen_new_label();
    TCGLabel *done = gen_new_label();
    // TCGLabel *begin = gen_new_label();
    // TCGLabel *end = gen_new_label();
    // gen_set_label(begin);
    
    gen_get_bit(sxm, cpu_st0, SXM_BIT, SXM_MASK);
    gen_ld_reg_half(shift, cpu_xt, true);
    tcg_gen_andi_i32(shift, shift, 0xf);
    tcg_gen_brcondi_i32(TCG_COND_NE, shift, 0, shift_not_zero);
    gen_seti_bit(cpu_st0, C_BIT, C_MASK, 0);
    tcg_gen_br(done);
    gen_set_label(shift_not_zero);
    tcg_gen_subi_i32(shift, shift, 1);
    tcg_gen_shr_i32(last_bit_out, cpu_acc, shift);
    tcg_gen_andi_i32(last_bit_out, last_bit_out, 1);
    gen_set_bit(cpu_st0, C_BIT, C_MASK, last_bit_out);
    tcg_gen_addi_i32(shift, shift, 1);
    tcg_gen_brcondi_i32(TCG_COND_EQ, sxm , 1, sxm_1);
    //sxm != 1
    tcg_gen_shr_i32(cpu_acc, cpu_acc, shift);
    tcg_gen_br(done);
    gen_set_label(sxm_1);
    //sxm = 1
    tcg_gen_sar_i32(cpu_acc, cpu_acc, shift);
    gen_set_label(done);
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);

    // tcg_gen_brcondi_i32(TCG_COND_EQ, cpu_rptc, 0, end);
    // tcg_gen_subi_i32(cpu_rptc, cpu_rptc, 1);
    // tcg_gen_br(begin);
    // gen_set_label(end);
}

// SPM shift
static void gen_spm_shift(DisasContext *ctx, uint32_t shift)
{
    gen_seti_bit(cpu_st0, PM_BIT, PM_MASK, shift);
}

//XOR ACC,loc16
static void gen_xor_acc_loc16(DisasContext *ctx, uint32_t mode)
{
    TCGv loc16 = cpu_shadow[0];

    TCGLabel *begin = gen_new_label();
    TCGLabel *end = gen_new_label();
    gen_set_label(begin);

    gen_ld_loc16(loc16, mode);
    tcg_gen_xor_i32(cpu_acc, cpu_acc, loc16);
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);

    tcg_gen_brcondi_i32(TCG_COND_EQ, cpu_rptc, 0, end);
    tcg_gen_subi_i32(cpu_rptc, cpu_rptc, 1);
    tcg_gen_br(begin);
    gen_set_label(end);
}

//XOR ACC,#16bit<<#0...16
static void gen_xor_acc_imm_shift(DisasContext *ctx, uint32_t imm, uint32_t shift)
{
    tcg_gen_xori_i32(cpu_acc, cpu_acc, imm << shift);
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
}

//XOR AX,loc16
static void gen_xor_ax_loc16(DisasContext *ctx, uint32_t mode, bool is_AH)
{
    TCGv loc16 = cpu_shadow[0];
    TCGv ax = cpu_shadow[1];

    gen_ld_loc16(loc16, mode);
    gen_ld_reg_half(ax, cpu_acc, is_AH);
    tcg_gen_xor_i32(ax, ax, loc16);
    gen_helper_test_N_Z_16(cpu_env, ax);

    if (is_AH)
        gen_st_reg_high_half(cpu_acc, ax);
    else
        gen_st_reg_low_half(cpu_acc, ax);
}

//XOR loc16,ax
static void gen_xor_loc16_ax(DisasContext *ctx, uint32_t mode, bool is_AH)
{
    TCGv loc16 = cpu_shadow[0];
    TCGv ax = cpu_shadow[1];

    if (is_reg_addressing_mode(mode, LOC16))
    {
        gen_ld_loc16(loc16, mode);
        gen_ld_reg_half(ax, cpu_acc, is_AH);
        tcg_gen_xor_i32(loc16, ax, loc16);
        gen_helper_test_N_Z_16(cpu_env, loc16);
        gen_st_loc16(mode, loc16);
    }
    else
    {
        TCGv addr = cpu_shadow[2];
        gen_get_loc_addr(addr, mode, LOC16);
        gen_ld16u_swap(loc16, addr);
        gen_ld_reg_half(ax, cpu_acc, is_AH);
        tcg_gen_xor_i32(loc16, ax, loc16);
        gen_helper_test_N_Z_16(cpu_env, loc16);
        gen_st16u_swap(loc16, addr);
    }
    
}

//XOR loc16,#16bit
static void gen_xor_loc16_16bit(DisasContext *ctx, uint32_t mode, uint32_t imm)
{
    TCGv loc16 = cpu_shadow[0];

    if (is_reg_addressing_mode(mode, LOC16))
    {
        gen_ld_loc16(loc16, mode);
        tcg_gen_xori_i32(loc16, loc16, imm);
        gen_helper_test_N_Z_16(cpu_env, loc16);
        gen_st_loc16(mode, loc16);
    }
    else
    {
        TCGv addr = cpu_shadow[2];
        gen_get_loc_addr(addr, mode, LOC16);
        gen_ld16u_swap(loc16, addr);
        tcg_gen_xori_i32(loc16, loc16, imm);
        gen_helper_test_N_Z_16(cpu_env, loc16);
        gen_st16u_swap(loc16, addr);
    }
    
}

//XORB AX,#8bit
static void gen_xorb_ax_8bit(DisasContext *ctx, uint32_t imm, bool is_AH)
{
    TCGv ax = cpu_shadow[0];
    gen_ld_reg_half(ax, cpu_acc, is_AH);
    tcg_gen_xori_i32(ax, ax, imm);
    gen_helper_test_N_Z_16(cpu_env, ax);
    if (is_AH)
        gen_st_reg_high_half(cpu_acc, ax);
    else
        gen_st_reg_low_half(cpu_acc, ax);
}

//ZALR ACC,loc16
static void gen_zalr_acc_loc16(DisasContext *ctx, uint32_t mode)
{
    TCGv ah = cpu_shadow[0];
    TCGv al = cpu_shadow[1];

    tcg_gen_movi_i32(al, 0x8000);
    gen_ld_loc16(ah, mode);
    gen_st_reg_low_half(cpu_acc, al);
    gen_st_reg_high_half(cpu_acc, ah);
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
}

//ZAPA
static void gen_zapa(DisasContext *ctx)
{
    gen_seti_bit(cpu_st0, OVC_BIT, OVC_MASK, 0);
    tcg_gen_movi_i32(cpu_acc, 0);
    tcg_gen_movi_i32(cpu_p, 0);
    gen_seti_bit(cpu_st0, N_BIT, N_MASK, 0);
    gen_seti_bit(cpu_st0, Z_BIT, Z_MASK, 1);
}
