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
    // get low 16bit in helper func
    gen_st_loc16(mem16, cpu_rh[a]);
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
    gen_st_loc32(mem32, cpu_rh[a]);
}

// MOV32 mem32, STF
static void gen_mov32_mem32_stf(DisasContext *ctx, uint32_t mem32)
{
    gen_st_loc32(mem32, cpu_stf);
}

// MOV32 RaH, mem32 {, CNDF} 
static void gen_mov32_rah_mem32_cndf(DisasContext *ctx, uint32_t a, uint32_t mem32, uint32_t cndf)
{
    TCGLabel *done = gen_new_label();
    TCGv test = cpu_tmp[0];
    TCGv condf_tcg = cpu_tmp[1];
    TCGv tmp = cpu_tmp[2];
    TCGv tmp2 = cpu_tmp[3];
    tcg_gen_movi_i32(condf_tcg, cndf);
    //if (CNDF == TRUE)
    gen_helper_test_condf(test, cpu_env, condf_tcg);
    gen_ld_loc32(tmp, mem32);//if test false, still do the load
    tcg_gen_brcondi_i32(TCG_COND_EQ, test, 0, done);
    //if (CNDF == TRUE) RaH = [mem32]
    tcg_gen_mov_i32(cpu_rh[a], tmp);//save to reg
    gen_sync_fpu_mem(a);//save to mem
    //modify stf
    TCGLabel *rah3023_not_zero = gen_new_label();
    TCGLabel *rah_not_zero = gen_new_label();
    if (cndf == 15) //UNCF
    {
        tcg_gen_andi_i32(tmp, cpu_rh[a], 0x7f800000);//get rh[30:23]
        tcg_gen_shri_i32(tmp2, cpu_rh[a], 31);//rah[31]
        // NF = RaH(31);
        gen_set_bit(cpu_stf, NF_BIT, NF_MASK, tmp2);
        // ZF = 0;
        gen_seti_bit(cpu_stf, ZF_BIT, ZF_MASK, 0);
        // if(RaH[30:23] == 0) { ZF = 1; NF = 0; }
        tcg_gen_brcondi_i32(TCG_COND_NE, tmp, 0, rah3023_not_zero);
        gen_seti_bit(cpu_stf, NF_BIT, NF_MASK, 0);
        gen_seti_bit(cpu_stf, ZF_BIT, ZF_MASK, 1);
        gen_set_label(rah3023_not_zero);
        // NI = RaH[31];
        gen_set_bit(cpu_stf, NI_BIT, NI_MASK, tmp2);
        // ZI = 0;
        gen_seti_bit(cpu_stf, ZI_BIT, ZI_MASK, 0);
        // if(RaH[31:0] == 0) ZI = 1;
        tcg_gen_brcondi_i32(TCG_COND_NE, cpu_rh[a], 0, rah_not_zero);
        gen_seti_bit(cpu_stf, ZI_BIT, ZI_MASK, 1);
        gen_set_label(rah_not_zero);

    }
    gen_set_label(done);
    
}

// MOV32 STF, mem32
static void gen_mov32_stf_mem32(DisasContext *ctx, uint32_t mem32)
{
    TCGv tmp = cpu_tmp[2];
    gen_ld_loc32(tmp, mem32);
    tcg_gen_andi_i32(tmp, tmp, 0x8000027f);//mask stf
    tcg_gen_mov_i32(cpu_stf, tmp);//save to reg
    gen_sync_fpu_mem(-1);//save to mem
}