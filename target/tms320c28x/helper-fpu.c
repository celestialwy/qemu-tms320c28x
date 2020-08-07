#include "qemu/osdep.h"
#include "cpu.h"
#include "exec/helper-proto.h"
#include "exception.h"
#include "fpu/softfloat.h"

uint32_t HELPER(fpu_absf)(CPUTms320c28xState *env, uint32_t value)
{
    return float32_abs(make_float32(value));
}