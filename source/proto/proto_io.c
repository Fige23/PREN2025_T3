/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (               )          
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )  
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(  
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_)) 
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())  
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|  
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |    
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|    
proto_io.c	Created on: 15.03.2026	   Author: Fige23	Team 3                                                                
*/


#include "proto_io.h"

#include <stdarg.h>
#include <stdio.h>

#include "robot_config.h"
#include "serial_port.h"

void proto_reply_raw(const char *s)
{
#if USE_SEMIHOST_CONSOLE
    printf("%s", s);
    fflush(stdout);
#else
    serial_puts(s);
#endif
}

void proto_reply_printf(const char *fmt, ...)
{
    char buf[PROTO_REPLY_BUFFER_LEN];

    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    proto_reply_raw(buf);
}
