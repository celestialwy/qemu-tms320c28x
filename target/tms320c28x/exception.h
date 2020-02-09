/*
 * Tms320c28x exception header.
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

#ifndef TARGET_TMS320C28X_EXCEPTION_H
#define TARGET_TMS320C28X_EXCEPTION_H

#include "cpu.h"

void QEMU_NORETURN raise_exception(Tms320c28xCPU *cpu, uint32_t excp);

#endif /* TARGET_TMS320C28X_EXCEPTION_H */
