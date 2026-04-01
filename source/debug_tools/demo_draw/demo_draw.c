/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (               )          
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )  
(()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(  
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_)) 
(_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())  
| _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|  
|  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |    
|_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|    
demo_draw.c	Created on: 23.03.2026	   Author: Fige23	Team 3                                                                
*/

#include "robot_config.h"

#include "bot_engine.h"
#include "protocol.h"
#include "demo_draw.h"

#include <stdint.h>
#include <stdbool.h>




static uint16_t s_demo_req_id = 60000;

static bool demo_enqueue_move_mm(int32_t x_mm, int32_t y_mm)
{
    bot_action_s a = {
        .type = ACT_MOVE,
        .target_pos = {
            .x_mm_scaled   = x_mm * SCALE_MM,
            .y_mm_scaled   = y_mm * SCALE_MM,
            .z_mm_scaled   = g_status.pos_internal.z_mm_scaled,
            .phi_deg_scaled = g_status.pos_internal.phi_deg_scaled
        },
        .magnet_on = false,
        .request_id = s_demo_req_id++
    };

    return bot_enqueue(&a);
}

static bool demo_enqueue_star(int32_t cx_mm, int32_t cy_mm, int32_t r_mm)
{
    int32_t p0x = cx_mm + (  0 * r_mm) / 100;
    int32_t p0y = cy_mm + (100 * r_mm) / 100;

    int32_t p1x = cx_mm + ( 59 * r_mm) / 100;
    int32_t p1y = cy_mm - ( 81 * r_mm) / 100;

    int32_t p2x = cx_mm - ( 95 * r_mm) / 100;
    int32_t p2y = cy_mm + ( 31 * r_mm) / 100;

    int32_t p3x = cx_mm + ( 95 * r_mm) / 100;
    int32_t p3y = cy_mm + ( 31 * r_mm) / 100;

    int32_t p4x = cx_mm - ( 59 * r_mm) / 100;
    int32_t p4y = cy_mm - ( 81 * r_mm) / 100;

    if (!demo_enqueue_move_mm(p0x, p0y)) return false;
    if (!demo_enqueue_move_mm(p1x, p1y)) return false;
    if (!demo_enqueue_move_mm(p2x, p2y)) return false;
    if (!demo_enqueue_move_mm(p3x, p3y)) return false;
    if (!demo_enqueue_move_mm(p4x, p4y)) return false;
    if (!demo_enqueue_move_mm(p0x, p0y)) return false;

    return true;
}

static bool demo_enqueue_square(int32_t x0_mm, int32_t y0_mm, int32_t size_mm)
{
    if (!demo_enqueue_move_mm(x0_mm,           y0_mm)) return false;
    if (!demo_enqueue_move_mm(x0_mm + size_mm, y0_mm)) return false;
    if (!demo_enqueue_move_mm(x0_mm + size_mm, y0_mm + size_mm)) return false;
    if (!demo_enqueue_move_mm(x0_mm,           y0_mm + size_mm)) return false;
    if (!demo_enqueue_move_mm(x0_mm,           y0_mm)) return false;

    return true;
}

static bool demo_enqueue_triangle(int32_t cx_mm, int32_t cy_mm, int32_t size_mm)
{
    if (!demo_enqueue_move_mm(cx_mm,            cy_mm + size_mm)) return false;
    if (!demo_enqueue_move_mm(cx_mm - size_mm,  cy_mm - size_mm)) return false;
    if (!demo_enqueue_move_mm(cx_mm + size_mm,  cy_mm - size_mm)) return false;
    if (!demo_enqueue_move_mm(cx_mm,            cy_mm + size_mm)) return false;

    return true;
}

static bool demo_enqueue_spiral(int32_t x0_mm, int32_t y0_mm, int32_t step_mm, int32_t turns)
{
    int32_t x = x0_mm;
    int32_t y = y0_mm;
    int32_t len = step_mm;

    if (!demo_enqueue_move_mm(x, y)) return false;

    for (int32_t i = 0; i < turns; i++) {
        x += len; if (!demo_enqueue_move_mm(x, y)) return false;
        y += len; if (!demo_enqueue_move_mm(x, y)) return false;

        len += step_mm;
        x -= len; if (!demo_enqueue_move_mm(x, y)) return false;
        y -= len; if (!demo_enqueue_move_mm(x, y)) return false;

        len += step_mm;
    }

    return true;
}

