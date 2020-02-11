
static void gen_pop_ifr(DisasContext *ctx) {
    tcg_gen_subi_i32(cpu_sp, cpu_sp, 1);
    gen_ld16u_swap(cpu_ifr, cpu_sp);
}