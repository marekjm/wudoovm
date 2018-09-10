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

#ifndef VIUA_TOOLING_LIBS_STATIC_ANALYSER_H
#define VIUA_TOOLING_LIBS_STATIC_ANALYSER_H

#include <set>
#include <viua/tooling/libs/parser/parser.h>

namespace viua {
namespace tooling {
namespace libs {
namespace static_analyser {

/* struct Register_index { */
/* }; */

/* struct Register_information { */
/* }; */

/* struct Register_usage { */
/*     std::map<Register_index, Register_information> current_state; */
/* }; */

struct Analyser_state {
    std::set<std::string> functions_called;
};

class Function_state {
    viua::internals::types::register_index const local_registers_allocated = 0;
    std::vector<viua::tooling::libs::lexer::Token> local_registers_allocated_where;

    std::map<viua::internals::types::register_index, viua::tooling::libs::parser::Name_directive>
        register_renames;
    std::map<std::string, viua::internals::types::register_index>
        register_name_to_index;
    std::map<viua::internals::types::register_index, std::string>
        register_index_to_name;

    viua::internals::types::register_index iota_value = 1;

  public:
    auto rename_register(
        viua::internals::types::register_index const
        , std::string
        , viua::tooling::libs::parser::Name_directive
    ) -> void;

    auto iota(viua::tooling::libs::lexer::Token) -> viua::internals::types::register_index;

    Function_state(
            viua::internals::types::register_index const
            , std::vector<viua::tooling::libs::lexer::Token>
    );
};

auto analyse(viua::tooling::libs::parser::Cooked_fragments const&) -> void;

}}}}

#endif
