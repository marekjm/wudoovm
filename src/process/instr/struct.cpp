/*
 *  Copyright (C) 2017-2019 Marek Marecki
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

#include <utility>
#include <viua/assert.h>
#include <viua/bytecode/decoder/operands.h>
#include <viua/process.h>
#include <viua/types/atom.h>
#include <viua/types/pointer.h>
#include <viua/types/struct.h>
#include <viua/types/vector.h>


auto viua::process::Process::opstruct(Op_address_type addr) -> Op_address_type
{
    viua::kernel::Register* target = nullptr;
    std::tie(addr, target) =
        viua::bytecode::decoder::operands::fetch_register(addr, this);

    *target = std::make_unique<viua::types::Struct>();

    return addr;
}

auto viua::process::Process::opstructinsert(Op_address_type addr)
    -> Op_address_type
{
    viua::types::Struct* struct_operand = nullptr;
    std::tie(addr, struct_operand) =
        viua::bytecode::decoder::operands::fetch_object_of<viua::types::Struct>(
            addr, this);

    viua::types::Atom* key = nullptr;
    std::tie(addr, key) =
        viua::bytecode::decoder::operands::fetch_object_of<viua::types::Atom>(
            addr, this);

    if (viua::bytecode::decoder::operands::get_operand_type(addr)
        == OT_POINTER) {
        viua::types::Value* source = nullptr;
        std::tie(addr, source) =
            viua::bytecode::decoder::operands::fetch_object(addr, this);
        struct_operand->insert(*key, source->copy());
    } else {
        viua::kernel::Register* source = nullptr;
        std::tie(addr, source) =
            viua::bytecode::decoder::operands::fetch_register(addr, this);
        struct_operand->insert(*key, source->give());
    }

    return addr;
}

auto viua::process::Process::opstructremove(Op_address_type addr)
    -> Op_address_type
{
    bool void_target = viua::bytecode::decoder::operands::is_void(addr);
    viua::kernel::Register* target = nullptr;

    if (not void_target) {
        std::tie(addr, target) =
            viua::bytecode::decoder::operands::fetch_register(addr, this);
    } else {
        addr = viua::bytecode::decoder::operands::fetch_void(addr);
    }

    viua::types::Struct* struct_operand = nullptr;
    std::tie(addr, struct_operand) =
        viua::bytecode::decoder::operands::fetch_object_of<viua::types::Struct>(
            addr, this);

    viua::types::Atom* key = nullptr;
    std::tie(addr, key) =
        viua::bytecode::decoder::operands::fetch_object_of<viua::types::Atom>(
            addr, this);

    std::unique_ptr<viua::types::Value> result{struct_operand->remove(*key)};
    if (not void_target) {
        *target = std::move(result);
    }

    return addr;
}

auto viua::process::Process::opstructat(Op_address_type addr) -> Op_address_type
{
    bool void_target = viua::bytecode::decoder::operands::is_void(addr);
    viua::kernel::Register* target = nullptr;

    if (not void_target) {
        std::tie(addr, target) =
            viua::bytecode::decoder::operands::fetch_register(addr, this);
    } else {
        addr = viua::bytecode::decoder::operands::fetch_void(addr);
    }

    viua::types::Struct* struct_operand = nullptr;
    std::tie(addr, struct_operand) =
        viua::bytecode::decoder::operands::fetch_object_of<viua::types::Struct>(
            addr, this);

    viua::types::Atom* key = nullptr;
    std::tie(addr, key) =
        viua::bytecode::decoder::operands::fetch_object_of<viua::types::Atom>(
            addr, this);

    if (not void_target) {
        *target = struct_operand->at(*key)->pointer(this);
    }

    return addr;
}

auto viua::process::Process::opstructkeys(Op_address_type addr)
    -> Op_address_type
{
    viua::kernel::Register* target = nullptr;
    std::tie(addr, target) =
        viua::bytecode::decoder::operands::fetch_register(addr, this);

    viua::types::Struct* struct_operand = nullptr;
    std::tie(addr, struct_operand) =
        viua::bytecode::decoder::operands::fetch_object_of<viua::types::Struct>(
            addr, this);

    auto struct_keys = struct_operand->keys();
    auto keys        = std::make_unique<viua::types::Vector>();
    for (auto const& each : struct_keys) {
        keys->push(std::make_unique<viua::types::Atom>(each));
    }

    *target = std::move(keys);

    return addr;
}
