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

// MOV32 *(0:16bitAddr), loc32
// MOV32 RaH, ACC
static void gen_mov32_addr16_loc32(DisasContext *ctx, uint32_t addr, uint32_t mode)
{
    TCGv tmp = cpu_shadow[0];
    TCGv addr_param = cpu_shadow[1];
    tcg_gen_movi_i32(addr_param, addr);
    //load loc32
    gen_ld_loc32(tmp, mode);
    //mask stf
    gen_mask_st32_stf(addr, tmp);
    //store
    gen_st32u_swap(tmp, addr_param);
    //sync mem with fpu register
    gen_sync_fpu_reg(addr, tmp);
}