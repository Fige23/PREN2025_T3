/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (               )          
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )  
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(  
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_)) 
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())  
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|  
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |    
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|    
parse_kv.h	Created on: 13.11.2025	   Author: Fige23	Team 3                                                                
*/

#ifndef UTILS_PARSE_KV_H_
#define UTILS_PARSE_KV_H_

#include <stdint.h>
#include <stddef.h>
#include "protocol.h"

// Bitmasken für Pos-Keys:
#define KV_X   (1u<<0)
#define KV_Y   (1u<<1)
#define KV_Z   (1u<<2)
#define KV_PHI (1u<<3)

// spezifikation für key=value -> skalierte ints
typedef struct {
    const char *key_lc;       // "x","y","z","phi" (lowercase)
    int32_t *dst_scaled;      // Ziel in Fixed-Point (z.B. 0.001mm)
    int32_t min_scaled;       // min in Fixed-Point
    int32_t max_scaled;       // max in Fixed-Point
    uint8_t bit;              // KV_X, KV_Y, KV_Z, KV_PHI
    int32_t scale;            // SCALE_MM (=1000) oder SCALE_DEG (=100)
} kv_fixed_spec_s;

err_e kv_fixed_any_lower(const char *tok,
                         const kv_fixed_spec_s *specs,
                         size_t n,
                         uint8_t *seen_mask);

// Parst argv[start..argc-1] für x,y,z,phi in Fixed-Point.
// require_mask: Keys, die zwingend vorkommen müssen (z.B. KV_X|KV_Y)
// allowed_mask: Keys, die erlaubt sind (Whitelist)
// seen_out: optionaler Rückgabebitmaskenwert (kann NULL sein)
// Rückgabe: ERR_NONE ok; ERR_SYNTAX/ERR_RANGE aus kv_* oder Policy-Verstöße.
err_e parse_pos_tokens_mask(int argc, char **argv, int start,
                            int32_t *x_s, int32_t *y_s,
                            int32_t *z_s, int32_t *ph_s,
                            uint8_t require_mask,
                            uint8_t allowed_mask,
                            uint8_t *seen_out);


#endif /* UTILS_PARSE_KV_H_ */