static bool demo_enqueue_zigzag(int32_t x0_mm, int32_t y0_mm, int32_t width_mm, int32_t height_mm, int32_t n)
{
    if (n <= 0) return false;

    int32_t dx = width_mm / n;
    int32_t x = x0_mm;
    int32_t y = y0_mm;

    if (!demo_enqueue_move_mm(x, y)) return false;

    for (int32_t i = 0; i < n; i++) {
        x += dx;
        y = (i % 2 == 0) ? (y0_mm + height_mm) : y0_mm;
        if (!demo_enqueue_move_mm(x, y)) return false;
    }

    return true;
}

static bool demo_enqueue_diamond(int32_t cx_mm, int32_t cy_mm, int32_t w_mm, int32_t h_mm)
{
    if (!demo_enqueue_move_mm(cx_mm,        cy_mm + h_mm)) return false;
    if (!demo_enqueue_move_mm(cx_mm + w_mm, cy_mm))        return false;
    if (!demo_enqueue_move_mm(cx_mm,        cy_mm - h_mm)) return false;
    if (!demo_enqueue_move_mm(cx_mm - w_mm, cy_mm))        return false;
    if (!demo_enqueue_move_mm(cx_mm,        cy_mm + h_mm)) return false;

    return true;
}

static bool demo_enqueue_cross(int32_t cx_mm, int32_t cy_mm, int32_t size_mm)
{
    if (!demo_enqueue_move_mm(cx_mm,           cy_mm + size_mm)) return false;
    if (!demo_enqueue_move_mm(cx_mm,           cy_mm - size_mm)) return false;
    if (!demo_enqueue_move_mm(cx_mm,           cy_mm))           return false;
    if (!demo_enqueue_move_mm(cx_mm - size_mm, cy_mm))           return false;
    if (!demo_enqueue_move_mm(cx_mm + size_mm, cy_mm))           return false;

    return true;
}

static bool demo_enqueue_house(int32_t x0_mm, int32_t y0_mm, int32_t size_mm)
{
    int32_t x1 = x0_mm + size_mm;
    int32_t y1 = y0_mm + size_mm;
    int32_t xr = x0_mm + size_mm / 2;
    int32_t yt = y0_mm + (3 * size_mm) / 2;

    if (!demo_enqueue_move_mm(x0_mm, y0_mm)) return false;
    if (!demo_enqueue_move_mm(x1,    y0_mm)) return false;
    if (!demo_enqueue_move_mm(x1,    y1))    return false;
    if (!demo_enqueue_move_mm(xr,    yt))    return false;
    if (!demo_enqueue_move_mm(x0_mm, y1))    return false;
    if (!demo_enqueue_move_mm(x0_mm, y0_mm)) return false;

    return true;
}

static bool demo_enqueue_arrow(int32_t x0_mm, int32_t y0_mm, int32_t len_mm)
{
    int32_t head = len_mm / 3;

    if (!demo_enqueue_move_mm(x0_mm,                  y0_mm)) return false;
    if (!demo_enqueue_move_mm(x0_mm + len_mm,         y0_mm)) return false;
    if (!demo_enqueue_move_mm(x0_mm + len_mm - head,  y0_mm + head)) return false;
    if (!demo_enqueue_move_mm(x0_mm + len_mm,         y0_mm)) return false;
    if (!demo_enqueue_move_mm(x0_mm + len_mm - head,  y0_mm - head)) return false;

    return true;
}

static bool demo_enqueue_heart(int32_t cx_mm, int32_t cy_mm, int32_t size_mm)
{
    if (!demo_enqueue_move_mm(cx_mm,               cy_mm - size_mm))     return false;
    if (!demo_enqueue_move_mm(cx_mm + size_mm,     cy_mm))               return false;
    if (!demo_enqueue_move_mm(cx_mm + size_mm / 2, cy_mm + size_mm))     return false;
    if (!demo_enqueue_move_mm(cx_mm,               cy_mm + size_mm / 2)) return false;
    if (!demo_enqueue_move_mm(cx_mm - size_mm / 2, cy_mm + size_mm))     return false;
    if (!demo_enqueue_move_mm(cx_mm - size_mm,     cy_mm))               return false;
    if (!demo_enqueue_move_mm(cx_mm,               cy_mm - size_mm))     return false;

    return true;
}

