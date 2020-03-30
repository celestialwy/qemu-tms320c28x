
// CMP AX,loc16
static void gen_cmp_ax_loc16(DisasContext *ctx, uint32_t mode, bool is_AH)
{
    TCGv a = tcg_temp_new();
    gen_ld_reg_half(a, cpu_acc, is_AH);

    TCGv b = tcg_temp_new();
    gen_ld_loc16(b, mode);

    gen_helper_cmp16_N_Z_C(cpu_env, a, b);

    tcg_temp_free(a);
    tcg_temp_free(b);
}

// CMP loc16,#16bitSigned
static void gen_cmp_loc16_16bit(DisasContext *ctx, uint32_t mode, uint32_t imm)
{
    TCGv a = tcg_temp_new();
    gen_ld_loc16(a, mode);

    TCGv b = tcg_const_i32(imm);

    gen_helper_cmp16_N_Z_C(cpu_env, a, b);

    tcg_temp_free(a);
    tcg_temp_free(b);
}

// CMP64 ACC:P
static void gen_cmp64_acc_p(DisasContext *ctx)
{
    gen_helper_cmp64_acc_p(cpu_env);
}

// CMPB AX,#8bit
static void gen_cmp_ax_8bit(DisasContext *ctx, uint32_t imm, bool is_AH)
{
    TCGv a = tcg_temp_new();
    gen_ld_reg_half(a, cpu_acc, is_AH);

    TCGv b = tcg_const_i32(imm);

    gen_helper_cmp16_N_Z_C(cpu_env, a, b);

    tcg_temp_free(a);
    tcg_temp_free(b);
}

// CMPL ACC,loc32
static void gen_cmpl_acc_loc32(DisasContext *ctx, uint32_t mode)
{
    TCGv b = tcg_temp_new();
    gen_ld_loc32(b, mode);

    gen_helper_cmp32_N_Z_C(cpu_env, cpu_acc, b);

    tcg_temp_free(b);
}

// CMPL ACC,P<<PM
static void gen_cmpl_acc_p_pm(DisasContext *ctx)
{
    TCGv b = tcg_temp_new();
    gen_helper_shift_by_pm(b, cpu_env, cpu_p);

    gen_helper_cmp32_N_Z_C(cpu_env, cpu_acc, b);
    tcg_temp_free(b);
}

// CMPR 0/1/2/3
static void gen_cmpr_0123(DisasContext *ctx, uint32_t mode)
{
    uint32_t COND = -1;
    if (mode == 0)
    {
        COND = TCG_COND_EQ;
    }
    else if (mode == 1)
    {
        COND = TCG_COND_GT;
    }
    else if (mode == 2)
    {
        COND = TCG_COND_LT;
    }
    else if (mode == 3)
    {
        COND = TCG_COND_NE;
    }

    if (COND != -1)
    {
        TCGv ar0 = tcg_temp_new();
        gen_ld_reg_half(ar0, cpu_xar[0], 0);
        TCGv arn = tcg_temp_new();
        gen_helper_ld_xar_arp(arn, cpu_env);
        TCGLabel *label = gen_new_label();
        TCGLabel *done = gen_new_label();

        tcg_gen_brcond_i32(COND, ar0, arn, label);
        gen_seti_bit(cpu_st0, TC_BIT, TC_MASK, 0);
        tcg_gen_br(done);
        gen_set_label(label);
        gen_seti_bit(cpu_st0, TC_BIT, TC_MASK, 1);
        gen_set_label(done);
        tcg_temp_free(ar0);
        tcg_temp_free(arn);
    }
    else
    {
        gen_exception(ctx, EXCP_INTERRUPT_ILLEGAL);
    }
}