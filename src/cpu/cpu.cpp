#include <dlfcn.h>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <functional>
#include <regex>
#include <viua/bytecode/bytetypedef.h>
#include <viua/bytecode/opcodes.h>
#include <viua/bytecode/maps.h>
#include <viua/types/type.h>
#include <viua/types/integer.h>
#include <viua/types/byte.h>
#include <viua/types/string.h>
#include <viua/types/vector.h>
#include <viua/types/exception.h>
#include <viua/types/reference.h>
#include <viua/support/pointer.h>
#include <viua/support/string.h>
#include <viua/support/env.h>
#include <viua/loader.h>
#include <viua/include/module.h>
#include <viua/cpu/cpu.h>
using namespace std;


CPU& CPU::load(byte* bc) {
    /*  Load bytecode into the CPU.
     *  CPU becomes owner of loaded bytecode - meaning it will consider itself responsible for proper
     *  destruction of it, so make sure you have a copy of the bytecode.
     *
     *  Any previously loaded bytecode is freed.
     *  To free bytecode without loading anything new it is possible to call .load(0).
     *
     *  :params:
     *
     *  bc:char*    - pointer to byte array containing bytecode with a program to run
     */
    if (bytecode) { delete[] bytecode; }
    bytecode = bc;
    jump_base = bytecode;
    return (*this);
}

CPU& CPU::bytes(uint64_t sz) {
    /*  Set bytecode size, so the CPU can stop execution even if it doesn't reach HALT instruction but reaches
     *  bytecode address out of bounds.
     */
    bytecode_size = sz;
    return (*this);
}

CPU& CPU::preload() {
    /** This method preloads dynamic libraries specified by environment.
     */
    vector<string> preload_native = support::env::getpaths("VIUAPRELINK");
    for (unsigned i = 0; i < preload_native.size(); ++i) {
        loadNativeLibrary(preload_native[i]);
    }

    vector<string> preload_foreign = support::env::getpaths("VIUAPREIMPORT");
    for (unsigned i = 0; i < preload_foreign.size(); ++i) {
        loadForeignLibrary(preload_foreign[i]);
    }

    return (*this);
}

CPU& CPU::mapfunction(const string& name, uint64_t address) {
    /** Maps function name to bytecode address.
     */
    function_addresses[name] = address;
    return (*this);
}

CPU& CPU::mapblock(const string& name, uint64_t address) {
    /** Maps block name to bytecode address.
     */
    block_addresses[name] = address;
    return (*this);
}

CPU& CPU::registerExternalFunction(const string& name, ExternalFunction* function_ptr) {
    /** Registers external function in CPU.
     */
    foreign_functions[name] = function_ptr;
    return (*this);
}

CPU& CPU::registerForeignPrototype(const string& name, Prototype* proto) {
    /** Registers foreign prototype in CPU.
     */
    typesystem[name] = proto;
    return (*this);
}

CPU& CPU::registerForeignMethod(const string& name, ForeignMethod method) {
    /** Registers foreign prototype in CPU.
     */
    foreign_methods[name] = method;
    return (*this);
}


Type* CPU::fetch(unsigned index) const {
    /*  Return pointer to object at given register.
     *  This method safeguards against reaching for out-of-bounds registers and
     *  reading from an empty register.
     *
     *  :params:
     *
     *  index:int   - index of a register to fetch
     */
    Type* object = uregset->get(index);
    if (dynamic_cast<Reference*>(object)) {
        object = static_cast<Reference*>(object)->pointsTo();
    }
    return object;
}


template<class T> inline void copyvalue(Type* a, Type* b) {
    /** This is a short inline, template function to copy value between two `Type` pointers of the same polymorphic type.
     *  It is used internally by CPU.
     */
    static_cast<T>(a)->value() = static_cast<T>(b)->value();
}

void CPU::updaterefs(Type* before, Type* now) {
    /** This method updates references to a given address present in registers.
     *  It swaps old address for the new one in every register that points to the old address.
     *
     *  There is no need to delete old object in this function, as it will be deleted as soon as
     *  it is replaced in the origin register (i.e. the register that holds the original pointer to
     *  the object - the one from which all references had been derived).
     */
    // FIXME: this function should update references in all registersets
    for (unsigned i = 0; i < uregset->size(); ++i) {
        if (uregset->at(i) == before) {
            if (debug) {
                cout << "\nCPU: updating reference address in register " << i << hex << ": " << before << " -> " << now << dec << endl;
            }
            mask_t had_mask = uregset->getmask(i);
            uregset->empty(i);
            uregset->set(i, now);
            uregset->setmask(i, had_mask);
        }
    }
}

bool CPU::hasrefs(unsigned index) {
    /** This method checks if object at a given address exists as a reference in another register.
     */
    bool has = false;
    // FIXME: this should check for references in every register set; gonna be slow, isn't it?
    for (unsigned i = 0; i < uregset->size(); ++i) {
        if (i == index) continue;
        if (uregset->at(i) == uregset->at(index)) {
            has = true;
            break;
        }
    }
    return has;
}

void CPU::place(unsigned index, Type* obj) {
    /** Place an object in register with given index.
     *
     *  Before placing an object in register, a check is preformed if the register is empty.
     *  If not - the `Type` previously stored in it is destroyed.
     *
     */
    Type* old_ref_ptr = (hasrefs(index) ? uregset->at(index) : nullptr);
    uregset->set(index, obj);

    // update references *if, and only if* the register being set has references and
    // is *not marked a reference* itself, i.e. is the origin register
    if (old_ref_ptr and not uregset->isflagged(index, REFERENCE)) {
        updaterefs(old_ref_ptr, obj);
    }
}

void CPU::ensureStaticRegisters(string function_name) {
    /** Makes sure that static register set for requested function is initialized.
     */
    try {
        static_registers.at(function_name);
    } catch (const std::out_of_range& e) {
        // FIXME: amount of static registers should be customizable
        static_registers[function_name] = new RegisterSet(16);
    }
}


Frame* CPU::requestNewFrame(int arguments_size, int registers_size) {
    /** Request new frame to be prepared.
     *
     *  Creates new frame if the new-frame hook is empty.
     *  Throws an exception otherwise.
     *  Returns pointer to the newly created frame.
     */
    if (frame_new != nullptr) { throw "requested new frame while last one is unused"; }
    return (frame_new = new Frame(nullptr, arguments_size, registers_size));
}

void CPU::pushFrame() {
    /** Pushes new frame to be the current (top-most) one.
     */
    if (frames.size() > MAX_STACK_SIZE) {
        ostringstream oss;
        oss << "stack size (" << MAX_STACK_SIZE << ") exceeded with call to '" << frame_new->function_name << '/' << frame_new->args->size() << '\'';
        throw new Exception(oss.str());
    }

    uregset = frame_new->regset;
    // FIXME: remove this print
    //cout << "\npushing new frame on stack: " << hex << frame_new << dec << " (for function: " << frame_new->function_name << ')' << endl;
    if (find(frames.begin(), frames.end(), frame_new) != frames.end()) {
        ostringstream oss;
        oss << "stack corruption: frame " << hex << frame_new << dec << " for function " << frame_new->function_name << '/' << frame_new->args->size() << " pushed more than once";
        throw oss.str();
    }
    frames.push_back(frame_new);
    frame_new = nullptr;
}
void CPU::dropFrame() {
    /** Drops top-most frame from call stack.
     */
    delete frames.back();
    frames.pop_back();

    if (frames.size()) {
        uregset = frames.back()->regset;
    } else {
        uregset = regset;
    }
}


byte* CPU::callNative(byte* addr, const string& call_name, const bool& return_ref, const int& return_index, const string& real_call_name) {
    byte* call_address = nullptr;
    if (function_addresses.count(call_name)) {
        call_address = bytecode+function_addresses.at(call_name);
        jump_base = bytecode;
    } else {
        call_address = linked_functions.at(call_name).second;
        jump_base = linked_modules.at(linked_functions.at(call_name).first).second;
    }
    if (real_call_name.size()) {
        addr += (real_call_name.size()+1);
    } else {
        addr += (call_name.size()+1);
    }


    // save return address for frame
    byte* return_address = addr;

    if (frame_new == nullptr) {
        throw new Exception("function call without first_operand_index frame: use `frame 0' in source code if the function takes no parameters");
    }
    // set function name and return address
    frame_new->function_name = call_name;
    frame_new->return_address = return_address;

    frame_new->resolve_return_value_register = return_ref;
    frame_new->place_return_value_in = return_index;

    pushFrame();

    return call_address;
}
byte* CPU::callForeign(byte* addr, const string& call_name, const bool& return_ref, const int& return_index, const string& real_call_name) {
    if (real_call_name.size()) {
        addr += (real_call_name.size()+1);
    } else {
        addr += (call_name.size()+1);
    }

    // save return address for frame
    byte* return_address = addr;

    if (frame_new == nullptr) {
        throw new Exception("external function call without a frame: use `frame 0' in source code if the function takes no parameters");
    }
    // set function name and return address
    frame_new->function_name = call_name;
    frame_new->return_address = return_address;

    frame_new->resolve_return_value_register = return_ref;
    frame_new->place_return_value_in = return_index;

    Frame* frame = frame_new;

    pushFrame();

    if (foreign_functions.count(call_name) == 0) {
        throw new Exception("call to unregistered external function: " + call_name);
    }

    /* FIXME: second parameter should be a pointer to static registers or
     *        0 if function does not have static registers registered
     * FIXME: should external functions always have static registers allocated?
     */
    ExternalFunction* callback = foreign_functions.at(call_name);
    (*callback)(frame, nullptr, regset);

    // FIXME: woohoo! segfault!
    Type* returned = nullptr;
    bool returned_is_reference = false;
    int return_value_register = frames.back()->place_return_value_in;
    bool resolve_return_value_register = frames.back()->resolve_return_value_register;
    if (return_value_register != 0) {
        // we check in 0. register because it's reserved for return values
        if (uregset->at(0) == nullptr) {
            throw new Exception("return value requested by frame but external function did not set return register");
        }
        if (uregset->isflagged(0, REFERENCE)) {
            returned = uregset->get(0);
            returned_is_reference = true;
        } else {
            returned = uregset->get(0)->copy();
        }
    }

    dropFrame();

    // place return value
    if (returned and frames.size() > 0) {
        if (resolve_return_value_register) {
            return_value_register = static_cast<Integer*>(fetch(return_value_register))->value();
        }
        place(return_value_register, returned);
        if (returned_is_reference) {
            uregset->flag(return_value_register, REFERENCE);
        }
    }

    return return_address;
}
byte* CPU::callForeignMethod(byte* addr, Type* object, const string& call_name, const bool& return_ref, const int& return_index, const string& real_call_name) {
    if (real_call_name.size()) {
        addr += (real_call_name.size()+1);
    } else {
        addr += (call_name.size()+1);
    }

    // save return address for frame
    byte* return_address = addr;

    if (frame_new == nullptr) {
        throw new Exception("foreign method call without a frame");
    }
    // set function name and return address
    frame_new->function_name = call_name;
    frame_new->return_address = return_address;

    frame_new->resolve_return_value_register = return_ref;
    frame_new->place_return_value_in = return_index;

    Frame* frame = frame_new;

    pushFrame();

    if (foreign_methods.count(call_name) == 0) {
        throw new Exception("call to unregistered foreign method: " + call_name);
    }

    Reference* rf = nullptr;
    if ((rf = dynamic_cast<Reference*>(object))) {
        object = rf->pointsTo();
    }

    try {
        // FIXME: supply static and global registers to foreign functions
        foreign_methods.at(call_name)(object, frame, nullptr, nullptr);
    } catch (const std::out_of_range& e) {
        throw new Exception(e.what());
    }

    // FIXME: woohoo! segfault!
    Type* returned = nullptr;
    bool returned_is_reference = false;
    int return_value_register = frames.back()->place_return_value_in;
    bool resolve_return_value_register = frames.back()->resolve_return_value_register;
    if (return_value_register != 0) {
        // we check in 0. register because it's reserved for return values
        if (uregset->at(0) == nullptr) {
            throw new Exception("return value requested by frame but foreign method did not set return register");
        }
        if (uregset->isflagged(0, REFERENCE)) {
            returned = uregset->get(0);
            returned_is_reference = true;
        } else {
            returned = uregset->get(0)->copy();
        }
    }

    dropFrame();

    // place return value
    if (returned and frames.size() > 0) {
        if (resolve_return_value_register) {
            return_value_register = static_cast<Integer*>(fetch(return_value_register))->value();
        }
        place(return_value_register, returned);
        if (returned_is_reference) {
            uregset->flag(return_value_register, REFERENCE);
        }
    }

    return return_address;
}

void CPU::loadNativeLibrary(const string& module) {
    regex double_colon("::");
    ostringstream oss;
    oss << regex_replace(module, double_colon, "/");
    string try_path = oss.str();
    string path = support::env::viua::getmodpath(try_path, "vlib", support::env::getpaths("VIUAPATH"));
    if (path.size() == 0) { path = support::env::viua::getmodpath(try_path, "vlib", VIUAPATH); }
    if (path.size() == 0) { path = support::env::viua::getmodpath(try_path, "vlib", support::env::getpaths("VIUAAFTERPATH")); }

    if (path.size()) {
        Loader loader(path);
        loader.load();

        byte* lnk_btcd = loader.getBytecode();
        linked_modules[module] = pair<unsigned, byte*>(static_cast<unsigned>(loader.getBytecodeSize()), lnk_btcd);

        vector<string> fn_names = loader.getFunctions();
        map<string, uint64_t> fn_addrs = loader.getFunctionAddresses();
        for (unsigned i = 0; i < fn_names.size(); ++i) {
            string fn_linkname = fn_names[i];
            linked_functions[fn_linkname] = pair<string, byte*>(module, (lnk_btcd+fn_addrs[fn_names[i]]));
        }

        vector<string> bl_names = loader.getBlocks();
        map<string, uint64_t> bl_addrs = loader.getBlockAddresses();
        for (unsigned i = 0; i < bl_names.size(); ++i) {
            string bl_linkname = bl_names[i];
            linked_blocks[bl_linkname] = pair<string, byte*>(module, (lnk_btcd+bl_addrs[bl_linkname]));
        }
    } else {
        throw new Exception("failed to link: " + module);
    }
}
void CPU::loadForeignLibrary(const string& module) {
    string path = "";
    path = support::env::viua::getmodpath(module, "so", support::env::getpaths("VIUAPATH"));
    if (path.size() == 0) { path = support::env::viua::getmodpath(module, "so", VIUAPATH); }
    if (path.size() == 0) { path = support::env::viua::getmodpath(module, "so", support::env::getpaths("VIUAAFTERPATH")); }

    if (path.size() == 0) {
        throw new Exception("LinkException", ("failed to link library: " + module));
    }

    void* handle = dlopen(path.c_str(), RTLD_LAZY);

    if (handle == nullptr) {
        throw new Exception("LinkException", ("failed to open handle: " + module));
    }

    ExternalFunctionSpec* (*exports)() = nullptr;
    if ((exports = (ExternalFunctionSpec*(*)())dlsym(handle, "exports")) == nullptr) {
        throw new Exception("failed to extract interface from module: " + module);
    }

    ExternalFunctionSpec* exported = (*exports)();

    unsigned i = 0;
    while (exported[i].name != NULL) {
        registerExternalFunction(exported[i].name, exported[i].fpointer);
        ++i;
    }

    cxx_dynamic_lib_handles.push_back(handle);
}


vector<string> CPU::inheritanceChainOf(const string& type_name) {
    /** This methods returns full inheritance chain of a type.
     */
    vector<string> ichain = typesystem.at(type_name)->getAncestors();
    for (unsigned i = 0; i < ichain.size(); ++i) {
        vector<string> sub_ichain = inheritanceChainOf(ichain[i]);
        for (unsigned j = 0; j < sub_ichain.size(); ++j) {
            ichain.push_back(sub_ichain[j]);
        }
    }

    vector<string> linearised_inheritance_chain;
    unordered_set<string> pushed;

    string element;
    for (unsigned i = 0; i < ichain.size(); ++i) {
        element = ichain[i];
        if (pushed.count(element)) {
            linearised_inheritance_chain.erase(remove(linearised_inheritance_chain.begin(), linearised_inheritance_chain.end(), element), linearised_inheritance_chain.end());
        } else {
            pushed.insert(element);
        }
        linearised_inheritance_chain.push_back(element);
    }

    return ichain;
}

byte* CPU::begin() {
    /** Set instruction pointer to the execution beginning position.
     */
    return (instruction_pointer = bytecode+executable_offset);
}

CPU& CPU::iframe(Frame* frm, unsigned r) {
    /** Set initial frame.
     */
    Frame *initial_frame;
    if (frm == nullptr) {
        initial_frame = new Frame(nullptr, 0, 2);
        initial_frame->function_name = "__entry";

        Vector* cmdline = new Vector();
        for (unsigned i = 0; i < commandline_arguments.size(); ++i) {
            cmdline->push(new String(commandline_arguments[i]));
        }
        initial_frame->regset->set(1, cmdline);
    } else {
        initial_frame = frm;

        /*  If a frame was supplied to us to be the initial one,
         *  set global registers to the locals of supplied frame.
         */
        delete regset;
    }

    // set global registers
    regset = new RegisterSet(r);

    /* // set currently used register set */
    /* uregset = initial_frame->regset; */

    /* frames.push_back(initial_frame); */

    threads.push_back(new Thread(initial_frame, this));

    return (*this);
}


byte* CPU::tick() {
    byte* ip = threads[0]->tick();  // returns instruction pointer
    if (threads[0]->terminated()) {
        return nullptr;
    }
    return ip;
}

int CPU::run() {
    /*  VM CPU implementation.
     */
    if (!bytecode) {
        throw "null bytecode (maybe not loaded?)";
    }

    iframe();
    threads[0]->begin();
    while (tick()) {
        /* string s; */
        /* getline(cin, s); */
    }

    if (threads[0]->terminated()) {
        cout << "thread '0:" << hex << threads[0] << dec << "' has terminated" << endl;
        Type* e = threads[0]->getActiveException();
        cout << e << endl;

        return_code = 1;
        terminating_exception = e;
    }
    /*

    if (return_code == 0 and regset->at(0)) {
        // if return code if the default one and
        // return register is not unused
        // copy value of return register as return code
        try {
            return_code = static_cast<Integer*>(regset->get(0))->value();
        } catch (const Exception* e) {
            return_code = 1;
            return_exception = e->type();
            return_message = e->what();
        }
    }

    // delete __entry function's frame and
    // global registers
    // otherwise we get huge memory leak
    // do not delete if execution was halted because of exception
    if (return_exception == "") {
        delete frames.back();
        delete regset;
    }
    */

    return return_code;
}
