// SPDX-License-Identifier: GPL-2.0-only
// Native entry to the actual MIDP AMS. No MIPS ELF or PS2 emulator is involved.
#include <cstdio>
#include <cstring>
extern "C" {
#include <jvm.h>
#include <midpAMS.h>
#include <midpInit.h>
#include <midpMalloc.h>
#include <pcsl_string.h>
#include <javacall_properties.h>
void ps3me_audio_register_natives();
void ps3me_audio_shutdown();
void ps3me_archive_reset();
void ps3me_set_runtime_paths(const char*,const char*);
}
extern "C" int ps3me_run_game(const char* fileUrl,const char* appDirectory,const char* configDirectory,const char* classLibrary) {
    if(!fileUrl||!appDirectory||!configDirectory||!classLibrary)return -1;
    std::fprintf(stderr,"[PS3ME MIDP] initializing JVM\n");
    ps3me_archive_reset();
    ps3me_audio_register_natives();
    JVM_Initialize();
    JVM_SetConfig(JVM_CONFIG_HEAP_CAPACITY,32*1024*1024);
    JVM_SetConfig(JVM_CONFIG_HEAP_MINIMUM,8*1024*1024);
    std::fprintf(stderr,"[PS3ME MIDP] heap capacity=%d minimum=%d\n",
                 JVM_GetConfig(JVM_CONFIG_HEAP_CAPACITY),JVM_GetConfig(JVM_CONFIG_HEAP_MINIMUM));
    std::fprintf(stderr,"[PS3ME MIDP] initializing memory\n");
    if(midpInitializeMemory(32*1024*1024)!=0){std::fprintf(stderr,"[PS3ME MIDP] memory initialization failed\n");return -1;}
    ps3me_set_runtime_paths(appDirectory,configDirectory);
    midpSetAppDir(appDirectory);midpSetConfigDir(configDirectory);
    if(javacall_initialize_configurations()!=JAVACALL_OK){std::fprintf(stderr,"[PS3ME MIDP] configuration initialization failed\n");return -1;}
    pcsl_string mainClass=PCSL_STRING_NULL,arg=PCSL_STRING_NULL;
    const char* loader="com.ps3me.loader.GameLoader";
    if(pcsl_string_convert_from_utf8((const jbyte*)loader,std::strlen(loader),&mainClass)!=PCSL_STRING_OK)return -1;
    if(pcsl_string_convert_from_utf8((const jbyte*)fileUrl,std::strlen(fileUrl),&arg)!=PCSL_STRING_OK){pcsl_string_free(&mainClass);return -1;}
    std::fprintf(stderr,"[PS3ME MIDP] starting GameLoader\n");
    int result=midp_run_midlet_with_args_cp(INTERNAL_SUITE_ID,&mainClass,&arg,&PCSL_STRING_NULL,&PCSL_STRING_NULL,0,const_cast<char*>(classLibrary));
    pcsl_string_free(&mainClass);pcsl_string_free(&arg);
    ps3me_audio_shutdown();
    midpFinalize();
    ps3me_archive_reset();
    std::fprintf(stderr,"[PS3ME MIDP] session finalized\n");
    return result;
}
