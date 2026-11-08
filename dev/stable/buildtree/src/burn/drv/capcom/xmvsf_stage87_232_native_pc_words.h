#ifndef XMVSF_STAGE87_232_NATIVE_PC_WORDS_H
#define XMVSF_STAGE87_232_NATIVE_PC_WORDS_H
// ROM-free helper: CPS2 decrypted 68000 program bytes are stored in
// host-word-swapped order. This is read-only; no original ROM patch.
#include <cstdint>
namespace XM87232NativePC {
static inline unsigned opcodeWord(const std::uint8_t* op,unsigned len,
                                   unsigned pc){
    if(!op || (pc&1u) || pc+1u>=len)return 0u;
    return (unsigned(op[pc^1u])<<8u)|unsigned(op[(pc+1u)^1u]);
}
}
#endif
