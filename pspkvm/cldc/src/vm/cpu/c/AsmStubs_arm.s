/*
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 *
 * (Fase 2b Vita: equivalente ARM del AsmStubs_mips.s usado con el
 * C-interpreter. Solo define el marcador end_of_the_world; sin
 * dependencias de SO porque el intérprete C no emite código.)
 */

.text
.align 4

/* mark the end of ASM functions */
    .global end_of_the_world
end_of_the_world:
