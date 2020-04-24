
//DMAC ACC:P,loc32,*XAR7/++
static void gen_dmac_accp_loc32_xar7(DisasContext *ctx, uint32_t mode1, uint32_t mode2)
{
    TCGLabel *begin = gen_new_label();
    TCGLabel *end = gen_new_label();
    TCGv v1 = tcg_temp_local_new();
    TCGv v2 = tcg_temp_local_new();
    TCGv v2_msb = tcg_temp_local_new();
    TCGv v2_lsb = tcg_temp_local_new();
    TCGv v1_lsb = tcg_temp_local_new();
    TCGv v1_msb = tcg_temp_local_new();
    TCGv a = tcg_temp_local_new();
    TCGv b = tcg_temp_local_new();
    gen_set_label(begin);

    //0xC7: *XAR7, 0x87: *XAR7++, 0x8F: *--XAR7
    if (mode1 == 0x8f) //*--XAR7
    {
        mode2 = 0xc7;
        gen_ld_loc32(v2, mode2);//get XAR7
        gen_ld_loc32(v1, mode1);//get *--XAR7
    }
    else if (mode1 == 0xc7)//*XAR7
    {
        if (mode2 == 0xc7)//*XAR7
        {
            gen_ld_loc32(v1, mode1);
            tcg_gen_mov_i32(v2, v1);
        }
        else //*XAR7++
        {
            gen_ld_loc32(v1, mode2);
            tcg_gen_mov_i32(v2, v1);
        }
    }
    else if (mode1 == 0x87)//*XAR7++
    {
        gen_ld_loc32(v1, mode2);
        tcg_gen_mov_i32(v2, v1);
    }
    else
    {
        gen_ld_loc32(v2, mode2);
        gen_ld_loc32(v1, mode1);
    }
    tcg_gen_mov_i32(cpu_xt, v1);
    gen_ld_reg_half_signed_extend(v1_msb, v1, true);
    gen_ld_reg_half_signed_extend(v1_lsb, v1, false);
    gen_ld_reg_half_signed_extend(v2_msb, v2, true);
    gen_ld_reg_half_signed_extend(v2_lsb, v2, false);

    tcg_gen_mov_i32(a, cpu_p);
    tcg_gen_mul_i32(b, v1_lsb, v2_lsb);
    gen_shift_by_pm(b, b);
    tcg_gen_add_i32(cpu_p, a, b);

    tcg_gen_mov_i32(a, cpu_acc);
    tcg_gen_mul_i32(b, v1_msb, v2_msb);
    gen_shift_by_pm(b, b);
    tcg_gen_add_i32(cpu_acc, a, b);


    gen_helper_test_V_32(cpu_env, a, b, cpu_acc);

    tcg_gen_brcondi_i32(TCG_COND_EQ, cpu_rptc, 0, end);
    tcg_gen_subi_i32(cpu_rptc, cpu_rptc, 1);
    tcg_gen_br(begin);

    gen_set_label(end);
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
    gen_helper_test_C_32(cpu_env, a, b, cpu_acc);
    gen_helper_test_OVC_OVM_32(cpu_env, a, b, cpu_acc);
    tcg_temp_free(a);
    tcg_temp_free(b);
    tcg_temp_free(v1);
    tcg_temp_free(v2); 
    tcg_temp_free(v1_lsb); 
    tcg_temp_free(v1_msb);
    tcg_temp_free(v2_lsb); 
    tcg_temp_free(v2_msb); 
}

