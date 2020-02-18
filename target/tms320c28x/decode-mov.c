// MOV ACC, loc16<<#0...16
static void gen_mov_acc_loc16_shift(DisasContext *ctx, uint32_t mode, uint32_t shift) {
    TCGv oprand = tcg_temp_new();
    gen_ld_loc16_sxm(oprand, mode);
    tcg_gen_shli_i32(cpu_acc, oprand, shift); // shift left, save to acc

    // set N,Z
    gen_test_N_Z_acc();
}

// MOV ACC, #16bit<<#0...15
static void gen_mov_acc_16bit_shift(DisasContext *ctx, uint32_t imm, uint32_t shift) {
    TCGv oprand = tcg_const_i32(imm);
    tcg_gen_shli_i32(cpu_acc, oprand, shift); // shift left, save to acc
    // set N,Z
    gen_test_N_Z_acc();
}

// MOV AH, loc16
static void gen_mov_ah_loc16(DisasContext *ctx, uint32_t mode) {
    TCGv ax = tcg_temp_new();
    gen_ld_loc16(ax, mode);

    gen_st_reg_high_half(cpu_acc, ax);
    gen_test_N_Z_ah();

    tcg_temp_free_i32(ax);
}

// MOV AL, loc16
static void gen_mov_al_loc16(DisasContext *ctx, uint32_t mode) {
    TCGv ax = tcg_temp_new();
    gen_ld_loc16(ax, mode);

    gen_st_reg_low_half(cpu_acc, ax);
    gen_test_N_Z_al();
    
    tcg_temp_free_i32(ax);

}


// MOV loc16,16bit
static void gen_mov_loc16_16bit(DisasContext *ctx, uint32_t mode, uint32_t imm) {
    for (int i = 0; i <= ctx->repeat_counter; i++) {// repeat
        gen_sti_loc16(mode, imm);
    }
    if (imm == 0b10101000) { //loc16 == @AH
        gen_test_N_Z_ah();
    }
    if (imm == 0b10101001) { //loc16 == @AL
        gen_test_N_Z_al();
    }
}

// MOV loc16,AH
static void gen_mov_loc16_ah(DisasContext *ctx, uint32_t mode)
{
    TCGv ax = tcg_temp_new();
    tcg_gen_sari_tl(ax, cpu_acc, 16);//get ah, signed
    gen_st_loc16(mode, ax);
    gen_test_N_Z_ah();
}

// MOV loc16,AL
static void gen_mov_loc16_al(DisasContext *ctx, uint32_t mode)
{
    TCGv ax = tcg_temp_new();

    tcg_gen_shli_i32(ax, cpu_acc, 16);// shift left acc 16
    tcg_gen_sari_tl(ax, ax, 16);//get al, signed
    gen_st_loc16(mode, ax);
    gen_test_N_Z_al();
}

// MOVL ACC,loc32
static void gen_movl_acc_loc32(DisasContext *ctx, uint32_t mode) {
    gen_ld_loc32(cpu_acc, mode);
    gen_test_N_Z_acc();
}

// MOVL loc32,ACC
static void gen_movl_loc32_acc(DisasContext *ctx, uint32_t mode) {
    gen_st_loc32(mode,cpu_acc);
    if (mode == 0b10101001) { //if loc32 == @ACC, test N,Z with ACC
        gen_test_N_Z_acc();
    }
}

// MOVL loc32,XARn
static void gen_movl_loc32_xarn(DisasContext *ctx, uint32_t mode, uint32_t n) {
    gen_st_loc32(mode, cpu_xar[n]);
    if (mode == 0b10101001) { //if loc32 == @ACC, test N,Z with ACC
        gen_test_N_Z_acc();
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
