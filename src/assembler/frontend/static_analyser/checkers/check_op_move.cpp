/*
 *  Copyright (C) 2018 Marek Marecki
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

#include <string>

#include <viua/assembler/frontend/static_analyser.h>

using viua::assembler::frontend::parser::Instruction;

namespace viua { namespace assembler { namespace frontend {
namespace static_analyser { namespace checkers {
auto check_op_move(Register_usage_profile& register_usage_profile,
                   Instruction const& instruction) -> void
{
    auto target = get_operand<Register_index>(instruction, 0);
    auto source = get_operand<Register_index>(instruction, 1);

    if ((not target) and (not source)) {
        throw invalid_syntax(instruction.operands.at(0)->tokens,
                             "invalid operand")
            .note("expected register index");
    }
    if ((not target)
        and source->rss != viua::bytecode::codec::Register_set::Parameters) {
        throw invalid_syntax(instruction.operands.at(0)->tokens,
                             "invalid operand")
            .note("expected register index");
    }

    if (target) {
        if (target->as
            == viua::internals::Access_specifier::REGISTER_INDIRECT) {
            auto r = *target;
            r.rss  = viua::bytecode::codec::Register_set::Local;
            check_use_of_register(register_usage_profile, r);
            assert_type_of_register<viua::internals::Value_types::INTEGER>(
                register_usage_profile, r);
        }
    }

    if (not source) {
        throw invalid_syntax(instruction.operands.at(1)->tokens,
                             "invalid operand")
            .note("expected register index");
    }

    check_use_of_register(
        register_usage_profile, *source, "move from", false, true);
    assert_type_of_register<viua::internals::Value_types::UNDEFINED>(
        register_usage_profile, *source);

    // we need to check if the register set is local because we only track state
    // of local registers
    if (target
        and (target->rss == viua::bytecode::codec::Register_set::Local)) {
        auto val       = Register(*target);
        val.value_type = register_usage_profile.at(*source).second.value_type;
        register_usage_profile.define(val, target->tokens.at(0));
    }

    if (source->rss == viua::bytecode::codec::Register_set::Local) {
        erase_if_direct_access(register_usage_profile, source, instruction);
    }
}
}}}}}  // namespace viua::assembler::frontend::static_analyser::checkers