//IMACL P,loc32,*XAR7/++
static void gen_imacl_p_loc32_xar7(DisasContext *ctx, uint32_t mode1, uint32_t mode2)
{
    TCGv v1 = tcg_temp_local_new();
    TCGv v2 = tcg_temp_local_new();
    TCGv a = tcg_temp_local_new();
    TCGv b = tcg_temp_local_new();
    TCGv temp_low = tcg_temp_local_new();
    TCGv temp_high = tcg_temp_local_new();
    TCGLabel *begin = gen_new_label();
    TCGLabel *end = gen_new_label();
    gen_set_label(begin);

    //0xC7: *XAR7, 0x87: *XAR7++, 0x8F: *--XAR7
    if (mode1 == 0x8f) //*--XAR7
    {
        mode2 = 0xc7;
        gen_ld_loc32(v2, mode2);//get XAR7
        gen_ld_loc32(v1, mode1);//get *--XAR7
    }
    else if (mode1 == 0xc7)//*XAR7
    {
        if (mode2 == 0xc7)//*XAR7
        {
            gen_ld_loc32(v1, mode1);
            tcg_gen_mov_i32(v2, v1);
        }
        else //*XAR7++
        {
            gen_ld_loc32(v1, mode2);
            tcg_gen_mov_i32(v2, v1);
        }
    }
    else if (mode1 == 0x87)//*XAR7++
    {
        gen_ld_loc32(v1, mode2);
        tcg_gen_mov_i32(v2, v1);
    }
    else
    {
        gen_ld_loc32(v2, mode2);
        gen_ld_loc32(v1, mode1);
    }
    tcg_gen_mov_i32(a, cpu_acc);
    tcg_gen_mov_i32(b, cpu_p);
    tcg_gen_add_i32(cpu_acc, a, b);
    gen_helper_test_V_32(cpu_env, a, b, cpu_acc);//acc = acc + unsigned p
    tcg_gen_muls2_i32(temp_low, temp_high, v1, v2);//temp(37:0) = lower 38bits ...
    gen_shift_by_pm2(cpu_p, temp_low, temp_high);

    tcg_gen_brcondi_i32(TCG_COND_EQ, cpu_rptc, 0, end);
    tcg_gen_subi_i32(cpu_rptc, cpu_rptc, 1);
    tcg_gen_br(begin);
    gen_set_label(end);

    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
    gen_helper_test_C_32(cpu_env, a, b, cpu_acc);
    gen_helper_test_OVC_OVM_32(cpu_env, a, b, cpu_acc);

    tcg_temp_free(v1);
    tcg_temp_free(v2);
    tcg_temp_free(a);
    tcg_temp_free(b);
    tcg_temp_free(temp_low);
    tcg_temp_free(temp_high);
}

//IMPYAL P,XT,loc32
static void gen_impyal_p_xt_loc32(DisasContext *ctx, uint32_t mode)
{
    TCGv v2 = tcg_temp_new();
    TCGv a = tcg_temp_local_new();
    TCGv b = tcg_temp_local_new();
    TCGv temp_low = tcg_temp_local_new();
    TCGv temp_high = tcg_temp_local_new();

    gen_ld_loc32(v2, mode);
    
    tcg_gen_mov_i32(a, cpu_acc);
    tcg_gen_mov_i32(b, cpu_p);
    tcg_gen_add_i32(cpu_acc, a, b);
    tcg_gen_muls2_i32(temp_low, temp_high, cpu_xt, v2);//temp(37:0) = lower 38bits ...
    gen_shift_by_pm2(cpu_p, temp_low, temp_high);

    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
    gen_helper_test_C_V_32(cpu_env, a, b, cpu_acc);
    gen_helper_test_OVC_OVM_32(cpu_env, a, b, cpu_acc);

    tcg_temp_free(v2);
    tcg_temp_free(a);
    tcg_temp_free(b);
    tcg_temp_free(temp_low);
    tcg_temp_free(temp_high);
}

//IMPYL ACC,XT,loc32
static void gen_impyl_acc_xt_loc32(DisasContext *ctx, uint32_t mode)
{
    TCGv b = tcg_temp_new();
    gen_ld_loc32(b, mode);
    tcg_gen_mul_i32(cpu_acc, cpu_xt, b);

    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
}

