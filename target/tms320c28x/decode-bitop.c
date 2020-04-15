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

    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
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
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
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
    //shift p
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

// SPM shift
static void gen_spm_shift(DisasContext *ctx, uint32_t shift)
{
    gen_seti_bit(cpu_st0, PM_BIT, PM_MASK, shift);
}
