
// CMP AX,loc16
static void gen_cmp_ax_loc16(DisasContext *ctx, uint32_t mode, bool is_AH)
{
    TCGv a = tcg_temp_new();
    gen_ld_reg_half(a, cpu_acc, is_AH);

    TCGv b = tcg_temp_new();
    gen_ld_loc16(b, mode);

    gen_helper_cmp16_N_Z_C(cpu_env, a, b);

    tcg_temp_free(a);
    tcg_temp_free(b);
}

// CMP loc16,#16bitSigned
static void gen_cmp_loc16_16bit(DisasContext *ctx, uint32_t mode, uint32_t imm)
{
    TCGv a = tcg_temp_new();
    gen_ld_loc16(a, mode);

    TCGv b = tcg_const_i32(imm);

    gen_helper_cmp16_N_Z_C(cpu_env, a, b);

    tcg_temp_free(a);
    tcg_temp_free(b);
}

// CMP64 ACC:P
static void gen_cmp64_acc_p(DisasContext *ctx)
{
    gen_helper_cmp64_acc_p(cpu_env);
}

// CMPB AX,#8bit
static void gen_cmp_ax_8bit(DisasContext *ctx, uint32_t imm, bool is_AH)
{
    TCGv a = tcg_temp_new();
    gen_ld_reg_half(a, cpu_acc, is_AH);

    TCGv b = tcg_const_i32(imm);

    gen_helper_cmp16_N_Z_C(cpu_env, a, b);

    tcg_temp_free(a);
    tcg_temp_free(b);
}

// CMPL ACC,loc32
static void gen_cmpl_acc_loc32(DisasContext *ctx, uint32_t mode)
{
    TCGv b = tcg_temp_new();
    gen_ld_loc32(b, mode);

    gen_helper_cmp32_N_Z_C(cpu_env, cpu_acc, b);

    tcg_temp_free(b);
}

// CMPL ACC,P<<PM
static void gen_cmpl_acc_p_pm(DisasContext *ctx)
{
    TCGv b = tcg_temp_new();
    gen_helper_shift_by_pm(b, cpu_env, cpu_p);

    gen_helper_cmp32_N_Z_C(cpu_env, cpu_acc, b);
    tcg_temp_free(b);
}

// CMPR 0/1/2/3
static void gen_cmpr_0123(DisasContext *ctx, uint32_t mode)
{
    uint32_t COND = -1;
    if (mode == 0)
    {
        COND = TCG_COND_EQ;
    }
    else if (mode == 1)
    {
        COND = TCG_COND_GT;
    }
    else if (mode == 2)
    {
        COND = TCG_COND_LT;
    }
    else if (mode == 3)
    {
        COND = TCG_COND_NE;
    }

    if (COND != -1)
    {
        TCGv ar0 = tcg_temp_new();
        gen_ld_reg_half(ar0, cpu_xar[0], 0);
        TCGv arn = tcg_temp_new();
        gen_helper_ld_xar_arp(arn, cpu_env);
        TCGLabel *label = gen_new_label();
        TCGLabel *done = gen_new_label();

        tcg_gen_brcond_i32(COND, ar0, arn, label);
        gen_seti_bit(cpu_st0, TC_BIT, TC_MASK, 0);
        tcg_gen_br(done);
        gen_set_label(label);
        gen_seti_bit(cpu_st0, TC_BIT, TC_MASK, 1);
        gen_set_label(done);
        tcg_temp_free(ar0);
        tcg_temp_free(arn);
    }
    else
    {
        gen_exception(ctx, EXCP_INTERRUPT_ILLEGAL);
    }
}

// CSB ACC
static void gen_csb_acc(DisasContext *ctx)
{
    TCGv sign_bit = tcg_temp_local_new();
    tcg_gen_shri_i32(sign_bit, cpu_acc, 31);//get acc sign bit
    TCGv compare_bit = tcg_temp_local_new();
    TCGv temp_acc = tcg_temp_local_new();
    tcg_gen_mov_i32(temp_acc, cpu_acc);
    TCGv count = tcg_const_local_i32(0);

    TCGLabel *repeat = gen_new_label();
    gen_set_label(repeat);
    tcg_gen_addi_i32(count, count, 1);
    tcg_gen_shli_i32(temp_acc, temp_acc, 1);
    tcg_gen_shri_i32(compare_bit, temp_acc, 31);
    tcg_gen_brcond_i32(TCG_COND_EQ, sign_bit, compare_bit, repeat);
    tcg_gen_subi_i32(count, count, 1);
    gen_st_reg_high_half(cpu_xt, count);

    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
    //test TC
    gen_set_bit(cpu_st0, TC_BIT, TC_MASK, sign_bit);
}

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
    gen_helper_shift_by_pm(b, cpu_env, b);
    tcg_gen_add_i32(cpu_p, a, b);

    tcg_gen_mov_i32(a, cpu_acc);
    tcg_gen_mul_i32(b, v1_msb, v2_msb);
    gen_helper_shift_by_pm(b, cpu_env, b);
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

// //IMACL P,loc32,*XAR7/++
// static void gen_imacl_p_loc32_xar7(DisasContext *ctx, uint32_t mode1, uint32_t mode2)
// {


//     TCGLabel *begin = gen_new_label();
//     TCGLabel *end = gen_new_label();
//     gen_set_label(begin);



//     tcg_gen_brcondi_i32(TCG_COND_EQ, cpu_rptc, 0, end);
//     tcg_gen_subi_i32(cpu_rptc, cpu_rptc, 1);
//     tcg_gen_br(begin);
//     gen_set_label(end);
// }

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

    gen_helper_shift_by_pm(b, cpu_env, cpu_p);//b = P<<PM
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
    gen_helper_shift_by_pm(b, cpu_env, cpu_p);//b = P<<PM
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