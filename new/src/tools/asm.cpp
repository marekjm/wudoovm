#include <viua/arch/arch.h>
#include <viua/arch/ops.h>

#include <iostream>
#include <iomanip>
#include <chrono>
#include <map>
#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include <utility>
#include <thread>
#include <type_traits>

#include <elf.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>


auto to_loading_parts_unsigned(uint64_t const value)
    -> std::pair<uint64_t, std::pair<std::pair<uint32_t, uint32_t>, uint32_t>>
{
    constexpr auto LOW_24  = uint64_t{0x0000000000ffffff};
    constexpr auto HIGH_36 = uint64_t{0xfffffffff0000000};

    auto const high_part = ((value & HIGH_36) >> 28);
    auto const low_part = static_cast<uint32_t>(value & ~HIGH_36);

    /*
     * If the low part consists of only 24 bits we can use just two
     * instructions:
     *
     *  1/ lui to load high 36 bits
     *  2/ addi to add low 24 bits
     *
     * This reduces the overhead of loading 64-bit values.
     */
    if ((low_part & LOW_24) == low_part) {
        return { high_part, { { low_part, 0 }, 0 } };
    }

    auto const multiplier = 16;
    auto const remainder = (low_part % multiplier);
    auto const base = (low_part - remainder) / multiplier;

    return { high_part, { { base, multiplier }, remainder } };
}

namespace {
    auto op_li(uint64_t* instructions, uint64_t const value) -> uint64_t*
    {
        auto const parts = to_loading_parts_unsigned(value);

        /*
         * Only use the lui instruction of there's a reason to ie, if some of
         * the highest 36 bits are set. Otherwise, the lui is just overhead.
         */
        if (parts.first) {
            *instructions++ = viua::arch::ops::E{
                (viua::arch::ops::GREEDY
                 | static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::LUIU))
                , viua::arch::Register_access::make_local(1)
                , parts.first
            }.encode();
        }

        auto const base = parts.second.first.first;
        auto const multiplier = parts.second.first.second;

        if (multiplier != 0) {
            *instructions++ = viua::arch::ops::R{
                (viua::arch::ops::GREEDY
                 | static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::ADDIU))
                , viua::arch::Register_access::make_local(2)
                , viua::arch::Register_access::make_void()
                , base
            }.encode();
            *instructions++ = viua::arch::ops::R{
                (viua::arch::ops::GREEDY
                 | static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::ADDIU))
                , viua::arch::Register_access::make_local(3)
                , viua::arch::Register_access::make_void()
                , multiplier
            }.encode();
            *instructions++ = viua::arch::ops::T{
                (viua::arch::ops::GREEDY
                 | static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::MUL))
                , viua::arch::Register_access::make_local(2)
                , viua::arch::Register_access::make_local(2)
                , viua::arch::Register_access::make_local(3)
            }.encode();

            auto const remainder = parts.second.second;
            *instructions++ = viua::arch::ops::R{
                (viua::arch::ops::GREEDY
                 | static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::ADDIU))
                , viua::arch::Register_access::make_local(3)
                , viua::arch::Register_access::make_void()
                , remainder
            }.encode();
            *instructions++ = viua::arch::ops::T{
                (viua::arch::ops::GREEDY
                 | static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::ADD))
                , viua::arch::Register_access::make_local(2)
                , viua::arch::Register_access::make_local(2)
                , viua::arch::Register_access::make_local(3)
            }.encode();

            *instructions++ = viua::arch::ops::T{
                 static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::ADD)
                , viua::arch::Register_access::make_local(1)
                , viua::arch::Register_access::make_local(1)
                , viua::arch::Register_access::make_local(2)
            }.encode();
        } else {
            *instructions++ = viua::arch::ops::R{
                  static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::ADDIU)
                , viua::arch::Register_access::make_local(1)
                , viua::arch::Register_access::make_void()
                , base
            }.encode();
        }

        return instructions;
    }
    auto op_li(uint64_t* instructions, int64_t const value) -> uint64_t*
    {
        auto const parts = to_loading_parts_unsigned(value);

        /*
         * Only use the lui instruction of there's a reason to ie, if some of
         * the highest 36 bits are set. Otherwise, the lui is just overhead.
         */
        if (parts.first) {
            *instructions++ = viua::arch::ops::E{
                (viua::arch::ops::GREEDY
                 | static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::LUI))
                , viua::arch::Register_access::make_local(1)
                , parts.first
            }.encode();
        }

        auto const base = parts.second.first.first;
        auto const multiplier = parts.second.first.second;

        if (multiplier != 0) {
            *instructions++ = viua::arch::ops::R{
                (viua::arch::ops::GREEDY
                 | static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::ADDI))
                , viua::arch::Register_access::make_local(2)
                , viua::arch::Register_access::make_void()
                , base
            }.encode();
            *instructions++ = viua::arch::ops::R{
                (viua::arch::ops::GREEDY
                 | static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::ADDI))
                , viua::arch::Register_access::make_local(3)
                , viua::arch::Register_access::make_void()
                , multiplier
            }.encode();
            *instructions++ = viua::arch::ops::T{
                (viua::arch::ops::GREEDY
                 | static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::MUL))
                , viua::arch::Register_access::make_local(2)
                , viua::arch::Register_access::make_local(2)
                , viua::arch::Register_access::make_local(3)
            }.encode();

            auto const remainder = parts.second.second;
            *instructions++ = viua::arch::ops::R{
                (viua::arch::ops::GREEDY
                 | static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::ADDI))
                , viua::arch::Register_access::make_local(3)
                , viua::arch::Register_access::make_void()
                , remainder
            }.encode();
            *instructions++ = viua::arch::ops::T{
                (viua::arch::ops::GREEDY
                 | static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::ADD))
                , viua::arch::Register_access::make_local(2)
                , viua::arch::Register_access::make_local(2)
                , viua::arch::Register_access::make_local(3)
            }.encode();

            *instructions++ = viua::arch::ops::T{
                 static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::ADD)
                , viua::arch::Register_access::make_local(1)
                , viua::arch::Register_access::make_local(1)
                , viua::arch::Register_access::make_local(2)
            }.encode();
        } else {
            *instructions++ = viua::arch::ops::R{
                  static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::ADDI)
                , viua::arch::Register_access::make_local(1)
                , viua::arch::Register_access::make_void()
                , base
            }.encode();
        }

        return instructions;
    }
}

