
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
    gen_shift_by_pm(b, cpu_p);

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

//MAX AX,loc16
static void gen_max_ax_loc16(DisasContext *ctx, uint32_t mode, bool is_AH)
{
    TCGv ax = tcg_temp_local_new_i32();
    TCGv b = tcg_temp_local_new_i32();
    TCGv result = tcg_temp_local_new_i32();
    TCGLabel *done = gen_new_label();

    TCGLabel *begin = gen_new_label();
    TCGLabel *end = gen_new_label();
    gen_set_label(begin);

    gen_ld_reg_half_signed_extend(ax, cpu_acc, is_AH);
    gen_ld_loc16(b, mode);
    tcg_gen_ext16s_tl(b, b);
    tcg_gen_brcond_tl(TCG_COND_GE, ax, b, done);
    if (is_AH)
        gen_st_reg_high_half(cpu_acc, b);
    else
        gen_st_reg_low_half(cpu_acc, b);
    gen_seti_bit(cpu_st0, V_BIT, V_MASK, 1);//if ax<loc16,set V
    gen_set_label(done);
    tcg_gen_sub_i32(result, ax, b);
    gen_helper_test_sub_C_32(cpu_env, ax, b, result);
    gen_helper_test_N_Z_32(cpu_env, result);

    tcg_gen_brcondi_i32(TCG_COND_EQ, cpu_rptc, 0, end);
    tcg_gen_subi_i32(cpu_rptc, cpu_rptc, 1);
    tcg_gen_br(begin);
    gen_set_label(end);

    tcg_temp_free(ax);
    tcg_temp_free(b);
    tcg_temp_free(result);
}

//MAXCUL P,loc32
static void gen_maxcul_p_loc32(DisasContext *ctx, uint32_t mode)
{
    TCGv a = tcg_temp_new_i32();
    gen_ld_loc32(a, mode);
    gen_helper_maxcul_p_loc32(cpu_env, a);
}

//MAXL ACC,loc32
static void gen_maxl_acc_loc32(DisasContext *ctx, uint32_t mode)
{
    TCGv a = tcg_temp_local_new_i32();
    TCGv b = tcg_temp_local_new_i32();
    TCGv result = tcg_temp_local_new_i32();
    TCGLabel *done = gen_new_label();

    TCGLabel *begin = gen_new_label();
    TCGLabel *end = gen_new_label();
    gen_set_label(begin);

    tcg_gen_mov_i32(a, cpu_acc);
    gen_ld_loc32(b, mode);
    tcg_gen_brcond_tl(TCG_COND_GE, a, b, done);
    tcg_gen_mov_i32(cpu_acc, b);
    gen_seti_bit(cpu_st0, V_BIT, V_MASK, 1);//if acc<loc32,set V
    gen_set_label(done);
    tcg_gen_sub_i32(result, a, b);
    gen_helper_test_sub_C_32(cpu_env, a, b, result);
    gen_helper_test_N_Z_32(cpu_env, result);

    tcg_gen_brcondi_i32(TCG_COND_EQ, cpu_rptc, 0, end);
    tcg_gen_subi_i32(cpu_rptc, cpu_rptc, 1);
    tcg_gen_br(begin);
    gen_set_label(end);

    tcg_temp_free(a);
    tcg_temp_free(b);
    tcg_temp_free(result);
}

