
// B 16bitOffset,COND
static void gen_b_16bitOffset_cond(DisasContext *ctx, int16_t offset, uint32_t cond)
{
    gen_reset_rptc(ctx);

    TCGv cond_tcg = tcg_const_i32(cond);
    TCGv test = tcg_temp_new();

    TCGLabel *label = gen_new_label();

    ctx->base.is_jmp = DISAS_JUMP;
    gen_helper_test_cond(test, cpu_env, cond_tcg);
    tcg_gen_brcondi_i32(TCG_COND_EQ, test, 0, label);
    gen_goto_tb(ctx, 0, ((uint32_t)ctx->base.pc_next >> 1) + offset);
    gen_set_label(label);
    gen_goto_tb(ctx, 1, ((uint32_t)ctx->base.pc_next >> 1) + 2);

    tcg_temp_free(cond_tcg);
    tcg_temp_free(test);
}

// BANZ 16bitOffset,ARn--
static void gen_banz_16bitOffset_arn(DisasContext *ctx, int16_t offset, uint32_t n)
{
    gen_reset_rptc(ctx);

    TCGLabel *label = gen_new_label();
    TCGv arn = tcg_temp_new();
    TCGv tmp = tcg_temp_new();
    gen_ld_reg_half(arn, cpu_xar[n], 0);//get arn
    tcg_gen_subi_i32(tmp, arn, 1);//tmp = arn - 1
    gen_st_reg_low_half(cpu_xar[n], tmp);//AR[n] = tmp
    tcg_gen_brcondi_i32(TCG_COND_EQ, arn, 0, label);
    //arn != 0
    gen_goto_tb(ctx, 0, ((uint32_t)ctx->base.pc_next >> 1) + offset);
    gen_set_label(label);
    //arn == 0
    gen_goto_tb(ctx, 1, ((uint32_t)ctx->base.pc_next >> 1) + 2);
    ctx->base.is_jmp = DISAS_JUMP;
    tcg_temp_free(arn);
    tcg_temp_free(tmp);
}

// BAR 16bitOffset,ARn,ARm,EQ/NEQ
static void gen_bar_16bitOffset_arn_arm_eq_neq(DisasContext *ctx, int16_t offset, uint32_t n, uint32_t m, uint32_t COND_EQ_NEQ)
{
    TCGLabel *label = gen_new_label();
    TCGv arn = tcg_temp_new();
    gen_ld_reg_half(arn, cpu_xar[n], 0);
    TCGv arm = tcg_temp_new();
    gen_ld_reg_half(arm, cpu_xar[m], 0);

    tcg_gen_brcond_i32(COND_EQ_NEQ, arn, arm, label);
    gen_goto_tb(ctx, 1, ((uint32_t)ctx->base.pc_next >> 1) + 2);
    gen_set_label(label);
    gen_goto_tb(ctx, 0, ((uint32_t)ctx->base.pc_next >> 1) + offset);
    
    ctx->base.is_jmp = DISAS_JUMP;
    tcg_temp_free(arn);
    tcg_temp_free(arm);
}

// BF 16bitOffset,COND
static void gen_bf_16bitOffset_cond(DisasContext *ctx, int16_t offset, uint32_t cond)
{
    gen_b_16bitOffset_cond(ctx, offset, cond);
}

// ESTOP0
static void gen_estop0(DisasContext *ctx) 
{
    //todo emulation software breakpoint
}

// ESTOP1
static void gen_estop1(DisasContext *ctx) 
{
    //todo emulation software breakpoint
    //increments the PC by 1
}

// LB 22bit
static void gen_lb_22bit(DisasContext *ctx, uint32_t imm) {
    gen_reset_rptc(ctx);

    tcg_gen_movi_tl(cpu_pc, imm);
    ctx->base.is_jmp = DISAS_JUMP;
}

// LRETR
static void gen_lretr(DisasContext *ctx)
{
    gen_reset_rptc(ctx);

    tcg_gen_mov_i32(cpu_pc, cpu_rpc); // PC = RPC
    tcg_gen_subi_i32(cpu_sp, cpu_sp, 1); // SP = SP - 1
    TCGv tmp = tcg_temp_local_new();
    gen_ld16u_swap(tmp, cpu_sp); //temp[31:16] = [SP]
    tcg_gen_subi_i32(cpu_sp, cpu_sp, 1); // SP = SP - 1
    TCGv tmp2 = tcg_temp_local_new();
    gen_ld16u_swap(tmp2, cpu_sp); //temp[15:0] = [SP]
    tcg_gen_shli_i32(tmp, tmp ,16);
    tcg_gen_andi_i32(tmp2, tmp2, 0xffff);
    tcg_gen_or_i32(tmp, tmp, tmp2);
    tcg_gen_andi_i32(cpu_rpc, tmp, 0x3fffff);//22bit
    
    ctx->base.is_jmp = DISAS_JUMP;
}

// RPT #8bit
static void gen_rpt_8bit(DisasContext *ctx, uint32_t imm)
{
    gen_reset_rptc(ctx);

    tcg_gen_movi_i32(cpu_rptc, imm);
}

// RPT loc16
static void gen_rpt_loc16(DisasContext *ctx, uint32_t mode)
{
    gen_reset_rptc(ctx);

    TCGv val = tcg_temp_new_i32();
    gen_ld_loc16(val, mode);
    tcg_gen_mov_i32(cpu_rptc, val);
}

// SB 8bitOffset,COND
static void gen_sb_8bitOffset_cond(DisasContext *ctx, int16_t offset, uint32_t cond) {
    gen_reset_rptc(ctx);

    TCGv cond_tcg = tcg_const_i32(cond);
    TCGv test = tcg_temp_new();

    TCGLabel *label = gen_new_label();

    ctx->base.is_jmp = DISAS_JUMP;
    gen_helper_test_cond(test, cpu_env, cond_tcg);
    tcg_gen_brcondi_i32(TCG_COND_EQ, test, 0, label);
    gen_goto_tb(ctx, 0, ((uint32_t)ctx->base.pc_next >> 1) + offset);
    gen_set_label(label);
    gen_goto_tb(ctx, 1, ((uint32_t)ctx->base.pc_next >> 1) + 1);

    tcg_temp_free(cond_tcg);
    tcg_temp_free(test);
}