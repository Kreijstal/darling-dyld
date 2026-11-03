#include <unistd.h>

#if defined(DIAGNOSTIC_STRONG)
extern int authored_missing_import(void);
static int (*volatile imported_function)(void) = authored_missing_import;
#elif defined(DIAGNOSTIC_DEPENDENCY)
extern int authored_missing_dependency(void);
static int (*volatile imported_function)(void) = authored_missing_dependency;
#elif defined(DIAGNOSTIC_WEAK)
extern int authored_weak_import(void) __attribute__((weak_import));
static int (*volatile imported_function)(void) = authored_weak_import;
#endif

int main(void) {
    if (getpid() <= 0)
        return 74;
#if defined(DIAGNOSTIC_RESOLVED)
    return 0;
#else
    return imported_function == 0 ? 0 : 75;
#endif
}
