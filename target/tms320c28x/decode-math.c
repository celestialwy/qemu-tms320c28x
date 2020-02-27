// ABS ACC
static void gen_abs_acc(DisasContext *ctx)
{
    gen_helper_abs_acc(cpu_env);
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
    tcg_gen_andi_i32(cpu_st0, cpu_st0, ~ST0_C);//clear C bit
}

// ABSTC ACC
static void gen_abstc_acc(DisasContext *ctx)
{
    gen_helper_abstc_acc(cpu_env);
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
    tcg_gen_andi_i32(cpu_st0, cpu_st0, ~ST0_C);//clear C bit
}

// ADD ACC,#16bit<<#0...15
static void gen_add_acc_16bit_shift(DisasContext *ctx, uint32_t imm, uint32_t shift)
{
    TCGv b = tcg_const_i32(imm);
    gen_helper_extend_low_sxm(b, cpu_env, b);//16bit signed extend with sxm
    tcg_gen_shli_i32(b, b, shift);
    
    TCGv a = tcg_const_i32(0);
    tcg_gen_mov_i32(a, cpu_acc);

    tcg_gen_add_i32(cpu_acc, a, b);

    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
    gen_helper_test_C_V_32(cpu_env, a, b, cpu_acc);
    gen_helper_test_OVC_OVM_32(cpu_env, a, b, cpu_acc);

    tcg_temp_free_i32(a);
    tcg_temp_free_i32(b);
}

