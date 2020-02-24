static void gen_ld16u_swap(TCGv value, TCGv addr_param) 
{
    TCGv addr = tcg_temp_local_new();
    tcg_gen_shli_i32(addr, addr_param, 1);// addr*2
    tcg_gen_qemu_ld16u(value, addr, 0);

    TCGv_i32 tmp = tcg_const_i32(0);
    tcg_gen_shli_tl(tmp, value, 8);
    tcg_gen_andi_i32(tmp, tmp, 0xff00);
    tcg_gen_shri_tl(value, value, 8);
    tcg_gen_or_i32(value, value, tmp);
    tcg_temp_free_i32(tmp);
    tcg_temp_free_i32(addr);
}

static void gen_ld32u_swap(TCGv value, TCGv addr_param) 
{
    TCGv addr = tcg_temp_local_new();
    tcg_gen_shli_i32(addr, addr_param, 1);// addr*2
    tcg_gen_qemu_ld32u(value, addr, 0);

    TCGv_i32 tmp = tcg_const_i32(0);
    // ABCD -> BADC
    tcg_gen_shli_tl(tmp, value, 8);
    tcg_gen_andi_tl(tmp, tmp, 0xff00ff00);
    tcg_gen_shri_tl(value, value, 8);
    tcg_gen_andi_tl(value, value, 0x00ff00ff);
    tcg_gen_or_i32(value, value, tmp);
    // BADC -> DCBA
    tcg_gen_shli_tl(tmp, value, 16); //swap low16bit and high16bit
    tcg_gen_shri_tl(value, value, 16);
    tcg_gen_or_i32(value, value, tmp);

    tcg_temp_free_i32(tmp);
    tcg_temp_free_i32(addr);
}

static void gen_st16u_swap(TCGv value, TCGv addr_param)
{
    TCGv addr = tcg_temp_local_new();
    tcg_gen_shli_i32(addr, addr_param, 1);// addr*2

    TCGv_i32 tmp = tcg_const_i32(0);
    TCGv_i32 tmp2 = tcg_const_i32(0);

    tcg_gen_shli_tl(tmp, value, 8);
    tcg_gen_andi_i32(tmp, tmp, 0xff00);
    tcg_gen_shri_tl(tmp2, value, 8);
    tcg_gen_or_i32(tmp2, tmp2, tmp);
    tcg_gen_qemu_st16(tmp2, addr, 0);
    tcg_temp_free_i32(tmp);
    tcg_temp_free_i32(tmp2);
    tcg_temp_free_i32(addr);
}

static void gen_st32u_swap(TCGv value, TCGv addr_param) 
{
    TCGv addr = tcg_temp_local_new();
    tcg_gen_shli_i32(addr, addr_param, 1);// addr*2

    TCGv_i32 tmp = tcg_const_i32(0);
    TCGv_i32 tmp2 = tcg_const_i32(0);
    // ABCD -> CDAB
    tcg_gen_shli_tl(tmp, value, 16);
    tcg_gen_shri_tl(tmp2, value, 16);
    tcg_gen_or_i32(tmp2, tmp2, tmp);
    //CDAB -> DCBA
    tcg_gen_shli_tl(tmp, tmp2, 8);
    tcg_gen_andi_tl(tmp, tmp, 0xff00ff00);
    tcg_gen_shri_tl(tmp2, tmp2, 8);
    tcg_gen_andi_tl(tmp2, tmp2, 0x00ff00ff);
    tcg_gen_or_i32(tmp2, tmp2, tmp);

    tcg_gen_qemu_st32(tmp2, addr, 0);
    tcg_temp_free_i32(tmp);
    tcg_temp_free_i32(tmp2);
    tcg_temp_free_i32(addr);
}

// load loc16 value to retval
static void gen_ld_loc16(TCGv retval, uint32_t mode)
{
    switch ((mode>>3) & 0b11111) {
        case 0b10100:
        {
            //@ar[n]
            uint32_t idx = mode & 0b111;
            tcg_gen_andi_tl(retval, cpu_xar[idx], 0x0000ffff);
            break;            
        }
        default:
            switch(mode) {
                case 0b10101000: //@AH
                    tcg_gen_shri_i32(retval, cpu_acc, 16);
                    break;
                case 0b10101001: //@AL
                    tcg_gen_andi_tl(retval, cpu_acc, 0x0000ffff);
                    break;
                case 0b10101010: //@PH
                    tcg_gen_shri_i32(retval, cpu_p, 16);
                    break;
                case 0b10101011: //@PL
                    tcg_gen_andi_tl(retval, cpu_p, 0x0000ffff);
                    break;
                case 0b10101100: //@TH
                    tcg_gen_shri_i32(retval, cpu_xt, 16);
                    break;
                case 0b10101101: //@SP
                    tcg_gen_andi_tl(retval, cpu_sp, 0x0000ffff);
                    break;
                default:
                {
                    TCGv_i32 address_mode = tcg_const_i32(mode);
                    TCGv_i32 addr = tcg_const_i32(0);
                    TCGv_i32 loc_type = tcg_const_i32(LOC16);
                    gen_helper_addressing_mode(addr, cpu_env, address_mode, loc_type);
                    gen_ld16u_swap(retval, addr);
                    tcg_temp_free_i32(address_mode);
                    tcg_temp_free_i32(addr);
                    tcg_temp_free_i32(loc_type);
                    break;
                }
            }
    }
}

