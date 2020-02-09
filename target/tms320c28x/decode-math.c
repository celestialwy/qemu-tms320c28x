
// ADD AH,loc16
static void gen_add_ah_loc16(DisasContext *ctx, uint32_t mode) {
    TCGv tmp = tcg_temp_new();
    TCGv ax = tcg_temp_new();
    gen_ld_loc16(tmp, mode);
    tcg_gen_shli_tl(tmp, tmp, 16);// left shift tmp by 16bit
    tcg_gen_sari_tl(tmp, tmp ,16);// get tmp , signed

    tcg_gen_sari_tl(ax, cpu_acc, 16);//get ah, signed
    tcg_gen_add_i32(ax, ax, tmp);//add tmp, ax
    gen_helper_test_C_V_16(cpu_env, ax);
    gen_st_reg_high_half(cpu_acc, ax);
    gen_test_N_Z_ah();
    

    tcg_temp_free_i32(tmp);
    tcg_temp_free_i32(ax);

}

// ADD AL,loc16
static void gen_add_al_loc16(DisasContext *ctx, uint32_t mode) {
    TCGv tmp = tcg_temp_new();
    TCGv ax = tcg_temp_new();
    gen_ld_loc16(tmp, mode);
    tcg_gen_shli_tl(tmp, tmp, 16);// left shift tmp by 16bit
    tcg_gen_sari_tl(tmp, tmp ,16);// get tmp , signed

    tcg_gen_shli_i32(ax, cpu_acc, 16);// shift left acc 16
    tcg_gen_sari_tl(ax, ax, 16);//left shift ax by 16, signed
    tcg_gen_add_i32(ax, ax, tmp);//add tmp, ax
    gen_st_reg_low_half(cpu_acc, ax);
    gen_test_N_Z_al();
    
    tcg_temp_free_i32(tmp);
    tcg_temp_free_i32(ax);


}

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


// MOVL XARn,#22bit
static void gen_movl_xarn_22bit(DisasContext *ctx, uint32_t n, uint32_t imm) {
    tcg_gen_movi_i32(cpu_xar[n], imm);
}

// MOVW DP,#16bit
static void gen_movw_dp_16bit(DisasContext *ctx, uint32_t imm) {
    tcg_gen_movi_tl(cpu_dp, imm);
}

// SUBB ACC,#8bit
static void gen_subb_acc_8bit(DisasContext *ctx, uint32_t imm) {
    imm = -imm;
    TCGv bh = tcg_const_i32(0x1); // sign bit is 1, for neg imm
    TCGv bl = tcg_const_i32(imm);
    TCGv acc_ext = tcg_const_i32(0);
    tcg_gen_sari_tl(acc_ext,cpu_acc,31);//extend acc to 64bit
    // tcg_gen_sub2_i32(cpu_acc, acc_ext, cpu_acc, acc_ext, bl, bh); //64bit acc - imm
    tcg_gen_add2_i32(cpu_acc, acc_ext, cpu_acc, acc_ext, bl, bh); //64bit acc - imm
    gen_helper_test_sub_C_V_32(cpu_env,acc_ext,cpu_acc);
    gen_test_N_Z_acc();
    gen_helper_test_OVC_32_set_acc(cpu_env, acc_ext, cpu_acc);

}
