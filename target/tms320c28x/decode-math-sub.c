// DEC loc16
static void gen_dec_loc16(DisasContext *ctx, uint32_t mode)
{
    TCGv a = tcg_temp_new();
    TCGv b = tcg_const_i32(1);
    TCGv result = tcg_temp_new();

    if (is_reg_addressing_mode(mode, LOC16)) {
        gen_ld_loc16(a, mode);
        tcg_gen_sub_i32(result, a, b);//sub
        gen_st_loc16(mode, result);//store
        gen_helper_test_sub_C_V_16(cpu_env, a, b, result);
        gen_helper_test_N_Z_16(cpu_env, result);
    }
    else {
        TCGv_i32 addr = tcg_temp_new();
        gen_get_loc_addr(addr, mode, LOC16);
        gen_ld16u_swap(a, addr);

        tcg_gen_sub_i32(result, a, b);//sub

        gen_helper_test_sub_C_V_16(cpu_env, a, b, result);
        gen_helper_test_N_Z_16(cpu_env, result);
        
        gen_st16u_swap(result, addr); //store

        tcg_temp_free_i32(addr);
    }

    tcg_temp_free_i32(a);
    tcg_temp_free_i32(b);
    tcg_temp_free_i32(result);
}

// SBBU ACC,loc16
static void gen_sbbu_acc_loc16(DisasContext *ctx, uint32_t mode)
{
    TCGv c = cpu_shadow[0];
    TCGv loc16 = cpu_shadow[1];
    TCGv acc = cpu_shadow[2];

    tcg_gen_mov_i32(acc, cpu_acc);
    gen_ld_loc16(loc16, mode);
    gen_get_bit(c, cpu_st0, C_BIT, C_MASK);
    tcg_gen_not_i32(c, c);
    tcg_gen_andi_i32(c, c, 1);

    tcg_gen_sub_i32(cpu_acc, acc, loc16);
    tcg_gen_sub_i32(cpu_acc, cpu_acc, c);

    gen_helper_test2_sub_C_V_OVC_OVM_32(cpu_env, acc, loc16, c, cpu_acc);
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
}

// SBRK #8bit
static void gen_sbrk_8bit(DisasContext *ctx, uint32_t imm)
{
    TCGv tmp = cpu_shadow[0];
    gen_helper_ld_xar_arp(tmp, cpu_env);
    tcg_gen_subi_i32(tmp, tmp, imm);
    gen_helper_st_xar_arp(cpu_env, tmp);
}

// SUBB ACC,loc16<<#0...16
static void gen_subb_acc_loc16_shift(DisasContext *ctx, uint32_t mode, uint32_t shift_imm)
{
    TCGv a = tcg_temp_local_new();
    TCGv b = tcg_temp_local_new();
    TCGLabel *begin = gen_new_label();
    TCGLabel *end = gen_new_label();
    gen_set_label(begin);

    tcg_gen_mov_i32(a, cpu_acc);
    gen_ld_loc16(b, mode);
    gen_helper_extend_low_sxm(b, cpu_env, b);
    tcg_gen_shli_i32(b, b, shift_imm);

    tcg_gen_sub_i32(cpu_acc, a, b);

    gen_helper_test_sub_V_32(cpu_env, a, b, cpu_acc);
    gen_helper_test_sub_OVC_OVM_32(cpu_env, a, b, cpu_acc);

    tcg_gen_brcondi_i32(TCG_COND_EQ, cpu_rptc, 0, end);
    tcg_gen_subi_i32(cpu_rptc, cpu_rptc, 1);
    tcg_gen_br(begin);
    gen_set_label(end);

    if (shift_imm == 16)
        gen_helper_test_sub_C_32_shift16(cpu_env, a, b, cpu_acc);
    else
        gen_helper_test_sub_C_32(cpu_env, a, b, cpu_acc);
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);

    tcg_temp_free_i32(a);
    tcg_temp_free_i32(b);
}

