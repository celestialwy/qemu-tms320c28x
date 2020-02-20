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

// ADD ACC,loc16<<T
static void gen_add_acc_loc16_t(DisasContext *ctx, uint32_t mode)
{
    TCGv a = tcg_temp_new();
    TCGv b = tcg_temp_new();
    TCGv shift = tcg_temp_new();
    // TCGv tmp = tcg_temp_new();

    TCGLabel *repeat = gen_new_label();

        tcg_gen_mov_i32(a, cpu_acc);//get a
        gen_ld_loc16(b, mode); //get b
        gen_helper_ld_low_sxm(b, cpu_env, b);//16bit signed extend with sxm
        tcg_gen_shri_i32(shift, cpu_xt, 16);
        tcg_gen_andi_i32(shift, shift, 0x7);//T(3:0)
        tcg_gen_shl_i32(b, b, shift);//b = b<<T

        tcg_gen_add_i32(cpu_acc, a, b);//add

        gen_helper_test_V_32(cpu_env, a, b, cpu_acc);
        gen_helper_test_OVC_OVM_32(cpu_env, a, b, cpu_acc);

        tcg_gen_mov_i32(ctx->temp[0], a);
        tcg_gen_mov_i32(ctx->temp[1], b);

    tcg_gen_brcondi_i32(TCG_COND_GT, cpu_rptc, 0, repeat);

        gen_helper_test_C_32(cpu_env, ctx->temp[0], ctx->temp[1], cpu_acc);
        gen_test_N_Z_acc();
        gen_goto_tb(ctx, 0, (ctx->base.pc_next >> 1) + 2);

    gen_set_label(repeat);
        tcg_gen_subi_i32(cpu_rptc, cpu_rptc, 1);
        gen_goto_tb(ctx, 1, (ctx->base.pc_next >> 1));

    tcg_temp_free_i32(a);
    tcg_temp_free_i32(b);
    tcg_temp_free_i32(shift);
    
    ctx->base.is_jmp = DISAS_NORETURN;
}

// ADD ACC,loc16<<#0...16
// static void gen_add_acc_loc16_shift(DisasContext *ctx, uint32_t mode, uint32_t shift_imm)
// {
//     TCGv a = tcg_temp_new();
//     TCGv b = tcg_temp_new();
//     TCGv shift = tcg_const_i32(shift_imm);

//     for (int i = 0; i <= ctx->repeat_counter; i++) {// repeat
//         tcg_gen_mov_i32(a, cpu_acc);//get a
//         gen_ld_loc16(b, mode); //get b
//         gen_helper_ld_low_sxm(b, cpu_env, b);//16bit signed extend with sxm
//         tcg_gen_shl_i32(b, b, shift);//b = b<<shift_imm

//         tcg_gen_add_i32(cpu_acc, a, b);//add

//         gen_helper_test_V_32(cpu_env, a, b, cpu_acc);
//         gen_helper_test_OVC_OVM_32(cpu_env, a, b, cpu_acc);
//     }
//     gen_helper_test_C_32(cpu_env, a, b, cpu_acc);
//     gen_test_N_Z_acc();

//     tcg_temp_free_i32(a);
//     tcg_temp_free_i32(b);
//     tcg_temp_free_i32(shift);
// }


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
