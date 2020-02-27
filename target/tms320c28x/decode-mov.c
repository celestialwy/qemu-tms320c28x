// MOV *(0:16bit),loc16
static void gen_16bit_loc16(DisasContext *ctx, uint32_t mode, uint32_t imm)
{
    TCGv imm_tcg = tcg_const_i32(imm);
    TCGv mode_tcg = tcg_const_i32(mode);
    TCGv is_rpt_tcg = tcg_const_i32(ctx->rpt_set);
    gen_helper_mov_16bit_loc16(cpu_env,mode_tcg,imm_tcg,is_rpt_tcg);
    tcg_temp_free(imm_tcg);
    tcg_temp_free(mode_tcg);
    tcg_temp_free(is_rpt_tcg);
}

// MOV ACC, loc16<<#0...15
static void gen_mov_acc_loc16_shift(DisasContext *ctx, uint32_t mode, uint32_t shift) 
{
    TCGv oprand = tcg_temp_new();
    gen_ld_loc16(oprand, mode);
    gen_helper_extend_low_sxm(oprand, cpu_env, oprand);
    tcg_gen_shli_i32(cpu_acc, oprand, shift); // shift left, save to acc

    // set N,Z
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);

    tcg_temp_free(oprand);
}

// MOV ACC,loc16<<T
static void gen_mov_acc_loc16_t(DisasContext *ctx, uint32_t mode)
{
    TCGv a = tcg_temp_new();
    gen_ld_loc16(a, mode);
    gen_helper_extend_low_sxm(a, cpu_env, a);

    TCGv shift = tcg_temp_new();
    tcg_gen_shri_i32(shift, cpu_xt, 16);
    tcg_gen_andi_i32(shift, shift, 0x7);//T(3:0)
    tcg_gen_shl_i32(cpu_acc, a, shift);//acc = a<<T
    
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);

    tcg_temp_free(a);
    tcg_temp_free(shift);
}

// MOV ACC, #16bit<<#0...15
static void gen_mov_acc_16bit_shift(DisasContext *ctx, uint32_t imm, uint32_t shift) 
{
    TCGv oprand = tcg_const_i32(imm);
    gen_helper_extend_low_sxm(oprand, cpu_env, oprand);//16bit signed extend with sxm

    tcg_gen_shli_i32(cpu_acc, oprand, shift); // shift left, save to acc
    // set N,Z
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);

    tcg_temp_free(oprand);
}

// MOV ARn,loc16
static void gen_mov_arn_loc16(DisasContext *ctx, uint32_t mode, uint32_t n)
{
    TCGv a = tcg_temp_new();
    gen_ld_loc16(a, mode);
    gen_st_reg_low_half(cpu_xar[n], a);

    tcg_temp_free(a);
}

// MOV AX, loc16
static void gen_mov_ax_loc16(DisasContext *ctx, uint32_t mode, bool is_AH) {
    TCGv ax = tcg_temp_new();
    gen_ld_loc16(ax, mode);

    if (is_AH) {
        gen_st_reg_high_half(cpu_acc, ax);
    }
    else {
        gen_st_reg_low_half(cpu_acc, ax);
    }
    gen_helper_test_N_Z_16(cpu_env, ax);

    tcg_temp_free_i32(ax);
}

// MOV DP,#10bit
static void gen_mov_dp_10bit(DisasContext *ctx, uint32_t imm)
{
    TCGv a = tcg_const_i32(imm);
    tcg_gen_andi_i32(cpu_dp, cpu_dp, 0xfc00);
    tcg_gen_or_i32(cpu_dp, cpu_dp, a);
    tcg_temp_free(a);
}

// MOV IER,loc16
static void gen_mov_ier_loc16(DisasContext *ctx, uint32_t mode)
{
    gen_ld_loc16(cpu_ier, mode);
}

// MOV loc16,#16bit
static void gen_mov_loc16_16bit(DisasContext *ctx, uint32_t mode, uint32_t imm) {
    TCGLabel *repeat = gen_new_label();

    TCGv imm_tcg = tcg_const_local_i32(imm);
    gen_st_loc16(mode, imm_tcg);

    tcg_gen_brcondi_i32(TCG_COND_GT, cpu_rptc, 0, repeat);

    gen_test_ax_N_Z(mode);

    gen_goto_tb(ctx, 0, (ctx->base.pc_next >> 1) + 2);

    gen_set_label(repeat);
    tcg_gen_subi_i32(cpu_rptc, cpu_rptc, 1);
    gen_goto_tb(ctx, 1, (ctx->base.pc_next >> 1));

    tcg_temp_free(imm_tcg);
    ctx->base.is_jmp = DISAS_NORETURN;
}

// MOV loc16,*(0:16bit)
static void gen_loc16_16bit(DisasContext *ctx, uint32_t mode, uint32_t imm)
{
    TCGv imm_tcg = tcg_const_i32(imm);
    TCGv mode_tcg = tcg_const_i32(mode);
    TCGv is_rpt_tcg = tcg_const_i32(ctx->rpt_set);
    gen_helper_mov_loc16_16bit(cpu_env,mode_tcg,imm_tcg,is_rpt_tcg);
    tcg_temp_free(imm_tcg);
    tcg_temp_free(mode_tcg);
    tcg_temp_free(is_rpt_tcg);

    gen_test_ax_N_Z(mode);
}