//IMPYL P,XT,loc32
static void gen_impyl_p_xt_loc32(DisasContext *ctx, uint32_t mode)
{
    TCGv v2 = tcg_temp_new();
    TCGv temp_low = tcg_temp_local_new();
    TCGv temp_high = tcg_temp_local_new();

    gen_ld_loc32(v2, mode);
    
    tcg_gen_muls2_i32(temp_low, temp_high, cpu_xt, v2);//temp(37:0) = lower 38bits ...
    gen_shift_by_pm2(cpu_p, temp_low, temp_high);

    tcg_temp_free(v2);
    tcg_temp_free(temp_low);
    tcg_temp_free(temp_high);
}

//IMPYSL P,XT,loc32
static void gen_impysl_p_xt_loc32(DisasContext *ctx, uint32_t mode)
{
    TCGv v2 = tcg_temp_new();
    TCGv a = tcg_temp_local_new();
    TCGv b = tcg_temp_local_new();
    TCGv temp_low = tcg_temp_local_new();
    TCGv temp_high = tcg_temp_local_new();

    gen_ld_loc32(v2, mode);
    
    tcg_gen_mov_i32(a, cpu_acc);
    tcg_gen_mov_i32(b, cpu_p);
    tcg_gen_sub_i32(cpu_acc, a, b);
    tcg_gen_muls2_i32(temp_low, temp_high, cpu_xt, v2);//temp(37:0) = lower 38bits ...
    gen_shift_by_pm2(cpu_p, temp_low, temp_high);

    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
    gen_helper_test_sub_C_V_32(cpu_env, a, b, cpu_acc);
    gen_helper_test_sub_OVCU_32(cpu_env, a, b, cpu_acc);

    tcg_temp_free(v2);
    tcg_temp_free(a);
    tcg_temp_free(b);
    tcg_temp_free(temp_low);
    tcg_temp_free(temp_high);
}

//IMPYXUL P,XT,loc32
static void gen_impyxul_p_xt_loc32(DisasContext *ctx, uint32_t mode)
{
    TCGv b = tcg_temp_new();
    TCGv tmp = tcg_temp_new();
    TCGv tmp2 = tcg_temp_new();
    TCGv temp_low = tcg_temp_local_new();
    TCGv temp_high = tcg_temp_local_new();

    gen_ld_loc32(b, mode);
    tcg_gen_shri_i32(tmp, b, 31);//tmp is sign bit of b
    tcg_gen_andi_i32(b, b, 0x7fffffff);//clear sign bit of b
    
    tcg_gen_muls2_i32(temp_low, temp_high, cpu_xt, b);//temp(37:0) = lower 38bits ...
    tcg_gen_mul_i32(tmp, cpu_xt, tmp);//tmp = sign_bit_b * cpu_xt
    tcg_gen_shli_i32(tmp2, tmp, 31);//low = cpuxt_[0] at 32bit
    tcg_gen_shri_i32(tmp, tmp, 1);//high = cpu_xt[30:1]
    tcg_gen_add2_i32(temp_low, temp_high, temp_low, temp_high, tmp2, tmp);//64bit add

    gen_shift_by_pm2(cpu_p, temp_low, temp_high);

    tcg_temp_free(b);
    tcg_temp_free(temp_low);
    tcg_temp_free(temp_high);
}


