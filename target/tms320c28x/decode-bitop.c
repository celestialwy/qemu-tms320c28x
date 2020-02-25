
// SETC Mode
static void gen_setc_mode(DisasContext *ctx, uint32_t mode)
{
    uint32_t st0_mask = mode & 0xf;
    uint32_t st1_mask = (mode >> 4) & 0xf;
    tcg_gen_ori_i32(cpu_st0, cpu_st0, st0_mask);
    tcg_gen_ori_i32(cpu_st1, cpu_st1, st1_mask);
}

// SPM shift
static void gen_spm_shift(DisasContext *ctx, uint32_t shift)
{
    uint32_t st0_mask = 0xfc7f;
    uint32_t st0_mask2 = (shift & 0b111) << 7;
    tcg_gen_andi_i32(cpu_st0, cpu_st0, st0_mask); //clear pm
    tcg_gen_ori_i32(cpu_st0, cpu_st0, st0_mask2); // set new pm
}
