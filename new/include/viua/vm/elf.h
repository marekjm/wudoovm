/*
 *  Copyright (C) 2022 Marek Marecki
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

#ifndef VIUA_VM_ELF_H
#define VIUA_VM_ELF_H

#include <sys/types.h>
#include <elf.h>
#include <string.h>
#include <unistd.h>

#include <map>
#include <string>
#include <optional>
#include <vector>
#include <functional>


namespace viua::vm::elf {
struct Fragment {
    size_t index{};
    std::optional<Elf64_Phdr> program_header{};
    Elf64_Shdr section_header{};
    std::vector<uint8_t> data;
};

struct Loaded_elf {
    Elf64_Ehdr header;
    std::vector<std::pair<std::string, Fragment>> fragments;

    auto find_fragment(std::string_view const) const
        -> std::optional<std::reference_wrapper<Fragment const>>;
    auto entry_point() const -> std::optional<size_t>;

    static auto fn_at(std::vector<uint8_t> const&, size_t const)
        -> std::pair<std::string, size_t>;
    auto name_function_at(size_t const offset) const
        -> std::pair<std::string, size_t>;
    auto function_table() const
        -> std::map<size_t, std::pair<std::string, size_t>>;

    static auto load(int const elf_fd) -> Loaded_elf;
};

constexpr inline auto VIUA_MAGIC = std::string_view{"\x7fVIUA\x00\x00\x00", 8};
}

#endif
