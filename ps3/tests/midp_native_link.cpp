// SPDX-License-Identifier: GPL-2.0-only
// Development integration test. This has no front end and is not a release EBOOT.
extern "C" int ps3me_run_game(const char*,const char*,const char*,const char*);
int main(int argc,char** argv) {
    if(argc!=5)return 2;
    return ps3me_run_game(argv[1],argv[2],argv[3],argv[4]);
}
