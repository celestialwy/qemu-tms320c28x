// SUBB ACC,loc16<<#0...16
static void gen_sub_acc_loc16_shift(DisasContext *ctx, uint32_t mode, uint32_t shift_imm)
{
    uint32_t insn_length = 2;
    if ((shift_imm == 16) || (shift_imm == 0))
        insn_length = 1;

    TCGv a = tcg_temp_local_new();
    TCGv b = tcg_temp_local_new();
    TCGLabel *repeat = gen_new_label();

    tcg_gen_mov_i32(a, cpu_acc);
    gen_ld_loc16(b, mode);
    gen_helper_extend_low_sxm(b, cpu_env, b);
    tcg_gen_shli_i32(b, b, shift_imm);

    tcg_gen_sub_i32(cpu_acc, a, b);

    gen_helper_test_sub_V_32(cpu_env, a, b, cpu_acc);
    gen_helper_test_sub_OVC_OVM_32(cpu_env, a, b, cpu_acc);

    tcg_gen_brcondi_i32(TCG_COND_GT, cpu_rptc, 0, repeat);
    if (shift_imm == 16)
        gen_helper_test_sub_C_32_shift16(cpu_env, a, b, cpu_acc);
    else
        gen_helper_test_sub_C_32(cpu_env, a, b, cpu_acc);
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
    gen_goto_tb(ctx, 0, (ctx->base.pc_next >> 1) + insn_length);

    gen_set_label(repeat);
    tcg_gen_subi_i32(cpu_rptc, cpu_rptc, 1);
    gen_goto_tb(ctx, 1, (ctx->base.pc_next >> 1));

    tcg_temp_free_i32(a);
    tcg_temp_free_i32(b);
    ctx->base.is_jmp = DISAS_REPEAT;
}

// SUBB ACC,#8bit
static void gen_subb_acc_8bit(DisasContext *ctx, uint32_t imm) {
    TCGv b = tcg_const_i32(imm);
    TCGv a = tcg_temp_new();
    tcg_gen_mov_i32(a, cpu_acc);
    tcg_gen_sub_i32(cpu_acc, a, b);

    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
    gen_helper_test_sub_C_V_32(cpu_env, a, b, cpu_acc);
    gen_helper_test_sub_OVC_OVM_32(cpu_env, a, b, cpu_acc);

    tcg_temp_free_i32(a);
    tcg_temp_free_i32(b);
}

// SUBB SP,#7bit
static void gen_subb_sp_7bit(DisasContext *ctx, uint32_t imm)
{
    tcg_gen_subi_i32(cpu_sp, cpu_sp, imm);
}

// SUBB XARn,#7bit
static void gen_subb_xarn_7bit(DisasContext *ctx, uint32_t n, uint32_t imm)
{
    tcg_gen_subi_i32(cpu_xar[n], cpu_xar[n], imm);
}

// SUBL ACC,loc32
static void gen_subl_acc_loc32(DisasContext *ctx, uint32_t mode)
{
    TCGv a = tcg_temp_new();
    tcg_gen_mov_i32(a, cpu_acc);
    TCGv b = tcg_temp_new();
    gen_ld_loc32(b, mode);

    tcg_gen_sub_i32(cpu_acc, a, b);

    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
    gen_helper_test_sub_C_V_32(cpu_env, a, b, cpu_acc);
    gen_helper_test_sub_OVC_OVM_32(cpu_env, a, b, cpu_acc);
}