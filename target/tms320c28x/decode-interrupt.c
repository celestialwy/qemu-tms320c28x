
// POP IFR
static void gen_pop_ifr(DisasContext *ctx) {
    TCGv tmp = tcg_const_i32(0);
    tcg_gen_subi_i32(cpu_sp, cpu_sp, 1);
    tcg_gen_shli_i32(tmp, cpu_sp, 1);// addr*2
    gen_ld16u_swap(cpu_ifr, tmp);
    tcg_temp_free(tmp);
}

static void gen_intr(DisasContext *ctx, Tms320c28xCPU *cpu, uint32_t int_n) {
    if (int_n > 0 && int_n < 15) { // n == 1... 14
        TCGv tmp = tcg_const_i32(int_n);
        TCGv pc = tcg_const_i32(ctx->base.pc_next >> 1);// current pc
        gen_helper_intr(cpu_env, tmp, pc);
        tcg_temp_free(tmp);
        tcg_temp_free(pc);

        ctx->base.is_jmp = DISAS_JUMP;
    }
    else
    {
        //exception
    }
    
}
