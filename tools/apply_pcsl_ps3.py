#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
from patch_utils import replace,write_text_if_changed
def apply(source):
    p=source/'pcsl/network/stubs/pcsl_network.c'
    replace(p,'javacall_result res;', 'int res; /* offline stub status */')
    root=source/'pcsl/file/posix'
    p=root/'pcsl_dir_posix.c'
    replace(p,'#include <sys/statvfs.h>',
      '#if defined(PS3ME_PPU)\n#include <lv2/sysfs.h>\n#else\n#include <sys/statvfs.h>\n#endif')
    s=p.read_text()
    if 'PS3ME storage query' not in s:
        start=s.index('    struct statvfs sbuf;',s.index('jlong pcsl_file_getfreesize('))
        end=s.index('\n}',start)
        original=s[start:end]
        native='''    /* PS3ME storage query: actual LV2 free space. */
    const jbyte* text=pcsl_string_get_utf8_data(path);
    if(!text)return -1;
    u32 block_size=0;u64 free_blocks=0;
    int status=sysFsGetFreeSize((const char*)text,&block_size,&free_blocks);
    pcsl_string_release_utf8_data(text,path);
    return status==0 ? (jlong)block_size*free_blocks : -1;'''
        s=s[:start]+'#if defined(PS3ME_PPU)\n'+native+'\n#else\n'+original+'\n#endif'+s[end:]
        start=s.index('    struct statvfs sbuf;',s.index('jlong pcsl_file_gettotalsize('))
        end=s.index('\n}',start);original=s[start:end]
        s=s[:start]+'#if defined(PS3ME_PPU)\n    (void)path;\n    return -1; /* total capacity is not exposed by this adapter */\n#else\n'+original+'\n#endif'+s[end:]
        write_text_if_changed(p,s)
    p=root/'pcsl_posix.c';s=p.read_text()
    if '#include <stdint.h>' not in s:s=s.replace('#include <stdio.h>','#include <stdio.h>\n#include <stdint.h>')
    s=s.replace('(int)handle','(int)(intptr_t)handle')
    s=s.replace('(void *)fd','(void *)(intptr_t)fd').replace('(void*)fd','(void*)(intptr_t)fd')
    write_text_if_changed(p,s)