//MAC P,loc16,0:pma
static void gen_mac_p_loc16_pma(DisasContext *ctx, uint32_t mode, uint32_t addr)
{
    TCGLabel *begin = gen_new_label();
    TCGLabel *end = gen_new_label();
    TCGv v1 = tcg_temp_local_new();
    TCGv v2 = tcg_temp_local_new();
    TCGv addr_tcg = tcg_const_local_i32(addr);
    TCGv a = tcg_temp_local_new();
    TCGv b = tcg_temp_local_new();

    gen_set_label(begin);

    gen_shift_by_pm(b, cpu_p);//b = P<<PM
    tcg_gen_mov_i32(a, cpu_acc);//a = ACC
    tcg_gen_add_i32(cpu_acc, a, b); //ACC = ACC + P<<PM

    gen_ld_loc16(v1, mode);
    tcg_gen_ext16s_tl(v1, v1);//sigend extend
    gen_ld16u_swap(v2, addr_tcg);
    tcg_gen_ext16s_tl(v2, v2);//signed extend
    tcg_gen_mul_i32(cpu_p, v1, v2);//P = signed T * signed Prog[*XAR7 or *XAR7++], use v1 instead of T
    gen_st_reg_high_half(cpu_xt, v1);//T = [loc16], store v1 to T
    tcg_gen_addi_i32(addr_tcg, addr_tcg, 1);//inc pma by 1, for each repetition
    gen_helper_test_V_32(cpu_env, a, b, cpu_acc);

    tcg_gen_brcondi_i32(TCG_COND_EQ, cpu_rptc, 0, end);
    tcg_gen_subi_i32(cpu_rptc, cpu_rptc, 1);
    tcg_gen_br(begin);

    gen_set_label(end);
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
    gen_helper_test_C_32(cpu_env, a, b, cpu_acc);
    gen_helper_test_OVC_OVM_32(cpu_env, a, b, cpu_acc);
    tcg_temp_free(a);
    tcg_temp_free(b);    
    tcg_temp_free(v1);
    tcg_temp_free(v2);
    tcg_temp_free(addr_tcg);
}

//MAC P,loc16,*XAR7/++
static void gen_mac_p_loc16_xar7(DisasContext *ctx, uint32_t mode1, uint32_t mode2)
{
    TCGLabel *begin = gen_new_label();
    TCGLabel *end = gen_new_label();
    TCGv a = tcg_temp_local_new();
    TCGv b = tcg_temp_local_new();
    TCGv v1 = tcg_temp_local_new();
    TCGv v2 = tcg_temp_local_new();

    gen_set_label(begin);
    gen_shift_by_pm(b, cpu_p);//b = P<<PM
    tcg_gen_mov_i32(a, cpu_acc);//a = ACC
    tcg_gen_add_i32(cpu_acc, a, b); //ACC = ACC + P<<PM

    //0xC7: *XAR7, 0x87: *XAR7++, 0x8F: *--XAR7
    if (mode1 == 0x8f) //*--XAR7
    {
        mode2 = 0xc7;
        gen_ld_loc16(v2, mode2);//get XAR7
        gen_ld_loc16(v1, mode1);//get *--XAR7
    }
    else if (mode1 == 0xc7)//*XAR7
    {
        if (mode2 == 0xc7)//*XAR7
        {
            gen_ld_loc16(v1, mode1);
            tcg_gen_mov_i32(v2, v1);
        }
        else //*XAR7++
        {
            gen_ld_loc16(v1, mode2);
            tcg_gen_mov_i32(v2, v1);
        }
    }
    else if (mode1 == 0x87)//*XAR7++
    {
        gen_ld_loc16(v1, mode2);
        tcg_gen_mov_i32(v2, v1);
    }
    else
    {
        gen_ld_loc16(v2, mode2);
        gen_ld_loc16(v1, mode1);
    }
    tcg_gen_ext16s_tl(v1, v1);//sigend extend
    tcg_gen_ext16s_tl(v2, v2);//signed extend
    tcg_gen_mul_i32(cpu_p, v1, v2);//P = signed T * signed Prog[*XAR7 or *XAR7++], use v1 instead of T
    gen_st_reg_high_half(cpu_xt, v1);//T = [loc16], store v1 to T

    gen_helper_test_V_32(cpu_env, a, b, cpu_acc);

    tcg_gen_brcondi_i32(TCG_COND_EQ, cpu_rptc, 0, end);
    tcg_gen_subi_i32(cpu_rptc, cpu_rptc, 1);
    tcg_gen_br(begin);

    gen_set_label(end);
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
    gen_helper_test_C_32(cpu_env, a, b, cpu_acc);
    gen_helper_test_OVC_OVM_32(cpu_env, a, b, cpu_acc);
    tcg_temp_free(a);
    tcg_temp_free(b);
    tcg_temp_free(v1);
    tcg_temp_free(v2);
}

