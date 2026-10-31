#include "DarlingTSDImage.h"
#include <stdint.h>
#include <string.h>
struct Segment { uint32_t cmdsize, nsects; uint64_t vmaddr, fileoff, filesize; };
struct Section { uint64_t addr,size; uint32_t offset,flags; };
struct Command { Segment segment; Section sections[2]; };
int main() {
    const uint32_t first = MLDR_A64_MRS_TPIDRRO_EL0 | 9;
    const uint32_t second = MLDR_A64_AND_X_CLEAR_LOW3 | (9u << 5) | 9;
    uint32_t memory[] = {first,second,first,second};
    Command command = {{sizeof(Command),2,0x1000,0,sizeof(memory)},{{0x1000,8,0,1},{0x1008,8,8,0}}};
    auto code = [](uint32_t flags){return flags==1;};
    int phase=0;
    auto protect = [&phase](bool write){if(write){if(phase!=0)return false;phase=1;}else{if(phase!=2)return false;phase=3;}return true;};
    auto flush = [&phase](uint8_t*,size_t){if(phase==1)phase=2;};
    auto run = [&](uintptr_t slot){return DarlingTSDImage::rewrite<Segment,Section>(&command.segment,sizeof(command),reinterpret_cast<uint8_t*>(memory),sizeof(memory),slot,true,code,protect,flush);};
    if(mldr_tsd_count_reads(memory,2)!=1 || run(16)!=DarlingTSDImage::Success || phase!=3 || memory[0]==first || memory[2]!=first || memory[3]!=second)return 1;
    auto reset=[&](){memory[0]=memory[2]=first;memory[1]=memory[3]=second;phase=0;};
    reset(); if(run(UINTPTR_MAX)!=DarlingTSDImage::Success || phase || memory[0]!=first)return 2;
    command.segment.nsects=100; if(run(16)!=DarlingTSDImage::InvalidMetadata || phase)return 3; command.segment.nsects=2;
    command.segment.cmdsize=sizeof(Segment)-1;if(run(16)!=DarlingTSDImage::InvalidMetadata)return 4;command.segment.cmdsize=sizeof(command);
    command.sections[0].size=sizeof(memory)+4;if(run(16)!=DarlingTSDImage::InvalidMetadata)return 5;command.sections[0].size=8;
    command.sections[0].addr=0x1004;if(run(16)!=DarlingTSDImage::InvalidMetadata)return 6;command.sections[0].addr=0x1000;
    command.sections[0].size=4;if(run(16)!=DarlingTSDImage::Success || phase || memory[0]!=first)return 7;command.sections[0].size=8;
    auto denied=[](bool){return false;};
    if(DarlingTSDImage::rewrite<Segment,Section>(&command.segment,sizeof(command),reinterpret_cast<uint8_t*>(memory),sizeof(memory),16,true,code,denied,flush)!=DarlingTSDImage::ProtectionFailed || memory[0]!=first)return 8;
    auto restoreDenied=[](bool writing){return writing;};
    if(DarlingTSDImage::rewrite<Segment,Section>(&command.segment,sizeof(command),reinterpret_cast<uint8_t*>(memory),sizeof(memory),16,true,code,restoreDenied,flush)!=DarlingTSDImage::RestoreFailed)return 9;
    reset();if(DarlingTSDImage::rewrite<Segment,Section>(&command.segment,sizeof(command),reinterpret_cast<uint8_t*>(memory),sizeof(memory),16,false,code,protect,flush)!=DarlingTSDImage::UnreadableMapping || phase)return 10;
    alignas(Segment) uint8_t unaligned[sizeof(Command)+1] = {};
    if(DarlingTSDImage::rewrite<Segment,Section>(reinterpret_cast<const Segment*>(unaligned+1),sizeof(command),reinterpret_cast<uint8_t*>(memory),sizeof(memory),16,true,code,protect,flush)!=DarlingTSDImage::InvalidMetadata)return 11;
    if(DarlingTSDImage::rewrite<Segment,Section>(&command.segment,sizeof(command),reinterpret_cast<uint8_t*>(UINTPTR_MAX-3),sizeof(memory),16,true,code,protect,flush)!=DarlingTSDImage::InvalidMetadata)return 12;
    if(!DarlingTSDImage::protectionRangeInputValid(0x4000, 1, 0x4000) ||
       DarlingTSDImage::protectionRangeInputValid(0x4001, 1, 0x4000) ||
       DarlingTSDImage::protectionRangeInputValid(0, UINTPTR_MAX, 0x4000)) return 13;
    if(!DarlingTSDImage::protectionRangeEndValid(0x4000, 1, 0x4000) ||
       DarlingTSDImage::protectionRangeEndValid(UINTPTR_MAX-0x3fff, 1, 0x4000) ||
       DarlingTSDImage::protectionRangeEndValid(0, 1, 0)) return 14;
    return 0;
}