auto main(int argc, char*[]) -> int
{
    if constexpr (false) {
        {
            auto const tm = viua::arch::ops::T{
                  0xdead
                , viua::arch::Register_access{viua::arch::REGISTER_SET::LOCAL, true, 0xff}
                , viua::arch::Register_access{viua::arch::REGISTER_SET::LOCAL, true, 0x01}
                , viua::arch::Register_access{viua::arch::REGISTER_SET::LOCAL, true, 0x02}
            };
            std::cout << std::hex << std::setw(16) << std::setfill('0') << tm.encode() << "\n";
            auto const td = viua::arch::ops::T::decode(tm.encode());
            std::cout
                << (tm.opcode == td.opcode)
                << (tm.out == td.out)
                << (tm.lhs == td.lhs)
                << (tm.rhs == td.rhs)
                << "\n";
        }
        {
            auto const tm = viua::arch::ops::D{
                  0xdead
                , viua::arch::Register_access{viua::arch::REGISTER_SET::LOCAL, true, 0xff}
                , viua::arch::Register_access{viua::arch::REGISTER_SET::LOCAL, true, 0x01}
            };
            std::cout << std::hex << std::setw(16) << std::setfill('0') << tm.encode() << "\n";
            auto const td = viua::arch::ops::D::decode(tm.encode());
            std::cout
                << (tm.opcode == td.opcode)
                << (tm.out == td.out)
                << (tm.in == td.in)
                << "\n";
        }
        {
            auto const tm = viua::arch::ops::S{
                  0xdead
                , viua::arch::Register_access{viua::arch::REGISTER_SET::LOCAL, true, 0xff}
            };
            std::cout << std::hex << std::setw(16) << std::setfill('0') << tm.encode() << "\n";
            auto const td = viua::arch::ops::S::decode(tm.encode());
            std::cout
                << (tm.opcode == td.opcode)
                << (tm.out == td.out)
                << "\n";
        }
        {
            constexpr auto original_value = 3.14f;

            auto imm_in = uint32_t{};
            memcpy(&imm_in, &original_value, sizeof(imm_in));

            auto const tm = viua::arch::ops::F{
                  0xdead
                , viua::arch::Register_access{viua::arch::REGISTER_SET::LOCAL, true, 0xff}
                , imm_in
            };
            std::cout << std::hex << std::setw(16) << std::setfill('0') << tm.encode() << "\n";
            auto const td = viua::arch::ops::F::decode(tm.encode());

            auto imm_out = float{};
            memcpy(&imm_out, &td.immediate, sizeof(imm_out));

            std::cout
                << (tm.opcode == td.opcode)
                << (tm.out == td.out)
                << (tm.immediate == td.immediate)
                << (imm_out == original_value)
                << "\n";
        }
        {
            auto const tm = viua::arch::ops::E{
                  0xdead
                , viua::arch::Register_access{viua::arch::REGISTER_SET::LOCAL, true, 0xff}
                , 0xabcdef012
            };
            std::cout << std::hex << std::setw(16) << std::setfill('0') << tm.encode() << "\n";
            auto const td = viua::arch::ops::E::decode(tm.encode());
            std::cout
                << (tm.opcode == td.opcode)
                << (tm.out == td.out)
                << (tm.immediate == td.immediate)
                << "\n";
        }
        {
            auto const tm = viua::arch::ops::R{
                  0xdead
                , viua::arch::Register_access{viua::arch::REGISTER_SET::LOCAL, true, 0x55}
                , viua::arch::Register_access{viua::arch::REGISTER_SET::LOCAL, true, 0x22}
                , 0xabcdef
            };
            std::cout << std::hex << std::setw(16) << std::setfill('0') << tm.encode() << "\n";
            auto const td = viua::arch::ops::R::decode(tm.encode());
            std::cout
                << (tm.opcode == td.opcode)
                << (tm.out == td.out)
                << (tm.in == td.in)
                << (tm.immediate == td.immediate)
                << "\n";
        }
    }

    if constexpr (false) {
        auto const test_these = std::vector<uint64_t>{
              0x0000000000000000
            , 0x0000000000000001
            , 0x0000000000bedead /* low 24 */
            , 0x00000000deadbeef /* low 32 */
            , 0xdeadbeefd0adbeef /* high 36 and low 24 (special case) */
            , 0xdeadbeefd1adbeef /* all bits */
            , 0xdeadbeefd2adbeef /* all bits */
            , 0xdeadbeefd3adbeef /* all bits */
            , 0xdeadbeefd4adbeef /* all bits */
            , 0xdeadbeefd5adbeef /* all bits */
            , 0xdeadbeefd6adbeef /* all bits */
            , 0xdeadbeefd7adbeef /* all bits */
            , 0xdeadbeefd8adbeef /* all bits */
            , 0xdeadbeefd9adbeef /* all bits */
            , 0xdeadbeefdaadbeef /* all bits */
            , 0xdeadbeefdbadbeef /* all bits */
            , 0xdeadbeefdcadbeef /* all bits */
            , 0xdeadbeefddadbeef /* all bits */
            , 0xdeadbeefdeadbeef /* all bits */
            , 0xdeadbeeffdadbeef /* all bits */
            , 0xffffffffffffffff
        };

        for (auto const wanted : test_these) {
            std::cout << "\n";

            auto const parts = to_loading_parts_unsigned(wanted);

            auto high = (parts.first << 28);
            auto const low = (parts.second.first.second != 0)
                ?  ((parts.second.first.first * parts.second.first.second)
                 + parts.second.second)
                : parts.second.first.first;
            auto const got = (high | low);

            std::cout << std::hex << std::setw(16) << std::setfill('0') << wanted << "\n";
            std::cout << std::hex << std::setw(16) << std::setfill('0') << got << "\n";
            if (wanted != got) {
                std::cerr << "BAD BAD BAD!\n";
                break;
            }
        }
    }

    if constexpr (false) {
        std::cout << viua::arch::ops::to_string(0x0000) << "\n";
        std::cout << viua::arch::ops::to_string(0x0001) << "\n";
        std::cout << viua::arch::ops::to_string(0x1001) << "\n";
        std::cout << viua::arch::ops::to_string(0x9001) << "\n";
        std::cout << viua::arch::ops::to_string(0x1002) << "\n";
        std::cout << viua::arch::ops::to_string(0x1003) << "\n";
        std::cout << viua::arch::ops::to_string(0x1004) << "\n";
        std::cout << viua::arch::ops::to_string(0x5001) << "\n";
    }

    /*
     * If invoked directly, emit a sample executable binary. This makes
     * testing easy as we always can have a sample, working, known-good
     * binary produced.
     */
    if (argc == 1) {
        std::array<viua::arch::instruction_type, 32> text {};
        auto ip = text.data();

        {
            ip = op_li(ip, 0xdeadbeefdeadbeef);
            *ip++ = viua::arch::ops::S{
                (viua::arch::ops::GREEDY |
                  static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::DELETE))
                , viua::arch::Register_access::make_local(2)
            }.encode();
            *ip++ = viua::arch::ops::S{
                  static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::DELETE)
                , viua::arch::Register_access::make_local(3)
            }.encode();
            *ip++ = static_cast<uint64_t>(viua::arch::ops::OPCODE::EBREAK);

            ip = op_li(ip, 42l);
            *ip++ = static_cast<uint64_t>(viua::arch::ops::OPCODE::EBREAK);

            ip = op_li(ip, -1l);
            *ip++ = viua::arch::ops::S{
                (viua::arch::ops::GREEDY |
                  static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::DELETE))
                , viua::arch::Register_access::make_local(2)
            }.encode();
            *ip++ = viua::arch::ops::S{
                  static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::DELETE)
                , viua::arch::Register_access::make_local(3)
            }.encode();
            *ip++ = static_cast<uint64_t>(viua::arch::ops::OPCODE::EBREAK);
            *ip++ = static_cast<uint64_t>(viua::arch::ops::OPCODE::HALT);
        }

        auto const a_out = open(
              "./a.out"
            , O_CREAT|O_TRUNC|O_WRONLY
            , S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH
        );
        if (a_out == -1) {
            close(a_out);
            exit(1);
        }

        constexpr auto VIUA_MAGIC[[maybe_unused]] = "\x7fVIUA\x00\x00\x00";
        auto const VIUAVM_INTERP = std::string{"viua-vm"};

        {
            auto const ops_count = (ip - text.begin());
            auto const text_size = (ops_count * sizeof(decltype(text)::value_type));
            auto const text_offset = (
                  sizeof(Elf64_Ehdr)
                + (3 * sizeof(Elf64_Phdr))
                + (VIUAVM_INTERP.size() + 1));

            // see elf(5)
            Elf64_Ehdr elf_header {};
            elf_header.e_ident[EI_MAG0] = '\x7f';
            elf_header.e_ident[EI_MAG1] = 'E';
            elf_header.e_ident[EI_MAG2] = 'L';
            elf_header.e_ident[EI_MAG3] = 'F';
            elf_header.e_ident[EI_CLASS] = ELFCLASS64;
            elf_header.e_ident[EI_DATA] = ELFDATA2LSB;
            elf_header.e_ident[EI_VERSION] = EV_CURRENT;
            elf_header.e_ident[EI_OSABI] = ELFOSABI_STANDALONE;
            elf_header.e_ident[EI_ABIVERSION] = 0;
            elf_header.e_type = ET_EXEC;
            elf_header.e_machine = ET_NONE;
            elf_header.e_version = elf_header.e_ident[EI_VERSION];
            elf_header.e_entry = text_offset;
            elf_header.e_phoff = sizeof(elf_header);
            elf_header.e_phentsize = sizeof(Elf64_Phdr);
            elf_header.e_phnum = 3;
            elf_header.e_shoff = 0; // FIXME section header table
            elf_header.e_flags = 0; // processor-specific flags, should be 0
            elf_header.e_ehsize = sizeof(elf_header);
            write(a_out, &elf_header, sizeof(elf_header));

            Elf64_Phdr magic_for_binfmt_misc {};
            magic_for_binfmt_misc.p_type = PT_NULL;
            magic_for_binfmt_misc.p_offset = 0;
            memcpy(&magic_for_binfmt_misc.p_offset, VIUA_MAGIC, 8);
            write(a_out, &magic_for_binfmt_misc, sizeof(magic_for_binfmt_misc));

            Elf64_Phdr interpreter {};
            interpreter.p_type = PT_INTERP;
            interpreter.p_offset = (sizeof(elf_header) + 3 * sizeof(Elf64_Phdr));
            interpreter.p_filesz = VIUAVM_INTERP.size() + 1;
            interpreter.p_flags = PF_R;
            write(a_out, &interpreter, sizeof(interpreter));

            Elf64_Phdr text_segment {};
            text_segment.p_type = PT_LOAD;
            text_segment.p_offset = text_offset;
            text_segment.p_filesz = text_size;
            text_segment.p_memsz = text_size;
            text_segment.p_flags = PF_R|PF_X;
            text_segment.p_align = sizeof(viua::arch::instruction_type);
            write(a_out, &text_segment, sizeof(text_segment));

            write(a_out, VIUAVM_INTERP.c_str(), VIUAVM_INTERP.size() + 1);

            write(a_out, text.data(), text_size);
        }

        close(a_out);

        return 0;
    }

    return 0;
}
