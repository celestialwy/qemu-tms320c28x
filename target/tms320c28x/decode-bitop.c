// AND ACC,#16bit<<#0...16
static void gen_and_acc_16bit_shift(DisasContext *ctx, uint32_t imm, uint32_t shift)
{
    tcg_gen_andi_i32(cpu_acc, cpu_acc, imm<<shift);
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
}

// AND ACC,loc16
static void gen_and_acc_loc16(DisasContext *ctx, uint32_t mode)
{
    TCGLabel *repeat = gen_new_label();

    TCGv a = tcg_temp_local_new();
    gen_ld_loc16(a, mode);
    tcg_gen_and_i32(cpu_acc, cpu_acc, a);

    tcg_gen_brcondi_i32(TCG_COND_GT, cpu_rptc, 0, repeat);

    gen_helper_test_N_Z_32(cpu_env, cpu_acc);

    gen_goto_tb(ctx, 0, (ctx->base.pc_next >> 1) + 1);
    gen_set_label(repeat);
    tcg_gen_subi_i32(cpu_rptc, cpu_rptc, 1);
    gen_goto_tb(ctx, 1, (ctx->base.pc_next >> 1));

    tcg_temp_free(a);
    ctx->base.is_jmp = DISAS_NORETURN;
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

// SETC Mode
static void gen_setc_mode(DisasContext *ctx, uint32_t mode)
{
    uint32_t st0_mask = mode & 0xf;
    uint32_t st1_mask = (mode >> 4) & 0xf;
    tcg_gen_ori_i32(cpu_st0, cpu_st0, st0_mask);
    tcg_gen_ori_i32(cpu_st1, cpu_st1, st1_mask);
}

// SPM shift
static void gen_spm_shift(DisasContext *ctx, uint32_t shift)
{
    uint32_t st0_mask = 0xfc7f;
    uint32_t st0_mask2 = (shift & 0b111) << 7;
    tcg_gen_andi_i32(cpu_st0, cpu_st0, st0_mask); //clear pm
    tcg_gen_ori_i32(cpu_st0, cpu_st0, st0_mask2); // set new pm
}