// ADD ACC,loc16<<T
static void gen_add_acc_loc16_t(DisasContext *ctx, uint32_t mode)
{
    TCGv a = tcg_temp_local_new();
    TCGv b = tcg_temp_local_new();
    TCGv shift = tcg_temp_new();

    TCGLabel *repeat = gen_new_label();

        tcg_gen_mov_i32(a, cpu_acc);//get a
        gen_ld_loc16(b, mode); //get b
        gen_helper_extend_low_sxm(b, cpu_env, b);//16bit signed extend with sxm
        tcg_gen_shri_i32(shift, cpu_xt, 16);
        tcg_gen_andi_i32(shift, shift, 0x7);//T(3:0)
        tcg_gen_shl_i32(b, b, shift);//b = b<<T

        tcg_gen_add_i32(cpu_acc, a, b);//add

        gen_helper_test_V_32(cpu_env, a, b, cpu_acc);
        gen_helper_test_OVC_OVM_32(cpu_env, a, b, cpu_acc);

    tcg_gen_brcondi_i32(TCG_COND_GT, cpu_rptc, 0, repeat);
    
        gen_helper_test_C_32(cpu_env, a, b, cpu_acc);
        gen_helper_test_N_Z_32(cpu_env, cpu_acc);

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
static void gen_add_acc_loc16_shift(DisasContext *ctx, uint32_t mode, uint32_t shift_imm)
{
    uint32_t insn_length = 2;
    if ((shift_imm == 16) || (shift_imm == 0))
        insn_length = 1;


    TCGv a = tcg_temp_local_new();
    TCGv b = tcg_temp_local_new();

    TCGLabel *repeat = gen_new_label();

        tcg_gen_mov_i32(a, cpu_acc);//get a
        gen_ld_loc16(b, mode); //get b
        gen_helper_extend_low_sxm(b, cpu_env, b);//16bit signed extend with sxm
        tcg_gen_shli_i32(b, b, shift_imm);//b = b<<shift

        tcg_gen_add_i32(cpu_acc, a, b);//add

        gen_helper_test_V_32(cpu_env, a, b, cpu_acc);
        gen_helper_test_OVC_OVM_32(cpu_env, a, b, cpu_acc);

    tcg_gen_brcondi_i32(TCG_COND_GT, cpu_rptc, 0, repeat);
    
        gen_helper_test_C_32(cpu_env, a, b, cpu_acc);
        gen_helper_test_N_Z_32(cpu_env, cpu_acc);


        gen_goto_tb(ctx, 0, (ctx->base.pc_next >> 1) + insn_length);

    gen_set_label(repeat);
        tcg_gen_subi_i32(cpu_rptc, cpu_rptc, 1);
        gen_goto_tb(ctx, 1, (ctx->base.pc_next >> 1));
    
    tcg_temp_free_i32(a);
    tcg_temp_free_i32(b);

    ctx->base.is_jmp = DISAS_NORETURN;
    
}


// ADD AX,loc16
static void gen_add_ax_loc16(DisasContext *ctx, uint32_t mode, bool is_AH) {
    TCGv b = tcg_temp_new();
    TCGv ax = tcg_temp_new();
    TCGv a = tcg_temp_new();
    gen_ld_loc16(b, mode);

    gen_ld_reg_half(ax, cpu_acc, is_AH);

    tcg_gen_add_i32(ax, a, b);//add

    gen_helper_test_N_Z_16(cpu_env, ax);
    gen_helper_test_C_V_16(cpu_env, a, b, ax);

    if (is_AH) {
        gen_st_reg_high_half(cpu_acc, ax);
    }
    else {
        gen_st_reg_low_half(cpu_acc, ax);
    }

    tcg_temp_free_i32(a);
    tcg_temp_free_i32(b);
    tcg_temp_free_i32(ax);
}

// ADD loc16,AL
static void gen_add_loc16_al(DisasContext *ctx, uint32_t mode) 
{
    TCGv a = tcg_temp_new();
    TCGv b = tcg_temp_new();
    TCGv ax = tcg_temp_new();
    gen_ld_loc16(b, mode);

    tcg_gen_andi_i32(a, cpu_acc, 0xffff); //AL

    tcg_gen_add_i32(ax, a, b);//add


    gen_helper_test_C_V_16(cpu_env, a, b, ax);
    gen_helper_test_N_Z_16(cpu_env, ax);
    
    gen_st_loc16(mode, ax);

    tcg_temp_free_i32(a);
    tcg_temp_free_i32(b);
    tcg_temp_free_i32(ax);
}

// ADD loc16,AH
static void gen_add_loc16_ah(DisasContext *ctx, uint32_t mode) 
{
    TCGv b = tcg_temp_new();
    TCGv ax = tcg_temp_new();
    TCGv a = tcg_temp_new();
    gen_ld_loc16(b, mode);

    tcg_gen_shri_tl(a, cpu_acc, 16);//get ah

    tcg_gen_add_i32(ax, a, b);//add

    gen_helper_test_C_V_16(cpu_env, a, b, ax);
    gen_helper_test_N_Z_16(cpu_env, ax);

    gen_st_loc16(mode, ax);

    tcg_temp_free_i32(a);
    tcg_temp_free_i32(b);
    tcg_temp_free_i32(ax);
}

// ADD loc16,#16bitSigned
static void gen_add_loc16_16bit(DisasContext *ctx, uint32_t mode, uint32_t imm)
{
    TCGv a = tcg_temp_new();
    gen_ld_loc16(a, mode);
    TCGv b = tcg_const_i32(imm);
    TCGv tmp = tcg_temp_new();

    tcg_gen_add_i32(tmp, a, b);

    gen_helper_test_C_V_16(cpu_env, a, b, tmp);
    gen_helper_test_N_Z_16(cpu_env, tmp);

    gen_st_loc16(mode, tmp);
    
    tcg_temp_free_i32(a);
    tcg_temp_free_i32(b);
    tcg_temp_free_i32(tmp);
}

// ADDB ACC,#8bit
static void gen_addb_acc_8bit(DisasContext *ctx, uint32_t imm)
{
    TCGv a = tcg_temp_new();
    tcg_gen_mov_i32(a, cpu_acc);
    TCGv b = tcg_const_i32(imm);

    tcg_gen_add_i32(cpu_acc, a, b);

    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
    gen_helper_test_C_V_32(cpu_env, a, b, cpu_acc);
    gen_helper_test_OVC_OVM_32(cpu_env, a, b, cpu_acc);

    tcg_temp_free_i32(a);
    tcg_temp_free_i32(b);
}

// ADDB AX,#8bitSigned
static void gen_addb_ax_8bit(DisasContext *ctx, uint32_t imm, bool is_AH)
{
    if ((imm >> 7) == 1) {
        imm = imm | 0xff00;
    }
    TCGv b = tcg_const_i32(imm);

    TCGv a = tcg_temp_new();
    gen_ld_reg_half(a, cpu_acc, is_AH);


    TCGv tmp = tcg_temp_new();
    tcg_gen_add_i32(tmp, a, b);

    gen_helper_test_N_Z_16(cpu_env, tmp);
    gen_helper_test_C_V_16(cpu_env, a, b, tmp);

    if (is_AH) {
        gen_st_reg_high_half(cpu_acc, tmp);
    }
    else {
        gen_st_reg_low_half(cpu_acc, tmp);
    }

    tcg_temp_free_i32(a);
    tcg_temp_free_i32(b);
    tcg_temp_free_i32(tmp);
}

// ADDB SP,#7bit
static void gen_addb_sp_7bit(DisasContext *ctx, uint32_t imm)
{
    tcg_gen_addi_i32(cpu_sp, cpu_sp, imm);
}

// ADDB XARn,#7bit
static void gen_addb_xarn_7bit(DisasContext *ctx, uint32_t n, uint32_t imm)
{
    tcg_gen_addi_i32(cpu_xar[n], cpu_xar[n], imm);
}

// ADDCL ACC,loc32
static void gen_addcl_acc_loc32(DisasContext *ctx, uint32_t mode)
{
    TCGv a = tcg_temp_local_new();
    tcg_gen_mov_i32(a, cpu_acc);

    TCGv b = tcg_temp_local_new();
    gen_ld_loc32(b, mode);

    TCGv c = tcg_temp_local_new();
    tcg_gen_shri_i32(c, cpu_st0, 3);
    tcg_gen_andi_i32(c, c, 1);

    TCGv tmp = tcg_temp_local_new();
    tcg_gen_add_i32(tmp, a, b);
    tcg_gen_add_i32(cpu_acc, tmp, c);

    gen_helper_test2_C_V_OVC_OVM_32(cpu_env, a, b,c, cpu_acc);
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);

    tcg_temp_free_i32(a);
    tcg_temp_free_i32(b);
    tcg_temp_free_i32(tmp);
}