// SUB ACC,loc16<<T
static void gen_sub_acc_loc16_t(DisasContext *ctx, uint32_t mode)
{
    TCGv a = tcg_temp_local_new();
    TCGv b = tcg_temp_local_new();
    TCGv shift = tcg_temp_new();

    TCGLabel *begin = gen_new_label();
    TCGLabel *end = gen_new_label();
    gen_set_label(begin);

        tcg_gen_mov_i32(a, cpu_acc);//get a
        gen_ld_loc16(b, mode); //get b
        gen_helper_extend_low_sxm(b, cpu_env, b);//16bit signed extend with sxm
        tcg_gen_shri_i32(shift, cpu_xt, 16);
        tcg_gen_andi_i32(shift, shift, 0x7);//T(3:0)
        tcg_gen_shl_i32(b, b, shift);//b = b<<T

        tcg_gen_sub_i32(cpu_acc, a, b);//add

        gen_helper_test_sub_V_32(cpu_env, a, b, cpu_acc);
        gen_helper_test_sub_OVC_OVM_32(cpu_env, a, b, cpu_acc);

    tcg_gen_brcondi_i32(TCG_COND_EQ, cpu_rptc, 0, end);
    tcg_gen_subi_i32(cpu_rptc, cpu_rptc, 1);
    tcg_gen_br(begin);
    gen_set_label(end);

        gen_helper_test_N_Z_32(cpu_env, cpu_acc);

    tcg_temp_free_i32(a);
    tcg_temp_free_i32(b);
    tcg_temp_free_i32(shift);
}

// SUB ACC,#16bit<<#0...15
static void gen_sub_acc_16bit_shift(DisasContext *ctx, uint32_t imm, uint32_t shift)
{
    TCGv b = tcg_const_i32(imm);
    gen_helper_extend_low_sxm(b, cpu_env, b);//16bit signed extend with sxm
    tcg_gen_shli_i32(b, b, shift);
    
    TCGv a = tcg_const_i32(0);
    tcg_gen_mov_i32(a, cpu_acc);

    tcg_gen_sub_i32(cpu_acc, a, b);

    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
    gen_helper_test_sub_C_V_32(cpu_env, a, b, cpu_acc);
    gen_helper_test_sub_OVC_OVM_32(cpu_env, a, b, cpu_acc);

    tcg_temp_free_i32(a);
    tcg_temp_free_i32(b);
}

// SUB AX,loc16
static void gen_sub_ax_loc16(DisasContext *ctx, uint32_t mode, bool is_AH) 
{
    TCGv b = tcg_temp_new();
    TCGv ax = tcg_temp_new();
    TCGv a = tcg_temp_new();
    gen_ld_loc16(b, mode);

    gen_ld_reg_half(a, cpu_acc, is_AH);

    tcg_gen_sub_i32(ax, a, b);//sub

    gen_helper_test_N_Z_16(cpu_env, ax);
    gen_helper_test_sub_C_V_16(cpu_env, a, b, ax);

    if (is_AH) {
        gen_st_reg_high_half(cpu_acc, ax);
    }
    else {
        gen_st_reg_low_half(cpu_acc, ax);
    }

    tcg_temp_free_i32(a);
    tcg_temp_free_i32(b);
    tcg_temp_free_i32(ax);
}