// load loc16 value to retval, sign extended
static void gen_ld_loc16_sxm(TCGv retval, uint32_t mode)
{
    switch ((mode>>3) & 0b11111) {
        case 0b10100:
        {
            //@ar[n]
            uint32_t idx = mode & 0b111;
            gen_helper_ld_low_sxm(retval, cpu_env, cpu_xar[idx]);
            break;            
        }
        default:
            switch(mode) {
                case 0b10101000: //@AH
                    gen_helper_ld_high_sxm(retval, cpu_env, cpu_acc);
                    break;
                case 0b10101001: //@AL
                    gen_helper_ld_low_sxm(retval, cpu_env, cpu_acc);
                    break;
                case 0b10101010: //@PH
                    gen_helper_ld_high_sxm(retval, cpu_env, cpu_p);
                    break;
                case 0b10101011: //@PL
                    gen_helper_ld_low_sxm(retval, cpu_env, cpu_p);
                    break;
                case 0b10101100: //@TH
                    gen_helper_ld_high_sxm(retval, cpu_env, cpu_xt);
                    break;
                case 0b10101101: //@SP
                    gen_helper_ld_low_sxm(retval, cpu_env, cpu_sp);
                    break;
                default:
                {
                    TCGv_i32 address_mode = tcg_const_i32(mode);
                    TCGv_i32 addr = tcg_const_i32(0);
                    TCGv_i32 loc_type = tcg_const_i32(LOC16);
                    gen_helper_addressing_mode(addr, cpu_env, address_mode, loc_type);
                    gen_ld16u_swap(retval, addr); // ld from addr
                    gen_helper_ld_low_sxm(retval, cpu_env, retval);
                    tcg_temp_free_i32(address_mode);
                    tcg_temp_free_i32(addr);
                    tcg_temp_free_i32(loc_type);
                    break;
                }
            }
    }
}

// load loc32 value to retval
static void gen_ld_loc32(TCGv retval, uint32_t mode)
{
    switch ((mode>>3) & 0b11111) {
        case 0b10100:
        {
            //@ar[n]
            uint32_t idx = mode & 0b111;
            tcg_gen_andi_tl(retval, cpu_xar[idx], 0x0000ffff);
            break;            
        }
        default:
            switch(mode) {
                case 0b10101000: //@AH
                    tcg_gen_shri_i32(retval, cpu_acc, 16);
                    break;
                case 0b10101001: //@AL
                    tcg_gen_andi_tl(retval, cpu_acc, 0x0000ffff);
                    break;
                case 0b10101010: //@PH
                    tcg_gen_shri_i32(retval, cpu_p, 16);
                    break;
                case 0b10101011: //@PL
                    tcg_gen_andi_tl(retval, cpu_p, 0x0000ffff);
                    break;
                case 0b10101100: //@TH
                    tcg_gen_shri_i32(retval, cpu_xt, 16);
                    break;
                case 0b10101101: //@SP
                    tcg_gen_andi_tl(retval, cpu_sp, 0x0000ffff);
                    break;
                default:
                {
                    TCGv_i32 address_mode = tcg_const_i32(mode);
                    TCGv_i32 addr = tcg_const_i32(0);
                    TCGv_i32 loc_type = tcg_const_i32(LOC32);
                    gen_helper_addressing_mode(addr, cpu_env, address_mode, loc_type);
                    gen_ld32u_swap(retval, addr);
                    tcg_temp_free_i32(address_mode);
                    tcg_temp_free_i32(addr);
                    tcg_temp_free_i32(loc_type);
                    break;
                }
            }
    }
}

static void gen_st_reg_high_half(TCGv reg, TCGv_i32 oprand)
{
    TCGv_i32 tmp = tcg_const_i32(0x0000ffff);
    tcg_gen_and_tl(reg, reg, tmp);// get low 16bit
    tcg_gen_shli_tl(oprand, oprand, 16); // shift imm value
    tcg_gen_or_tl(reg, reg, oprand);// concat high 16bit
    tcg_temp_free_i32(tmp);
}

// save oprand to reg high half
static void gen_sti_reg_high_half(TCGv reg, uint32_t oprand)
{
    TCGv_i32 tmp = tcg_const_i32(0x0000ffff);
    tcg_gen_and_tl(reg, reg, tmp);// get low 16bit
    oprand = oprand << 16; // shift imm value
    tcg_gen_ori_tl(reg, reg, oprand);// concat high 16bit
    tcg_temp_free_i32(tmp);
}

static void gen_st_reg_low_half(TCGv reg, TCGv_i32 oprand)
{
    TCGv_i32 tmp = tcg_const_i32(0xffff0000);
    tcg_gen_and_tl(reg, reg, tmp);// get high 16bit
    tcg_gen_or_tl(reg, reg, oprand);// concat low 16bit
    tcg_temp_free_i32(tmp);
}

