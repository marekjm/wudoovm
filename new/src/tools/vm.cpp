#include <viua/arch/arch.h>
#include <viua/arch/ops.h>
#include <viua/arch/ins.h>

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


struct Value {
    enum class Unboxed_type : uint8_t {
        Void = 0,
        Byte,
        Integer_signed,
        Integer_unsigned,
        Float_single,
        Float_double,
    };
    Unboxed_type type_of_unboxed;
    std::variant<uint64_t, void*> value;

    auto is_boxed() const -> bool
    {
        return std::holds_alternative<void*>(value);
    }
    auto is_void() const -> bool
    {
        return ((not is_boxed()) and type_of_unboxed == Value::Unboxed_type::Void);
    }
};

namespace machine::core::ins {
    auto execute(std::vector<Value>& registers, viua::arch::ins::ADD const op) -> void
    {
        auto& out = registers.at(op.instruction.out.index);
        auto& lhs = registers.at(op.instruction.lhs.index);
        auto& rhs = registers.at(op.instruction.rhs.index);

        out.type_of_unboxed = lhs.type_of_unboxed;
        out.value = (std::get<uint64_t>(lhs.value) + std::get<uint64_t>(rhs.value));

        std::cerr << "    " + viua::arch::ops::to_string(op.instruction.opcode)
            + " %" + std::to_string(static_cast<int>(op.instruction.out.index))
            + ", %" + std::to_string(static_cast<int>(op.instruction.lhs.index))
            + ", %" + std::to_string(static_cast<int>(op.instruction.rhs.index))
            + "\n";
    }
    auto execute(std::vector<Value>& registers, viua::arch::ins::SUB const op) -> void
    {
        auto& out = registers.at(op.instruction.out.index);
        auto& lhs = registers.at(op.instruction.lhs.index);
        auto& rhs = registers.at(op.instruction.rhs.index);

        out.type_of_unboxed = lhs.type_of_unboxed;
        out.value = (std::get<uint64_t>(lhs.value) - std::get<uint64_t>(rhs.value));

        std::cerr << "    " + viua::arch::ops::to_string(op.instruction.opcode)
            + " %" + std::to_string(static_cast<int>(op.instruction.out.index))
            + ", %" + std::to_string(static_cast<int>(op.instruction.lhs.index))
            + ", %" + std::to_string(static_cast<int>(op.instruction.rhs.index))
            + "\n";
    }
    auto execute(std::vector<Value>& registers, viua::arch::ins::MUL const op) -> void
    {
        auto& out = registers.at(op.instruction.out.index);
        auto& lhs = registers.at(op.instruction.lhs.index);
        auto& rhs = registers.at(op.instruction.rhs.index);

        out.type_of_unboxed = lhs.type_of_unboxed;
        out.value = (std::get<uint64_t>(lhs.value) * std::get<uint64_t>(rhs.value));

        std::cerr << "    " + viua::arch::ops::to_string(op.instruction.opcode)
            + " %" + std::to_string(static_cast<int>(op.instruction.out.index))
            + ", %" + std::to_string(static_cast<int>(op.instruction.lhs.index))
            + ", %" + std::to_string(static_cast<int>(op.instruction.rhs.index))
            + "\n";
    }
    auto execute(std::vector<Value>& registers, viua::arch::ins::DIV const op) -> void
    {
        auto& out = registers.at(op.instruction.out.index);
        auto& lhs = registers.at(op.instruction.lhs.index);
        auto& rhs = registers.at(op.instruction.rhs.index);

        out.type_of_unboxed = lhs.type_of_unboxed;
        out.value = (std::get<uint64_t>(lhs.value) / std::get<uint64_t>(rhs.value));

        std::cerr << "    " + viua::arch::ops::to_string(op.instruction.opcode)
            + " %" + std::to_string(static_cast<int>(op.instruction.out.index))
            + ", %" + std::to_string(static_cast<int>(op.instruction.lhs.index))
            + ", %" + std::to_string(static_cast<int>(op.instruction.rhs.index))
            + "\n";
    }

    auto execute(std::vector<Value>& registers, viua::arch::ins::DELETE const op) -> void
    {
        auto& target = registers.at(op.instruction.out.index);

        target.type_of_unboxed = Value::Unboxed_type::Void;
        target.value = uint64_t{0};

        std::cerr << "    " + viua::arch::ops::to_string(op.instruction.opcode)
            + ", %" + std::to_string(static_cast<int>(op.instruction.out.index))
            + "\n";
    }

