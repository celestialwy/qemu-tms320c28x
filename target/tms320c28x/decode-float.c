// ABSF32 RaH,RbH
static void gen_absf32_rah_rbh(DisasContext *ctx, uint32_t a, uint32_t b)
{
    //todo
}

// MOV32 RaH, ACC
static void gen_mov32_rah_acc(DisasContext *ctx, uint32_t a)
{
    tcg_gen_mov_i32(cpu_rh[a], cpu_acc);
}