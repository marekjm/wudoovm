/*
 *  Copyright (C) 2021 Marek Marecki
 *
 *  This file is part of Viua VM.
 *
 *  Viua VM is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Viua VM is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Viua VM.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef VIUA_ARCH_INS_H
#define VIUA_ARCH_INS_H

#include <stdint.h>

#include <functional>
#include <string>

#include <viua/arch/arch.h>
#include <viua/arch/ops.h>


namespace viua::arch::ins {
struct Instruction {
};

struct NOOP : Instruction {
};
struct EBREAK : Instruction {
    viua::arch::ops::N instruction;

    EBREAK(viua::arch::ops::N i) : instruction{i}
    {}
};
struct RETURN : Instruction {
    viua::arch::ops::S instruction;

    RETURN(viua::arch::ops::S i) : instruction{i}
    {}
};

struct ADD : Instruction {
    viua::arch::ops::T instruction;

    ADD(viua::arch::ops::T i) : instruction{i}
    {}
};
struct SUB : Instruction {
    viua::arch::ops::T instruction;

    SUB(viua::arch::ops::T i) : instruction{i}
    {}
};
struct MUL : Instruction {
    viua::arch::ops::T instruction;

    MUL(viua::arch::ops::T i) : instruction{i}
    {}
};
struct DIV : Instruction {
    viua::arch::ops::T instruction;

    DIV(viua::arch::ops::T i) : instruction{i}
    {}
};
struct MOD : Instruction {
    viua::arch::ops::T instruction;

    MOD(viua::arch::ops::T i) : instruction{i}
    {}
};
struct BITSHL : Instruction {
    viua::arch::ops::T instruction;

    BITSHL(viua::arch::ops::T i): instruction{i} {}
};
struct BITSHR : Instruction {
    viua::arch::ops::T instruction;

    BITSHR(viua::arch::ops::T i): instruction{i} {}
};
struct BITASHR : Instruction {
    viua::arch::ops::T instruction;

    BITASHR(viua::arch::ops::T i): instruction{i} {}
};
struct BITROL : Instruction {
    viua::arch::ops::T instruction;

    BITROL(viua::arch::ops::T i): instruction{i} {}
};
struct BITROR : Instruction {
    viua::arch::ops::T instruction;

    BITROR(viua::arch::ops::T i): instruction{i} {}
};
struct BITAND : Instruction {
    viua::arch::ops::T instruction;

    BITAND(viua::arch::ops::T i): instruction{i} {}
};
struct BITOR : Instruction {
    viua::arch::ops::T instruction;

    BITOR(viua::arch::ops::T i): instruction{i} {}
};
struct BITXOR : Instruction {
    viua::arch::ops::T instruction;

    BITXOR(viua::arch::ops::T i): instruction{i} {}
};
struct EQ : Instruction {
    viua::arch::ops::T instruction;

    EQ(viua::arch::ops::T i): instruction{i} {}
};
struct LT : Instruction {
    viua::arch::ops::T instruction;

    LT(viua::arch::ops::T i): instruction{i} {}
};
struct GT : Instruction {
    viua::arch::ops::T instruction;

    GT(viua::arch::ops::T i): instruction{i} {}
};
struct CMP : Instruction {
    viua::arch::ops::T instruction;

    CMP(viua::arch::ops::T i): instruction{i} {}
};
struct AND : Instruction {
    viua::arch::ops::T instruction;

    AND(viua::arch::ops::T i): instruction{i} {}
};
struct OR : Instruction {
    viua::arch::ops::T instruction;

    OR(viua::arch::ops::T i): instruction{i} {}
};

struct CALL : Instruction {
    viua::arch::ops::D instruction;

    CALL(viua::arch::ops::D i) : instruction{i}
    {}
};
struct BITNOT : Instruction {
    viua::arch::ops::D instruction;

    BITNOT(viua::arch::ops::D i): instruction{i} {}
};
struct NOT : Instruction {
    viua::arch::ops::D instruction;

    NOT(viua::arch::ops::D i): instruction{i} {}
};

/*
 * DELETE clears a register and deletes a value that it contained. For
 * unboxed values the bit-pattern is simply deleted; for boxed values their
 * destructor is invoked.
 */
struct DELETE : Instruction {
    viua::arch::ops::S instruction;

    DELETE(viua::arch::ops::S i) : instruction{i}
    {}
};

struct STRING : Instruction {
    viua::arch::ops::S instruction;

    STRING(viua::arch::ops::S i) : instruction{i}
    {}
};

struct FRAME : Instruction {
    viua::arch::ops::S instruction;

    FRAME(viua::arch::ops::S i) : instruction{i}
    {}
};

/*
 * LUI loads upper bits of a 64-bit value, sign-extending it to register
 * width. It produces a signed integer.
 *
 * LUIU is the unsigned version of LUI and does not perform sign-extension.
 */
struct LUI : Instruction {
    viua::arch::ops::E instruction;

    LUI(viua::arch::ops::E i) : instruction{i}
    {}
};
struct LUIU : Instruction {
    viua::arch::ops::E instruction;

    LUIU(viua::arch::ops::E i) : instruction{i}
    {}
};

/*
 * ADDIU adds 24-bit immediate unsigned integer as right-hand operand, to a
 * left-hand operand taken from a register. The left-hand operand is
 * converted to an unsigned integer, and the value produced is an unsigned
 * integer.
 */
struct ADDI : Instruction {
    using value_type = int64_t;
    using functor_type = std::plus<value_type>;

    viua::arch::ops::R instruction;

    ADDI(viua::arch::ops::R i) : instruction{i}
    {}
};
struct ADDIU : Instruction {
    using value_type = uint64_t;
    using functor_type = std::plus<value_type>;

    viua::arch::ops::R instruction;

    ADDIU(viua::arch::ops::R i) : instruction{i}
    {}
};
struct SUBI : Instruction {
    using value_type = int64_t;
    using functor_type = std::minus<value_type>;

    viua::arch::ops::R instruction;

    SUBI(viua::arch::ops::R i) : instruction{i}
    {}
};
struct SUBIU : Instruction {
    using value_type = uint64_t;
    using functor_type = std::minus<value_type>;

    viua::arch::ops::R instruction;

    SUBIU(viua::arch::ops::R i) : instruction{i}
    {}
};
struct MULI : Instruction {
    using value_type = int64_t;
    using functor_type = std::multiplies<value_type>;

    viua::arch::ops::R instruction;

    MULI(viua::arch::ops::R i) : instruction{i}
    {}
};
struct MULIU : Instruction {
    using value_type = uint64_t;
    using functor_type = std::multiplies<value_type>;

    viua::arch::ops::R instruction;

    MULIU(viua::arch::ops::R i) : instruction{i}
    {}
};
struct DIVI : Instruction {
    using value_type = int64_t;
    using functor_type = std::divides<value_type>;

    viua::arch::ops::R instruction;

    DIVI(viua::arch::ops::R i) : instruction{i}
    {}
};
struct DIVIU : Instruction {
    using value_type = uint64_t;
    using functor_type = std::divides<value_type>;

    viua::arch::ops::R instruction;

    DIVIU(viua::arch::ops::R i) : instruction{i}
    {}
};
}  // namespace viua::arch::ins

#endif
