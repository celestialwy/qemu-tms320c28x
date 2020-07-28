// ABSF32 RaH,RbH
static void gen_absf32_rah_rbh(DisasContext *ctx, uint32_t a, uint32_t b)
{
    //todo
}

// MOV16 mem16, RaH
static void gen_mov16_mem16_rah(DisasContext *ctx, uint32_t mem16, uint32_t a)
{
    gen_st_loc16(mem16, cpu_rh[a]);
}



// MOV32 RaH, ACC
static void gen_mov32_rah_acc(DisasContext *ctx, uint32_t a)
{
    tcg_gen_mov_i32(cpu_rh[a], cpu_acc);
}