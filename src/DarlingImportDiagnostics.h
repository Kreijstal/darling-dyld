#ifndef DARLING_IMPORT_DIAGNOSTICS_H
#define DARLING_IMPORT_DIAGNOSTICS_H

#include "ImageLoader.h"
#include <darling-boundary-diagnostics.h>

static inline void darlingRecordMissingImport(const ImageLoader::LinkContext& context,
                                              const ImageLoader* requestor, const char* name)
{
    if (requestor && requestor == context.mainExecutable && context.allowEnvVarsPrint)
        darling_missing_import_emit(darling_boundary_endpoint(context.envp), name);
}

#endif
