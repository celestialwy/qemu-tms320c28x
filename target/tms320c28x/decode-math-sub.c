
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