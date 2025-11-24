/*Project: PREN_Puzzleroboter
 (   (          )   (            )   ) (               )          
 )\ ))\ )    ( /(   )\ )      ( /(( /( )\ )      (  ( /(   *   )  
 (()/(()/((   )\()) (()/(   (  )\())\()|()/( (  ( )\ )\())` )  /(
 /(_))(_))\ ((_)\   /(_))  )\((_)((_)\ /(_)))\ )((_|(_)\  ( )(_)) 
 (_))(_))((_) _((_) (_)) _ ((_)_((_)((_|_)) ((_|(_)_  ((_)(_(_())
 | _ \ _ \ __| \| | | _ \ | | |_  /_  /| |  | __| _ )/ _ \|_   _|
 |  _/   / _|| .` | |  _/ |_| |/ / / / | |__| _|| _ \ (_) | | |
 |_| |_|_\___|_|\_| |_|  \___//___/___||____|___|___/\___/  |_|
 parse_kv.c	Created on: 13.11.2025	   Author: Fige23	Team 3

 Parser für key=value Parameter (x=..., y=..., z=..., phi=...).
 - Wird von cmd.c benutzt, um Float-Text direkt in Fixed-Point ints umzuwandeln.
 - Keine float-Operationen nötig -> robust auf TinyK22 ohne FPU-Overhead.
 - Unterstützt:
     * case-insensitive keys (X=.., x=.., Phi=..)
     * Dezimalpunkt oder Komma (12.3 oder 12,3)
     * optionales Vorzeichen
     * Rundung auf nächstes Fixed-Point LSB
 - Enforced zusätzlich:
     * min/max Limits pro Achse
     * require_mask (Pflicht-Keys)
     * allowed_mask (Whitelist, z.B. Z bei PICK/PLACE verbieten)
*/

#include "parse_kv.h"
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <ctype.h>

// -----------------------------------------------------------------------------
// kleine String/Key Utilities
// -----------------------------------------------------------------------------

// to-lower für ASCII ohne locale-Overhead.
static inline char tl(char c) {
	return (c >= 'A' && c <= 'Z') ? (char) (c - 'A' + 'a') : c;
}

// Anzahl Dezimalstellen aus einem 10er-Scale bestimmen.
// scale=1000 -> d=3  /  scale=100 -> d=2  /  scale=1 -> d=0
static int dec_from_scale(int32_t s) {
	int d = 0;
	while (s > 1) {
		s /= 10;
		d++;
	}
	return d;
}

// Vergleich key des Tokens mit einem erwarteten lowercase key.
// keylen ist Länge vor '='.
// return 1 wenn match, sonst 0.
static int key_eq_lc(const char *tok, size_t keylen, const char *key_lc) {
	for (size_t i = 0; i < keylen; i++)
		if (tl(tok[i]) != key_lc[i])
			return 0;
	return key_lc[keylen] == '\0';
}

// -----------------------------------------------------------------------------
// kv_fixed_any_lower()
// -----------------------------------------------------------------------------
/*
 * Parst EIN token "key=value" in ein skaliertes int32.
 * specs beschreibt, welche keys erlaubt sind und wie sie skaliert werden.
 *
 * Ablauf:
 *  1) token bei '=' splitten -> key + value
 *  2) passenden spec-Eintrag finden (case-insensitive)
 *  3) prüfen, ob key schon gesehen wurde (doppelte Parameter sind Syntax-Fehler)
 *  4) value-String normalisieren (',' -> '.')
 *  5) Zahl manuell in Integer-Teil + Fraction-Teil zerlegen
 *  6) Fraction auf spec.scale runden (Half-up)
 *  7) Limit-Check (min/max + int32 range)
 *  8) dst_scaled setzen + seen_mask bit setzen
 */
err_e kv_fixed_any_lower(const char *tok, const kv_fixed_spec_s *specs,
                         size_t n, uint8_t *seen_mask)
{
    const char *eq = strchr(tok, '=');
    if (!eq || eq == tok) return ERR_SYNTAX;

    size_t klen = (size_t)(eq - tok);
    const char *val = eq + 1;
    if (*val == '\0') return ERR_SYNTAX;

    size_t idx = SIZE_MAX;
    for (size_t i = 0; i < n; i++) {
        size_t kk = strlen(specs[i].key_lc);
        if (kk == klen && key_eq_lc(tok, klen, specs[i].key_lc)) {
            idx = i;
            break;
        }
    }
    if (idx == SIZE_MAX) return ERR_SYNTAX;
    if ((*seen_mask) & specs[idx].bit) return ERR_SYNTAX; // doppelt

    // Dezimaltrennzeichen normalisieren, damit "12,3" und "12.3" gleich behandelt werden.
    char buf[32];
    size_t bl = 0;
    for (const char *p = val; *p && bl < sizeof(buf) - 1; ++p)
        buf[bl++] = (*p == ',') ? '.' : *p;
    buf[bl] = '\0';

    // Zahl in Fixed-Point ohne float:
    // ip = Integer-Teil, fr = Fraction-Teil, fd = Anzahl Fraction-Ziffern.
    const char *p = buf;
    int sign = 1;
    if (*p == '+' || *p == '-') {
        if (*p == '-') sign = -1;
        p++;
    }
    if (!(*p >= '0' && *p <= '9')) return ERR_SYNTAX;

    int64_t ip = 0, fr = 0;
    int fd = 0;

    while (*p >= '0' && *p <= '9') {
        ip = ip * 10 + (*p - '0');
        p++;
    }

    if (*p == '.') {
        p++;
        while (*p >= '0' && *p <= '9') {
            if (fd < 6) { // Fraction-Länge begrenzen, damit pow10 nicht explodiert.
                fr = fr * 10 + (*p - '0');
                fd++;
            }
            p++;
        }
    }

    // Alles muss bereits konsumiert sein -> sonst Syntaxfehler.
    if (*p != '\0') return ERR_SYNTAX;

    int64_t scale = specs[idx].scale;
    int64_t pow10 = 1;
    for (int i = 0; i < fd; i++) pow10 *= 10;

    // Fraction auf spec.scale bringen.
    // q = gerundeter Fraction-Anteil, rem = Rest für Half-up Rundung.
    int64_t q   = (fd > 0) ? ((fr * scale) / pow10) : 0;
    int64_t rem = (fd > 0) ? ((fr * scale) % pow10) : 0;
    if (fd > 0 && (rem * 2 >= pow10)) q++; // .5 aufwärts runden

    int64_t s = ip * scale + q;
    if (sign < 0) s = -s;

    // Range-Checks (pro Achse + int32 Overflow verhindern).
    if (s < specs[idx].min_scaled || s > specs[idx].max_scaled)
        return ERR_RANGE;
    if (s < INT32_MIN || s > INT32_MAX)
        return ERR_RANGE;

    *specs[idx].dst_scaled = (int32_t)s;
    *seen_mask |= specs[idx].bit;
    return ERR_NONE;
}


