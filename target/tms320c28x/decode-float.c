// ABSF32 RaH,RbH
static void gen_absf32_rah_rbh(DisasContext *ctx, uint32_t a, uint32_t b)
{
    //todo
}

// MOV16 mem16, RaH
static void gen_mov16_mem16_rah(DisasContext *ctx, uint32_t mem16, uint32_t a)
{
    // TCGv tmp = cpu_tmp[0];
    // tcg_gen_andi_i32(tmp, cpu_rh[a], 0xffff);

    if(!is_reg_addressing_mode(mem16, LOC16))
    {
        // get low 16bit in helper func
        gen_st_loc16(mem16, cpu_rh[a]);
    }
}

// MOV32 *(0:16bitAddr), loc32
// MOV32 RaH, ACC
// MOV32 RaH, XARn
// MOV32 RaH, XT
static void gen_mov32_addr16_loc32(DisasContext *ctx, uint32_t addr, uint32_t mode)
{
    TCGv tmp = cpu_tmp[0];
    TCGv addr_param = cpu_tmp[1];
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

// MOV32 loc32, *(0:16bitAddr)
// MOV32 ACC, RaH
// MOV32 XARn, RaH
// MOV32 XT, RaH
static void gen_mov32_loc32_addr16(DisasContext *ctx, uint32_t addr, uint32_t loc32)
{
    TCGv tmp = cpu_tmp[0];
    TCGv addr_param = cpu_tmp[0];
    tcg_gen_movi_i32(addr_param, addr);
    gen_ld32u_swap(tmp, addr_param);
    gen_st_loc32(loc32, tmp);
    gen_test_acc_N_Z(loc32);
}

// MOV32 mem32, RaH
static void gen_mov32_mem32_rah(DisasContext *ctx, uint32_t mem32, uint32_t a)
{
    if(!is_reg_addressing_mode(mem32, LOC32))
    {
        gen_st_loc32(mem32, cpu_rh[a]);
    }
}

// MOV32 mem32, STF
static void gen_mov32_mem32_stf(DisasContext *ctx, uint32_t mem32)
{
    if(!is_reg_addressing_mode(mem32, LOC32))
    {
        gen_st_loc32(mem32, cpu_stf);
    }
}

// MOV32 RaH, mem32 {, CNDF} 
static void gen_mov32_rah_mem32_cndf(DisasContext *ctx, uint32_t a, uint32_t mem32, uint32_t cndf)
{
    if(is_reg_addressing_mode(mem32, LOC32))
    {
        return;
    }
    TCGLabel *done = gen_new_label();
    TCGv test = cpu_tmp[0];
    TCGv condf_tcg = cpu_tmp[1];
    TCGv tmp = cpu_tmp[2];
    tcg_gen_movi_i32(condf_tcg, cndf);
    //if (CNDF == TRUE)
    gen_helper_test_condf(test, cpu_env, condf_tcg);
    gen_ld_loc32(tmp, mem32);//if test false, still do the load
    tcg_gen_brcondi_i32(TCG_COND_EQ, test, 0, done);
    //if (CNDF == TRUE) RaH = [mem32]
    tcg_gen_mov_i32(cpu_rh[a], tmp);//save to reg
    gen_sync_fpu_mem(a);//save to mem
    //modify stf
    if (cndf == 15) //UNCF
    {
        gen_test_nf_ni_zf_zi(cpu_rh[a]);
    }
    gen_set_label(done);
    
}

// MOV32 STF, mem32
static void gen_mov32_stf_mem32(DisasContext *ctx, uint32_t mem32)
{
    if(is_reg_addressing_mode(mem32, LOC32))
    {
        return;
    }
    TCGv tmp = cpu_tmp[2];
    gen_ld_loc32(tmp, mem32);
    tcg_gen_andi_i32(tmp, tmp, 0x8000027f);//mask stf
    tcg_gen_mov_i32(cpu_stf, tmp);//save to reg
    gen_sync_fpu_mem(-1);//save to mem
}

// MOVD32 RaH, mem32
static void gen_movd32_rah_mem32(DisasContext *ctx, uint32_t a, uint32_t mem32)
{
    if(is_reg_addressing_mode(mem32, LOC32))
    {
        return;
    }
    TCGv addr = cpu_tmp[0];
    gen_get_loc_addr(addr, mem32, LOC32); //get mem32
    // RaH = [mem32]
    gen_ld32u_swap(cpu_rh[a], addr);
    gen_sync_fpu_mem(a);
    // [mem32+2] = [mem32]
    tcg_gen_addi_i32(addr, addr, 2);
    gen_st32u_swap(cpu_rh[a], addr);
    //modify stf
    gen_test_nf_ni_zf_zi(cpu_rh[a]);
}


// MOVIZ RaH, #16FHiHex
static void gen_moviz_rah_16fhihex(DisasContext *ctx, uint32_t a, uint32_t hi)
{
    //RaH[31:16] = #16FHiHex
    //RaH[15:0] = 0
    hi = hi << 16;
    tcg_gen_movi_i32(cpu_rh[a], hi);
    gen_sync_fpu_mem(a);
}