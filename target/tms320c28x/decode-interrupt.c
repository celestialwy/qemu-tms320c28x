
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