static bool demo_enqueue_wave(int32_t x0_mm, int32_t y0_mm, int32_t width_mm, int32_t amp_mm, int32_t periods)
{
    if (periods <= 0) return false;

    int32_t step = width_mm / (periods * 4);
    int32_t x = x0_mm;

    if (!demo_enqueue_move_mm(x, y0_mm)) return false;

    for (int32_t i = 0; i < periods; i++) {
        x += step; if (!demo_enqueue_move_mm(x, y0_mm + amp_mm)) return false;
        x += step; if (!demo_enqueue_move_mm(x, y0_mm))          return false;
        x += step; if (!demo_enqueue_move_mm(x, y0_mm - amp_mm)) return false;
        x += step; if (!demo_enqueue_move_mm(x, y0_mm))          return false;
    }

    return true;
}

static bool demo_enqueue_flower(int32_t cx_mm, int32_t cy_mm, int32_t size_mm)
{
    if (!demo_enqueue_move_mm(cx_mm,             cy_mm))             return false;
    if (!demo_enqueue_move_mm(cx_mm,             cy_mm + size_mm))   return false;
    if (!demo_enqueue_move_mm(cx_mm,             cy_mm))             return false;
    if (!demo_enqueue_move_mm(cx_mm + size_mm,   cy_mm))             return false;
    if (!demo_enqueue_move_mm(cx_mm,             cy_mm))             return false;
    if (!demo_enqueue_move_mm(cx_mm,             cy_mm - size_mm))   return false;
    if (!demo_enqueue_move_mm(cx_mm,             cy_mm))             return false;
    if (!demo_enqueue_move_mm(cx_mm - size_mm,   cy_mm))             return false;
    if (!demo_enqueue_move_mm(cx_mm,             cy_mm))             return false;

    if (!demo_enqueue_move_mm(cx_mm + size_mm/2, cy_mm + size_mm/2)) return false;
    if (!demo_enqueue_move_mm(cx_mm,             cy_mm))             return false;
    if (!demo_enqueue_move_mm(cx_mm + size_mm/2, cy_mm - size_mm/2)) return false;
    if (!demo_enqueue_move_mm(cx_mm,             cy_mm))             return false;
    if (!demo_enqueue_move_mm(cx_mm - size_mm/2, cy_mm - size_mm/2)) return false;
    if (!demo_enqueue_move_mm(cx_mm,             cy_mm))             return false;
    if (!demo_enqueue_move_mm(cx_mm - size_mm/2, cy_mm + size_mm/2)) return false;
    if (!demo_enqueue_move_mm(cx_mm,             cy_mm))             return false;

    return true;
}

static bool demo_enqueue_smiley_singleline(int32_t cx_mm, int32_t cy_mm, int32_t r_mm)
{
    if (!demo_enqueue_move_mm(cx_mm - r_mm,      cy_mm))            return false;
    if (!demo_enqueue_move_mm(cx_mm - r_mm / 2,  cy_mm + r_mm / 2)) return false;
    if (!demo_enqueue_move_mm(cx_mm,             cy_mm + r_mm))     return false;
    if (!demo_enqueue_move_mm(cx_mm + r_mm / 2,  cy_mm + r_mm / 2)) return false;
    if (!demo_enqueue_move_mm(cx_mm + r_mm,      cy_mm))            return false;
    if (!demo_enqueue_move_mm(cx_mm + r_mm / 2,  cy_mm - r_mm / 2)) return false;
    if (!demo_enqueue_move_mm(cx_mm,             cy_mm - r_mm))     return false;
    if (!demo_enqueue_move_mm(cx_mm - r_mm / 2,  cy_mm - r_mm / 2)) return false;
    if (!demo_enqueue_move_mm(cx_mm - r_mm,      cy_mm))            return false;

    if (!demo_enqueue_move_mm(cx_mm - r_mm / 3,  cy_mm + r_mm / 4)) return false;
    if (!demo_enqueue_move_mm(cx_mm - r_mm / 5,  cy_mm + r_mm / 3)) return false;
    if (!demo_enqueue_move_mm(cx_mm - r_mm / 4,  cy_mm + r_mm / 5)) return false;

    if (!demo_enqueue_move_mm(cx_mm + r_mm / 3,  cy_mm + r_mm / 4)) return false;
    if (!demo_enqueue_move_mm(cx_mm + r_mm / 5,  cy_mm + r_mm / 3)) return false;
    if (!demo_enqueue_move_mm(cx_mm + r_mm / 4,  cy_mm + r_mm / 5)) return false;

    if (!demo_enqueue_move_mm(cx_mm + r_mm / 2,  cy_mm - r_mm / 6)) return false;
    if (!demo_enqueue_move_mm(cx_mm + r_mm / 4,  cy_mm - r_mm / 2)) return false;
    if (!demo_enqueue_move_mm(cx_mm,             cy_mm - r_mm / 3)) return false;
    if (!demo_enqueue_move_mm(cx_mm - r_mm / 4,  cy_mm - r_mm / 2)) return false;
    if (!demo_enqueue_move_mm(cx_mm - r_mm / 2,  cy_mm - r_mm / 6)) return false;

    return true;
}

