
static void gen_test_N_Z_acc(void) 
{
    tcg_gen_andi_tl(cpu_st0, cpu_st0, 0xffcf); // clear N,Z

    TCGv acc = tcg_temp_new();
    tcg_gen_shri_tl(acc, cpu_acc, 31); //acc[31]
    tcg_gen_shli_tl(acc, acc, 5); // used to set N
    tcg_gen_or_tl(cpu_st0, cpu_st0, acc);

    TCGv_i32 op = tcg_const_i32(0);
    gen_helper_test_Z(cpu_env, cpu_acc, op); //if ah == 0, set Z

    tcg_temp_free_i32(acc);
    tcg_temp_free_i32(op);
}

static void gen_test_N_Z_ah(void) 
{
    tcg_gen_andi_tl(cpu_st0, cpu_st0, 0xffcf); // clear N,Z
    TCGv ah = tcg_temp_new();
    tcg_gen_shri_tl(ah, cpu_acc, 31); //ah[15]
    tcg_gen_shli_tl(ah, ah, 5); // used to set N
    tcg_gen_or_tl(cpu_st0, cpu_st0, ah);

    TCGv_i32 op = tcg_const_i32(0);
    tcg_gen_shri_tl(ah, cpu_acc, 16); //ah
    gen_helper_test_Z(cpu_env, ah, op); //if ah == 0, set Z

    tcg_temp_free_i32(ah);
    tcg_temp_free_i32(op);
}

static void gen_test_N_Z_al(void) 
{
    tcg_gen_andi_tl(cpu_st0, cpu_st0, 0xffcf); // clear N,Z
    TCGv al = tcg_temp_new();
    tcg_gen_shri_tl(al, cpu_acc, 15); //al[15]
    tcg_gen_andi_tl(al, al, 1); //al[15]
    tcg_gen_shli_tl(al, al, 5); // used to set N
    tcg_gen_or_tl(cpu_st0, cpu_st0, al);

    TCGv_i32 op = tcg_const_i32(0);
    tcg_gen_andi_tl(al, cpu_acc, 0xffff); // al
    gen_helper_test_Z(cpu_env, al, op);

    tcg_temp_free_i32(al);
    tcg_temp_free_i32(op);
}