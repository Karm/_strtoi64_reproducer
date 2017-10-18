/*
@author Michal Karm Babacek <karm@fedoraproject.org>
Abstracted from https://github.com/apache/apr/blob/1.6.x/test/teststr.c
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define APR_INT64_STRFN _strtoi64
#define APR_INT64_C(val) (val##i64)

#define MY_LLONG_MAX (APR_INT64_C(9223372036854775807))
#define MY_LLONG_MIN (-MY_LLONG_MAX - APR_INT64_C(1))

typedef __int64 apr_int64_t;

__int64 apr_strtoi64(const char *nptr, char **endptr, int base) {
    errno = 0;
    return APR_INT64_STRFN(nptr, endptr, base);
}

int main(void) {
    static const struct {
        int errnum, base;
        const char *in, *end;
        apr_int64_t result;
    } ts[] = {
        { 0, 16, "   0x20000000D", NULL, APR_INT64_C(8589934605) },
        { ERANGE, 10, "999999999999999999999999999999999", "", MY_LLONG_MAX },
        { ERANGE, 10, "-999999999999999999999999999999999", "", MY_LLONG_MIN },
    };
    int n;
    for (n = 0; n < sizeof(ts) / sizeof(ts[0]); n++) {
        char *end = "end ptr not changed";
        apr_int64_t result;
        int errnum;
        errno = 0;
        result = apr_strtoi64(ts[n].in, &end, ts[n].base);
        errnum = errno;
        printf("result was: %lld\nerrnum was: %d\nerrnum expected: %d\n\n", result, errnum, ts[n].errnum);
    }
}