    auto execute(std::vector<Value>& registers, viua::arch::ins::LUI const op) -> void
    {
        auto& value = registers.at(op.instruction.out.index);
        value.type_of_unboxed = Value::Unboxed_type::Integer_signed;
        value.value = (op.instruction.immediate << 28);

        std::cerr << "    " + viua::arch::ops::to_string(op.instruction.opcode)
            + " %" + std::to_string(static_cast<int>(op.instruction.out.index))
            + ", " + std::to_string(op.instruction.immediate)
            + "\n";
    }
    auto execute(std::vector<Value>& registers, viua::arch::ins::LUIU const op) -> void
    {
        auto& value = registers.at(op.instruction.out.index);
        value.type_of_unboxed = Value::Unboxed_type::Integer_unsigned;
        value.value = (op.instruction.immediate << 28);

        std::cerr << "    " + viua::arch::ops::to_string(op.instruction.opcode)
            + " %" + std::to_string(static_cast<int>(op.instruction.out.index))
            + ", " + std::to_string(op.instruction.immediate)
            + "\n";
    }

    auto execute(std::vector<Value>& registers, viua::arch::ins::ADDI const op) -> void
    {
        auto& out = registers.at(op.instruction.out.index);
        auto const base = (op.instruction.in.is_void()
            ? 0
            : std::get<uint64_t>(registers.at(op.instruction.in.index).value));

        out.type_of_unboxed = Value::Unboxed_type::Integer_signed;
        out.value = (base + op.instruction.immediate);

        std::cerr << "    " + viua::arch::ops::to_string(op.instruction.opcode)
            + " %" + std::to_string(static_cast<int>(op.instruction.out.index))
            + ", void" // FIXME it's not always void
            + ", " + std::to_string(op.instruction.immediate)
            + "\n";
    }
    auto execute(std::vector<Value>& registers, viua::arch::ins::ADDIU const op) -> void
    {
        auto& out = registers.at(op.instruction.out.index);
        auto const base = (op.instruction.in.is_void()
            ? 0
            : std::get<uint64_t>(registers.at(op.instruction.in.index).value));

        out.type_of_unboxed = Value::Unboxed_type::Integer_unsigned;
        out.value = (base + op.instruction.immediate);

        std::cerr << "    " + viua::arch::ops::to_string(op.instruction.opcode)
            + " %" + std::to_string(static_cast<int>(op.instruction.out.index))
            + ", void" // FIXME it's not always void
            + ", " + std::to_string(op.instruction.immediate)
            + "\n";
    }

    auto execute(std::vector<Value>& registers, viua::arch::ins::EBREAK const) -> void
    {
        for (auto i = size_t{0}; i < registers.size(); ++i) {
            auto const& each = registers.at(i);
            if (each.is_void()) {
                continue;
            }

            std::cerr << "[" << std::setw(3) << i << "] ";

            if (each.is_boxed()) {
                std::cerr << "<boxed>\n";
                continue;
            }

            switch (each.type_of_unboxed) {
                case Value::Unboxed_type::Void:
                    break;
                case Value::Unboxed_type::Byte:
                    std::cerr
                        << "by "
                        << std::hex
                        << std::setw(2)
                        << std::setfill('0')
                        << static_cast<uint8_t>(std::get<uint64_t>(each.value))
                        << "\n";
                    break;
                case Value::Unboxed_type::Integer_signed:
                    std::cerr
                        << "is "
                        << std::hex
                        << std::setw(16)
                        << std::setfill('0')
                        << std::get<uint64_t>(each.value)
                        << " "
                        << std::dec
                        << static_cast<int64_t>(std::get<uint64_t>(each.value))
                        << "\n";
                    break;
                case Value::Unboxed_type::Integer_unsigned:
                    std::cerr
                        << "iu "
                        << std::hex
                        << std::setw(16)
                        << std::setfill('0')
                        << std::get<uint64_t>(each.value)
                        << " "
                        << std::dec
                        << std::get<uint64_t>(each.value)
                        << "\n";
                    break;
                case Value::Unboxed_type::Float_single:
                    std::cerr
                        << "fl "
                        << std::hex
                        << std::setw(8)
                        << std::setfill('0')
                        << static_cast<float>(std::get<uint64_t>(each.value))
                        << " "
                        << std::dec
                        << static_cast<float>(std::get<uint64_t>(each.value))
                        << "\n";
                    break;
                case Value::Unboxed_type::Float_double:
                    std::cerr
                        << "db "
                        << std::hex
                        << std::setw(16)
                        << std::setfill('0')
                        << static_cast<double>(std::get<uint64_t>(each.value))
                        << " "
                        << std::dec
                        << static_cast<double>(std::get<uint64_t>(each.value))
                        << "\n";
                    break;
            }
        }
    }