// SUB loc16,AX
static void gen_sub_loc16_ax(DisasContext *ctx, uint32_t mode, bool is_AH) 
{
    TCGv a = tcg_temp_new();
    TCGv b = tcg_temp_new();
    TCGv result = tcg_temp_new();

    if (is_reg_addressing_mode(mode, LOC16)) {
        gen_ld_loc16(a, mode);
        gen_ld_reg_half(b, cpu_acc, is_AH);
        tcg_gen_sub_i32(result, a, b);//sub
        gen_st_loc16(mode, result);//store
        gen_helper_test_sub_C_V_16(cpu_env, a, b, result);
        gen_helper_test_N_Z_16(cpu_env, result);
    }
    else {
        TCGv_i32 addr = tcg_temp_new();
        gen_get_loc_addr(addr, mode, LOC16);
        gen_ld16u_swap(a, addr);

        gen_ld_reg_half(b, cpu_acc, is_AH);

        tcg_gen_sub_i32(result, a, b);//sub

        gen_helper_test_sub_C_V_16(cpu_env, a, b, result);
        gen_helper_test_N_Z_16(cpu_env, result);
        
        gen_st16u_swap(result, addr); //store

        tcg_temp_free_i32(addr);
    }

    tcg_temp_free_i32(a);
    tcg_temp_free_i32(b);
    tcg_temp_free_i32(result);
}

// SUBB ACC,#8bit
static void gen_subb_acc_8bit(DisasContext *ctx, uint32_t imm) {
    TCGv b = tcg_const_i32(imm);
    TCGv a = tcg_temp_new();
    tcg_gen_mov_i32(a, cpu_acc);
    tcg_gen_sub_i32(cpu_acc, a, b);

    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
    gen_helper_test_sub_C_V_32(cpu_env, a, b, cpu_acc);
    gen_helper_test_sub_OVC_OVM_32(cpu_env, a, b, cpu_acc);

    tcg_temp_free_i32(a);
    tcg_temp_free_i32(b);
}

// SUBB SP,#7bit
static void gen_subb_sp_7bit(DisasContext *ctx, uint32_t imm)
{
    tcg_gen_subi_i32(cpu_sp, cpu_sp, imm);
}

// SUBB XARn,#7bit
static void gen_subb_xarn_7bit(DisasContext *ctx, uint32_t n, uint32_t imm)
{
    tcg_gen_subi_i32(cpu_xar[n], cpu_xar[n], imm);
}

// SUBBL ACC,loc32
static void gen_subbl_acc_loc32(DisasContext *ctx, uint32_t mode)
{
    TCGv a = tcg_temp_local_new();
    tcg_gen_mov_i32(a, cpu_acc);

    TCGv b = tcg_temp_local_new();
    gen_ld_loc32(b, mode);

    TCGv c = tcg_temp_local_new();
    gen_get_bit(c, cpu_st0, C_BIT, C_MASK);
    tcg_gen_not_i32(c, c);
    tcg_gen_andi_i32(c, c, 1);

    TCGv tmp = tcg_temp_local_new();
    tcg_gen_sub_i32(tmp, a, b);
    tcg_gen_sub_i32(cpu_acc, tmp, c);//ACC = ACC - loc32 + ~C

    gen_helper_test2_sub_C_V_OVC_OVM_32(cpu_env, a, b,c, cpu_acc);
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);

    tcg_temp_free_i32(a);
    tcg_temp_free_i32(b);
    tcg_temp_free_i32(tmp);
}

// SUBCU ACC,loc16
static void gen_subcu_acc_loc16(DisasContext *ctx, uint32_t mode)
{

    TCGLabel *begin = gen_new_label();
    TCGLabel *end = gen_new_label();
    gen_set_label(begin);

    TCGv a = tcg_temp_new_i32();
    gen_ld_loc16(a, mode);
    gen_helper_subcu(cpu_env, a);//call helper function: calculate result, update flag bit N C Z
    tcg_temp_free(a);

    tcg_gen_brcondi_i32(TCG_COND_EQ, cpu_rptc, 0, end);
    tcg_gen_subi_i32(cpu_rptc, cpu_rptc, 1);
    tcg_gen_br(begin);
    gen_set_label(end);

}

// SUBCUL ACC,loc32
static void gen_subcul_acc_loc32(DisasContext *ctx, uint32_t mode)
{
    TCGLabel *begin = gen_new_label();
    TCGLabel *end = gen_new_label();
    gen_set_label(begin);

    TCGv a = tcg_temp_new_i32();
    gen_ld_loc32(a, mode);
    gen_helper_subcul(cpu_env, a);//call helper function: calculate result, update flag bit N C Z
    tcg_temp_free(a);

    tcg_gen_brcondi_i32(TCG_COND_EQ, cpu_rptc, 0, end);
    tcg_gen_subi_i32(cpu_rptc, cpu_rptc, 1);
    tcg_gen_br(begin);
    gen_set_label(end);
}

