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

// SUB ACC,loc16<<T
static void gen_sub_acc_loc16_t(DisasContext *ctx, uint32_t mode)
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

        tcg_gen_sub_i32(cpu_acc, a, b);//add

        gen_helper_test_sub_V_32(cpu_env, a, b, cpu_acc);
        gen_helper_test_sub_OVC_OVM_32(cpu_env, a, b, cpu_acc);

    tcg_gen_brcondi_i32(TCG_COND_GT, cpu_rptc, 0, repeat);
    
        gen_helper_test_N_Z_32(cpu_env, cpu_acc);

        gen_goto_tb(ctx, 0, (ctx->base.pc_next >> 1) + 2);
    gen_set_label(repeat);
        tcg_gen_subi_i32(cpu_rptc, cpu_rptc, 1);
        gen_goto_tb(ctx, 1, (ctx->base.pc_next >> 1));
    
    tcg_temp_free_i32(a);
    tcg_temp_free_i32(b);
    tcg_temp_free_i32(shift);

    ctx->base.is_jmp = DISAS_REPEAT;
}

// SUB ACC,#16bit<<#0...15
static void gen_sub_acc_16bit_shift(DisasContext *ctx, uint32_t imm, uint32_t shift)
{
    TCGv b = tcg_const_i32(imm);
    gen_helper_extend_low_sxm(b, cpu_env, b);//16bit signed extend with sxm
    tcg_gen_shli_i32(b, b, shift);
    
    TCGv a = tcg_const_i32(0);
    tcg_gen_mov_i32(a, cpu_acc);

    tcg_gen_sub_i32(cpu_acc, a, b);

    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
    gen_helper_test_sub_C_V_32(cpu_env, a, b, cpu_acc);
    gen_helper_test_sub_OVC_OVM_32(cpu_env, a, b, cpu_acc);

    tcg_temp_free_i32(a);
    tcg_temp_free_i32(b);
}

// SUB AX,loc16
static void gen_sub_ax_loc16(DisasContext *ctx, uint32_t mode, bool is_AH) 
{
    TCGv b = tcg_temp_new();
    TCGv ax = tcg_temp_new();
    TCGv a = tcg_temp_new();
    gen_ld_loc16(b, mode);

    gen_ld_reg_half(a, cpu_acc, is_AH);

    tcg_gen_sub_i32(ax, a, b);//sub

    gen_helper_test_N_Z_16(cpu_env, ax);
    gen_helper_test_sub_C_V_16(cpu_env, a, b, ax);

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

// SUB loc16,AX
static void gen_sub_loc16_ax(DisasContext *ctx, uint32_t mode, bool is_AH) 
{
    TCGv a = tcg_temp_new();
    TCGv b = tcg_temp_new();
    TCGv result = tcg_temp_new();

    if (is_reg_addressing_mode(mode, LOC16)) {
        gen_ld_loc16(a, mode);
        gen_ld_reg_half(b, cpu_acc, is_AH);
        tcg_gen_sub_i32(result, a, b);//sub
        gen_st_loc16(mode, result);//store
        gen_helper_test_sub_C_V_16(cpu_env, a, b, result);
        gen_helper_test_N_Z_16(cpu_env, result);
    }
    else {
        TCGv_i32 addr = tcg_temp_new();
        gen_get_loc_addr(addr, mode, LOC16);
        gen_ld16u_swap(a, addr);

        gen_ld_reg_half(b, cpu_acc, is_AH);

        tcg_gen_sub_i32(result, a, b);//sub

        gen_helper_test_sub_C_V_16(cpu_env, a, b, result);
        gen_helper_test_N_Z_16(cpu_env, result);
        
        gen_st16u_swap(result, addr); //store

        tcg_temp_free_i32(addr);
    }

    tcg_temp_free_i32(a);
    tcg_temp_free_i32(b);
    tcg_temp_free_i32(result);
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

// SUBBL ACC,loc32
static void gen_subbl_acc_loc32(DisasContext *ctx, uint32_t mode)
{
    TCGv a = tcg_temp_local_new();
    tcg_gen_mov_i32(a, cpu_acc);

    TCGv b = tcg_temp_local_new();
    gen_ld_loc32(b, mode);

    TCGv c = tcg_temp_local_new();
    gen_get_bit(c, cpu_st0, C_BIT, C_MASK);
    tcg_gen_not_i32(c, c);
    tcg_gen_andi_i32(c, c, 1);

    TCGv tmp = tcg_temp_local_new();
    tcg_gen_sub_i32(tmp, a, b);
    tcg_gen_sub_i32(cpu_acc, tmp, c);//ACC = ACC - loc32 + ~C

    gen_helper_test2_sub_C_V_OVC_OVM_32(cpu_env, a, b,c, cpu_acc);
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);

    tcg_temp_free_i32(a);
    tcg_temp_free_i32(b);
    tcg_temp_free_i32(tmp);
}

// SUBCU ACC,loc16
static void gen_subcu_acc_loc16(DisasContext *ctx, uint32_t mode)
{
    TCGv a = tcg_temp_new_i32();
    gen_ld_loc16(a, mode);
    gen_helper_subcu(cpu_env, a);//call helper function: calculate result, update flag bit N C Z
    tcg_temp_free(a);
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