// save oprand to reg low half
static void gen_sti_reg_low_half(TCGv reg, uint32_t oprand)
{
    TCGv_i32 tmp = tcg_const_i32(0xffff0000);
    tcg_gen_and_tl(reg, reg, tmp);// get high 16bit
    tcg_gen_ori_tl(reg, reg, oprand);// concat low 16bit
    tcg_temp_free_i32(tmp);
}


// store oprand to loc16
static void gen_sti_loc16(uint32_t mode, uint32_t oprand)
{
    oprand = oprand & 0xffff;

    switch ((mode>>3) & 0b11111) {
        case 0b10100:
        {
            //@ar[n]
            uint32_t idx = mode & 0b111;
            gen_sti_reg_low_half(cpu_xar[idx], oprand);
            break;            
        }
        default:
            switch(mode) {
                case 0b10101000: //@AH
                    gen_sti_reg_high_half(cpu_acc, oprand);
                    break;
                case 0b10101001: //@AL
                    gen_sti_reg_low_half(cpu_acc, oprand);
                    break;
                case 0b10101010: //@PH
                    gen_sti_reg_high_half(cpu_p, oprand);
                    break;
                case 0b10101011: //@PL
                    gen_sti_reg_low_half(cpu_p, oprand);
                    break;
                case 0b10101100: //@TH
                    gen_sti_reg_high_half(cpu_xt, oprand);
                    break;
                case 0b10101101: //@SP
                    tcg_gen_movi_tl(cpu_sp, oprand);
                    break;
                default:
                {
                    TCGv_i32 address_mode = tcg_const_i32(mode);
                    TCGv_i32 addr = tcg_const_i32(0);
                    TCGv_i32 value = tcg_const_i32(oprand);
                    TCGv_i32 loc_type = tcg_const_i32(LOC16);
                    gen_helper_addressing_mode(addr, cpu_env, address_mode, loc_type);
                    gen_st16u_swap(value, addr);
                    tcg_temp_free_i32(address_mode);
                    tcg_temp_free_i32(addr);
                    tcg_temp_free_i32(value);
                    tcg_temp_free_i32(loc_type);
                    break;
                }
            }
    }
}

static void gen_st_loc16(uint32_t mode, TCGv oprand)
{
    tcg_gen_andi_i32(oprand, oprand, 0xffff);
    
    switch ((mode>>3) & 0b11111) {
        case 0b10100:
        {
            //@ar[n]
            uint32_t idx = mode & 0b111;
            gen_st_reg_low_half(cpu_xar[idx], oprand);
            break;            
        }
        default:
            switch(mode) {
                case 0b10101000: //@AH
                    gen_st_reg_high_half(cpu_acc, oprand);
                    break;
                case 0b10101001: //@AL
                    gen_st_reg_low_half(cpu_acc, oprand);
                    break;
                case 0b10101010: //@PH
                    gen_st_reg_high_half(cpu_p, oprand);
                    break;
                case 0b10101011: //@PL
                    gen_st_reg_low_half(cpu_p, oprand);
                    break;
                case 0b10101100: //@TH
                    gen_st_reg_high_half(cpu_xt, oprand);
                    break;
                case 0b10101101: //@SP
                    tcg_gen_mov_tl(cpu_sp, oprand);
                    break;
                default:
                {
                    TCGv_i32 address_mode = tcg_const_i32(mode);
                    TCGv_i32 addr = tcg_const_i32(0);
                    TCGv_i32 loc_type = tcg_const_i32(LOC16);
                    gen_helper_addressing_mode(addr, cpu_env, address_mode, loc_type);
                    gen_st16u_swap(oprand, addr);
                    tcg_temp_free_i32(address_mode);
                    tcg_temp_free_i32(addr);
                    tcg_temp_free_i32(loc_type);
                    break;
                }
            }
    }
}

static void gen_st_loc32(uint32_t mode, TCGv_i32 oprand)
{
    switch ((mode>>3) & 0b11111) {
        case 0b10100:
        {
            //xarn
            uint32_t idx = mode & 0b111;
            tcg_gen_mov_tl(cpu_xar[idx], oprand);
            break;
        }
        default:
            switch(mode) {
                case 0b10101001: //@ACC
                    tcg_gen_mov_tl(cpu_acc, oprand);
                    break;
                case 0b10101011: //@P
                    tcg_gen_mov_tl(cpu_p, oprand);
                    break;
                case 0b10101100: //@XT
                    tcg_gen_mov_tl(cpu_xt, oprand);
                    break;
                default:
                {
                    TCGv_i32 address_mode = tcg_const_i32(mode);
                    TCGv_i32 addr = tcg_const_i32(0);
                    TCGv_i32 loc_type = tcg_const_i32(LOC32);

                    gen_helper_addressing_mode(addr, cpu_env, address_mode, loc_type);                
                    gen_st32u_swap(oprand, addr);

                    tcg_temp_free_i32(address_mode);
                    tcg_temp_free_i32(addr);
                    break;
                }
            }
    }
}
