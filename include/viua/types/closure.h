/*
 *  Copyright (C) 2015, 2016 Marek Marecki
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

#ifndef VIUA_TYPE_CLOSURE_H
#define VIUA_TYPE_CLOSURE_H

#pragma once

#include <string>
#include <memory>
#include <viua/bytecode/bytetypedef.h>
#include <viua/kernel/registerset.h>
#include <viua/types/function.h>


namespace viua {
    namespace types {
        class Closure : public Function {
            /** Closure type.
             */
                std::unique_ptr<viua::kernel::RegisterSet> local_register_set;
                std::string function_name;

            public:

                virtual std::string type() const;
                virtual std::string str() const;
                virtual std::string repr() const;

                virtual bool boolean() const;

                virtual std::unique_ptr<Type> copy() const override;

                std::string name() const;
                viua::kernel::RegisterSet* rs() const;
                void set(unsigned, std::unique_ptr<viua::types::Type>);

                Closure(const std::string&, std::unique_ptr<viua::kernel::RegisterSet>);
                virtual ~Closure();
        };
    }
}


#endif