static bool demo_enqueue_infinity(int32_t cx_mm, int32_t cy_mm, int32_t size_mm)
{
    if (!demo_enqueue_move_mm(cx_mm - size_mm,    cy_mm))             return false;
    if (!demo_enqueue_move_mm(cx_mm - size_mm/2,  cy_mm + size_mm/2)) return false;
    if (!demo_enqueue_move_mm(cx_mm,              cy_mm))             return false;
    if (!demo_enqueue_move_mm(cx_mm + size_mm/2,  cy_mm - size_mm/2)) return false;
    if (!demo_enqueue_move_mm(cx_mm + size_mm,    cy_mm))             return false;
    if (!demo_enqueue_move_mm(cx_mm + size_mm/2,  cy_mm + size_mm/2)) return false;
    if (!demo_enqueue_move_mm(cx_mm,              cy_mm))             return false;
    if (!demo_enqueue_move_mm(cx_mm - size_mm/2,  cy_mm - size_mm/2)) return false;
    if (!demo_enqueue_move_mm(cx_mm - size_mm,    cy_mm))             return false;

    return true;
}

static bool demo_enqueue_snowflake(int32_t cx_mm, int32_t cy_mm, int32_t size_mm)
{
    if (!demo_enqueue_move_mm(cx_mm,           cy_mm + size_mm)) return false;
    if (!demo_enqueue_move_mm(cx_mm,           cy_mm - size_mm)) return false;
    if (!demo_enqueue_move_mm(cx_mm,           cy_mm))           return false;
    if (!demo_enqueue_move_mm(cx_mm - size_mm, cy_mm))           return false;
    if (!demo_enqueue_move_mm(cx_mm + size_mm, cy_mm))           return false;
    if (!demo_enqueue_move_mm(cx_mm,           cy_mm))           return false;
    if (!demo_enqueue_move_mm(cx_mm - size_mm, cy_mm - size_mm)) return false;
    if (!demo_enqueue_move_mm(cx_mm + size_mm, cy_mm + size_mm)) return false;
    if (!demo_enqueue_move_mm(cx_mm,           cy_mm))           return false;
    if (!demo_enqueue_move_mm(cx_mm - size_mm, cy_mm + size_mm)) return false;
    if (!demo_enqueue_move_mm(cx_mm + size_mm, cy_mm - size_mm)) return false;

    return true;
}

bool demo_enqueue_pattern(demo_pattern_e pattern)
{
    switch (pattern) {
        case DEMO_PATTERN_STAR:
            return demo_enqueue_star(100, 100, 55);

        case DEMO_PATTERN_SQUARE:
            return demo_enqueue_square(40, 40, 100);

        case DEMO_PATTERN_TRIANGLE:
            return demo_enqueue_triangle(100, 95, 55);

        case DEMO_PATTERN_SPIRAL:
            return demo_enqueue_spiral(100, 100, 10, 5);

        case DEMO_PATTERN_ZIGZAG:
            return demo_enqueue_zigzag(20, 60, 160, 80, 8);

        case DEMO_PATTERN_DIAMOND:
            return demo_enqueue_diamond(100, 100, 55, 75);

        case DEMO_PATTERN_CROSS:
            return demo_enqueue_cross(100, 100, 60);

        case DEMO_PATTERN_HOUSE:
            return demo_enqueue_house(50, 30, 80);

        case DEMO_PATTERN_ARROW:
            return demo_enqueue_arrow(30, 100, 140);

        case DEMO_PATTERN_HEART:
            return demo_enqueue_heart(100, 85, 35);

        case DEMO_PATTERN_WAVE:
            return demo_enqueue_wave(20, 100, 160, 30, 4);

        case DEMO_PATTERN_FLOWER:
            return demo_enqueue_flower(100, 100, 35);

        case DEMO_PATTERN_SMILEY:
            return demo_enqueue_smiley_singleline(100, 100, 45);

        case DEMO_PATTERN_INFINITY:
            return demo_enqueue_infinity(100, 100, 40);

        case DEMO_PATTERN_SNOWFLAKE:
            return demo_enqueue_snowflake(100, 100, 45);

        default:
            return false;
    }
}






