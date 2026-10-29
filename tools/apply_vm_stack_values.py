#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Decode C-interpreter scalar slots consistently on 64-bit big-endian hosts."""
from pathlib import Path
from patch_utils import replace

def apply(source):
    replace(Path(source)/'cldc/src/vm/cpu/c/Interpreter_c.cpp','PUSH(0xDEADBEEF);',
                 'WORD_PUSH((ps3me_vm_slot_t)(uint32_t)0xDEADBEEF);',True)
    path=Path(source)/'cldc/src/vm/share/runtime/Frame.hpp'
    replace(path,'jint*     int_addr()  { return (jint*) base(); }',
'''jint*     int_addr()  {
    return (jint*)(base() + (HARDWARE_LITTLE_ENDIAN ? 0 : BytesPerWord-sizeof(jint)));
  }''')
    replace(path,'jint      as_int()    { return *((jint*) base()); }',
                 'jint      as_int()    { return ps3me_slot_read_int(base()); }')
    replace(path,'jint      as_ret()    { return *((jint*) base()); }',
                 'jint      as_ret()    { return ps3me_slot_read_int(base()); }')
    replace(path,'caller_sp + JavaStackDirection * (int)sizeof(jint)',
                 'caller_sp + JavaStackDirection * BytesPerWord')
    replace(path,'jfloat    as_float()  { return *((jfloat*)base()); }',
                 'jfloat    as_float()  { union { jint i; jfloat f; } v; v.i=as_int(); return v.f; }')
    replace(path,'''    jint *addr = int_addr();
    return JavaStackDirection < 0
      ? jlong_from_low_high(addr[0], addr[WordsPerStackElement])
      : jlong_from_low_high(addr[-WordsPerStackElement], addr[0]);''',
'''    return JavaStackDirection < 0
      ? jlong_from_low_high(ps3me_slot_read_int(base()), ps3me_slot_read_int(base()+BytesPerStackElement))
      : jlong_from_low_high(ps3me_slot_read_int(base()-BytesPerStackElement), ps3me_slot_read_int(base()));''')
    replace(path,'''    jint *addr = int_addr();
    temp.long_value = val;
    if (JavaStackDirection < 0) { 
      addr[WordsPerStackElement]  = temp.words[1];
      addr[0]                     = temp.words[0];
    } else { 
      addr[0]                     = temp.words[1];
      addr[-WordsPerStackElement] = temp.words[0];        
    }''', '''    temp.long_value = val;
    if (JavaStackDirection < 0) {
      ps3me_slot_write_int(base()+BytesPerStackElement, temp.words[1]);
      ps3me_slot_write_int(base(), temp.words[0]);
    } else {
      ps3me_slot_write_int(base(), temp.words[1]);
      ps3me_slot_write_int(base()-BytesPerStackElement, temp.words[0]);
    }''')
    # End-of-chain frames have no Java method. Trace collection must stop there.
    replace(Path(source)/'cldc/src/vm/share/natives/Natives.cpp',
'''bool FrameStream::skip() {
  while (_fr.is_entry_frame()) {
    if (_fr.as_EntryFrame().is_first_frame()) {
      return true;
    }
    _fr.as_EntryFrame().caller_is(_fr);
  }
  return false;
}''', '''bool FrameStream::skip() {
  while (_fr.fp() != NULL) {
    if (!_fr.is_entry_frame()) {
      OopDesc* method = _fr.as_JavaFrame().method();
      if (method == NULL || ((uintptr_t)method & (BytesPerWord-1)) != 0) {
        tty->print_cr("[PS3ME VM] invalid frame; stack trace truncated");
        return true;
      }
      return false;
    }
    if (_fr.as_EntryFrame().is_first_frame()) {
      return true;
    }
    _fr.as_EntryFrame().caller_is(_fr);
  }
  return true;
}''')

if __name__ == '__main__':
    import argparse
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('phoneme',type=Path)
    apply(p.parse_args().phoneme)
