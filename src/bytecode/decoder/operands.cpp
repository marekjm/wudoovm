/*
 *  Copyright (C) 2016 Marek Marecki
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
#include <viua/bytecode/operand_types.h>
#include <viua/types/type.h>
#include <viua/types/integer.h>
#include <viua/types/pointer.h>
#include <viua/process.h>
#include <viua/exceptions.h>
#include <viua/bytecode/decoder/operands.h>
using namespace std;


template<class T> static auto extract(viua::internals::types::byte *ip) -> T {
    return *reinterpret_cast<T*>(ip);
}
template<class T> static auto extract(const viua::internals::types::byte *ip) -> T {
    return *reinterpret_cast<T*>(ip);
}
template<class T> static auto extract_ptr(viua::internals::types::byte *ip) -> T* {
    return reinterpret_cast<T*>(ip);
}

auto viua::bytecode::decoder::operands::get_operand_type(const viua::internals::types::byte *ip) -> OperandType {
    return extract<const OperandType>(ip);
}

auto viua::bytecode::decoder::operands::is_void(const viua::internals::types::byte *ip) -> bool {
    OperandType ot = get_operand_type(ip);
    return (ot == OT_VOID);
}

auto viua::bytecode::decoder::operands::fetch_void(viua::internals::types::byte *ip) -> viua::internals::types::byte* {
    OperandType ot = get_operand_type(ip);
    ++ip;

    if (ot != OT_VOID) {
        throw new viua::types::Exception("decoded invalid operand type");
    }

    return ip;
}

auto viua::bytecode::decoder::operands::fetch_operand_type(viua::internals::types::byte *ip) -> tuple<viua::internals::types::byte*, OperandType> {
    OperandType ot = get_operand_type(ip);
    ++ip;
    return tuple<viua::internals::types::byte*, OperandType>{ip, ot};
}

static auto extract_register_index(viua::internals::types::byte *ip, viua::process::Process *process, bool pointers_allowed = false) -> tuple<viua::internals::types::byte*, viua::internals::types::register_index> {
    OperandType ot = viua::bytecode::decoder::operands::get_operand_type(ip);
    ++ip;

    viua::internals::types::register_index register_index = 0;
    if (ot == OT_REGISTER_INDEX or ot == OT_REGISTER_REFERENCE or (pointers_allowed and ot == OT_POINTER)) {
        // FIXME extract RS type
        ip += sizeof(viua::internals::RegisterSets);

        register_index = extract<viua::internals::types::register_index>(ip);
        ip += sizeof(viua::internals::types::register_index);
    } else {
        throw new viua::types::Exception("decoded invalid operand type");
    }
    if (ot == OT_REGISTER_REFERENCE) {
        auto i = static_cast<viua::types::Integer*>(process->obtain(register_index));
        // FIXME Number::negative() -> bool is needed
        if (i->as_int32() < 0) {
            throw new viua::types::Exception("register indexes cannot be negative");
        }
        register_index = i->as_uint32();
    }
    return tuple<viua::internals::types::byte*, viua::internals::types::register_index>(ip, register_index);
}
static auto extract_register_type_and_index(viua::internals::types::byte *ip, viua::process::Process *process, bool pointers_allowed = false) -> tuple<viua::internals::types::byte*, viua::internals::RegisterSets, viua::internals::types::register_index> {
    OperandType ot = viua::bytecode::decoder::operands::get_operand_type(ip);
    ++ip;

    viua::internals::RegisterSets register_type = viua::internals::RegisterSets::LOCAL;
    viua::internals::types::register_index register_index = 0;
    if (ot == OT_REGISTER_INDEX or ot == OT_REGISTER_REFERENCE or (pointers_allowed and ot == OT_POINTER)) {
        register_type = extract<viua::internals::RegisterSets>(ip);
        // FIXME extract RS type
        ip += sizeof(viua::internals::RegisterSets);

        register_index = extract<viua::internals::types::register_index>(ip);
        ip += sizeof(viua::internals::types::register_index);
    } else {
        throw new viua::types::Exception("decoded invalid operand type");
    }
    if (ot == OT_REGISTER_REFERENCE) {
        auto i = static_cast<viua::types::Integer*>(process->obtain(register_index));
        // FIXME Number::negative() -> bool is needed
        if (i->as_int32() < 0) {
            throw new viua::types::Exception("register indexes cannot be negative");
        }
        register_index = i->as_uint32();
    }
    return tuple<viua::internals::types::byte*, viua::internals::RegisterSets, viua::internals::types::register_index>(ip, register_type, register_index);
}
auto viua::bytecode::decoder::operands::fetch_register_index(viua::internals::types::byte *ip, viua::process::Process *process) -> tuple<viua::internals::types::byte*, viua::internals::types::register_index> {
    return extract_register_index(ip, process);
}

auto viua::bytecode::decoder::operands::fetch_register(viua::internals::types::byte *ip, viua::process::Process *process) -> tuple<viua::internals::types::byte*, viua::kernel::Register*> {
    viua::internals::RegisterSets register_type = viua::internals::RegisterSets::LOCAL;
    viua::internals::types::register_index target = 0;
    tie(ip, register_type, target) = extract_register_type_and_index(ip, process);
    return tuple<viua::internals::types::byte*, viua::kernel::Register*>(ip, process->register_at(target, register_type));
}

auto viua::bytecode::decoder::operands::fetch_timeout(viua::internals::types::byte *ip, viua::process::Process*) -> tuple<viua::internals::types::byte*, viua::internals::types::timeout> {
    OperandType ot = viua::bytecode::decoder::operands::get_operand_type(ip);
    ++ip;

    viua::internals::types::timeout value = 0;
    if (ot == OT_INT) {
        value = *reinterpret_cast<decltype(value)*>(ip);
        ip += sizeof(decltype(value));
    } else {
        throw new viua::types::Exception("decoded invalid operand type");
    }
    return tuple<viua::internals::types::byte*, viua::internals::types::timeout>(ip, value);
}

auto viua::bytecode::decoder::operands::fetch_primitive_uint(viua::internals::types::byte *ip, viua::process::Process *process) -> tuple<viua::internals::types::byte*, viua::internals::types::register_index> {
    return fetch_register_index(ip, process);
}

auto viua::bytecode::decoder::operands::fetch_registerset_type(viua::internals::types::byte *ip, viua::process::Process*) -> tuple<viua::internals::types::byte*, viua::internals::types::registerset_type_marker> {
    viua::internals::types::registerset_type_marker rs_type = extract<decltype(rs_type)>(ip);
    ip += sizeof(decltype(rs_type));
    return tuple<viua::internals::types::byte*, decltype(rs_type)>(ip, rs_type);
}

auto viua::bytecode::decoder::operands::fetch_primitive_uint64(viua::internals::types::byte *ip, viua::process::Process*) -> tuple<viua::internals::types::byte*, uint64_t> {
    uint64_t integer = extract<decltype(integer)>(ip);
    ip += sizeof(decltype(integer));
    return tuple<viua::internals::types::byte*, decltype(integer)>(ip, integer);
}

auto viua::bytecode::decoder::operands::fetch_primitive_int(viua::internals::types::byte *ip, viua::process::Process* p) -> tuple<viua::internals::types::byte*, viua::internals::types::plain_int> {
    OperandType ot = viua::bytecode::decoder::operands::get_operand_type(ip);
    ++ip;

    viua::internals::types::plain_int value = 0;
    if (ot == OT_REGISTER_REFERENCE) {
        // FIXME decode rs type
        ip += sizeof(viua::internals::RegisterSets);

        auto index = *reinterpret_cast<viua::internals::types::register_index*>(ip);
        ip += sizeof(viua::internals::types::register_index);
        // FIXME once dynamic operand types are implemented the need for this cast will go away
        // because the operand *will* be encoded as a real uint
        viua::types::Integer *i = static_cast<viua::types::Integer*>(p->obtain(index));
        value = i->as_int32();
    } else if (ot == OT_INT) {
        value = *reinterpret_cast<decltype(value)*>(ip);
        ip += sizeof(decltype(value));
    } else {
        throw new viua::types::Exception("decoded invalid operand type");
    }
    return tuple<viua::internals::types::byte*, viua::internals::types::plain_int>(ip, value);
}

auto viua::bytecode::decoder::operands::fetch_raw_int(viua::internals::types::byte *ip, viua::process::Process*) -> tuple<viua::internals::types::byte*, viua::internals::types::plain_int> {
    return tuple<viua::internals::types::byte*, viua::internals::types::plain_int>((ip+sizeof(viua::internals::types::plain_int)), extract<viua::internals::types::plain_int>(ip));
}

auto viua::bytecode::decoder::operands::fetch_raw_float(viua::internals::types::byte *ip, viua::process::Process*) -> tuple<viua::internals::types::byte*, viua::internals::types::plain_float> {
    return tuple<viua::internals::types::byte*, viua::internals::types::plain_float>((ip+sizeof(viua::internals::types::plain_float)), extract<viua::internals::types::plain_float>(ip));
}

auto viua::bytecode::decoder::operands::extract_primitive_uint64(viua::internals::types::byte *ip, viua::process::Process*) -> uint64_t {
    return extract<uint64_t>(ip);
}

auto viua::bytecode::decoder::operands::fetch_primitive_string(viua::internals::types::byte *ip, viua::process::Process*) -> tuple<viua::internals::types::byte*, string> {
    string s(extract_ptr<const char>(ip));
    ip += (s.size() + 1);
    return tuple<viua::internals::types::byte*, string>(ip, s);
}

auto viua::bytecode::decoder::operands::fetch_atom(viua::internals::types::byte *ip, viua::process::Process*) -> tuple<viua::internals::types::byte*, string> {
    string s(extract_ptr<const char>(ip));
    ip += (s.size() + 1);
    return tuple<viua::internals::types::byte*, string>(ip, s);
}

auto viua::bytecode::decoder::operands::fetch_object(viua::internals::types::byte *ip, viua::process::Process *p) -> tuple<viua::internals::types::byte*, viua::types::Type*> {
    viua::internals::types::register_index register_index = 0;

    bool is_pointer = (get_operand_type(ip) == OT_POINTER);

    tie(ip, register_index) = extract_register_index(ip, p, true);
    auto object = p->obtain(register_index);

    if (is_pointer) {
        auto pointer_object = dynamic_cast<viua::types::Pointer*>(object);
        if (pointer_object == nullptr) {
            throw new viua::types::Exception("dereferenced type is not a pointer: " + object->type());
        }
        object = pointer_object->to();
    }

    return tuple<viua::internals::types::byte*, viua::types::Type*>(ip, object);
}