    auto execute(std::vector<Value>& registers, viua::arch::instruction_type const* const ip)
        -> viua::arch::instruction_type const*
    {
        auto const raw = *ip;

        auto const opcode = static_cast<viua::arch::opcode_type>(raw & viua::arch::ops::OPCODE_MASK);
        auto const format = static_cast<viua::arch::ops::FORMAT>(opcode & viua::arch::ops::FORMAT_MASK);

        switch (format) {
            case viua::arch::ops::FORMAT::T:
            {
                auto instruction = viua::arch::ops::T::decode(raw);
                switch (static_cast<viua::arch::ops::OPCODE_T>(opcode)) {
                    case viua::arch::ops::OPCODE_T::ADD:
                        execute(registers, viua::arch::ins::ADD{instruction});
                        break;
                    case viua::arch::ops::OPCODE_T::MUL:
                        execute(registers, viua::arch::ins::MUL{instruction});
                        break;
                    default:
                        std::cerr << "unimplemented T instruction\n";
                        return nullptr;
                }
                break;
            }
            case viua::arch::ops::FORMAT::S:
            {
                auto instruction = viua::arch::ops::S::decode(raw);
                switch (static_cast<viua::arch::ops::OPCODE_S>(opcode)) {
                    case viua::arch::ops::OPCODE_S::DELETE:
                        execute(registers, viua::arch::ins::DELETE{instruction});
                        break;
                    default:
                        std::cerr << "unimplemented S instruction\n";
                        return nullptr;
                }
                break;
            }
            case viua::arch::ops::FORMAT::E:
            {
                auto instruction = viua::arch::ops::E::decode(raw);
                switch (static_cast<viua::arch::ops::OPCODE_E>(opcode)) {
                    case viua::arch::ops::OPCODE_E::LUI:
                        execute(registers, viua::arch::ins::LUI{instruction});
                        break;
                    case viua::arch::ops::OPCODE_E::LUIU:
                        execute(registers, viua::arch::ins::LUIU{instruction});
                        break;
                }
                break;
            }
            case viua::arch::ops::FORMAT::R:
            {
                auto instruction = viua::arch::ops::R::decode(raw);
                switch (static_cast<viua::arch::ops::OPCODE_R>(opcode)) {
                    case viua::arch::ops::OPCODE_R::ADDI:
                        execute(registers, viua::arch::ins::ADDI{instruction});
                        break;
                    case viua::arch::ops::OPCODE_R::ADDIU:
                        execute(registers, viua::arch::ins::ADDIU{instruction});
                        break;
                }
                break;
            }
            case viua::arch::ops::FORMAT::N:
            {
                std::cerr << "    " + viua::arch::ops::to_string(opcode) + "\n";
                switch (static_cast<viua::arch::ops::OPCODE_N>(opcode)) {
                    case viua::arch::ops::OPCODE_N::NOOP:
                        break;
                    case viua::arch::ops::OPCODE_N::HALT:
                        return nullptr;
                    case viua::arch::ops::OPCODE_N::EBREAK:
                        execute(registers, viua::arch::ins::EBREAK{
                            viua::arch::ops::N::decode(raw)});
                        break;
                }
                break;
            }
            case viua::arch::ops::FORMAT::D:
            case viua::arch::ops::FORMAT::F:
                std::cerr << "unimplemented instruction: "
                    << viua::arch::ops::to_string(opcode)
                    << "\n";
                return nullptr;
        }

        return (ip + 1);
    }
}

