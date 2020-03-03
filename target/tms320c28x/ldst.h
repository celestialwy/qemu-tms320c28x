
#ifndef TMS320C28X_LDST_H
#define TMS320C28X_LDST_H

#include "cpu.h"
#ifndef CONFIG_USER_ONLY
#include "hw/loader.h"
#endif

static inline void st32_swap(CPUTms320c28xState *env, target_ulong addr, uint32_t value)
{
    uint32_t tmp, tmp2;

    // ABCD -> CDAB
    tmp = value << 16;
    tmp2 = value >> 16;
    value = tmp | tmp2;

    //CDAB -> DCBA
    tmp = value << 8;
    tmp = tmp & 0xff00ff00;
    tmp2 = value >> 8;
    tmp2 = tmp2 & 0x00ff00ff;
    value = tmp | tmp2;

    cpu_stl_data(env, addr * 2, value);
}

static inline uint32_t ld32_swap(CPUTms320c28xState *env, target_ulong addr)
{
    uint32_t value = cpu_ldl_data(env, addr * 2);

    uint32_t tmp, tmp2;
    // ABCD -> BADC
    tmp = value << 8;
    tmp = tmp & 0xff00ff00;
    tmp2 = value >> 8;
    tmp2 = tmp2 & 0x00ff00ff;
    value = tmp | tmp2;
    // BADC -> DCBA
    tmp = value << 16;
    tmp2 = value >> 16;
    value = tmp | tmp2;

    return value;
}

static inline uint32_t ld16_swap(CPUTms320c28xState *env, target_ulong addr)
{
    uint32_t value = cpu_lduw_data(env, addr * 2);

    uint32_t tmp, tmp2;
    // AB -> BA
    tmp = value << 8;
    tmp = tmp & 0xff00;
    tmp2 = value >> 8;
    tmp2 = tmp2 & 0xff;

    value = tmp | tmp2;

    return value;
}

static inline void st16_lsb(CPUTms320c28xState *env, target_ulong addr, uint32_t value)
{
    value = value & 0xff;
    cpu_stb_data(env, addr * 2, value);
}

static inline void st16_swap(CPUTms320c28xState *env, target_ulong addr, uint32_t value)
{
    uint32_t tmp, tmp2;

    //CDAB -> DCBA
    tmp = value << 8;
    tmp = tmp & 0xff00;
    tmp2 = value >> 8;
    tmp2 = tmp2 & 0x00ff;
    value = tmp | tmp2;

    cpu_stw_data(env, addr * 2, value);
}

static inline uint32_t st_low_half(uint32_t reg, uint32_t value)
{
    reg = reg & 0xffff0000;
    value = value & 0xffff;
    return reg | value;
}

static inline uint32_t st_high_half(uint32_t reg, uint32_t value)
{
    reg = reg & 0xffff;
    value = value << 16;
    return reg | value;
}

static inline uint32_t bit_inverse_low_half(uint32_t n) {
    n = ((n>>1) & 0x55555555) | ((n<<1) & 0xaaaaaaaa);
    n = ((n>>2) & 0x33333333) | ((n<<2) & 0xcccccccc);
    n = ((n>>4) & 0x0f0f0f0f) | ((n<<4) & 0xf0f0f0f0);
    n = ((n>>8) & 0x00ff00ff) | ((n<<8) & 0xff00ff00);
    n = n & 0x0000ffff;
    return n;
}

static inline uint32_t sign_extend_16(uint32_t a) {
    if ((a >> 15) & 1) {
        return a | 0xffff0000;
    }
    else {
        return a & 0xffff;
    }
}
#endif