// SUBL ACC,loc32
static void gen_subl_acc_loc32(DisasContext *ctx, uint32_t mode)
{
    TCGv a = tcg_temp_new();
    tcg_gen_mov_i32(a, cpu_acc);
    TCGv b = tcg_temp_new();
    gen_ld_loc32(b, mode);

    tcg_gen_sub_i32(cpu_acc, a, b);

    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
    gen_helper_test_sub_C_V_32(cpu_env, a, b, cpu_acc);
    gen_helper_test_sub_OVC_OVM_32(cpu_env, a, b, cpu_acc);
}

// SUBL loc32,ACC
static void gen_subl_loc32_acc(DisasContext *ctx, uint32_t mode)
{
    if (is_reg_addressing_mode(mode, LOC32))
    {
        TCGv a = tcg_temp_new();
        gen_ld_loc32(a, mode);

        TCGv result = tcg_temp_new();
        tcg_gen_sub_i32(result, a, cpu_acc);

        gen_helper_test_N_Z_32(cpu_env, result);
        gen_helper_test_sub_C_V_32(cpu_env, a, cpu_acc, result);
        gen_helper_test_sub_OVC_OVM_32(cpu_env, a, cpu_acc, result);

        gen_st_loc32(mode, result);
        tcg_temp_free_i32(result);
        tcg_temp_free_i32(a);
    }
    else 
    {
        TCGv a = tcg_temp_new();

        TCGv addr = tcg_temp_new();
        gen_get_loc_addr(addr, mode, LOC32);
        gen_ld32u_swap(a, addr);//load loc32

        TCGv result = tcg_temp_new();
        tcg_gen_sub_i32(result, a, cpu_acc);

        gen_helper_test_N_Z_32(cpu_env, result);
        gen_helper_test_sub_C_V_32(cpu_env, a, cpu_acc, result);
        gen_helper_test_sub_OVC_OVM_32(cpu_env, a, cpu_acc, result);

        gen_st32u_swap(result, addr);

        tcg_temp_free_i32(result);
        tcg_temp_free_i32(a);
        tcg_temp_free(addr);
    }
}

// SUBR loc16,AX
static void gen_subr_loc16_ax(DisasContext *ctx, uint32_t mode, bool is_AH) 
{
    //reverse substract: b - a
    TCGv a = tcg_temp_new();
    TCGv b = tcg_temp_new();
    TCGv result = tcg_temp_new();

    if (is_reg_addressing_mode(mode, LOC16)) {
        gen_ld_loc16(a, mode);
        gen_ld_reg_half(b, cpu_acc, is_AH);
        tcg_gen_sub_i32(result, b, a);//sub
        gen_st_loc16(mode, result);//store
        gen_helper_test_sub_C_V_16(cpu_env, b, a, result);
        gen_helper_test_N_Z_16(cpu_env, result);
    }
    else {
        TCGv_i32 addr = tcg_temp_new();
        gen_get_loc_addr(addr, mode, LOC16);
        gen_ld16u_swap(a, addr);

        gen_ld_reg_half(b, cpu_acc, is_AH);

        tcg_gen_sub_i32(result, b, a);//sub

        gen_helper_test_sub_C_V_16(cpu_env, b, a, result);
        gen_helper_test_N_Z_16(cpu_env, result);
        
        gen_st16u_swap(result, addr); //store

        tcg_temp_free_i32(addr);
    }

    tcg_temp_free_i32(a);
    tcg_temp_free_i32(b);
    tcg_temp_free_i32(result);
}