//MIN AX,loc16
static void gen_min_ax_loc16(DisasContext *ctx, uint32_t mode, bool is_AH)
{
    TCGv ax = tcg_temp_local_new_i32();
    TCGv b = tcg_temp_local_new_i32();
    TCGv result = tcg_temp_local_new_i32();
    TCGLabel *done = gen_new_label();

    TCGLabel *begin = gen_new_label();
    TCGLabel *end = gen_new_label();
    gen_set_label(begin);

    gen_ld_reg_half_signed_extend(ax, cpu_acc, is_AH);
    gen_ld_loc16(b, mode);
    tcg_gen_ext16s_tl(b, b);
    tcg_gen_brcond_tl(TCG_COND_LE, ax, b, done);
    if (is_AH)
        gen_st_reg_high_half(cpu_acc, b);
    else
        gen_st_reg_low_half(cpu_acc, b);
    gen_seti_bit(cpu_st0, V_BIT, V_MASK, 1);//if ax>loc16,set V
    gen_set_label(done);
    tcg_gen_sub_i32(result, ax, b);
    gen_helper_test_sub_C_32(cpu_env, ax, b, result);
    gen_helper_test_N_Z_32(cpu_env, result);

    tcg_gen_brcondi_i32(TCG_COND_EQ, cpu_rptc, 0, end);
    tcg_gen_subi_i32(cpu_rptc, cpu_rptc, 1);
    tcg_gen_br(begin);
    gen_set_label(end);

    tcg_temp_free(ax);
    tcg_temp_free(b);
    tcg_temp_free(result);
}

//MINCUL P,loc32
static void gen_mincul_p_loc32(DisasContext *ctx, uint32_t mode)
{
    TCGv a = tcg_temp_new_i32();
    gen_ld_loc32(a, mode);
    gen_helper_mincul_p_loc32(cpu_env, a);
}

//MINL ACC,loc32
static void gen_minl_acc_loc32(DisasContext *ctx, uint32_t mode)
{
    TCGv a = tcg_temp_local_new_i32();
    TCGv b = tcg_temp_local_new_i32();
    TCGv result = tcg_temp_local_new_i32();
    TCGLabel *done = gen_new_label();

    TCGLabel *begin = gen_new_label();
    TCGLabel *end = gen_new_label();
    gen_set_label(begin);

    tcg_gen_mov_i32(a, cpu_acc);
    gen_ld_loc32(b, mode);
    tcg_gen_brcond_tl(TCG_COND_LE, a, b, done);
    tcg_gen_mov_i32(cpu_acc, b);
    gen_seti_bit(cpu_st0, V_BIT, V_MASK, 1);//if acc>loc32,set V
    gen_set_label(done);
    tcg_gen_sub_i32(result, a, b);
    gen_helper_test_sub_C_32(cpu_env, a, b, result);
    gen_helper_test_N_Z_32(cpu_env, result);

    tcg_gen_brcondi_i32(TCG_COND_EQ, cpu_rptc, 0, end);
    tcg_gen_subi_i32(cpu_rptc, cpu_rptc, 1);
    tcg_gen_br(begin);
    gen_set_label(end);

    tcg_temp_free(a);
    tcg_temp_free(b);
    tcg_temp_free(result);
}

//NORM ACC,*ind/XARn++/XARn--
//type =0...7 --> xarn--
//type = 8...15 --> xarn++
//type = 184: *; 185:*++; 186:*--; 187:*0++; 188:*0--
static void gen_norm_acc_type(DisasContext *ctx, uint32_t type)
{
    TCGv bit31 = tcg_temp_local_new_i32();
    TCGv bit30 = tcg_temp_local_new_i32();
    TCGLabel *done = gen_new_label();
    TCGLabel *cond_else = gen_new_label();

    TCGLabel *begin = gen_new_label();
    TCGLabel *end = gen_new_label();
    gen_set_label(begin);

    tcg_gen_brcondi_i32(TCG_COND_EQ, cpu_acc, 0, done);
    tcg_gen_shri_i32(bit31, cpu_acc, 31);
    tcg_gen_shri_i32(bit30, cpu_acc, 30);
    tcg_gen_xor_i32(bit31, bit31, bit30);
    tcg_gen_andi_i32(bit31, bit31, 1);
    tcg_gen_brcondi_i32(TCG_COND_NE, bit31, 0, cond_else);
    tcg_gen_shli_i32(cpu_acc, cpu_acc, 1);
    gen_seti_bit(cpu_st0, TC_BIT, TC_MASK, 0);
    if (type < 8)//xarn--
    {
        tcg_gen_subi_i32(cpu_xar[type], cpu_xar[type], 1);
    }
    else if(type < 16)//xarn++
    {
        type = type - 8;
        tcg_gen_addi_i32(cpu_xar[type], cpu_xar[type], 1);
    }
    else if (type <= 188 && type >= 184) //*,*++,*--,*0++,*0--
    {
        gen_get_loc_addr(bit31, type, LOC16);
    }
    tcg_gen_br(done);
    gen_set_label(cond_else);
    gen_seti_bit(cpu_st0, TC_BIT, TC_MASK, 1);
    gen_set_label(done);
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);

    tcg_gen_brcondi_i32(TCG_COND_EQ, cpu_rptc, 0, end);
    tcg_gen_subi_i32(cpu_rptc, cpu_rptc, 1);
    tcg_gen_br(begin);
    gen_set_label(end);

    tcg_temp_free(bit30);
    tcg_temp_free(bit31);
}

