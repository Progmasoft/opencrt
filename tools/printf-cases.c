/*
 * SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com>
 * SPDX-License-Identifier: MIT
 */

/*
 * Writes tests/PrintfCases.inc from the printf of the host C library, which rounds exactly:
 *
 *   cc -o build/printf-cases tools/printf-cases.c -lm && build/printf-cases > tests/PrintfCases.inc
 *
 * glibc differs from OpenCRT on purpose in %La (it does not put a 1 before the point) and in %a of
 * subnormals (it writes 0x0.); those cases are written by hand in tests/PrintfTests.c.
 */

#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void escape(const char* s) {
        putchar('"');
        for (; *s; s++) {
                if (*s == '"' || *s == '\\') { putchar('\\'); putchar(*s); }
                else putchar(*s);
        }
        putchar('"');
}

static const char* dformats[] = {
        "%f", "%.0f", "%.1f", "%.2f", "%.3f", "%.10f", "%.17f", "%e", "%.0e", "%.3e", "%.16e", "%E",
        "%g", "%.0g", "%.1g", "%.3g", "%.10g", "%.17g", "%G", "%#g", "%#.0f", "%#.0e", "%+f", "% f",
        "%12.3f", "%-12.3f|", "%012.3f", "%+012.3e", "%a", "%.0a", "%.1a", "%.3a", "%A", "%#.0a", "%20.4a", "%-20a|", "%020a",
};
static const char* dvalues_text[] = {
        "0.0", "-0.0", "1.0", "0.5", "1.5", "2.5", "-2.5", "0.125", "0.1", "0.3", "1e-5", "123456.789",
        "9.999999", "99.995", "0.000123456", "1e100", "1e-100", "DBL_MAX", "DBL_MIN", "2.0/3.0", "1e21", "1e15",
        "5e-324", "1.7976931348623157e308", "4.35", "1048575.9999999999", "0.05", "1e-310",
};
static double dvalues[] = {
        0.0, -0.0, 1.0, 0.5, 1.5, 2.5, -2.5, 0.125, 0.1, 0.3, 1e-5, 123456.789,
        9.999999, 99.995, 0.000123456, 1e100, 1e-100, DBL_MAX, DBL_MIN, 2.0/3.0, 1e21, 1e15,
        5e-324, 1.7976931348623157e308, 4.35, 1048575.9999999999, 0.05, 1e-310,
};
static const char* lformats[] = { "%Lf", "%.3Lf", "%Le", "%.20Le", "%Lg", "%.21Lg", "%.0Lf" };
static const char* lvalues_text[] = { "1.0L", "0.1L", "-1.5L", "1e4000L", "1e-4000L", "LDBL_MAX", "LDBL_MIN", "0x0.0000000000000002p-16382L", "2.0L/3.0L", "123456789012345678.5L" };
static long double lvalues[] = { 1.0L, 0.1L, -1.5L, 1e4000L, 1e-4000L, LDBL_MAX, LDBL_MIN, 0x0.0000000000000002p-16382L, 2.0L/3.0L, 123456789012345678.5L };

int main(void) {
        char buf[20000];
        printf("/* SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com> */\n/* SPDX-License-Identifier: MIT */\n\n/* Written by tools/printf-cases.c from the printf of the host C library. */\n\n");
        printf("static void\nDoubles(void)\n{\n");
        for (size_t v = 0; v < sizeof(dvalues)/sizeof(dvalues[0]); v++) {
                for (size_t f = 0; f < sizeof(dformats)/sizeof(dformats[0]); f++) {
                        if (strchr(dformats[f], 'a') || strchr(dformats[f], 'A')) {
                                if (fpclassify(dvalues[v]) == FP_SUBNORMAL) continue;
                        }
                        snprintf(buf, sizeof buf, dformats[f], dvalues[v]);
                        printf("        CheckFormat(__LINE__, "); escape(buf); printf(", "); escape(dformats[f]); printf(", %s);\n", dvalues_text[v]);
                }
        }
        printf("}\n\nstatic void\nLongDoubles(void)\n{\n");
        for (size_t v = 0; v < sizeof(lvalues)/sizeof(lvalues[0]); v++) {
                for (size_t f = 0; f < sizeof(lformats)/sizeof(lformats[0]); f++) {
                        snprintf(buf, sizeof buf, lformats[f], lvalues[v]);
                        if (strlen(buf) > 300) { /* keep the test file small: check length and a prefix */
                                printf("        CheckLongPrefix(__LINE__, %zu, ", strlen(buf)); 
                                char head[41]; memcpy(head, buf, 40); head[40] = 0; escape(head);
                                printf(", "); escape(lformats[f]); printf(", %s);\n", lvalues_text[v]);
                        } else {
                                printf("        CheckFormat(__LINE__, "); escape(buf); printf(", "); escape(lformats[f]); printf(", %s);\n", lvalues_text[v]);
                        }
                }
        }
        printf("}\n");
        return 0;
}