// ADDCU ACC,loc16
static void gen_addcu_acc_loc16(DisasContext *ctx, uint32_t mode)
{
    TCGv a = tcg_temp_local_new();
    tcg_gen_mov_i32(a, cpu_acc);

    TCGv b = tcg_temp_local_new();
    gen_ld_loc16(b, mode);

    TCGv c = tcg_temp_local_new();
    tcg_gen_shri_i32(c, cpu_st0, 3);
    tcg_gen_andi_i32(c, c, 1);

    TCGv tmp = tcg_temp_local_new();
    tcg_gen_add_i32(tmp, a, b);
    tcg_gen_add_i32(cpu_acc, tmp, c);

    gen_helper_test2_C_V_OVC_OVM_32(cpu_env, a, b,c, cpu_acc);
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);

    tcg_temp_free_i32(a);
    tcg_temp_free_i32(b);
    tcg_temp_free_i32(c);
    tcg_temp_free_i32(tmp);
}

// ADDL ACC,loc32
static void gen_addl_acc_loc32(DisasContext *ctx, uint32_t mode)
{
    TCGv a = tcg_temp_local_new();
    tcg_gen_mov_i32(a, cpu_acc);

    TCGv b = tcg_temp_local_new();
    gen_ld_loc32(b, mode);

    tcg_gen_add_i32(cpu_acc, a, b);

    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
    gen_helper_test_C_V_32(cpu_env, a, b, cpu_acc);
    gen_helper_test_OVC_OVM_32(cpu_env, a, b, cpu_acc);

    tcg_temp_free_i32(a);
    tcg_temp_free_i32(b);
}

