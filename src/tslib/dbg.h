// Last Update:2019-04-02 10:39:24
/**
 * @file dbg.h
 * @brief 
 * @author liyq
 * @version 0.1.00
 * @date 2018-10-26
 */

#ifndef DBG_H
#define DBG_H

#include <assert.h>

#define BASIC() printf("[ %s %s +%d ] ", __FILE__, __FUNCTION__, __LINE__ )
#define LOG(args...) BASIC();printf(args)
#define LOGI(args...) BASIC();printf(args)
#define LOGE(args...) BASIC();printf(args)
#define VAL( v ) LOG(#v" = %d\n", v )
#define STR( s ) LOG(#s" = %s\n", s )

#define ASSERT( cond ) assert(cond)
#define DUMPBUF( buf, size ) DbgDumpBuf( __LINE__, __FUNCTION__, buf, size ) 

#endif  /*DBG_H*/
