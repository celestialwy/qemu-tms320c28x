
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