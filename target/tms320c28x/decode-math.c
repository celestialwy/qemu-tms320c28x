
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
