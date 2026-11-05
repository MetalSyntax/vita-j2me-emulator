#!/bin/sh
# build-opencore-amr.sh — decodificador AMR-NB (opencore-amr 0.1.6,
# Apache 2.0) para la Vita, SIN configure: solo el decodificador, con la
# misma lista de fuentes que amrnb/Makefile.am. Lo usa multimedia.c
# (audio/amr de los juegos de Gameloft, sesión 55).
#
# Uso: build-tools/build-opencore-amr.sh <ruta a opencore-amr-0.1.6 extraído>
# Fuente oficial: https://sourceforge.net/projects/opencore-amr/files/opencore-amr/
#   opencore-amr-0.1.6.tar.gz  sha256 483eb4061088e2b34b358e47540b5d495a96cd468e361050fae615b1809dc4a1
set -e
SRC="$1"
[ -f "$SRC/amrnb/wrapper.cpp" ] || { echo "uso: $0 <opencore-amr-0.1.6>"; exit 1; }
: "${VITASDK:=$HOME/vitasdk}"
CXX="$VITASDK/bin/arm-vita-eabi-g++"
AR="$VITASDK/bin/arm-vita-eabi-ar"
HERE="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$HERE/_prebuilt/lib/libopencore-amrnb.a"
INC="$HERE/_prebuilt/include/opencore-amrnb"
AMR="$SRC/opencore/codecs_v2/audio/gsm_amr"
DEC="$AMR/amr_nb/dec/src"; COM="$AMR/amr_nb/common/src"
OBJ="$(mktemp -d)"
CFLAGS="-O2 -mcpu=cortex-a9 -mfpu=neon -fno-exceptions -fno-rtti -DDISABLE_AMRNB_ENCODER
 -I$SRC/oscl -I$DEC -I$AMR/amr_nb/common/include -I$AMR/amr_nb/dec/include
 -I$AMR/common/dec/include -I$AMR/amr_nb/enc/src"
DEC_SRCS="agc amrdecode a_refl b_cn_cod bgnscd c_g_aver d1035pf d2_11pf d2_9pf
 d3_14pf d4_17pf d8_31pf dec_amr dec_gain dec_input_format_tab dec_lag3
 dec_lag6 d_gain_c d_gain_p d_plsf_3 d_plsf_5 d_plsf dtx_dec ec_gains ex_ctrl
 if2_to_ets int_lsf lsp_avg ph_disp post_pro preemph pstfilt qgain475_tab
 sp_dec wmf_to_ets"
COM_SRCS="add az_lsp bitno_tab bitreorder_tab c2_9pf_tab div_s extract_h
 extract_l gains_tbl gc_pred get_const_tbls gmed_n gray_tbl grid_tbl int_lpc
 inv_sqrt inv_sqrt_tbl l_deposit_h l_deposit_l log2 log2_norm log2_tbl lsfwt
 l_shr_r lsp_az lsp lsp_lsf lsp_lsf_tbl lsp_tab mult_r negate norm_l norm_s
 overflow_tbl ph_disp_tab pow2 pow2_tbl pred_lt q_plsf_3 q_plsf_3_tbl q_plsf_5
 q_plsf_5_tbl q_plsf qua_gain_tbl reorder residu round set_zero shr shr_r
 sqrt_l sqrt_l_tbl sub syn_filt weight_a window_tab"
$CXX $CFLAGS -c "$SRC/amrnb/wrapper.cpp" -o "$OBJ/wrapper.o"
for f in $DEC_SRCS; do $CXX $CFLAGS -c "$DEC/$f.cpp" -o "$OBJ/d_$f.o"; done
for f in $COM_SRCS; do $CXX $CFLAGS -c "$COM/$f.cpp" -o "$OBJ/c_$f.o"; done
rm -f "$OUT"; $AR rcs "$OUT" "$OBJ"/*.o
mkdir -p "$INC"; cp "$SRC/amrnb/interf_dec.h" "$SRC/LICENSE" "$INC/"
rm -rf "$OBJ"
echo "OK: $OUT ($(wc -c < "$OUT") B) + $INC/interf_dec.h"
