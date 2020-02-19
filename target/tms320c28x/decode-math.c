// ABS ACC
static void gen_abs_acc(DisasContext *ctx)
{
    gen_helper_abs_acc(cpu_env);
    gen_test_N_Z_acc();
    tcg_gen_andi_i32(cpu_st0, cpu_st0, ~ST0_C);//clear C bit
}

// ABSTC ACC
static void gen_abstc_acc(DisasContext *ctx)
{
    gen_helper_abstc_acc(cpu_env);
    gen_test_N_Z_acc();
    tcg_gen_andi_i32(cpu_st0, cpu_st0, ~ST0_C);//clear C bit
}

// ADD ACC,#16bit<<#0...15
static void gen_add_acc_16bit_shift(DisasContext *ctx, uint32_t imm, uint32_t shift)
{
    TCGv b = tcg_const_i32(imm);
    gen_helper_ld_low_sxm(b, cpu_env, b);//16bit signed extend with sxm
    tcg_gen_shli_i32(b, b, shift);
    
    TCGv a = tcg_const_i32(0);
    tcg_gen_mov_i32(a, cpu_acc);

    tcg_gen_add_i32(cpu_acc, a, b);

    gen_test_N_Z_acc();
    gen_helper_test_C_V_32(cpu_env, a, b, cpu_acc);
    gen_helper_test_OVC_OVM_32(cpu_env, a, b, cpu_acc);

    tcg_temp_free_i32(a);
    tcg_temp_free_i32(b);
}

// ADD AH,loc16
static void gen_add_ah_loc16(DisasContext *ctx, uint32_t mode) {
    TCGv b = tcg_temp_new();
    TCGv ax = tcg_temp_new();
    TCGv a = tcg_temp_new();
    gen_ld_loc16(b, mode);

    tcg_gen_shri_tl(a, cpu_acc, 16);//get ah

    tcg_gen_add_i32(ax, a, b);//add
    gen_st_reg_high_half(cpu_acc, ax);

    gen_helper_test_C_V_16(cpu_env, a, b, ax);
    gen_test_N_Z_ah();

    tcg_temp_free_i32(a);
    tcg_temp_free_i32(b);
    tcg_temp_free_i32(ax);
}

// ADD AL,loc16
static void gen_add_al_loc16(DisasContext *ctx, uint32_t mode) {
    TCGv a = tcg_temp_new();
    TCGv b = tcg_temp_new();
    TCGv ax = tcg_temp_new();
    gen_ld_loc16(b, mode);

    tcg_gen_andi_i32(a, cpu_acc, 0xffff);

    tcg_gen_add_i32(ax, a, b);//add
    gen_st_reg_low_half(cpu_acc, ax);

    gen_helper_test_C_V_16(cpu_env, a, b, ax);
    gen_test_N_Z_al();
    
    tcg_temp_free_i32(a);
    tcg_temp_free_i32(b);
    tcg_temp_free_i32(ax);
}


// SUBB ACC,#8bit
static void gen_subb_acc_8bit(DisasContext *ctx, uint32_t imm) {
    imm = -imm;

    TCGv b = tcg_const_i32(imm);
    TCGv a = tcg_const_i32(0);
    tcg_gen_mov_i32(a, cpu_acc);
    tcg_gen_add_i32(cpu_acc, a, b);

    gen_test_N_Z_acc();
    gen_helper_test_C_V_32(cpu_env, a, b, cpu_acc);
    gen_helper_test_OVC_OVM_32(cpu_env, a, b, cpu_acc);

    tcg_temp_free_i32(a);
    tcg_temp_free_i32(b);
}
