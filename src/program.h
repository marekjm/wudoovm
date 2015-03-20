#ifndef WUDOO_PROGRAM_H
#define WUDOO_PROGRAM_H

#include <string>
#include <vector>
#include <tuple>
#include <map>
#include "bytecode/bytetypedef.h"


typedef std::tuple<bool, int> int_op;
typedef std::tuple<bool, byte> byte_op;
typedef std::tuple<bool, float> float_op;


class Program {
    byte* program;
    int bytes;

    byte* addr_ptr;

    std::vector<byte*> branches;
    std::vector<byte*> branches_absolute;

    bool debug;

    int getInstructionBytecodeOffset(int, int count = -1);

    public:
    // instruction insertion interface
    Program& nop        ();

    Program& izero      (int_op);
    Program& istore     (int_op, int_op);
    Program& iadd       (int_op, int_op, int_op);
    Program& isub       (int_op, int_op, int_op);
    Program& imul       (int_op, int_op, int_op);
    Program& idiv       (int_op, int_op, int_op);

    Program& iinc       (int_op);
    Program& idec       (int_op);

    Program& ilt        (int_op, int_op, int_op);
    Program& ilte       (int_op, int_op, int_op);
    Program& igt        (int_op, int_op, int_op);
    Program& igte       (int_op, int_op, int_op);
    Program& ieq        (int_op, int_op, int_op);

    Program& fstore     (int_op, float);
    Program& fadd       (int_op, int_op, int_op);
    Program& fsub       (int_op, int_op, int_op);
    Program& fmul       (int_op, int_op, int_op);
    Program& fdiv       (int_op, int_op, int_op);

    Program& flt        (int_op, int_op, int_op);
    Program& flte       (int_op, int_op, int_op);
    Program& fgt        (int_op, int_op, int_op);
    Program& fgte       (int_op, int_op, int_op);
    Program& feq        (int_op, int_op, int_op);

    Program& bstore     (int_op, byte_op);

    Program& itof       (int_op, int_op);
    Program& ftoi       (int_op, int_op);

    Program& strstore   (int_op, std::string);

    Program& vec        (int_op);
    Program& vinsert    (int_op, int_op, int_op);
    Program& vpush      (int_op, int_op);
    Program& vpop       (int_op, int_op, int_op);
    Program& vat        (int_op, int_op, int_op);
    Program& vlen       (int_op, int_op);

    Program& lognot     (int_op);
    Program& logand     (int_op, int_op, int_op);
    Program& logor      (int_op, int_op, int_op);

    Program& move       (int_op, int_op);
    Program& copy       (int_op, int_op);
    Program& ref        (int_op, int_op);
    Program& swap       (int_op, int_op);
    Program& ress       (std::string);
    Program& tmpri      (int_op);
    Program& tmpro      (int_op);
    Program& free       (int_op);
    Program& empty      (int_op);
    Program& isnull     (int_op, int_op);

    Program& print      (int_op);
    Program& echo       (int_op);

    Program& closure    (std::string, int_op);
    Program& clframe    (int_op);
    Program& clcall     (int_op, int_op);

    Program& frame      (int_op, int_op);
    Program& param      (int_op, int_op);
    Program& paref      (int_op, int_op);
    Program& arg        (int_op, int_op);

    Program& call       (std::string, int_op);
    Program& jump       (int, bool);
    Program& branch     (int_op, int, bool, int, bool);

    Program& end        ();
    Program& halt       ();


    // after-insertion calculations
    Program& calculateBranches(unsigned offset = 0);
    Program& calculateJumps(std::vector<std::tuple<int, int> >);
    std::vector<unsigned> jumps();
    std::vector<unsigned> jumpsAbsolute();

    byte* bytecode();
    Program& fill(byte*);

    Program& setdebug(bool d = true);

    int size();
    int instructionCount();

    static uint16_t countBytes(const std::vector<std::string>&);

    Program(int bts = 2): bytes(bts), debug(false) {
        program = new byte[bytes];
        /* Filling bytecode with zeroes (which are interpreted by CPU as NOP instructions) is a safe way
         * to prevent many hiccups.
         */
        for (int i = 0; i < bytes; ++i) { program[i] = byte(0); }
        addr_ptr = program;
    }
    Program(const Program& that): program(0), bytes(that.bytes), addr_ptr(0), branches({}) {
        program = new byte[bytes];
        for (int i = 0; i < bytes; ++i) {
            program[i] = that.program[i];
        }
        addr_ptr = program+long(that.addr_ptr - that.program);
        for (unsigned i = 0; i < that.branches.size(); ++i) {
            branches.push_back(program+long(that.branches[i]-that.program));
        }
    }
    ~Program() {
        delete[] program;
    }

    Program& operator=(const Program& that) {
        if (this != &that) {
            delete[] program;
            bytes = that.bytes;
            program = new byte[bytes];
            for (int i = 0; i < bytes; ++i) {
                program[i] = that.program[i];
            }
            addr_ptr = program+long(that.addr_ptr - that.program);
            while (branches.size()) { branches.pop_back(); }
            for (unsigned i = 0; i < that.branches.size(); ++i) {
                branches.push_back(program+long(that.branches[i]-that.program));
            }
        }
        return (*this);
    }
};


#endif
