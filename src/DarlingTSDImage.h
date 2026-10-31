// Bounded dynamic-image adapter for the shared Darling ARM64 TSD rewriter.
// Segment/Section are the loader's public Mach-O metadata types. The template
// keeps host tests independent of a Darwin SDK and does not define another ABI.
#ifndef DARLING_TSD_IMAGE_H
#define DARLING_TSD_IMAGE_H
#include <stddef.h>
#include <stdint.h>
#include "tsd_rewrite.h"

namespace DarlingTSDImage {
enum Result { Success, InvalidMetadata, UnreadableMapping, ProtectionFailed, RestoreFailed };
inline bool protectionRangeInputValid(uintptr_t address, size_t bytes, uintptr_t pageSize) {
    return pageSize && !(pageSize & (pageSize - 1)) &&
        address % pageSize == 0 && bytes <= UINTPTR_MAX - (pageSize - 1);
}
inline bool protectionRangeEndValid(uintptr_t address, size_t bytes, uintptr_t roundedBytes) {
    return (!bytes || roundedBytes) && roundedBytes >= bytes &&
        roundedBytes <= UINTPTR_MAX - address;
}


template<class Segment, class Section, class IsCode, class Protect, class Flush>
Result rewrite(const Segment* segment, size_t commandBytes, uint8_t* memory,
               size_t mappedBytes, uintptr_t slot, bool readable,
               IsCode isCode, Protect protect, Flush flush)
{
    if (!mldr_tsd_slot_offset_valid(slot)) return Success;
    if (!segment || commandBytes < sizeof(Segment) ||
        reinterpret_cast<uintptr_t>(segment) % alignof(Segment)) return InvalidMetadata;
    if (segment->cmdsize < sizeof(Segment) || segment->cmdsize > commandBytes ||
        segment->nsects > (segment->cmdsize - sizeof(Segment)) / sizeof(Section) ||
        mappedBytes > segment->filesize || (!memory && mappedBytes) ||
        mappedBytes > UINTPTR_MAX - reinterpret_cast<uintptr_t>(memory)) return InvalidMetadata;
    const Section* sections = reinterpret_cast<const Section*>(
        reinterpret_cast<const uint8_t*>(segment) + sizeof(Segment));
    size_t matches = 0;
    for (uint32_t i = 0; i < segment->nsects; ++i) {
        const Section& section = sections[i];
        if (!isCode(section.flags)) continue;
        if (section.addr < segment->vmaddr || section.offset < segment->fileoff)
            return InvalidMetadata;
        uint64_t delta = section.addr - segment->vmaddr;
        if (delta != section.offset - segment->fileoff || delta > mappedBytes ||
            section.size > mappedBytes - delta || (delta & 3) ||
            ((reinterpret_cast<uintptr_t>(memory) + delta) & 3)) return InvalidMetadata;
        if (!section.size) continue;
        if (!readable) return UnreadableMapping;
        size_t words = static_cast<size_t>(section.size / sizeof(uint32_t));
        const uint32_t* code = reinterpret_cast<const uint32_t*>(memory + delta);
        size_t count = mldr_tsd_count_reads(code, words);
        if (count > SIZE_MAX - matches) return InvalidMetadata;
        matches += count;
    }
    if (!matches) return Success;
    // The caller provides a non-executable RW transition and exact restoration.
    if (!protect(true)) return ProtectionFailed;
    for (uint32_t i = 0; i < segment->nsects; ++i) {
        const Section& section = sections[i];
        if (!isCode(section.flags)) continue;
        size_t delta = static_cast<size_t>(section.addr - segment->vmaddr);
        mldr_tsd_rewrite_reads(reinterpret_cast<uint32_t*>(memory + delta),
            static_cast<size_t>(section.size / sizeof(uint32_t)), slot);
    }
    flush(memory, mappedBytes);
    if (!protect(false)) return RestoreFailed;
    return Success;
}
}
#endif
