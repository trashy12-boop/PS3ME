#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Emit MIDP native layouts from loaded classes without serializing a 32-bit ROM.

Padding follows the real ClassFileParser offsets and descriptor instance sizes.
The existing GenerateROMStructs option selects layouts-only on 64-bit hosts.
"""
from patch_utils import replace,write_text_if_changed
import re

def apply(source):
    vm=source/'cldc/src/vm/share'
    p=vm/'ROM/SourceROMWriter.cpp'
    if 'PS3ME: emitting native layouts' not in p.read_text():
      replace(p,'  ROMWriter::start(JVM_SINGLE_ARG_CHECK);\n  _optimizer.initialize',
      '  ROMWriter::start(JVM_SINGLE_ARG_CHECK);\n'
      '  if(sizeof(void*)==8 && GenerateROMStructs && !GenerateROMStructsStatics) {\n'
      '    tty->print_cr("PS3ME: emitting native layouts; ROM serialization is disabled");\n'
      '    ROMStructsWriter layouts;\n    layouts.write(this JVM_CHECK);\n'
      '    set_state(STATE_DONE);\n    save_file_streams();\n    return;\n  }\n'
      '  _optimizer.initialize')
    replace(p,'    ROMStructsWriter layouts;\n    layouts.write(this JVM_CHECK);',
      '    _optimizer.initialize(&_optimizer_log_stream JVM_CHECK);\n'
      '    ROMStructsWriter layouts;\n    layouts.write(this JVM_CHECK);')
    replace(p,'bool SourceROMWriter::execute0(JVM_SINGLE_ARG_TRAPS) {',
      'bool SourceROMWriter::execute0(JVM_SINGLE_ARG_TRAPS) {\n  if(state()>=STATE_DONE) return false;')
    p=vm/'ROM/ROMStructsWriter.hpp'
    replace(p,'  static const char *reserved_words[];',
      '  static const char *reserved_words[];\n'
      '  static int _struct_offset;\n  static int _pad_serial;')
    p=vm/'ROM/ROMStructsWriter.cpp';s=p.read_text()
    if 'int ROMStructsWriter::_struct_offset=0;' not in s:
        at=s.index('const char *ROMStructsWriter::reserved_words')
        s=s[:at]+'int ROMStructsWriter::_struct_offset=0;\nint ROMStructsWriter::_pad_serial=0;\n\n'+s[at:]
    write_text_if_changed(p,s)
    replace(p,'  stream->print_cr("    int length;");',
      '  stream->print_cr("    int length;");\n'
      '  if(Array::base_offset() > (int)(sizeof(void*)+sizeof(jint)))\n'
      '    stream->print_cr("    unsigned char __array_pad[%d];",Array::base_offset()-(int)(sizeof(void*)+sizeof(jint)));')
    replace(p,'  stream->print_cr("struct %s {", byte_array.base_address());',
                '  _struct_offset=0;\n  stream->print_cr("struct %s {", byte_array.base_address());')
    replace(p,'    stream->print_cr("\\tvoid * __do_not_use__;");',
                '    stream->print_cr("\\tvoid * __do_not_use__;");\n    _struct_offset=sizeof(void*);')
    s=p.read_text()
    if 'native layout gap' not in s:
        s=s.replace('      last_size = write_rom_struct_type(stream, &f JVM_CHECK);',
          '      // native layout gap: Java ints occupy native slots in this port.\n'
          '      while(_struct_offset<offset) {\n'
          '        stream->print_cr("jbyte ___pad%d;",++_pad_serial);\n'
          '        ++_struct_offset;\n        stream->print("    ");\n      }\n'
          '      last_size = write_rom_struct_type(stream, &f JVM_CHECK);\n'
          '      _struct_offset=offset+last_size;')
        begin=s.index('  // Explicitly add padding whenever')
        end=s.index('\n}\n',begin)
        s=s[:begin]+'''  // Preserve the complete superclass size before emitting subclass fields.
  const int limit=klass->instance_size().fixed_value();
  while(_struct_offset<limit) {
    stream->print_cr("    jbyte ___pad%d;",++_pad_serial);
    ++_struct_offset;
  }'''+s[end:]
    s=s.replace('  int size = 4;', '  int size = sizeof(void*);')
    s=s.replace('    size = 4;\n    FieldType::Raw ft',
                '    size = sizeof(void*);\n    FieldType::Raw ft')
    write_text_if_changed(p,s)