// SUBRL loc32,ACC
static void gen_subrl_loc32_acc(DisasContext *ctx, uint32_t mode)
{
    if (is_reg_addressing_mode(mode, LOC32))
    {
        TCGv b = tcg_temp_new();
        gen_ld_loc32(b, mode);

        TCGv result = tcg_temp_new();
        tcg_gen_sub_i32(result, cpu_acc, b);

        gen_helper_test_N_Z_32(cpu_env, result);
        gen_helper_test_sub_C_V_32(cpu_env, cpu_acc, b, result);
        gen_helper_test_sub_OVC_OVM_32(cpu_env, cpu_acc, b, result);

        gen_st_loc32(mode, result);
        tcg_temp_free_i32(result);
        tcg_temp_free_i32(b);
    }
    else 
    {
        TCGv b = tcg_temp_new();

        TCGv addr = tcg_temp_new();
        gen_get_loc_addr(addr, mode, LOC32);
        gen_ld32u_swap(b, addr);//load loc32

        TCGv result = tcg_temp_new();
        tcg_gen_sub_i32(result, cpu_acc, b);

        gen_helper_test_N_Z_32(cpu_env, result);
        gen_helper_test_sub_C_V_32(cpu_env, cpu_acc, b, result);
        gen_helper_test_sub_OVC_OVM_32(cpu_env, cpu_acc, b, result);

        gen_st32u_swap(result, addr);

        tcg_temp_free_i32(result);
        tcg_temp_free_i32(b);
        tcg_temp_free(addr);
    }
}

// SUBU ACC,loc16
static void gen_subu_acc_loc16(DisasContext *ctx, uint32_t mode)
{
    TCGv a = tcg_temp_local_new();
    TCGv b = tcg_temp_local_new();

    TCGLabel *begin = gen_new_label();
    TCGLabel *end = gen_new_label();
    gen_set_label(begin);

    gen_ld_loc16(b, mode);
    tcg_gen_mov_i32(a, cpu_acc);
    tcg_gen_sub_i32(cpu_acc, a, b);
    gen_helper_test_sub_V_32(cpu_env, a, b, cpu_acc);
    gen_helper_test_sub_OVC_OVM_32(cpu_env, a, b, cpu_acc);

    tcg_gen_brcondi_i32(TCG_COND_EQ, cpu_rptc, 0, end);
    tcg_gen_subi_i32(cpu_rptc, cpu_rptc, 1);
    tcg_gen_br(begin);
    gen_set_label(end);

    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
    gen_helper_test_sub_C_32(cpu_env, a, b, cpu_acc);

    tcg_temp_free(a);
    tcg_temp_free(b);
}

// SUBUL ACC,loc32
static void gen_subul_acc_loc32(DisasContext *ctx, uint32_t mode)
{
    TCGv a = tcg_temp_local_new();
    TCGv b = tcg_temp_local_new();
    gen_ld_loc32(b, mode);
    tcg_gen_mov_i32(a, cpu_acc);
    tcg_gen_sub_i32(cpu_acc, a, b);

    gen_helper_test_sub_C_V_32(cpu_env, a, b, cpu_acc);
    gen_helper_test_sub_OVCU_32(cpu_env, a, b, cpu_acc);
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);

    tcg_temp_free(a);
    tcg_temp_free(b);
}

// SUBUL P,loc32
static void gen_subul_p_loc32(DisasContext *ctx, uint32_t mode)
{
    TCGv a = tcg_temp_local_new();
    TCGv b = tcg_temp_local_new();
    gen_ld_loc32(b, mode);
    tcg_gen_mov_i32(a, cpu_p);
    tcg_gen_sub_i32(cpu_p, a, b);

    gen_helper_test_sub_C_V_32(cpu_env, a, b, cpu_p);
    gen_helper_test_sub_OVCU_32(cpu_env, a, b, cpu_p);
    gen_helper_test_N_Z_32(cpu_env, cpu_p);

    tcg_temp_free(a);
    tcg_temp_free(b);
}