//MPY ACC,loc16,#16bit
static void gen_mpy_acc_loc16_16bit(DisasContext *ctx, uint32_t mode, uint32_t imm)
{
    int32_t oprand = (int16_t)imm;
    TCGv t  = tcg_temp_new_i32();
    gen_ld_loc16(t, mode);
    tcg_gen_ext16s_tl(t, t);//sigend extend
    //T = [loc16]
    gen_st_reg_high_half(cpu_xt, t);
    //ACC = signed T * signed 16bit
    tcg_gen_muli_i32(cpu_acc, t, oprand);
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
    //free
    tcg_temp_free(t);
}

//MPY ACC,T,loc16
static void gen_mpy_acc_t_loc16(DisasContext *ctx, uint32_t mode)
{
    TCGv t  = tcg_temp_new_i32();
    TCGv oprand  = tcg_temp_new_i32();
    //load T
    gen_ld_reg_half_signed_extend(t, cpu_xt, true);
    //load loc16
    gen_ld_loc16(oprand, mode);
    tcg_gen_ext16s_tl(oprand, oprand);//sigend extend
    //ACC = signed  T * signed [loc16]
    tcg_gen_mul_i32(cpu_acc, t, oprand);
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
    //free
    tcg_temp_free(t);
    tcg_temp_free(oprand);
}

//MPY P,loc16,#16bit
static void gen_mpy_p_loc16_16bit(DisasContext *ctx, uint32_t mode, uint32_t imm)
{
    int32_t oprand = (int16_t)imm;
    TCGv t  = tcg_temp_new_i32();
    gen_ld_loc16(t, mode);
    tcg_gen_ext16s_tl(t, t);//sigend extend
    //P = signed [loc16] * signed 16bit
    tcg_gen_muli_i32(cpu_p, t, oprand);
    //free
    tcg_temp_free(t);
}

//MPY P,T,loc16
static void gen_mpy_p_t_loc16(DisasContext *ctx, uint32_t mode)
{
    TCGv t  = tcg_temp_new_i32();
    TCGv oprand  = tcg_temp_new_i32();
    //load T
    gen_ld_reg_half_signed_extend(t, cpu_xt, true);
    //load loc16
    gen_ld_loc16(oprand, mode);
    tcg_gen_ext16s_tl(oprand, oprand);//sigend extend
    //P = signed  T * signed [loc16]
    tcg_gen_mul_i32(cpu_p, t, oprand);
    //free
    tcg_temp_free(t);
    tcg_temp_free(oprand);
}

//MPYA P,loc16,#16bit
static void gen_mpya_p_loc16_16bit(DisasContext *ctx, uint32_t mode, uint32_t imm)
{
    int32_t oprand = (int16_t)imm;
    TCGv a  = tcg_temp_local_new_i32();
    TCGv b  = tcg_temp_local_new_i32();
    TCGv t  = tcg_temp_new_i32();
    //ACC = ACC + P<<PM
    tcg_gen_mov_i32(a, cpu_acc);//a = acc
    gen_shift_by_pm(b, cpu_p);//b = p<<pm
    tcg_gen_add_i32(cpu_acc, a, b);
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
    gen_helper_test_C_V_32(cpu_env, a, b, cpu_acc);
    gen_helper_test_OVC_OVM_32(cpu_env, a, b, cpu_acc);
    //load loc16
    gen_ld_loc16(t, mode);
    gen_st_reg_high_half(cpu_xt, t);//T = [loc16]
    tcg_gen_ext16s_tl(t, t);//sigend extend
    //P = signed [loc16] * signed 16bit
    tcg_gen_muli_i32(cpu_p, t, oprand);
    //free
    tcg_temp_free(t);
    tcg_temp_free(a);
    tcg_temp_free(b);
}

