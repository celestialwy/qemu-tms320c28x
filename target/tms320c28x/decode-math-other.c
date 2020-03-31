
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

// CSB ACC
static void gen_csb_acc(DisasContext *ctx)
{
    TCGv sign_bit = tcg_temp_local_new();
    tcg_gen_shri_i32(sign_bit, cpu_acc, 31);//get acc sign bit
    TCGv compare_bit = tcg_temp_local_new();
    TCGv temp_acc = tcg_temp_local_new();
    tcg_gen_mov_i32(temp_acc, cpu_acc);
    TCGv count = tcg_const_local_i32(0);

    TCGLabel *repeat = gen_new_label();
    gen_set_label(repeat);
    tcg_gen_addi_i32(count, count, 1);
    tcg_gen_shli_i32(temp_acc, temp_acc, 1);
    tcg_gen_shri_i32(compare_bit, temp_acc, 31);
    tcg_gen_brcond_i32(TCG_COND_EQ, sign_bit, compare_bit, repeat);
    tcg_gen_subi_i32(count, count, 1);
    gen_st_reg_high_half(cpu_xt, count);

    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
    //test TC
    gen_set_bit(cpu_st0, TC_BIT, TC_MASK, sign_bit);
}
