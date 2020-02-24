// MOV ACC, loc16<<#0...16
static void gen_mov_acc_loc16_shift(DisasContext *ctx, uint32_t mode, uint32_t shift) {
    TCGv oprand = tcg_temp_new();
    gen_ld_loc16_sxm(oprand, mode);
    tcg_gen_shli_i32(cpu_acc, oprand, shift); // shift left, save to acc

    // set N,Z
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
}

// MOV ACC, #16bit<<#0...15
static void gen_mov_acc_16bit_shift(DisasContext *ctx, uint32_t imm, uint32_t shift) 
{
    TCGv oprand = tcg_const_i32(imm);
    gen_helper_ld_low_sxm(oprand, cpu_env, oprand);//16bit signed extend with sxm

    tcg_gen_shli_i32(cpu_acc, oprand, shift); // shift left, save to acc
    // set N,Z
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
}

// MOV AH, loc16
static void gen_mov_ah_loc16(DisasContext *ctx, uint32_t mode) {
    TCGv ax = tcg_temp_new();
    gen_ld_loc16(ax, mode);

    gen_st_reg_high_half(cpu_acc, ax);
    gen_helper_test_N_Z_16(cpu_env, ax);

    tcg_temp_free_i32(ax);
}

// MOV AL, loc16
static void gen_mov_al_loc16(DisasContext *ctx, uint32_t mode) {
    TCGv ax = tcg_temp_new();
    gen_ld_loc16(ax, mode);

    gen_st_reg_low_half(cpu_acc, ax);
    gen_helper_test_N_Z_16(cpu_env, ax);
    
    tcg_temp_free_i32(ax);

}


// MOV loc16,16bit
static void gen_mov_loc16_16bit(DisasContext *ctx, uint32_t mode, uint32_t imm) {
    TCGLabel *repeat = gen_new_label();

    gen_sti_loc16(mode, imm);

    if (ctx->rpt_set) {
        tcg_gen_brcondi_i32(TCG_COND_GT, cpu_rptc, 0, repeat);
    }

    if (imm == 0b10101000) { //loc16 == @AH
        TCGv ah = tcg_temp_new();
        tcg_gen_shri_i32(ah, cpu_acc, 16);
        gen_helper_test_N_Z_16(cpu_env, ah);
        tcg_temp_free(ah);
    }
    if (imm == 0b10101001) { //loc16 == @AL
        TCGv al = tcg_temp_new();
        tcg_gen_andi_i32(al, cpu_acc, 0xffff);
        gen_helper_test_N_Z_16(cpu_env, cpu_acc);
        tcg_temp_free(al);
    }


    if (ctx->rpt_set) {
        gen_goto_tb(ctx, 0, (ctx->base.pc_next >> 1) + 2);

        gen_set_label(repeat);
        tcg_gen_subi_i32(cpu_rptc, cpu_rptc, 1);
        gen_goto_tb(ctx, 1, (ctx->base.pc_next >> 1));
    }
    if (ctx->rpt_set) {
        ctx->base.is_jmp = DISAS_NORETURN;
    }

}

// MOV loc16,AH
static void gen_mov_loc16_ah(DisasContext *ctx, uint32_t mode)
{
    TCGv ax = tcg_temp_new();
    tcg_gen_sari_tl(ax, cpu_acc, 16);//get ah, signed
    gen_st_loc16(mode, ax);
    if ((mode == 0xa8) || (mode == 0xa9)) {
        gen_helper_test_N_Z_16(cpu_env, ax);
    }
}

// MOV loc16,AL
static void gen_mov_loc16_al(DisasContext *ctx, uint32_t mode)
{
    TCGv ax = tcg_temp_new();

    tcg_gen_shli_i32(ax, cpu_acc, 16);// shift left acc 16
    tcg_gen_sari_tl(ax, ax, 16);//get al, signed
    gen_st_loc16(mode, ax);
    if ((mode == 0xa8) || (mode == 0xa9)) {
        gen_helper_test_N_Z_16(cpu_env, ax);
    }
}

// MOVL ACC,loc32
static void gen_movl_acc_loc32(DisasContext *ctx, uint32_t mode) {
    gen_ld_loc32(cpu_acc, mode);
    gen_helper_test_N_Z_32(cpu_env, cpu_acc);
}

// MOVL loc32,ACC
static void gen_movl_loc32_acc(DisasContext *ctx, uint32_t mode) {
    gen_st_loc32(mode,cpu_acc);
    if (mode == 0b10101001) { //if loc32 == @ACC, test N,Z with ACC
        gen_helper_test_N_Z_32(cpu_env, cpu_acc);
    }
}

// MOVL loc32,XARn
static void gen_movl_loc32_xarn(DisasContext *ctx, uint32_t mode, uint32_t n) {
    gen_st_loc32(mode, cpu_xar[n]);
    if (mode == 0b10101001) { //if loc32 == @ACC, test N,Z with ACC
        gen_helper_test_N_Z_32(cpu_env, cpu_acc);
    }
}

// MOVL XARn,#22bit
static void gen_movl_xarn_22bit(DisasContext *ctx, uint32_t n, uint32_t imm) {
    tcg_gen_movi_i32(cpu_xar[n], imm);
}

// MOVW DP,#16bit
static void gen_movw_dp_16bit(DisasContext *ctx, uint32_t imm) {
    tcg_gen_movi_tl(cpu_dp, imm);
}

// POP IFR
static void gen_pop_ifr(DisasContext *ctx) {
    tcg_gen_subi_i32(cpu_sp, cpu_sp, 1);
    gen_ld16u_swap(cpu_ifr, cpu_sp);
}


// POP AR1H:AR0H
static void gen_pop_ar1h_ar0h(DisasContext *ctx) {
    TCGv tmp = tcg_temp_local_new();
    TCGv tmp2 = tcg_temp_local_new();
    tcg_gen_subi_i32(cpu_sp, cpu_sp, 2);
    gen_ld32u_swap(tmp, cpu_sp);
    tcg_gen_shri_i32(tmp2, tmp, 16);//ar1h
    tcg_gen_andi_i32(tmp, tmp, 0xffff);//ar0h

    gen_st_reg_high_half(cpu_xar[0], tmp);
    gen_st_reg_high_half(cpu_xar[1], tmp2);

    tcg_temp_free(tmp);
    tcg_temp_free(tmp2);
}

// POP RPC
static void gen_pop_rpc(DisasContext *ctx)
{
    tcg_gen_subi_i32(cpu_sp, cpu_sp, 2);

    gen_ld32u_swap(cpu_rpc, cpu_sp);
    tcg_gen_andi_i32(cpu_rpc, cpu_rpc, 0x3fffff);//rpc is 22bit 
}


// PUSH RPC
static void gen_push_rpc(DisasContext *ctx)
{
    gen_st32u_swap(cpu_rpc, cpu_sp);
    tcg_gen_addi_i32(cpu_sp, cpu_sp, 2);
}

// PUSH AR1H:AR0H
static void gen_push_ar1h_ar0h(DisasContext *ctx)
{
    TCGv tmp = tcg_temp_local_new();
    TCGv tmp2 = tcg_temp_local_new();
    tcg_gen_andi_i32(tmp, cpu_xar[1], 0xffff0000);
    tcg_gen_shri_i32(tmp2, cpu_xar[0], 16);
    tcg_gen_or_i32(tmp, tmp, tmp2);
    gen_st32u_swap(tmp, cpu_sp);
    tcg_gen_addi_i32(cpu_sp, cpu_sp, 2);

}