//MPYA P,T,loc16
static void gen_mpya_p_t_loc16(DisasContext *ctx, uint32_t mode)
{
    TCGv a  = tcg_temp_local_new_i32();
    TCGv b  = tcg_temp_local_new_i32();

    TCGLabel *begin = gen_new_label();
    TCGLabel *end = gen_new_label();
    gen_set_label(begin);

    //ACC = ACC + P<<PM
    tcg_gen_mov_i32(a, cpu_acc);//a = acc
    gen_shift_by_pm(b, cpu_p);//b = p<<pm
    tcg_gen_add_i32(cpu_acc, a, b);
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
    gen_helper_test_C_V_32(cpu_env, a, b, cpu_acc);
    gen_helper_test_OVC_OVM_32(cpu_env, a, b, cpu_acc);
    //P = signed T * signed [loc16]
    gen_ld_reg_half_signed_extend(a, cpu_xt, true);//a = T, signed extend
    gen_ld_loc16(b, mode);
    tcg_gen_ext16s_tl(b, b);//b = loc16, signed extend
    tcg_gen_mul_i32(cpu_p, a, b);

    tcg_gen_brcondi_i32(TCG_COND_EQ, cpu_rptc, 0, end);
    tcg_gen_subi_i32(cpu_rptc, cpu_rptc, 1);
    tcg_gen_br(begin);
    gen_set_label(end);
    //free
    tcg_temp_free(a);
    tcg_temp_free(b);
}

//MPYB ACC,T,#8bit
static void gen_mpyb_acc_t_8bit(DisasContext *ctx, uint32_t imm)
{
    TCGv t = tcg_temp_new_i32();
    gen_ld_reg_half_signed_extend(t, cpu_xt, true);//T, signed extend
    tcg_gen_muli_i32(cpu_acc, t, imm);
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
    tcg_temp_free(t);
}

//MPYB P,T,#8bit
static void gen_mpyb_p_t_8bit(DisasContext *ctx, uint32_t imm)
{
    TCGv t = tcg_temp_new_i32();
    gen_ld_reg_half_signed_extend(t, cpu_xt, true);//T, signed extend
    tcg_gen_muli_i32(cpu_p, t, imm);
    tcg_temp_free(t);
}

//MPYS P,T,loc16
static void gen_mpys_p_t_loc16(DisasContext *ctx, uint32_t mode)
{
    TCGv a  = tcg_temp_local_new_i32();
    TCGv b  = tcg_temp_local_new_i32();

    TCGLabel *begin = gen_new_label();
    TCGLabel *end = gen_new_label();
    gen_set_label(begin);

    //ACC = ACC - P<<PM
    tcg_gen_mov_i32(a, cpu_acc);//a = acc
    gen_shift_by_pm(b, cpu_p);//b = p<<pm
    tcg_gen_sub_i32(cpu_acc, a, b);
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
    gen_helper_test_sub_C_V_32(cpu_env, a, b, cpu_acc);
    gen_helper_test_sub_OVC_OVM_32(cpu_env, a, b, cpu_acc);
    //P = signed T * signed [loc16]
    gen_ld_reg_half_signed_extend(a, cpu_xt, true);//a = T, signed extend
    gen_ld_loc16(b, mode);
    tcg_gen_ext16s_tl(b, b);//b = loc16, signed extend
    tcg_gen_mul_i32(cpu_p, a, b);

    tcg_gen_brcondi_i32(TCG_COND_EQ, cpu_rptc, 0, end);
    tcg_gen_subi_i32(cpu_rptc, cpu_rptc, 1);
    tcg_gen_br(begin);
    gen_set_label(end);
    //free
    tcg_temp_free(a);
    tcg_temp_free(b);
}