// MOV loc16,#0
static void gen_mov_loc16_0(DisasContext *ctx, uint32_t mode)
{
    TCGLabel *repeat = gen_new_label();

    TCGv a = tcg_const_i32(0);
    gen_st_loc16(mode, a);
    gen_test_ax_N_Z(mode);

    tcg_gen_brcondi_i32(TCG_COND_GT, cpu_rptc, 0, repeat);
    gen_goto_tb(ctx, 0, (ctx->base.pc_next >> 1) + 1);
    gen_set_label(repeat);
    tcg_gen_subi_i32(cpu_rptc, cpu_rptc, 1);
    gen_goto_tb(ctx, 1, (ctx->base.pc_next >> 1));

    tcg_temp_free(a);
    ctx->base.is_jmp = DISAS_NORETURN;
}

// MOV loc16,ACC<<1...8
static void gen_mov_loc16_acc_shift(DisasContext *ctx, uint32_t mode, uint32_t shift, uint32_t insn_len)
{
    TCGLabel *repeat = gen_new_label();

    TCGv a = tcg_temp_local_new();
    tcg_gen_shli_i32(a, cpu_acc, shift);
    tcg_gen_andi_i32(a, a, 0xffff);
    gen_st_loc16(mode, a);
    gen_test_ax_N_Z(mode);

    tcg_gen_brcondi_i32(TCG_COND_GT, cpu_rptc, 0, repeat);
    gen_goto_tb(ctx, 0, (ctx->base.pc_next >> 1) + insn_len);
    gen_set_label(repeat);
    tcg_gen_subi_i32(cpu_rptc, cpu_rptc, 1);
    gen_goto_tb(ctx, 1, (ctx->base.pc_next >> 1));

    tcg_temp_free(a);
    ctx->base.is_jmp = DISAS_NORETURN;
}

// MOV loc16,ARn
static void gen_mov_loc16_arn(DisasContext *ctx, uint32_t mode, uint32_t n)
{
    TCGv a = tcg_temp_new();
    tcg_gen_andi_i32(a, cpu_xar[n], 0xffff);
    gen_st_loc16(mode, a);

    gen_test_ax_N_Z(mode);
    tcg_temp_free(a);
}

// MOV loc16,AH
static void gen_mov_loc16_ax(DisasContext *ctx, uint32_t mode, bool is_AH)
{
    TCGLabel *repeat = gen_new_label();

    TCGv ax = tcg_temp_new();
    gen_ld_reg_half(ax, cpu_acc, is_AH);

    gen_st_loc16(mode, ax);

    tcg_gen_brcondi_i32(TCG_COND_GT, cpu_rptc, 0, repeat);

    gen_test_ax_N_Z(mode);

    gen_goto_tb(ctx, 0, (ctx->base.pc_next >> 1) + 1);
    gen_set_label(repeat);
    tcg_gen_subi_i32(cpu_rptc, cpu_rptc, 1);
    gen_goto_tb(ctx, 1, (ctx->base.pc_next >> 1));

    tcg_temp_free(ax);
    ctx->base.is_jmp = DISAS_NORETURN;
}

// MOV loc16,AX,COND
static void gen_mov_loc16_ax_cond(DisasContext *ctx, uint32_t mode, uint32_t cond, bool is_AH)
{
    TCGLabel *loc16_modification = gen_new_label();
    TCGLabel *done = gen_new_label();
    TCGv cond_tcg = tcg_const_i32(cond);
    TCGv test = tcg_temp_new();
    gen_helper_test_cond(test, cpu_env, cond_tcg);
    tcg_gen_brcondi_i32(TCG_COND_EQ, test, 0, loc16_modification);
    
    TCGv ax = tcg_temp_new();
    gen_ld_reg_half(ax, cpu_acc, is_AH);
    gen_st_loc16(mode, ax);
    gen_test_ax_N_Z(mode);
    tcg_gen_br(done);

//pre post modification addr mode
    gen_set_label(loc16_modification);

    gen_ld_loc16(test,mode);//pre and post mod for addr mode, by doing a load loc16
    
    gen_set_label(done);

    tcg_temp_free(cond_tcg);
    tcg_temp_free(test);
    tcg_temp_free(ax);
}

