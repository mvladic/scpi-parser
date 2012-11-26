/*-
 * Copyright (c) 2012-2013 Jan Breuer,
 *
 * All Rights Reserved
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file   scpi_units.c
 * @date   Thu Nov 15 10:58:45 UTC 2012
 * 
 * @brief  SCPI units
 * 
 * 
 */

#include <string.h>
#include "scpi.h"
#include "scpi_units.h"
#include "scpi_utils.h"
#include "scpi_error.h"


/*
 * multipliers IEEE 488.2-1992 tab 7-2
 * 1E18         EX
 * 1E15         PE
 * 1E12         T
 * 1E9          G
 * 1E6          MA (use M for OHM and HZ)
 * 1E3          K
 * 1E-3         M (disaalowed for OHM and HZ)
 * 1E-6         U
 * 1E-9         N
 * 1E-12        P
 * 1E-15        F
 * 1E-18        A
 */

/*
 * units definition IEEE 488.2-1992 tab 7-1
 */
const scpi_unit_def_t scpi_units_def[] = {
    /* voltage */
    { .name = "UV", .unit = SCPI_UNIT_VOLT, .mult = 1e-6},
    { .name = "MV", .unit = SCPI_UNIT_VOLT, .mult = 1e-3},
    { .name = "V", .unit = SCPI_UNIT_VOLT, .mult = 1},
    { .name = "KV", .unit = SCPI_UNIT_VOLT, .mult = 1e3},

    /* current */
    { .name = "UA", .unit = SCPI_UNIT_AMPER, .mult = 1e-6},
    { .name = "MA", .unit = SCPI_UNIT_AMPER, .mult = 1e-3},
    { .name = "A", .unit = SCPI_UNIT_AMPER, .mult = 1},
    { .name = "KA", .unit = SCPI_UNIT_AMPER, .mult = 1e3},

    /* resistance */
    { .name = "OHM", .unit = SCPI_UNIT_OHM, .mult = 1},
    { .name = "KOHM", .unit = SCPI_UNIT_OHM, .mult = 1e3},
    { .name = "MOHM", .unit = SCPI_UNIT_OHM, .mult = 1e6},

    /* frequency */
    { .name = "HZ", .unit = SCPI_UNIT_HERTZ, .mult = 1},
    { .name = "KHZ", .unit = SCPI_UNIT_HERTZ, .mult = 1e3},
    { .name = "MHZ", .unit = SCPI_UNIT_HERTZ, .mult = 1e6},
    { .name = "GHZ", .unit = SCPI_UNIT_HERTZ, .mult = 1e9},

    /* temperature */
    { .name = "CEL", .unit = SCPI_UNIT_CELSIUS, .mult = 1},

    /* time */
    { .name = "PS", .unit = SCPI_UNIT_SECONDS, .mult = 1e-12},
    { .name = "NS", .unit = SCPI_UNIT_SECONDS, .mult = 1e-9},
    { .name = "US", .unit = SCPI_UNIT_SECONDS, .mult = 1e-6},
    { .name = "MS", .unit = SCPI_UNIT_SECONDS, .mult = 1e-3},
    { .name = "S", .unit = SCPI_UNIT_SECONDS, .mult = 1},
    { .name = "MIN", .unit = SCPI_UNIT_SECONDS, .mult = 60},
    { .name = "HR", .unit = SCPI_UNIT_SECONDS, .mult = 3600},

    SCPI_UNITS_LIST_END,
};


const scpi_special_number_def_t scpi_special_numbers_def[] = {
    { .name = "MIN", .type = SCPI_NUM_MIN},
    { .name = "MINIMUM", .type = SCPI_NUM_MIN},
    { .name = "MAX", .type = SCPI_NUM_MAX},
    { .name = "MAXIMUM", .type = SCPI_NUM_MAX},
    { .name = "DEF", .type = SCPI_NUM_DEF},
    { .name = "DEFAULT", .type = SCPI_NUM_DEF},
    { .name = "NAN", .type = SCPI_NUM_NAN},
    { .name = "INF", .type = SCPI_NUM_INF},
    { .name = "NINF", .type = SCPI_NUM_NINF},
    SCPI_SPECIAL_NUMBERS_LIST_END,
};

static scpi_special_number_t translateSpecialNumber(const char * str, size_t len) {
    int i;

    for (i = 0; scpi_special_numbers_def[i].name != NULL; i++) {
        if (compareStr(str, len, scpi_special_numbers_def[i].name, strlen(scpi_special_numbers_def[i].name))) {
            return scpi_special_numbers_def[i].type;
        }
    }

    return SCPI_NUM_NUMBER;
}

static const scpi_unit_def_t * searchUnit(const char * unit, size_t len) {
    int i;
    for(i = 0; scpi_units_def[i].name != NULL; i++) {
        if (compareStr(unit, len, scpi_units_def[i].name, strlen(scpi_units_def[i].name))) {
            return &scpi_units_def[i];
        }
    }
    
    return NULL;
}

static bool_t transformNumber(const char * unit, size_t len, scpi_number_t * value) {
    size_t s;
    const scpi_unit_def_t * unitDef;
    s = skipWhitespace(unit, len);

    if (s == len) {
        value->unit = SCPI_UNIT_NONE;
        return TRUE;
    }
    
    unitDef = searchUnit(unit + s, len - s);
    
    if (unitDef == NULL) {
        return FALSE;
    }
    
    value->value *= unitDef->mult;
    value->unit = unitDef->unit;
    
    return TRUE;
}

/**
 * Parse parameter as number, number with unit or special value (min, max, default, ...)
 * @param context
 * @param value return value
 * @param mandatory if the parameter is mandatory
 * @return 
 */
bool_t SCPI_ParamNumber(scpi_context_t * context, scpi_number_t * value, bool_t mandatory) {
    bool_t result;
    char * param;
    size_t len;
    size_t numlen;

    result = SCPI_ParamString(context, &param, &len, mandatory);

    if (!result) {
        return FALSE;
    }

    if (!value) {
        return FALSE;
    }

    value->unit = SCPI_UNIT_NONE;
    value->value = 0.0;
    value->type = translateSpecialNumber(param, len);

    if (value->type != SCPI_NUM_NUMBER) {
        // found special type
        return TRUE;
    }

    numlen = strToDouble(param, &value->value);

    if (numlen <= len) {
        if (transformNumber(param + numlen, len - numlen, value)) {
            return TRUE;
        } else {
            SCPI_ErrorPush(context, SCPI_ERROR_INVALID_SUFFIX);            
        }
    }
    return FALSE;

}

