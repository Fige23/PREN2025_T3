/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (               )          
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )  
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(  
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_)) 
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())  
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|  
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |    
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|    
debug.c	Created on: 15.03.2026	   Author: Fige23	Team 3                                                                
*/


#include "debug.h"

#include <stdarg.h>
#include <stdio.h>

#include "robot_config.h"
#include "serial_port.h"

void debug_printf(const char *fmt, ...)
{
#if DEBUG_ENABLE
    char buf[DEBUG_BUFFER_LEN];

    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    #if DEBUG_BACKEND == DEBUG_BACKEND_SEMIHOST
        printf("%s", buf);
        fflush(stdout);
    #elif DEBUG_BACKEND == DEBUG_BACKEND_UART
        serial_puts(buf);
    #else
        (void)buf;
    #endif
#else
    (void)fmt;
#endif
}