// ADDL loc32,ACC
static void gen_addl_loc32_acc(DisasContext *ctx, uint32_t mode)
{
    TCGv b = tcg_temp_local_new();
    gen_ld_loc32(b, mode);

    TCGv tmp = tcg_temp_local_new();
    tcg_gen_add_i32(tmp, cpu_acc, b);

    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
    gen_helper_test_C_V_32(cpu_env, cpu_acc, b, tmp);
    gen_helper_test_OVC_OVM_32(cpu_env, cpu_acc, b, tmp);

    gen_st_loc32(mode, tmp);

    tcg_temp_free_i32(tmp);
    tcg_temp_free_i32(b);
}

// ADDU ACC,loc16
static void gen_addu_acc_loc16(DisasContext *ctx, uint32_t mode)
{
    TCGLabel *repeat = gen_new_label();

    TCGv a = tcg_temp_local_new();
    tcg_gen_mov_i32(a, cpu_acc);

    TCGv b = tcg_temp_local_new();
    gen_ld_loc16(b, mode);

    tcg_gen_add_i32(cpu_acc, a, b);
    gen_helper_test_V_32(cpu_env, a, b, cpu_acc);
    gen_helper_test_OVC_OVM_32(cpu_env, a, b, cpu_acc);

    tcg_gen_brcondi_i32(TCG_COND_GT, cpu_rptc, 0, repeat);

    gen_helper_test_C_32(cpu_env, a, b, cpu_acc);
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);

    gen_goto_tb(ctx, 0, (ctx->base.pc_next >> 1) + 1);
    gen_set_label(repeat);
    tcg_gen_subi_i32(cpu_rptc, cpu_rptc, 1);

    gen_goto_tb(ctx, 1, (ctx->base.pc_next >> 1));

    tcg_temp_free_i32(a);
    tcg_temp_free_i32(b);

    ctx->base.is_jmp = DISAS_NORETURN;
}

// ADDUL P,loc32
static void gen_addul_p_loc32(DisasContext *ctx, uint32_t mode)
{
    TCGv a = tcg_temp_new();
    gen_ld_loc32(a, mode);

    TCGv b = tcg_temp_new();
    tcg_gen_mov_i32(b, cpu_p);

    tcg_gen_add_i32(cpu_p, a, b);

    gen_helper_test_N_Z_32(cpu_env, cpu_p);
    gen_helper_test_C_V_32(cpu_env, a, b, cpu_p);
    gen_helper_test_OVCU_32(cpu_env, a, b, cpu_p);

    tcg_temp_free(a);
    tcg_temp_free(b);
}

// ADDUL ACC,loc32
static void gen_addul_acc_loc32(DisasContext *ctx, uint32_t mode)
{
    TCGLabel *repeat = gen_new_label();

    TCGv a = tcg_temp_local_new();
    gen_ld_loc32(a, mode);

    TCGv b = tcg_temp_local_new();
    tcg_gen_mov_i32(b, cpu_acc);

    tcg_gen_add_i32(cpu_acc, a, b);
    gen_helper_test_V_32(cpu_env, a, b, cpu_acc);
    gen_helper_test_OVCU_32(cpu_env, a, b, cpu_acc);

    tcg_gen_brcondi_i32(TCG_COND_GT, cpu_rptc, 0, repeat);

    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
    gen_helper_test_C_32(cpu_env, a, b, cpu_acc);

    gen_goto_tb(ctx, 0, (ctx->base.pc_next >> 1) + 2);
    gen_set_label(repeat);
    tcg_gen_subi_i32(cpu_rptc, cpu_rptc, 1);
    gen_goto_tb(ctx, 1, (ctx->base.pc_next >> 1));

    tcg_temp_free(a);
    tcg_temp_free(b);

    ctx->base.is_jmp = DISAS_NORETURN;
}

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

// SUBB ACC,#8bit
static void gen_subb_acc_8bit(DisasContext *ctx, uint32_t imm) {
    imm = -imm;

    TCGv b = tcg_const_i32(imm);
    TCGv a = tcg_const_i32(0);
    tcg_gen_mov_i32(a, cpu_acc);
    tcg_gen_add_i32(cpu_acc, a, b);

    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
    gen_helper_test_C_V_32(cpu_env, a, b, cpu_acc);
    gen_helper_test_OVC_OVM_32(cpu_env, a, b, cpu_acc);

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