namespace {
    auto run_instruction(std::vector<Value>& registers, uint64_t const* ip) -> uint64_t const*
    {
        auto instruction = uint64_t{};
        do {
            instruction = *ip;
            ip = machine::core::ins::execute(registers, ip);
        } while ((ip != nullptr) and (instruction & viua::arch::ops::GREEDY));

        return ip;
    }

    auto run(
          std::vector<Value>& registers
        , uint64_t const* ip
        , std::tuple<std::string_view const, uint64_t const*, uint64_t const*> ip_range
    ) -> void
    {
        auto const [ module, ip_begin, ip_end ] = ip_range;

        constexpr auto PREEMPTION_THRESHOLD = size_t{2};

        while (ip != ip_end) {
            auto const ip_before = ip;

            std::cerr << "cycle at " << module << "+0x"
                << std::hex << std::setw(8) << std::setfill('0')
                << ((ip - ip_begin) * sizeof(viua::arch::instruction_type))
                << std::dec << "\n";

            for (auto i = size_t{0}; i < PREEMPTION_THRESHOLD and ip != ip_end; ++i) {
                /*
                 * This is needed to detect greedy bundles and adjust preemption
                 * counter appropriately. If a greedy bundle contains more
                 * instructions than the preemption threshold allows the process
                 * will be suspended immediately.
                 */
                auto const greedy = (*ip & viua::arch::ops::GREEDY);
                auto const bundle_ip = ip;

                std::cerr << "  "
                    << (greedy ? "bundle" : "single")
                    << " "
                    << std::hex << std::setw(2) << std::setfill('0')
                    << i << std::dec << "\n";

                ip = run_instruction(registers, ip);

                /*
                 * Halting instruction returns nullptr because it does not know
                 * where the end of bytecode lies. This is why we have to watch
                 * out for null pointer here.
                 */
                ip = (ip == nullptr ? ip_end : ip);

                /*
                 * If the instruction was a greedy bundle instead of a single
                 * one, the preemption counter has to be adjusted. It may be the
                 * case that the bundle already hit the preemption threshold.
                 */
                if (greedy and ip != ip_end) {
                    i += (ip - bundle_ip) - 1;
                }
            }

            if (ip == ip_end) {
                std::cerr << "halted\n";
                break;
            } else {
                std::cerr << "preempted after " << (ip - ip_before) << " ops\n";
            }

            if constexpr (false) {
                /*
                 * FIXME Limit the amount of instructions executed per second
                 * for debugging purposes. Once everything works as it should,
                 * remove this code.
                 */
                using namespace std::literals;
                std::this_thread::sleep_for(160ms);
            }
        }
    }
}

auto main(int argc, char* argv[]) -> int
{
    /*
     * If invoked with some operands, use the first of them as the
     * binary to load and execute. It most probably will be the sample
     * executable generated by an earlier invokation of the codec
     * testing program.
     */
    auto const executable_path = std::string{(argc > 1)
        ? argv[1]
        : "./a.out"};
    std::array<viua::arch::instruction_type, 128> text {};
    {
        auto const a_out = open(executable_path.c_str(), O_RDONLY);
        if (a_out == -1) {
            close(a_out);
            exit(1);
        }

        Elf64_Ehdr elf_header {};
        read(a_out, &elf_header, sizeof(elf_header));

        /*
         * We need to skip a few program headers which are just used to make
         * the file a proper ELF as recognised by file(1) and readelf(1).
         */
        Elf64_Phdr program_header {};
        read(a_out, &program_header, sizeof(program_header)); // skip magic PT_NULL
        read(a_out, &program_header, sizeof(program_header)); // skip PT_INTERP

        /*
         * Then comes the actually useful program header describing PT_LOAD
         * segment with .text section containing the instructions we need to
         * run the program.
         */
        read(a_out, &program_header, sizeof(program_header));

        lseek(a_out, program_header.p_offset, SEEK_SET);
        read(a_out, text.data(), program_header.p_filesz);

        std::cout
            << "[vm] loaded " << program_header.p_filesz
            << " byte(s) of .text section from PT_LOAD segment of "
            << executable_path << "\n";
        std::cout
            << "[vm] loaded " << (program_header.p_filesz / sizeof(decltype(text)::value_type))
            << " instructions\n";

        close(a_out);
    }

    auto registers = std::vector<Value>(256);
    run(registers, text.data(), { (executable_path + "[.text]"), text.begin(), text.end() });

    return 0;
}
