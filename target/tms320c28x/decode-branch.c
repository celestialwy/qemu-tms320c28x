
// BF 16bitOffset,COND
static void gen_bf_16bitOffset_cond(DisasContext *ctx, int16_t offset, uint32_t cond) 
{
    TCGv pc1 = tcg_const_i32(((uint32_t)ctx->base.pc_next >> 1) + offset);
    TCGv pc2 = tcg_const_i32(((uint32_t)ctx->base.pc_next >> 1) + 2);
    ctx->base.is_jmp = DISAS_JUMP;
    TCGv cond_tcg = tcg_const_i32(cond);
    gen_helper_branch_cond(cpu_env, cond_tcg, pc1, pc2);
    tcg_temp_free(cond_tcg);
    tcg_temp_free(pc1);
    tcg_temp_free(pc2);
}

// LB 22bit
static void gen_lb_22bit(DisasContext *ctx, uint32_t imm) {
    tcg_gen_movi_tl(cpu_pc, imm);
    ctx->base.is_jmp = DISAS_JUMP;
}

// LRETR
static void gen_lretr(DisasContext *ctx)
{
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
    // tcg_gen_movi_i32(ctx->rptc, imm);
    tcg_gen_movi_i32(cpu_rptc, imm);
    // ctx->base.is_jmp = DISAS_JUMP;
        // tcg_gen_movi_i32(cpu_pc, ((uint32_t)ctx->base.pc_next >> 1) + 1);
}

// SB 8bitOffset,COND
static void gen_sb_8bitOffset_cond(DisasContext *ctx, int16_t offset, uint32_t cond) {
    TCGv pc1 = tcg_const_i32(((uint32_t)ctx->base.pc_next >> 1) + offset);
    TCGv pc2 = tcg_const_i32(((uint32_t)ctx->base.pc_next >> 1) + 1);
    ctx->base.is_jmp = DISAS_JUMP;
    TCGv cond_tcg = tcg_const_i32(cond);
    gen_helper_branch_cond(cpu_env, cond_tcg, pc1, pc2);
    tcg_temp_free(cond_tcg);
    tcg_temp_free(pc1);
    tcg_temp_free(pc2);
}