// MOVA T,loc16
static void gen_mova_t_loc16(DisasContext *ctx, uint32_t mode)
{
    TCGLabel *repeat = gen_new_label();

    TCGv t = tcg_temp_local_new();
    gen_ld_loc16(t, mode);
    gen_st_reg_high_half(cpu_xt, t);

    TCGv pm = tcg_temp_local_new();
    tcg_gen_shri_i32(pm, cpu_st0, 7);//PM
    tcg_gen_andi_i32(pm, pm, 0x7);//PM

    TCGv b = tcg_temp_local_new();
    gen_helper_shift_by_pm(b, cpu_env, cpu_p);//b = P>>PM

    TCGv a = tcg_temp_local_new();
    tcg_gen_mov_i32(a, cpu_acc);
    tcg_gen_add_i32(cpu_acc, a, b);

    gen_helper_test_V_32(cpu_env, a, b, cpu_acc);
    gen_helper_test_OVC_OVM_32(cpu_env, a, b, cpu_acc);

    tcg_gen_brcondi_i32(TCG_COND_GT, cpu_rptc, 0, repeat);

    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
    gen_helper_test_C_32(cpu_env, a, b, cpu_acc);

    gen_goto_tb(ctx, 0, (ctx->base.pc_next >> 1) + 1);
    gen_set_label(repeat);
    tcg_gen_subi_i32(cpu_rptc, cpu_rptc, 1);
    gen_goto_tb(ctx, 1, (ctx->base.pc_next >> 1));

    tcg_temp_free_i32(a);
    tcg_temp_free_i32(b);
    tcg_temp_free_i32(t);
    tcg_temp_free_i32(pm);

    ctx->base.is_jmp = DISAS_NORETURN;
}

// MOVB AX,#8bit
static void gen_movb_ax_8bit(DisasContext *ctx, uint32_t imm, bool is_AH)
{
    TCGv a = tcg_const_i32(imm);
    if (is_AH)
    {
        gen_st_reg_high_half(cpu_acc, a);
    }
    else {
        gen_st_reg_low_half(cpu_acc, a);
    }
    gen_helper_test_N_Z_16(cpu_env, a);
    tcg_temp_free(a);
}

// MOVL ACC,loc32
static void gen_movl_acc_loc32(DisasContext *ctx, uint32_t mode) {
    gen_ld_loc32(cpu_acc, mode);
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
}

// MOVL loc32,ACC
static void gen_movl_loc32_acc(DisasContext *ctx, uint32_t mode) {
    gen_st_loc32(mode,cpu_acc);
    if (mode == 0b10101001) { //if loc32 == @ACC, test N,Z with ACC
        gen_helper_test_N_Z_32(cpu_env, cpu_acc);
    }
}

// MOVL loc32,XARn
static void gen_movl_loc32_xarn(DisasContext *ctx, uint32_t mode, uint32_t n) {
    gen_st_loc32(mode, cpu_xar[n]);
    if (mode == 0b10101001) { //if loc32 == @ACC, test N,Z with ACC
        gen_helper_test_N_Z_32(cpu_env, cpu_acc);
    }
}

// MOVL XARn,#22bit
static void gen_movl_xarn_22bit(DisasContext *ctx, uint32_t n, uint32_t imm) {
    tcg_gen_movi_i32(cpu_xar[n], imm);
}

// MOVW DP,#16bit
static void gen_movw_dp_16bit(DisasContext *ctx, uint32_t imm) {
    tcg_gen_movi_tl(cpu_dp, imm);
}

// POP IFR
static void gen_pop_ifr(DisasContext *ctx) {
    tcg_gen_subi_i32(cpu_sp, cpu_sp, 1);
    gen_ld16u_swap(cpu_ifr, cpu_sp);
}


// POP AR1H:AR0H
static void gen_pop_ar1h_ar0h(DisasContext *ctx) {
    TCGv tmp = tcg_temp_local_new();
    TCGv tmp2 = tcg_temp_local_new();
    tcg_gen_subi_i32(cpu_sp, cpu_sp, 2);
    gen_ld32u_swap(tmp, cpu_sp);
    tcg_gen_shri_i32(tmp2, tmp, 16);//ar1h
    tcg_gen_andi_i32(tmp, tmp, 0xffff);//ar0h

    gen_st_reg_high_half(cpu_xar[0], tmp);
    gen_st_reg_high_half(cpu_xar[1], tmp2);

    tcg_temp_free(tmp);
    tcg_temp_free(tmp2);
}

// POP RPC
static void gen_pop_rpc(DisasContext *ctx)
{
    tcg_gen_subi_i32(cpu_sp, cpu_sp, 2);

    gen_ld32u_swap(cpu_rpc, cpu_sp);
    tcg_gen_andi_i32(cpu_rpc, cpu_rpc, 0x3fffff);//rpc is 22bit 
}


// PUSH RPC
static void gen_push_rpc(DisasContext *ctx)
{
    gen_st32u_swap(cpu_rpc, cpu_sp);
    tcg_gen_addi_i32(cpu_sp, cpu_sp, 2);
}

// PUSH AR1H:AR0H
static void gen_push_ar1h_ar0h(DisasContext *ctx)
{
    TCGv tmp = tcg_temp_local_new();
    TCGv tmp2 = tcg_temp_local_new();
    tcg_gen_andi_i32(tmp, cpu_xar[1], 0xffff0000);
    tcg_gen_shri_i32(tmp2, cpu_xar[0], 16);
    tcg_gen_or_i32(tmp, tmp, tmp2);
    gen_st32u_swap(tmp, cpu_sp);
    tcg_gen_addi_i32(cpu_sp, cpu_sp, 2);

}
