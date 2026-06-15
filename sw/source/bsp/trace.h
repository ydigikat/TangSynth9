/*
 *  Estella : (c) Jason Wilden, 2026
 */
#ifndef __TRACE_H__
#define __TRACE_H__

#include "bsp.h"

void trace_init(TRACE_t* trace);
void trace_print(TRACE_t *trace, const char *str);
void trace_printf(TRACE_t *trace, const char *fmt, uint32_t arg1, uint32_t arg2, uint32_t arg3);



#endif /* __TRACE_H__*/