//SAT ACC
static void gen_sat_acc(DisasContext *ctx)
{
    TCGLabel *done = gen_new_label();
    TCGLabel *ovc_gt_0 = gen_new_label();
    TCGLabel *ovc_lt_0 = gen_new_label();

    TCGv ovc = cpu_shadow[0];
    gen_get_bit(ovc, cpu_st0, OVC_BIT, OVC_MASK);
    tcg_gen_brcondi_i32(TCG_COND_GT, ovc, 0b011111, ovc_lt_0);
    tcg_gen_brcondi_i32(TCG_COND_GT, ovc, 0, ovc_gt_0);
    //ovc == 0
    gen_seti_bit(cpu_st0, V_BIT, V_MASK, 0);
    tcg_gen_br(done);
    gen_set_label(ovc_gt_0);
    //ovc > 0
    tcg_gen_movi_i32(cpu_acc, 0x7fffffff);
    gen_seti_bit(cpu_st0, V_BIT, V_MASK, 1);
    tcg_gen_br(done);
    gen_set_label(ovc_lt_0);
    //ovc < 0
    tcg_gen_movi_i32(cpu_acc, 0x80000000);
    gen_seti_bit(cpu_st0, V_BIT, V_MASK, 1);
    gen_set_label(done);
    gen_seti_bit(cpu_st0, OVC_BIT, OVC_MASK, 0);
    gen_seti_bit(cpu_st0, C_BIT, C_MASK, 0);
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);

}

//SAT64 ACC:P
static void gen_sat64_acc_p(DisasContext *ctx)
{
    TCGLabel *done = gen_new_label();
    TCGLabel *ovc_gt_0 = gen_new_label();
    TCGLabel *ovc_lt_0 = gen_new_label();

    TCGv ovc = cpu_shadow[0];
    gen_get_bit(ovc, cpu_st0, OVC_BIT, OVC_MASK);
    tcg_gen_brcondi_i32(TCG_COND_GT, ovc, 0b011111, ovc_lt_0);
    tcg_gen_brcondi_i32(TCG_COND_GT, ovc, 0, ovc_gt_0);
    //ovc == 0
    gen_seti_bit(cpu_st0, V_BIT, V_MASK, 0);
    tcg_gen_br(done);
    gen_set_label(ovc_gt_0);
    //ovc > 0
    tcg_gen_movi_i32(cpu_acc, 0x7fffffff);
    tcg_gen_movi_i32(cpu_p, 0xffffffff);
    gen_seti_bit(cpu_st0, V_BIT, V_MASK, 1);
    tcg_gen_br(done);
    gen_set_label(ovc_lt_0);
    //ovc < 0
    tcg_gen_movi_i32(cpu_acc, 0x80000000);
    tcg_gen_movi_i32(cpu_p, 0);
    gen_seti_bit(cpu_st0, V_BIT, V_MASK, 1);
    gen_set_label(done);
    gen_seti_bit(cpu_st0, OVC_BIT, OVC_MASK, 0);
    gen_seti_bit(cpu_st0, C_BIT, C_MASK, 0);
    gen_helper_test_N_Z_64(cpu_env, cpu_acc, cpu_p);

}

