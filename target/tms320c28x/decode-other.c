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