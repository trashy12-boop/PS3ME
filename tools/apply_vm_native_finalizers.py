#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Refresh finalizer flags after non-ROM static native registration."""
from pathlib import Path
from patch_utils import replace

def apply(source):
    replace(Path(source)/'cldc/src/vm/share/runtime/ClassFileParser.cpp',
'''    Natives::register_natives_for(&this_class JVM_CHECK_0);
  }
''', '''    Natives::register_natives_for(&this_class JVM_CHECK_0);
    // PS3ME: finalizer flags were computed before static native registration.
    // A non-ROM build must clear the unresolved flag once the table binds it.
    if (this_class().has_unresolved_finalizer()) {
      Method::Fast finalizer = this_class().lookup_finalizer();
      if (finalizer.not_null() && finalizer().get_native_code() !=
                                      (address)Java_void_unimplemented) {
        AccessFlags flags = this_class().access_flags();
        flags.clear_has_unresolved_finalizer();
        this_class().set_access_flags(flags);
      }
    }
  }
''')

if __name__ == '__main__':
    import argparse
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('phoneme',type=Path)
    apply(parser.parse_args().phoneme)