//TBIT loc16,#bit
static void gen_tbit_loc16_bit(DisasContext *ctx, uint32_t mode, uint32_t bit)
{
    TCGv tc = cpu_shadow[0];
    gen_ld_loc16(tc, mode);
    tcg_gen_shri_i32(tc, tc, bit);
    tcg_gen_andi_i32(tc, tc, 1);
    gen_set_bit(cpu_st0, TC_BIT, TC_MASK, tc);
}

//TBIT loc16,T
static void gen_tbit_loc16_t(DisasContext *ctx, uint32_t mode)
{
    TCGv tc = cpu_shadow[0];
    TCGv bit = cpu_shadow[1];

    gen_ld_reg_half(bit, cpu_xt, true);
    tcg_gen_andi_i32(bit, bit, 0xf);
    tcg_gen_subfi_i32(bit, 15, bit);

    gen_ld_loc16(tc, mode);
    tcg_gen_shr_i32(tc, tc, bit);
    tcg_gen_andi_i32(tc, tc, 1);
    gen_set_bit(cpu_st0, TC_BIT, TC_MASK, tc);
}

//TCLR loc16,#bit
static void gen_tclr_loc16_bit(DisasContext *ctx, uint32_t mode, uint32_t bit)
{
    TCGv tc = cpu_shadow[0];
    TCGv loc16 = cpu_shadow[1];

    if (is_reg_addressing_mode(mode, LOC16))
    {
        gen_ld_loc16(loc16, mode);
        tcg_gen_shri_i32(tc, loc16, bit);
        tcg_gen_andi_i32(tc, tc, 1);
        gen_set_bit(cpu_st0, TC_BIT, TC_MASK, tc);

        tcg_gen_andi_i32(loc16, loc16, ~(1<<bit));
        gen_st_loc16(mode, loc16);
        gen_test_ax_N_Z(mode);
    }
    else 
    {
        TCGv addr = cpu_shadow[2];
        gen_get_loc_addr(addr, mode, LOC16);
        gen_ld16u_swap(loc16, addr);

        tcg_gen_shri_i32(tc, loc16, bit);
        tcg_gen_andi_i32(tc, tc, 1);
        gen_set_bit(cpu_st0, TC_BIT, TC_MASK, tc);

        tcg_gen_andi_i32(loc16, loc16, ~(1<<bit));
        gen_st16u_swap(loc16, addr);
    }
}

//TEST ACC
static void gen_test_acc(DisasContext *ctx)
{
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
}

//TSET loc16,#bit
static void gen_tset_loc16_bit(DisasContext *ctx, uint32_t mode, uint32_t bit)
{
    TCGv tc = cpu_shadow[0];
    TCGv loc16 = cpu_shadow[1];

    if (is_reg_addressing_mode(mode, LOC16))
    {
        gen_ld_loc16(loc16, mode);
        tcg_gen_shri_i32(tc, loc16, bit);
        tcg_gen_andi_i32(tc, tc, 1);
        gen_set_bit(cpu_st0, TC_BIT, TC_MASK, tc);

        tcg_gen_ori_i32(loc16, loc16, 1<<bit);
        gen_st_loc16(mode, loc16);
        gen_test_ax_N_Z(mode);
    }
    else 
    {
        TCGv addr = cpu_shadow[2];
        gen_get_loc_addr(addr, mode, LOC16);
        gen_ld16u_swap(loc16, addr);

        tcg_gen_shri_i32(tc, loc16, bit);
        tcg_gen_andi_i32(tc, tc, 1);
        gen_set_bit(cpu_st0, TC_BIT, TC_MASK, tc);

        tcg_gen_ori_i32(loc16, loc16, 1<<bit);
        gen_st16u_swap(loc16, addr);
    }
}