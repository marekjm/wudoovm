/*
 *  Copyright (C) 2015, 2016, 2017 Marek Marecki
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

#ifndef VIUA_FRONT_ASM_H
#define VIUA_FRONT_ASM_H

#pragma once

#include <map>
#include <string>
#include <vector>
#include <viua/cg/lex.h>
#include <viua/program.h>


namespace viua { namespace front { namespace assembler {

struct invocables_t {
    std::vector<std::string> names;
    std::vector<std::string> signatures;
    std::map<std::string, std::vector<viua::cg::lex::Token>> tokens;
};

struct compilationflags_t {
    bool as_lib;

    bool verbose;
    bool debug;
    bool scream;
};


auto decode_line_tokens(std::vector<std::string> const&)
    -> std::vector<std::vector<std::string>>;
auto decode_line(std::string const&) -> std::vector<std::vector<std::string>>;

auto gather_functions(std::vector<viua::cg::lex::Token> const&) -> invocables_t;
auto gather_blocks(std::vector<viua::cg::lex::Token> const&) -> invocables_t;
auto gather_meta_information(std::vector<viua::cg::lex::Token> const&)
    -> std::map<std::string, std::string>;

auto assemble_instruction(
    Program& program,
    viua::internals::types::bytecode_size& instruction,
    viua::internals::types::bytecode_size i,
    std::vector<viua::cg::lex::Token> const& tokens,
    std::map<std::string,
             std::remove_reference<decltype(tokens)>::type::size_type>& marks)
    -> viua::internals::types::bytecode_size;
auto generate(std::vector<viua::cg::lex::Token> const&,
              invocables_t&,
              invocables_t&,
              std::string const&,
              std::string&,
              std::vector<std::string> const&,
              compilationflags_t const&) -> void;
}}}  // namespace viua::front::assembler


#endif