// -----------------------------------------------------------------------------
// print_fp_scaled()
// -----------------------------------------------------------------------------
/*
 * Hilfsfunktion zum Formatieren von Fixed-Point Werten als String:
 *  scaled=12345, scale=1000  -> "12.345"
 *  scaled=900, scale=100     -> "9.00"
 * Wird aktuell kaum gebraucht (STATUS/POS haben eigene printer),
 * bleibt aber als generische Utility drin.
 */
void print_fp_scaled(char *buf, size_t n, int32_t scaled, int32_t scale) {
	int d = dec_from_scale(scale);
	int32_t v = scaled;
	char sign = (v < 0) ? '-' : 0;
	if (v < 0)
		v = -v;
	int32_t i = v / scale, f = v % scale;
	if (d == 0) {
		if (sign)
			snprintf(buf, n, "%c%ld", sign, (long) i);
		else
			snprintf(buf, n, "%ld", (long) i);
	} else {
		char frac[8];
		snprintf(frac, sizeof(frac), "%0*ld", d, (long) f);
		if (sign)
			snprintf(buf, n, "%c%ld.%s", sign, (long) i, frac);
		else
			snprintf(buf, n, "%ld.%s", (long) i, frac);
	}
}



// -----------------------------------------------------------------------------
// parse_pos_tokens_mask()
// -----------------------------------------------------------------------------
/*
 * Parst mehrere Tokens (argv[start..]) für Position/Angle.
 *
 * Inputs:
 *  - argc/argv/start: Tokenliste aus cmd.c
 *  - x_s/y_s/z_s/ph_s: Defaults rein, neue Werte raus (Fixed-Point)
 *  - require_mask: Keys die zwingend vorkommen müssen
 *  - allowed_mask: Keys die vorkommen dürfen (Whitelist)
 *  - seen_out: optional Rückgabe, welche Keys effektiv gesetzt wurden
 *
 * Verhalten:
 *  - ruft für jedes Token kv_fixed_any_lower()
 *  - bei erstem Fehler: sofort return ERR_*
 *  - nach dem Loop:
 *      * Whitelist check (seen darf nur allowed enthalten)
 *      * Pflicht check (require muss vollständig in seen sein)
 *      * wenn require=0 -> mindestens ein Key vorhanden
 */
err_e parse_pos_tokens_mask(int argc, char **argv, int start,
                            int32_t *x_s, int32_t *y_s,
                            int32_t *z_s, int32_t *ph_s,
                            uint8_t require_mask, uint8_t allowed_mask,
                            uint8_t *seen_out)
{
    uint8_t seen = 0;

    kv_fixed_spec_s spec[] = {
        { "x",   x_s,  LIM_X_MIN_S, LIM_X_MAX_S, KV_X,   SCALE_MM  },
        { "y",   y_s,  LIM_Y_MIN_S, LIM_Y_MAX_S, KV_Y,   SCALE_MM  },
        { "z",   z_s,  LIM_Z_MIN_S, LIM_Z_MAX_S, KV_Z,   SCALE_MM  },
        { "phi", ph_s, LIM_P_MIN_S, LIM_P_MAX_S, KV_PHI, SCALE_DEG },
    };

    for (int i = start; i < argc; ++i) {
        err_e e = kv_fixed_any_lower(argv[i],
                spec, sizeof(spec)/sizeof(spec[0]), &seen);
        if (e != ERR_NONE) return e;
    }

    // Whitelist erzwingen
    if ((seen & ~allowed_mask) != 0) return ERR_SYNTAX;

    // Pflicht-Keys vorhanden?
    if ((seen & require_mask) != require_mask) return ERR_SYNTAX;

    // wenn nichts Pflicht ist, muss mindestens 1 Key kommen
    if (require_mask == 0 && seen == 0) return ERR_SYNTAX;

    if (seen_out) *seen_out = seen;
    return ERR_NONE;
}
