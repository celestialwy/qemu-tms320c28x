/*
 * TMS320C28x helper defines
 *
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

DEF_HELPER_FLAGS_2(exception, TCG_CALL_NO_WG, void, env, i32)
// DEF_HELPER_3(addressing_mode, i32, env, i32, i32)

// addressing mode
DEF_HELPER_2(ld_loc16, i32, env, i32)
DEF_HELPER_2(ld_loc32, i32, env, i32)
DEF_HELPER_3(st_loc16, void, env, i32, i32)
DEF_HELPER_3(st_loc32, void, env, i32, i32)

DEF_HELPER_2(test_cond,i32,env,i32)

// test bit
DEF_HELPER_2(test_N_Z_16, void, env, i32)
DEF_HELPER_2(test_N_Z_32, void, env, i32)

DEF_HELPER_4(test_C_V_16, void, env, i32, i32, i32)
DEF_HELPER_4(test_C_32, void, env, i32, i32, i32)
DEF_HELPER_4(test_V_32, void, env, i32, i32, i32)
DEF_HELPER_4(test_C_V_32, void, env, i32, i32, i32)
DEF_HELPER_4(test_OVC_OVM_32, void, env, i32, i32, i32) // affect acc value
DEF_HELPER_5(test2_C_V_OVC_OVM_32, void, env, i32, i32, i32, i32) // affect acc value
DEF_HELPER_4(test_OVCU_32, void, env, i32, i32, i32) // affect acc value

//load with cond
DEF_HELPER_2(extend_low_sxm, i32, env, i32)

//print
DEF_HELPER_2(print, void, env, i32)
DEF_HELPER_1(print_env, void, env)

//interrupt
DEF_HELPER_1(aborti, void, env)
DEF_HELPER_3(intr, void, env, i32, i32)

//math
DEF_HELPER_1(abs_acc, void, env)
DEF_HELPER_1(abstc_acc, void, env)
DEF_HELPER_2(shift_by_pm, i32, env, i32)

//mov
DEF_HELPER_4(mov_16bit_loc16, void, env, i32, i32, i32)// MOV *(0:16bit),loc16