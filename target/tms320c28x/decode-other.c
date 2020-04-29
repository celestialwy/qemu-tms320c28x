// ESTOP0
static void gen_estop0(DisasContext *ctx) 
{
    //todo emulation software breakpoint
}

// ESTOP1
static void gen_estop1(DisasContext *ctx) 
{
    //todo emulation software breakpoint
    //increments the PC by 1
}

// IACK #16bit
static void gen_iack_16bit(DisasContext *ctx, uint32_t imm) 
{
    //todo 
    //data_bus(15:0) = 16bit
}

// IDLE
static void gen_idle(DisasContext *ctx) 
{
    //todo 
    //p198 idle mode
}

// IN loc16,*(PA)
static void gen_in_loc16_pa(DisasContext *ctx, uint32_t mode, uint32_t pa)
{
    //todo
    //p209
}

// NOP {*ind}{ARPn}
static void gen_nop_ind_arpn(DisasContext *ctx, uint32_t mode)
{
    TCGv tmp = tcg_temp_local_new_i32();
    TCGLabel *begin = gen_new_label();
    TCGLabel *end = gen_new_label();
    gen_set_label(begin);

    gen_get_loc_addr(tmp, mode, LOC16);

    tcg_gen_brcondi_i32(TCG_COND_EQ, cpu_rptc, 0, end);
    tcg_gen_subi_i32(cpu_rptc, cpu_rptc, 1);
    tcg_gen_br(begin);
    gen_set_label(end);

    tcg_temp_free_i32(tmp);
}

// OUT *(PA),loc16
static void gen_out_pa_loc16(DisasContext *ctx, uint32_t mode, uint32_t pa)
{
    //todo
    //p352
}
