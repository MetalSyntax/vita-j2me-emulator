	@  Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
	@  DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER

	@  This program is free software; you can redistribute it and/or
	@  modify it under the terms of the GNU General Public License version
	@  2 only, as published by the Free Software Foundation.

	@  This program is distributed in the hope that it will be useful, but
	@  WITHOUT ANY WARRANTY; without even the implied warranty of
	@  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
	@  General Public License version 2 for more details (a copy is
	@  included at /legal/license.txt).

	@  You should have received a copy of the GNU General Public License
	@  version 2 along with this work; if not, write to the Free Software
	@  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
	@  02110-1301 USA

	@  Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
	@  Clara, CA 95054 or visit www.sun.com if you need additional
	@  information or have any questions.


	@  ------------------------------------------------------
	@  Generated assembly file -- do *not* edit
.text
	@  ------------------------------------------------------
	@  Start segment: Interpreter signature area


	@  ------------------------------------------------------
	@  Interpreter signature

	.global	assembler_loop_type
assembler_loop_type:
	.long	0x41
	.pool
	@  End segment: Interpreter signature area


	@  ------------------------------------------------------
	@  Start segment: Interpreter method entry


	@  ------------------------------------------------------
	@  standard interpreter method entry

	.global	interpreter_method_entry
interpreter_method_entry:
	@  get parameter size in words
	ldrh	r2, [r0, #32]
	mov	r2, r2, lsl #23
	mov	r2, r2, lsr #23          @ r2 = parameter size in words
	add	r7, r6, r2, lsl #2
	sub	r7, r7, #4
	@  get maximum stack execution size
	ldrh	r3, [r0, #28]
	ldr	r1, [r5, #1024]          @ current_stack_limit 
	sub	r3, r6, r3, lsl #2
	sub	r3, r3, #28
	cmp	r3, r1
	adrls	r1, L0
	bls	interpreter_grow_stack
L0:
	@  get number of locals
	ldrh	r3, [r0, #30]
	@  pushing cleared locals
	sub	r3, r3, r2
	sub	r6, r6, r3, lsl #2
	@  get constants pool
	ldrh	r8, [r0, #24]            @ get access flags
	tst	r8, #0x200               @ compressed header?
	ldrne	r8, [r5, #1268]          @ rom_constant_pool_fast 
	ldreq	r8, [r0, #4]             @ method constant pool
	@  Reserve space on stack for frame
	sub	r6, r6, #28
	@  Fill in fields of the frame
	add	r8, r8, #12
	str	r6, [r6, #24]
	str	r7, [r6, #8]
	str	r8, [r6, #12]
	str	r0, [r6, #16]
	str	fp, [r6, #20]
	str	lr, [r6]
	@  Set the Frame pointer
	add	fp, r6, #20
	@  set bcp
	add	lr, r0, #40
	@  check if the method is synchronized
	ldrh	r2, [r0, #24]
	tst	r2, #32
	bne	L1
L2:
	@  Restore stack state from tos_on_stack
	ldr	r0, [r6], #4             @ cache topmost element of stack
	ldrb	r12, [lr]                @ prefetch next bytecode
	ldr	r2, [r5, #1032]          @ rt_timer_ticks  @ Timer tick?
	cmp	r2, #0
	bne	interpreter_timer_tick
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
synchronize_interpreted_method:
L1:
	@  synchronize method, register tmp0(r2) holds the access flags
	tst	r2, #8
	@  CC = ne: Synchronize on current class
	@  CC = eq: Synchronize on local 0
	ldrneh	r9, [r0, #26]
	ldrne	r4, [r5, #1204]          @ class_list_base 
	ldreq	r0, [r7]
	ldrne	r3, [r4, +r9, lsl #2]
	@  save interpreter state
	str	lr, [fp, #-16]
	@  if 'eq' then synchronizing on a local so skip all this mirror stuff
	beq	L4
	ldr	r4, [r5, #1208]          @ mirror_list_base 
	ldr	r4, [r4, +r9, lsl #2]
	ldr	r9, [r5, #1808]          @ task_class_init_marker 
	cmp	r4, r9
	bne	L4
L5:
	@  Class must be being initialized by the current thread then
	@  Get the class to search the clinit list of current thread
	@  get_mirror_from_clinit_list(mirror, klass, temp reg
	ldr	r9, [r5, #1804]          @ current_task 
	@  clinit_list (task mirror) into tmp2
	ldr	r4, [r9, #52]
L6:
	ldr	r9, [r4, #8]
	cmp	r3, r9
	beq	L7
	ldr	r4, [r4, #16]
	b	L6
L7:
	ldr	r0, [r4, #4]
	b	L3
L4:
	ldrne	r0, [r4, #4]
L3:
	@  object to lock is in r0
	bl	shared_lock_synchronized_method
	@  restore interpreter state
	ldr	r7, [fp, #-12]           @ restore locals
	ldr	r8, [fp, #-8]            @ restore cpool
	ldr	lr, [fp, #-16]           @ restore bcp
	b	L2
	.pool
	@  End segment: Interpreter method entry


	@  ------------------------------------------------------
	@  Start segment:  unnamed


	@  ------------------------------------------------------
	@  Fast interpreter method entry (0 extra locals)

	.global	fixed_interpreter_fast_method_entry_0
fixed_interpreter_fast_method_entry_0:
	nop

	.global	interpreter_fast_method_entry_0
interpreter_fast_method_entry_0:
	@  get parameter size in words
	ldrh	r2, [r0, #32]
	mov	r2, r2, lsl #23
	mov	r2, r2, lsr #23          @ r2 = parameter size in words
	add	r7, r6, r2, lsl #2
	sub	r7, r7, #4
	@  get maximum stack execution size
	ldrh	r3, [r0, #28]
	ldr	r1, [r5, #1024]          @ current_stack_limit 
	sub	r3, r6, r3, lsl #2
	sub	r3, r3, #28
	cmp	r3, r1
	adrls	r1, L8
	bls	interpreter_grow_stack
L8:
	@  get constants pool
	ldrh	r8, [r0, #24]            @ get access flags
	tst	r8, #0x200               @ compressed header?
	ldrne	r8, [r5, #1268]          @ rom_constant_pool_fast 
	ldreq	r8, [r0, #4]             @ method constant pool
	@  Reserve space on stack for frame
	sub	r6, r6, #28
	@  Fill in fields of the frame
	add	r8, r8, #12
	str	r6, [r6, #24]
	str	r7, [r6, #8]
	str	r8, [r6, #12]
	str	r0, [r6, #16]
	str	fp, [r6, #20]
	str	lr, [r6]
	@  Set the Frame pointer
	add	fp, r6, #20
	@  set bcp
	add	lr, r0, #40
	@  Restore stack state from tos_on_stack
	ldr	r0, [r6], #4             @ cache topmost element of stack
	ldrb	r12, [lr]                @ prefetch next bytecode
	ldr	r2, [r5, #1032]          @ rt_timer_ticks  @ Timer tick?
	cmp	r2, #0
	bne	interpreter_timer_tick
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed


	@  ------------------------------------------------------
	@  Fast interpreter method entry (1 extra locals)

	.global	fixed_interpreter_fast_method_entry_1
fixed_interpreter_fast_method_entry_1:
	nop

	.global	interpreter_fast_method_entry_1
interpreter_fast_method_entry_1:
	@  get parameter size in words
	ldrh	r2, [r0, #32]
	mov	r2, r2, lsl #23
	mov	r2, r2, lsr #23          @ r2 = parameter size in words
	add	r7, r6, r2, lsl #2
	sub	r7, r7, #4
	@  get maximum stack execution size
	ldrh	r3, [r0, #28]
	ldr	r1, [r5, #1024]          @ current_stack_limit 
	sub	r3, r6, r3, lsl #2
	sub	r3, r3, #28
	cmp	r3, r1
	adrls	r1, L9
	bls	interpreter_grow_stack
L9:
	@  push cleared locals
	sub	r6, r6, #4
	@  get constants pool
	ldrh	r8, [r0, #24]            @ get access flags
	tst	r8, #0x200               @ compressed header?
	ldrne	r8, [r5, #1268]          @ rom_constant_pool_fast 
	ldreq	r8, [r0, #4]             @ method constant pool
	@  Reserve space on stack for frame
	sub	r6, r6, #28
	@  Fill in fields of the frame
	add	r8, r8, #12
	str	r6, [r6, #24]
	str	r7, [r6, #8]
	str	r8, [r6, #12]
	str	r0, [r6, #16]
	str	fp, [r6, #20]
	str	lr, [r6]
	@  Set the Frame pointer
	add	fp, r6, #20
	@  set bcp
	add	lr, r0, #40
	@  Restore stack state from tos_on_stack
	ldr	r0, [r6], #4             @ cache topmost element of stack
	ldrb	r12, [lr]                @ prefetch next bytecode
	ldr	r2, [r5, #1032]          @ rt_timer_ticks  @ Timer tick?
	cmp	r2, #0
	bne	interpreter_timer_tick
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed


	@  ------------------------------------------------------
	@  Fast interpreter method entry (2 extra locals)

	.global	fixed_interpreter_fast_method_entry_2
fixed_interpreter_fast_method_entry_2:
	nop

	.global	interpreter_fast_method_entry_2
interpreter_fast_method_entry_2:
	@  get parameter size in words
	ldrh	r2, [r0, #32]
	mov	r2, r2, lsl #23
	mov	r2, r2, lsr #23          @ r2 = parameter size in words
	add	r7, r6, r2, lsl #2
	sub	r7, r7, #4
	@  get maximum stack execution size
	ldrh	r3, [r0, #28]
	ldr	r1, [r5, #1024]          @ current_stack_limit 
	sub	r3, r6, r3, lsl #2
	sub	r3, r3, #28
	cmp	r3, r1
	adrls	r1, L10
	bls	interpreter_grow_stack
L10:
	@  push cleared locals
	sub	r6, r6, #8
	@  get constants pool
	ldrh	r8, [r0, #24]            @ get access flags
	tst	r8, #0x200               @ compressed header?
	ldrne	r8, [r5, #1268]          @ rom_constant_pool_fast 
	ldreq	r8, [r0, #4]             @ method constant pool
	@  Reserve space on stack for frame
	sub	r6, r6, #28
	@  Fill in fields of the frame
	add	r8, r8, #12
	str	r6, [r6, #24]
	str	r7, [r6, #8]
	str	r8, [r6, #12]
	str	r0, [r6, #16]
	str	fp, [r6, #20]
	str	lr, [r6]
	@  Set the Frame pointer
	add	fp, r6, #20
	@  set bcp
	add	lr, r0, #40
	@  Restore stack state from tos_on_stack
	ldr	r0, [r6], #4             @ cache topmost element of stack
	ldrb	r12, [lr]                @ prefetch next bytecode
	ldr	r2, [r5, #1032]          @ rt_timer_ticks  @ Timer tick?
	cmp	r2, #0
	bne	interpreter_timer_tick
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed


	@  ------------------------------------------------------
	@  Fast interpreter method entry (3 extra locals)

	.global	fixed_interpreter_fast_method_entry_3
fixed_interpreter_fast_method_entry_3:
	nop

	.global	interpreter_fast_method_entry_3
interpreter_fast_method_entry_3:
	@  get parameter size in words
	ldrh	r2, [r0, #32]
	mov	r2, r2, lsl #23
	mov	r2, r2, lsr #23          @ r2 = parameter size in words
	add	r7, r6, r2, lsl #2
	sub	r7, r7, #4
	@  get maximum stack execution size
	ldrh	r3, [r0, #28]
	ldr	r1, [r5, #1024]          @ current_stack_limit 
	sub	r3, r6, r3, lsl #2
	sub	r3, r3, #28
	cmp	r3, r1
	adrls	r1, L11
	bls	interpreter_grow_stack
L11:
	@  push cleared locals
	sub	r6, r6, #12
	@  get constants pool
	ldrh	r8, [r0, #24]            @ get access flags
	tst	r8, #0x200               @ compressed header?
	ldrne	r8, [r5, #1268]          @ rom_constant_pool_fast 
	ldreq	r8, [r0, #4]             @ method constant pool
	@  Reserve space on stack for frame
	sub	r6, r6, #28
	@  Fill in fields of the frame
	add	r8, r8, #12
	str	r6, [r6, #24]
	str	r7, [r6, #8]
	str	r8, [r6, #12]
	str	r0, [r6, #16]
	str	fp, [r6, #20]
	str	lr, [r6]
	@  Set the Frame pointer
	add	fp, r6, #20
	@  set bcp
	add	lr, r0, #40
	@  Restore stack state from tos_on_stack
	ldr	r0, [r6], #4             @ cache topmost element of stack
	ldrb	r12, [lr]                @ prefetch next bytecode
	ldr	r2, [r5, #1032]          @ rt_timer_ticks  @ Timer tick?
	cmp	r2, #0
	bne	interpreter_timer_tick
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed


	@  ------------------------------------------------------
	@  Fast interpreter method entry (4 extra locals)

	.global	fixed_interpreter_fast_method_entry_4
fixed_interpreter_fast_method_entry_4:
	nop

	.global	interpreter_fast_method_entry_4
interpreter_fast_method_entry_4:
	@  get parameter size in words
	ldrh	r2, [r0, #32]
	mov	r2, r2, lsl #23
	mov	r2, r2, lsr #23          @ r2 = parameter size in words
	add	r7, r6, r2, lsl #2
	sub	r7, r7, #4
	@  get maximum stack execution size
	ldrh	r3, [r0, #28]
	ldr	r1, [r5, #1024]          @ current_stack_limit 
	sub	r3, r6, r3, lsl #2
	sub	r3, r3, #28
	cmp	r3, r1
	adrls	r1, L12
	bls	interpreter_grow_stack
L12:
	@  push cleared locals
	sub	r6, r6, #16
	@  get constants pool
	ldrh	r8, [r0, #24]            @ get access flags
	tst	r8, #0x200               @ compressed header?
	ldrne	r8, [r5, #1268]          @ rom_constant_pool_fast 
	ldreq	r8, [r0, #4]             @ method constant pool
	@  Reserve space on stack for frame
	sub	r6, r6, #28
	@  Fill in fields of the frame
	add	r8, r8, #12
	str	r6, [r6, #24]
	str	r7, [r6, #8]
	str	r8, [r6, #12]
	str	r0, [r6, #16]
	str	fp, [r6, #20]
	str	lr, [r6]
	@  Set the Frame pointer
	add	fp, r6, #20
	@  set bcp
	add	lr, r0, #40
	@  Restore stack state from tos_on_stack
	ldr	r0, [r6], #4             @ cache topmost element of stack
	ldrb	r12, [lr]                @ prefetch next bytecode
	ldr	r2, [r5, #1032]          @ rt_timer_ticks  @ Timer tick?
	cmp	r2, #0
	bne	interpreter_timer_tick
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed


	@  ------------------------------------------------------
	@  Quick native method entry (void)

	.global	quick_void_native_method_entry
quick_void_native_method_entry:
	mov	r3, #1
	str	r3, [r5, #1276]          @ jvm_in_quick_native_method 
	@  get parameter size in words
	ldrh	r2, [r0, #32]
	mov	r2, r2, lsl #23
	mov	r2, r2, lsr #23          @ r2 = parameter size in words
	@  get access flags
	ldrh	r4, [r0, #24]
	@  get the address of quick native function
	ldr	r3, [r0, #28]
	@  Calculate bottom of my parameters
	add	r6, r6, r2, lsl #2
	@  Test method staticness and set KNI parameters properly
	tst	r4, #8
	subeq	r4, r6, #4               @ Non-static method
	addne	r4, r6, #0               @ Static method, KNI-style, so first parameter at index 1
	str	r4, [r5, #1264]          @ kni_parameter_base 
	mov	r7, lr                   @ save return address of caller
	mov	lr, pc                   @ save my return address
	mov	pc, r3                   @ jump to native function
	@  Tell VM we're out of quick native methods
	ldr	r3, [r5, #1280]          @ jvm_quick_native_exception 
	mov	r4, #0
	str	r4, [r5, #1276]          @ jvm_in_quick_native_method 
	cmp	r3, #0
	moveq	r2, #0
	moveq	pc, r7                   @ return back to caller if no exception
L13:
	@  Let the exception be thrown by System.quickNativeThrow()
	mov	lr, r7
	.extern	persistent_handles
	ldr	r0, =persistent_handles + 244
	ldr	r0, [r0]
	b	interpreter_method_entry
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed


	@  ------------------------------------------------------
	@  Quick native method entry (int)

	.global	quick_int_native_method_entry
quick_int_native_method_entry:
	mov	r3, #1
	str	r3, [r5, #1276]          @ jvm_in_quick_native_method 
	@  get parameter size in words
	ldrh	r2, [r0, #32]
	mov	r2, r2, lsl #23
	mov	r2, r2, lsr #23          @ r2 = parameter size in words
	@  get access flags
	ldrh	r4, [r0, #24]
	@  get the address of quick native function
	ldr	r3, [r0, #28]
	@  Calculate bottom of my parameters
	add	r6, r6, r2, lsl #2
	@  Test method staticness and set KNI parameters properly
	tst	r4, #8
	subeq	r4, r6, #4               @ Non-static method
	addne	r4, r6, #0               @ Static method, KNI-style, so first parameter at index 1
	str	r4, [r5, #1264]          @ kni_parameter_base 
	mov	r7, lr                   @ save return address of caller
	mov	lr, pc                   @ save my return address
	mov	pc, r3                   @ jump to native function
	@  Tell VM we're out of quick native methods
	ldr	r3, [r5, #1280]          @ jvm_quick_native_exception 
	mov	r4, #0
	str	r4, [r5, #1276]          @ jvm_in_quick_native_method 
	cmp	r3, #0
	moveq	r2, #1
	moveq	pc, r7                   @ return back to caller if no exception
L14:
	@  Let the exception be thrown by System.quickNativeThrow()
	mov	lr, r7
	.extern	persistent_handles
	ldr	r0, =persistent_handles + 244
	ldr	r0, [r0]
	b	interpreter_method_entry
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed


	@  ------------------------------------------------------
	@  Quick native method entry (obj)

	.global	quick_obj_native_method_entry
quick_obj_native_method_entry:
	mov	r3, #1
	str	r3, [r5, #1276]          @ jvm_in_quick_native_method 
	@  get parameter size in words
	ldrh	r2, [r0, #32]
	mov	r2, r2, lsl #23
	mov	r2, r2, lsr #23          @ r2 = parameter size in words
	@  get access flags
	ldrh	r4, [r0, #24]
	@  get the address of quick native function
	ldr	r3, [r0, #28]
	@  Calculate bottom of my parameters
	add	r6, r6, r2, lsl #2
	@  Test method staticness and set KNI parameters properly
	tst	r4, #8
	subeq	r4, r6, #4               @ Non-static method
	addne	r4, r6, #0               @ Static method, KNI-style, so first parameter at index 1
	str	r4, [r5, #1264]          @ kni_parameter_base 
	mov	r7, lr                   @ save return address of caller
	mov	lr, pc                   @ save my return address
	mov	pc, r3                   @ jump to native function
	@  Tell VM we're out of quick native methods
	ldr	r3, [r5, #1280]          @ jvm_quick_native_exception 
	mov	r4, #0
	str	r4, [r5, #1276]          @ jvm_in_quick_native_method 
	cmp	r3, #0
	moveq	r2, #1
	moveq	pc, r7                   @ return back to caller if no exception
L15:
	@  Let the exception be thrown by System.quickNativeThrow()
	mov	lr, r7
	.extern	persistent_handles
	ldr	r0, =persistent_handles + 244
	ldr	r0, [r0]
	b	interpreter_method_entry
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment: Fast memory routines

	.global	jvm_memcmp
jvm_memcmp:
	@  for short areas use byte-by-byte comparision
	cmp	r2, #4
	ble	L16
	@  check, if strings both aligned or misaligned on the same offset
	eor	r3, r0, r1
	tst	r3, #3
	@  if no, go to the next check
	bne	L21
L20:
	@  align loop
	tst	r0, #3
	beq	L26
	ldrb	r3, [r0], #1
	ldrb	r12, [r1], #1
	cmp	r3, r12
	bne	L22
	sub	r2, r2, #1
	b	L20
L21:
	@  now test if we can be two bytes aligned
	tst	r3, #1
	@  go to byte-by-byte, if no
	bne	L19
	tst	r0, #1
	beq	L28
	@  2-byte align
	ldrb	r3, [r0], #1
	ldrb	r12, [r1], #1
	cmp	r3, r12
	bne	L22
	sub	r2, r2, #1
L28:
	@  avoid additional cmp in loop
	sub	r2, r2, #1
L27:
	ldrh	r3, [r0], #2
	ldrh	r12, [r1], #2
	cmp	r3, r12
	bne	L23
	subs	r2, r2, #2
	bgt	L27
	@  compensate
	adds	r2, r2, #1
	moveq	r0, #0
	moveq	pc, lr
	b	L16
L26:
	@  avoid additional cmp in loop
	cmp	r2, #4
	blt	L16
	sub	r2, r2, #3
L25:
	@  loop to do 4-byte aligned comparision
	ldr	r3, [r0], #4
	ldr	r12, [r1], #4
	cmp	r3, r12
	bne	L24
	subs	r2, r2, #4
	bgt	L25
	@  compensate
	add	r2, r2, #3
L16:
	@  we're here when data is heavily misaligned, or few bytes left
	cmp	r2, #0
	moveq	r0, #0
	moveq	pc, lr
L19:
	tst	r2, #1
	addne	r2, r2, #1
	bne	L18
L17:
	@  unrolled x2 loop for byte at the time processing
	ldrb	r3, [r0], #1
	ldrb	r12, [r1], #1
	cmp	r3, r12
	bne	L22
L18:
	ldrb	r3, [r0], #1
	ldrb	r12, [r1], #1
	cmp	r3, r12
	bne	L22
	subs	r2, r2, #2
	bne	L17
L22:
	sub	r0, r3, r12
	mov	pc, lr
L24:
	@  we found words not equal, find offender
L23:
	and	r0, r3, #255
	and	r1, r12, #255
	subs	r0, r0, r1
	movne	pc, lr
	mov	r3, r3, lsr #8
	mov	r12, r12, lsr #8
	b	L24

	.global	jvm_memcpy
jvm_memcpy:
	@  Check to see if the length is 0.  Also check to see
	@   which direction we need to do the copying in: 
	cmp	r2, #0
	cmpne	r0, r1
	@  Nothing to copy.  Return to caller. 
	moveq	pc, lr
	bhi	L40
L29:
	@   Do forward copying:
	cmp	r2, #4
	blt	L37
	@  Check if the r0 and r1 pointers are similarly aligned: 
	@  Equal bits will become 0. 
	eor	r3, r1, r0
	@   Check if low 2 bits are equal 
	tst	r3, #3
	bne	L37
	@   Do aligned copying:  Check if already word aligned
	tst	r1, #3
	bne	L35
L30:
	and	r12, r2, #3
	bic	r2, r2, #3
	subs	r2, r2, #4
	blt	L32
L31:
	ldr	r3, [r1], #4
	subs	r2, r2, #4
	str	r3, [r0], #4
	bge	L31
L32:
	@   Copy the trailing half word if necessary:
	tst	r12, #2
	beq	L33
	ldrh	r3, [r1], #2
	strh	r3, [r0], #2
L33:
	@   Copy the trailing byte if necessary:
	tst	r12, #1
	beq	L34
	ldrb	r3, [r1]
	strb	r3, [r0]
L34:
	@   Return to caller.
	mov	pc, lr
L35:
	@    Copy leading byte if necessary:
	tst	r1, #1
	beq	L36
	ldrb	r3, [r1], #1
	sub	r2, r2, #1
	strb	r3, [r0], #1
L36:
	@     Copy the leading halfword if necessary:
	tst	r1, #2
	beq	L30
	ldrh	r3, [r1], #2
	sub	r2, r2, #2
	strh	r3, [r0], #2
	b	L30
L37:
	subs	r2, r2, #1
	blt	L39
L38:
	ldrb	r3, [r1], #1
	subs	r2, r2, #1
	strb	r3, [r0], #1
	bge	L38
L39:
	@   Return to caller.
	mov	pc, lr
L40:
	@  If (dest >= src + length), then we can do forward copying: 
	add	r12, r1, r2
	cmp	r0, r12
	bcs	L29
L41:
	@  Do backward copying:
	subs	r2, r2, #1
	add	r1, r1, r2
	add	r0, r0, r2
	blt	L43
L42:
	ldrb	r3, [r1], #-1
	subs	r2, r2, #1
	strb	r3, [r0], #-1
	bge	L42
L43:
	@   Return to caller.
	mov	pc, lr
	.pool
	@  End segment: Fast memory routines


	@  ------------------------------------------------------
	@  Start segment: Interpreter grow stack

	.global	interpreter_grow_stack
interpreter_grow_stack:
	@  r0   contains the method
	@  r1   contains the return address of the interpreter_xxx
	@  lr   contains whoever called us
	@  tmp1 contains the amount of stack we need
	@  locals contains the current locals pointer
	@  get constants pool
	ldrh	r8, [r0, #24]            @ get access flags
	tst	r8, #0x200               @ compressed header?
	ldrne	r8, [r5, #1268]          @ rom_constant_pool_fast 
	ldreq	r8, [r0, #4]             @ method constant pool
	@  Reserve space on stack for frame
	sub	r6, r6, #28
	@  Fill in fields of the frame
	add	r8, r8, #12
	str	r6, [r6, #24]
	str	r7, [r6, #8]
	str	r8, [r6, #12]
	str	r0, [r6, #16]
	str	fp, [r6, #20]
	str	lr, [r6]
	@  Set the Frame pointer
	add	fp, r6, #20
	add	lr, r0, #40
	add	lr, lr, #0x20000000
	@  save interpreter state
	str	lr, [fp, #-16]
	str	r1, [r6, #-4]!
	@  stack_overflow(limit)
	mov	r1, r3
	.extern	stack_overflow
	ldr	r3, =stack_overflow
	bl	interpreter_call_vm
	@  Get real return address
	ldr	r4, [r6]
	ldr	r0, [fp, #-4]
	ldr	r7, [fp, #-12]
	ldr	lr, [fp, #-20]
	mov	r6, fp
	ldr	fp, [r6], #8
	ldrh	r2, [r0, #32]
	mov	r2, r2, lsl #23
	mov	r2, r2, lsr #23          @ r2 = parameter size in words
	mov	pc, r4
	.pool
	@  End segment: Interpreter grow stack




	@  ------------------------------------------------------
	@  Interpreter bytecode templates
	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_nop:

	.global	bc_impl_nop_internal
bc_impl_nop_internal:
	@  do nothing
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe0:
	.pool
	@  End segment: unnamed


	@  aconst_null is a duplicate of iconst_0
	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_iconst_m1:

	.global	bc_impl_iconst_m1_internal
bc_impl_iconst_m1_internal:
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	@  Entry: Count = 0
	str	r0, [r6, #-4]!
	mvn	r0, #0
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe1:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_iconst_0:

	.global	bc_impl_aconst_null
bc_impl_aconst_null:

	.global	bc_impl_fconst_0
bc_impl_fconst_0:

	.global	bc_impl_iconst_0_internal
bc_impl_iconst_0_internal:

	.global	bc_impl_aconst_null_internal
bc_impl_aconst_null_internal:

	.global	bc_impl_fconst_0_internal
bc_impl_fconst_0_internal:
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	@  Entry: Count = 0
	str	r0, [r6, #-4]!
	mov	r0, #0
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe2:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_iconst_1:

	.global	bc_impl_iconst_1_internal
bc_impl_iconst_1_internal:
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	@  Entry: Count = 0
	str	r0, [r6, #-4]!
	mov	r0, #1
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe3:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_iconst_2:

	.global	bc_impl_iconst_2_internal
bc_impl_iconst_2_internal:
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	@  Entry: Count = 0
	str	r0, [r6, #-4]!
	mov	r0, #2
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe4:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_iconst_3:

	.global	bc_impl_iconst_3_internal
bc_impl_iconst_3_internal:
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	@  Entry: Count = 0
	str	r0, [r6, #-4]!
	mov	r0, #3
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe5:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_iconst_4:

	.global	bc_impl_iconst_4_internal
bc_impl_iconst_4_internal:
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	@  Entry: Count = 0
	str	r0, [r6, #-4]!
	mov	r0, #4
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe6:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_iconst_5:

	.global	bc_impl_iconst_5_internal
bc_impl_iconst_5_internal:
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	@  Entry: Count = 0
	str	r0, [r6, #-4]!
	mov	r0, #5
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe7:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_lconst_0:

	.global	bc_impl_lconst_0_internal
bc_impl_lconst_0_internal:
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	@  Entry: Count = 0
	str	r0, [r6, #-4]!
	mov	r0, #0
	mov	r2, #0
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	str	r2, [r6, #-4]!
	mov	pc, r12                  @ go to next bytecode
.LLfe8:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_lconst_1:

	.global	bc_impl_lconst_1_internal
bc_impl_lconst_1_internal:
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	@  Entry: Count = 0
	str	r0, [r6, #-4]!
	mov	r0, #1
	mov	r2, #0
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	str	r2, [r6, #-4]!
	mov	pc, r12                  @ go to next bytecode
.LLfe9:
	.pool
	@  End segment: unnamed


	@  fconst_0 is a duplicate of iconst_0
	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_fconst_1:

	.global	bc_impl_fconst_1_internal
bc_impl_fconst_1_internal:
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	@  Entry: Count = 0
	str	r0, [r6, #-4]!
	mov	r0, #0x3f800000
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe10:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_fconst_2:

	.global	bc_impl_fconst_2_internal
bc_impl_fconst_2_internal:
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	@  Entry: Count = 0
	str	r0, [r6, #-4]!
	mov	r0, #0x40000000
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe11:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_dconst_0:

	.global	bc_impl_dconst_0_internal
bc_impl_dconst_0_internal:
	@  Entry: Count = 0
	str	r0, [r6, #-4]!
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	mov	r0, #0
	mov	r2, #0
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	str	r2, [r6, #-4]!
	mov	pc, r12                  @ go to next bytecode
.LLfe12:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_dconst_1:

	.global	bc_impl_dconst_1_internal
bc_impl_dconst_1_internal:
	@  Entry: Count = 0
	str	r0, [r6, #-4]!
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	mov	r0, #0
	mov	r2, #0xff00000
	add	r2, r2, #0x30000000
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	str	r2, [r6, #-4]!
	mov	pc, r12                  @ go to next bytecode
.LLfe13:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_bipush:

	.global	bc_impl_bipush_internal
bc_impl_bipush_internal:
	@  Entry: Count = 0
	str	r0, [r6, #-4]!
	ldrb	r12, [lr, #2]!           @ prefetch next bytecode
	ldrsb	r0, [lr, #-1]
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe14:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_sipush:

	.global	bc_impl_sipush_internal
bc_impl_sipush_internal:
	@  Entry: Count = 0
	str	r0, [r6, #-4]!
	ldrsb	r3, [lr, #1]
	ldrb	r4, [lr, #2]
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	orr	r0, r4, r3, lsl #8
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe15:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_ldc:

	.global	bc_impl_ldc_w
bc_impl_ldc_w:

	.global	bc_impl_ldc2_w
bc_impl_ldc2_w:

	.global	bc_impl_invokevirtual
bc_impl_invokevirtual:

	.global	bc_impl_invokespecial
bc_impl_invokespecial:

	.global	bc_impl_ldc_internal
bc_impl_ldc_internal:

	.global	bc_impl_ldc_w_internal
bc_impl_ldc_w_internal:

	.global	bc_impl_ldc2_w_internal
bc_impl_ldc2_w_internal:

	.global	bc_impl_invokevirtual_internal
bc_impl_invokevirtual_internal:

	.global	bc_impl_invokespecial_internal
bc_impl_invokespecial_internal:
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	.extern	quicken
	ldr	r3, =quicken
	@  save interpreter state
	str	lr, [fp, #-16]
	bl	interpreter_call_vm
	@  restore interpreter state
	ldr	r7, [fp, #-12]           @ restore locals
	ldr	r8, [fp, #-8]            @ restore cpool
	ldr	lr, [fp, #-16]           @ restore bcp
	ldrb	r12, [lr]                @ prefetch next bytecode
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	@  Restore stack state from tos_on_stack
	ldr	r0, [r6], #4             @ cache topmost element of stack
	mov	pc, r12                  @ go to next bytecode
.LLfe16:
	.pool
	@  End segment: unnamed


	@  ldc_w is a duplicate of ldc
	@  ldc2_w is a duplicate of ldc
	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_wide_iload:

	.global	bc_impl_wide_fload
bc_impl_wide_fload:

	.global	bc_impl_wide_aload
bc_impl_wide_aload:

	.global	bc_impl_wide_iload_internal
bc_impl_wide_iload_internal:

	.global	bc_impl_wide_fload_internal
bc_impl_wide_fload_internal:

	.global	bc_impl_wide_aload_internal
bc_impl_wide_aload_internal:
	@  Entry: Count = 0
	str	r0, [r6, #-4]!
	@  load unsigned 16bit word at bcp + 2
	ldrb	r9, [lr, #2]
	ldrb	r2, [lr, #3]
	ldrb	r12, [lr, #4]!           @ prefetch next bytecode
	orr	r9, r2, r9, lsl #8
	ldr	r0, [r7, -r9, lsl #2]
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe17:
	@  Data containing entry for wide bytecode
	.long	bc_impl_wide_aload
bc_impl_iload:

	.global	bc_impl_fload
bc_impl_fload:

	.global	bc_impl_aload
bc_impl_aload:

	.global	bc_impl_iload_internal
bc_impl_iload_internal:

	.global	bc_impl_fload_internal
bc_impl_fload_internal:

	.global	bc_impl_aload_internal
bc_impl_aload_internal:
	@  Entry: Count = 0
	str	r0, [r6, #-4]!
	@  load unsigned byte at bcp + 1
	ldrb	r9, [lr, #1]
	ldrb	r12, [lr, #2]!           @ prefetch next bytecode
	ldr	r0, [r7, -r9, lsl #2]
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe18:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_wide_lload:

	.global	bc_impl_wide_dload
bc_impl_wide_dload:

	.global	bc_impl_wide_lload_internal
bc_impl_wide_lload_internal:

	.global	bc_impl_wide_dload_internal
bc_impl_wide_dload_internal:
	@  Entry: Count = 0
	str	r0, [r6, #-4]!
	@  load unsigned 16bit word at bcp + 2
	ldrb	r9, [lr, #2]
	ldrb	r2, [lr, #3]
	ldrb	r12, [lr, #4]!           @ prefetch next bytecode
	orr	r9, r2, r9, lsl #8
	sub	r4, r7, r9, lsl #2       @ address of local #r9
	ldr	r0, [r4, #-4]
	ldr	r2, [r4]
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	str	r2, [r6, #-4]!
	mov	pc, r12                  @ go to next bytecode
.LLfe19:
	@  Data containing entry for wide bytecode
	.long	bc_impl_wide_dload
bc_impl_lload:

	.global	bc_impl_dload
bc_impl_dload:

	.global	bc_impl_lload_internal
bc_impl_lload_internal:

	.global	bc_impl_dload_internal
bc_impl_dload_internal:
	@  Entry: Count = 0
	str	r0, [r6, #-4]!
	@  load unsigned byte at bcp + 1
	ldrb	r9, [lr, #1]
	ldrb	r12, [lr, #2]!           @ prefetch next bytecode
	sub	r4, r7, r9, lsl #2       @ address of local #r9
	ldr	r0, [r4, #-4]
	ldr	r2, [r4]
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	str	r2, [r6, #-4]!
	mov	pc, r12                  @ go to next bytecode
.LLfe20:
	.pool
	@  End segment: unnamed


	@  fload is a duplicate of iload
	@  dload is a duplicate of lload
	@  aload is a duplicate of iload
	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_iload_0:

	.global	bc_impl_fload_0
bc_impl_fload_0:

	.global	bc_impl_aload_0
bc_impl_aload_0:

	.global	bc_impl_iload_0_internal
bc_impl_iload_0_internal:

	.global	bc_impl_fload_0_internal
bc_impl_fload_0_internal:

	.global	bc_impl_aload_0_internal
bc_impl_aload_0_internal:
	@  Entry: Count = 0
	str	r0, [r6, #-4]!
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	ldr	r0, [r7]
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe21:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_iload_1:

	.global	bc_impl_fload_1
bc_impl_fload_1:

	.global	bc_impl_aload_1
bc_impl_aload_1:

	.global	bc_impl_iload_1_internal
bc_impl_iload_1_internal:

	.global	bc_impl_fload_1_internal
bc_impl_fload_1_internal:

	.global	bc_impl_aload_1_internal
bc_impl_aload_1_internal:
	@  Entry: Count = 0
	str	r0, [r6, #-4]!
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	ldr	r0, [r7, #-4]
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe22:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_iload_2:

	.global	bc_impl_fload_2
bc_impl_fload_2:

	.global	bc_impl_aload_2
bc_impl_aload_2:

	.global	bc_impl_iload_2_internal
bc_impl_iload_2_internal:

	.global	bc_impl_fload_2_internal
bc_impl_fload_2_internal:

	.global	bc_impl_aload_2_internal
bc_impl_aload_2_internal:
	@  Entry: Count = 0
	str	r0, [r6, #-4]!
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	ldr	r0, [r7, #-8]
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe23:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_iload_3:

	.global	bc_impl_fload_3
bc_impl_fload_3:

	.global	bc_impl_aload_3
bc_impl_aload_3:

	.global	bc_impl_iload_3_internal
bc_impl_iload_3_internal:

	.global	bc_impl_fload_3_internal
bc_impl_fload_3_internal:

	.global	bc_impl_aload_3_internal
bc_impl_aload_3_internal:
	@  Entry: Count = 0
	str	r0, [r6, #-4]!
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	ldr	r0, [r7, #-12]
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe24:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_lload_0:

	.global	bc_impl_dload_0
bc_impl_dload_0:

	.global	bc_impl_lload_0_internal
bc_impl_lload_0_internal:

	.global	bc_impl_dload_0_internal
bc_impl_dload_0_internal:
	@  Entry: Count = 0
	str	r0, [r6, #-4]!
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	ldr	r2, [r7]
	ldr	r0, [r7, #-4]
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	str	r2, [r6, #-4]!
	mov	pc, r12                  @ go to next bytecode
.LLfe25:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_lload_1:

	.global	bc_impl_dload_1
bc_impl_dload_1:

	.global	bc_impl_lload_1_internal
bc_impl_lload_1_internal:

	.global	bc_impl_dload_1_internal
bc_impl_dload_1_internal:
	@  Entry: Count = 0
	str	r0, [r6, #-4]!
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	ldr	r2, [r7, #-4]
	ldr	r0, [r7, #-8]
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	str	r2, [r6, #-4]!
	mov	pc, r12                  @ go to next bytecode
.LLfe26:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_lload_2:

	.global	bc_impl_dload_2
bc_impl_dload_2:

	.global	bc_impl_lload_2_internal
bc_impl_lload_2_internal:

	.global	bc_impl_dload_2_internal
bc_impl_dload_2_internal:
	@  Entry: Count = 0
	str	r0, [r6, #-4]!
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	ldr	r2, [r7, #-8]
	ldr	r0, [r7, #-12]
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	str	r2, [r6, #-4]!
	mov	pc, r12                  @ go to next bytecode
.LLfe27:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_lload_3:

	.global	bc_impl_dload_3
bc_impl_dload_3:

	.global	bc_impl_lload_3_internal
bc_impl_lload_3_internal:

	.global	bc_impl_dload_3_internal
bc_impl_dload_3_internal:
	@  Entry: Count = 0
	str	r0, [r6, #-4]!
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	ldr	r2, [r7, #-12]
	ldr	r0, [r7, #-16]
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	str	r2, [r6, #-4]!
	mov	pc, r12                  @ go to next bytecode
.LLfe28:
	.pool
	@  End segment: unnamed


	@  fload_0 is a duplicate of iload_0
	@  fload_1 is a duplicate of iload_1
	@  fload_2 is a duplicate of iload_2
	@  fload_3 is a duplicate of iload_3
	@  dload_0 is a duplicate of lload_0
	@  dload_1 is a duplicate of lload_1
	@  dload_2 is a duplicate of lload_2
	@  dload_3 is a duplicate of lload_3
	@  aload_0 is a duplicate of iload_0
	@  aload_1 is a duplicate of iload_1
	@  aload_2 is a duplicate of iload_2
	@  aload_3 is a duplicate of iload_3
	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_iaload:

	.global	bc_impl_faload
bc_impl_faload:

	.global	bc_impl_aaload
bc_impl_aaload:

	.global	bc_impl_iaload_internal
bc_impl_iaload_internal:

	.global	bc_impl_faload_internal
bc_impl_faload_internal:

	.global	bc_impl_aaload_internal
bc_impl_aaload_internal:
	@  Entry: Count = 2
	ldr	r2, [r6], #4
	cmp	r2, #0
	ldrne	r3, [r2, #4]
	beq	L44
	cmp	r0, r3
	bcs	L45
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	add	r2, r2, r0, lsl #2       @ load array element
	ldr	r0, [r2, #8]
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
L44:
	@  Handle null pointer exception
	str	r2, [r6, #-4]!
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	b	interpreter_throw_NullPointerException
L45:
	@  Handle array index out of bounds exception
	str	r2, [r6, #-4]!
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	b	interpreter_throw_ArrayIndexOutOfBoundsException
.LLfe29:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_laload:

	.global	bc_impl_daload
bc_impl_daload:

	.global	bc_impl_laload_internal
bc_impl_laload_internal:

	.global	bc_impl_daload_internal
bc_impl_daload_internal:
	@  Entry: Count = 2
	ldr	r2, [r6], #4
	cmp	r2, #0
	ldrne	r3, [r2, #4]
	beq	L46
	cmp	r0, r3
	bcs	L47
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	add	r2, r2, r0, lsl #3       @ load array element
	ldr	r0, [r2, #8]
	ldr	r2, [r2, #12]
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	str	r2, [r6, #-4]!
	mov	pc, r12                  @ go to next bytecode
L46:
	@  Handle null pointer exception
	str	r2, [r6, #-4]!
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	b	interpreter_throw_NullPointerException
L47:
	@  Handle array index out of bounds exception
	str	r2, [r6, #-4]!
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	b	interpreter_throw_ArrayIndexOutOfBoundsException
.LLfe30:
	.pool
	@  End segment: unnamed


	@  faload is a duplicate of iaload
	@  daload is a duplicate of laload
	@  aaload is a duplicate of iaload
	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_baload:

	.global	bc_impl_baload_internal
bc_impl_baload_internal:
	@  Entry: Count = 2
	ldr	r2, [r6], #4
	cmp	r2, #0
	ldrne	r3, [r2, #4]
	beq	L48
	cmp	r0, r3
	bcs	L49
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	add	r2, r2, r0               @ load array element
	ldrsb	r0, [r2, #8]
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
L48:
	@  Handle null pointer exception
	str	r2, [r6, #-4]!
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	b	interpreter_throw_NullPointerException
L49:
	@  Handle array index out of bounds exception
	str	r2, [r6, #-4]!
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	b	interpreter_throw_ArrayIndexOutOfBoundsException
.LLfe31:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_caload:

	.global	bc_impl_caload_internal
bc_impl_caload_internal:
	@  Entry: Count = 2
	ldr	r2, [r6], #4
	cmp	r2, #0
	ldrne	r3, [r2, #4]
	beq	L50
	cmp	r0, r3
	bcs	L51
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	add	r2, r2, r0, lsl #1       @ load array element
	ldrh	r0, [r2, #8]
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
L50:
	@  Handle null pointer exception
	str	r2, [r6, #-4]!
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	b	interpreter_throw_NullPointerException
L51:
	@  Handle array index out of bounds exception
	str	r2, [r6, #-4]!
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	b	interpreter_throw_ArrayIndexOutOfBoundsException
.LLfe32:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_saload:

	.global	bc_impl_saload_internal
bc_impl_saload_internal:
	@  Entry: Count = 2
	ldr	r2, [r6], #4
	cmp	r2, #0
	ldrne	r3, [r2, #4]
	beq	L52
	cmp	r0, r3
	bcs	L53
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	add	r2, r2, r0, lsl #1       @ load array element
	ldrsh	r0, [r2, #8]
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
L52:
	@  Handle null pointer exception
	str	r2, [r6, #-4]!
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	b	interpreter_throw_NullPointerException
L53:
	@  Handle array index out of bounds exception
	str	r2, [r6, #-4]!
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	b	interpreter_throw_ArrayIndexOutOfBoundsException
.LLfe33:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_wide_istore:

	.global	bc_impl_wide_fstore
bc_impl_wide_fstore:

	.global	bc_impl_wide_astore
bc_impl_wide_astore:

	.global	bc_impl_wide_istore_internal
bc_impl_wide_istore_internal:

	.global	bc_impl_wide_fstore_internal
bc_impl_wide_fstore_internal:

	.global	bc_impl_wide_astore_internal
bc_impl_wide_astore_internal:
	@  Entry: Count = 1
	@  load unsigned 16bit word at bcp + 2
	ldrb	r9, [lr, #2]
	ldrb	r4, [lr, #3]
	ldrb	r12, [lr, #4]!           @ prefetch next bytecode
	orr	r9, r4, r9, lsl #8
	str	r0, [r7, -r9, lsl #2]
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	ldr	r0, [r6], #4
	mov	pc, r12                  @ go to next bytecode
.LLfe34:
	@  Data containing entry for wide bytecode
	.long	bc_impl_wide_astore
bc_impl_istore:

	.global	bc_impl_fstore
bc_impl_fstore:

	.global	bc_impl_astore
bc_impl_astore:

	.global	bc_impl_istore_internal
bc_impl_istore_internal:

	.global	bc_impl_fstore_internal
bc_impl_fstore_internal:

	.global	bc_impl_astore_internal
bc_impl_astore_internal:
	@  Entry: Count = 1
	@  load unsigned byte at bcp + 1
	ldrb	r9, [lr, #1]
	ldrb	r12, [lr, #2]!           @ prefetch next bytecode
	str	r0, [r7, -r9, lsl #2]
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	ldr	r0, [r6], #4
	mov	pc, r12                  @ go to next bytecode
.LLfe35:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_wide_lstore:

	.global	bc_impl_wide_dstore
bc_impl_wide_dstore:

	.global	bc_impl_wide_lstore_internal
bc_impl_wide_lstore_internal:

	.global	bc_impl_wide_dstore_internal
bc_impl_wide_dstore_internal:
	@  Entry: Count = 2
	ldr	r2, [r6], #4
	@  load unsigned 16bit word at bcp + 2
	ldrb	r9, [lr, #2]
	ldrb	r4, [lr, #3]
	ldrb	r12, [lr, #4]!           @ prefetch next bytecode
	orr	r9, r4, r9, lsl #8
	sub	r4, r7, r9, lsl #2       @ address of local #r9
	str	r0, [r4, #-4]
	str	r2, [r4]
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	ldr	r0, [r6], #4
	mov	pc, r12                  @ go to next bytecode
.LLfe36:
	@  Data containing entry for wide bytecode
	.long	bc_impl_wide_dstore
bc_impl_lstore:

	.global	bc_impl_dstore
bc_impl_dstore:

	.global	bc_impl_lstore_internal
bc_impl_lstore_internal:

	.global	bc_impl_dstore_internal
bc_impl_dstore_internal:
	@  Entry: Count = 2
	ldr	r2, [r6], #4
	@  load unsigned byte at bcp + 1
	ldrb	r9, [lr, #1]
	ldrb	r12, [lr, #2]!           @ prefetch next bytecode
	sub	r4, r7, r9, lsl #2       @ address of local #r9
	str	r0, [r4, #-4]
	str	r2, [r4]
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	ldr	r0, [r6], #4
	mov	pc, r12                  @ go to next bytecode
.LLfe37:
	.pool
	@  End segment: unnamed


	@  fstore is a duplicate of istore
	@  dstore is a duplicate of lstore
	@  astore is a duplicate of istore
	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_istore_0:

	.global	bc_impl_fstore_0
bc_impl_fstore_0:

	.global	bc_impl_astore_0
bc_impl_astore_0:

	.global	bc_impl_istore_0_internal
bc_impl_istore_0_internal:

	.global	bc_impl_fstore_0_internal
bc_impl_fstore_0_internal:

	.global	bc_impl_astore_0_internal
bc_impl_astore_0_internal:
	@  Entry: Count = 1
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	str	r0, [r7]
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	ldr	r0, [r6], #4
	mov	pc, r12                  @ go to next bytecode
.LLfe38:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_istore_1:

	.global	bc_impl_fstore_1
bc_impl_fstore_1:

	.global	bc_impl_astore_1
bc_impl_astore_1:

	.global	bc_impl_istore_1_internal
bc_impl_istore_1_internal:

	.global	bc_impl_fstore_1_internal
bc_impl_fstore_1_internal:

	.global	bc_impl_astore_1_internal
bc_impl_astore_1_internal:
	@  Entry: Count = 1
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	str	r0, [r7, #-4]
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	ldr	r0, [r6], #4
	mov	pc, r12                  @ go to next bytecode
.LLfe39:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_istore_2:

	.global	bc_impl_fstore_2
bc_impl_fstore_2:

	.global	bc_impl_astore_2
bc_impl_astore_2:

	.global	bc_impl_istore_2_internal
bc_impl_istore_2_internal:

	.global	bc_impl_fstore_2_internal
bc_impl_fstore_2_internal:

	.global	bc_impl_astore_2_internal
bc_impl_astore_2_internal:
	@  Entry: Count = 1
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	str	r0, [r7, #-8]
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	ldr	r0, [r6], #4
	mov	pc, r12                  @ go to next bytecode
.LLfe40:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_istore_3:

	.global	bc_impl_fstore_3
bc_impl_fstore_3:

	.global	bc_impl_astore_3
bc_impl_astore_3:

	.global	bc_impl_istore_3_internal
bc_impl_istore_3_internal:

	.global	bc_impl_fstore_3_internal
bc_impl_fstore_3_internal:

	.global	bc_impl_astore_3_internal
bc_impl_astore_3_internal:
	@  Entry: Count = 1
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	str	r0, [r7, #-12]
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	ldr	r0, [r6], #4
	mov	pc, r12                  @ go to next bytecode
.LLfe41:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_lstore_0:

	.global	bc_impl_dstore_0
bc_impl_dstore_0:

	.global	bc_impl_lstore_0_internal
bc_impl_lstore_0_internal:

	.global	bc_impl_dstore_0_internal
bc_impl_dstore_0_internal:
	@  Entry: Count = 2
	ldr	r2, [r6], #4
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	str	r0, [r7, #-4]
	str	r2, [r7]
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	ldr	r0, [r6], #4
	mov	pc, r12                  @ go to next bytecode
.LLfe42:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_lstore_1:

	.global	bc_impl_dstore_1
bc_impl_dstore_1:

	.global	bc_impl_lstore_1_internal
bc_impl_lstore_1_internal:

	.global	bc_impl_dstore_1_internal
bc_impl_dstore_1_internal:
	@  Entry: Count = 2
	ldr	r2, [r6], #4
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	str	r0, [r7, #-8]
	str	r2, [r7, #-4]
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	ldr	r0, [r6], #4
	mov	pc, r12                  @ go to next bytecode
.LLfe43:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_lstore_2:

	.global	bc_impl_dstore_2
bc_impl_dstore_2:

	.global	bc_impl_lstore_2_internal
bc_impl_lstore_2_internal:

	.global	bc_impl_dstore_2_internal
bc_impl_dstore_2_internal:
	@  Entry: Count = 2
	ldr	r2, [r6], #4
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	str	r0, [r7, #-12]
	str	r2, [r7, #-8]
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	ldr	r0, [r6], #4
	mov	pc, r12                  @ go to next bytecode
.LLfe44:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_lstore_3:

	.global	bc_impl_dstore_3
bc_impl_dstore_3:

	.global	bc_impl_lstore_3_internal
bc_impl_lstore_3_internal:

	.global	bc_impl_dstore_3_internal
bc_impl_dstore_3_internal:
	@  Entry: Count = 2
	ldr	r2, [r6], #4
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	str	r0, [r7, #-16]
	str	r2, [r7, #-12]
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	ldr	r0, [r6], #4
	mov	pc, r12                  @ go to next bytecode
.LLfe45:
	.pool
	@  End segment: unnamed


	@  fstore_0 is a duplicate of istore_0
	@  fstore_1 is a duplicate of istore_1
	@  fstore_2 is a duplicate of istore_2
	@  fstore_3 is a duplicate of istore_3
	@  dstore_0 is a duplicate of lstore_0
	@  dstore_1 is a duplicate of lstore_1
	@  dstore_2 is a duplicate of lstore_2
	@  dstore_3 is a duplicate of lstore_3
	@  astore_0 is a duplicate of istore_0
	@  astore_1 is a duplicate of istore_1
	@  astore_2 is a duplicate of istore_2
	@  astore_3 is a duplicate of istore_3
	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_iastore:

	.global	bc_impl_fastore
bc_impl_fastore:

	.global	bc_impl_iastore_internal
bc_impl_iastore_internal:

	.global	bc_impl_fastore_internal
bc_impl_fastore_internal:
	@  Entry: Count = 3
	ldr	r2, [r6], #4
	ldr	r4, [r6], #4
	cmp	r4, #0
	ldrne	r3, [r4, #4]
	beq	L54
	cmp	r2, r3
	bcs	L55
L57:
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	@  store array element
	add	r4, r4, r2, lsl #2
	str	r0, [r4, #8]
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	ldr	r0, [r6], #4
	mov	pc, r12                  @ go to next bytecode
L54:
	@  Handle null pointer exception
	str	r4, [r6, #-4]!
	str	r2, [r6, #-4]!
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	b	interpreter_throw_NullPointerException
L55:
	@  Handle array index out of bounds exception
	str	r4, [r6, #-4]!
	str	r2, [r6, #-4]!
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	b	interpreter_throw_ArrayIndexOutOfBoundsException
L56:
.LLfe46:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_lastore:

	.global	bc_impl_dastore
bc_impl_dastore:

	.global	bc_impl_lastore_internal
bc_impl_lastore_internal:

	.global	bc_impl_dastore_internal
bc_impl_dastore_internal:
	@  Entry: Count = 4
	ldr	r2, [r6], #4
	ldr	r4, [r6], #4
	ldr	r10, [r6], #4
	cmp	r10, #0
	ldrne	r3, [r10, #4]
	beq	L58
	cmp	r4, r3
	bcs	L59
L61:
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	@  store array element
	add	r10, r10, r4, lsl #3
	str	r0, [r10, #8]
	str	r2, [r10, #12]
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	ldr	r0, [r6], #4
	mov	pc, r12                  @ go to next bytecode
L58:
	@  Handle null pointer exception
	str	r10, [r6, #-4]!
	str	r4, [r6, #-4]!
	str	r2, [r6, #-4]!
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	b	interpreter_throw_NullPointerException
L59:
	@  Handle array index out of bounds exception
	str	r10, [r6, #-4]!
	str	r4, [r6, #-4]!
	str	r2, [r6, #-4]!
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	b	interpreter_throw_ArrayIndexOutOfBoundsException
L60:
.LLfe47:
	.pool
	@  End segment: unnamed


	@  fastore is a duplicate of iastore
	@  dastore is a duplicate of lastore
	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_aastore:

	.global	bc_impl_aastore_internal
bc_impl_aastore_internal:
	@  Entry: Count = 3
	ldr	r2, [r6], #4
	ldr	r4, [r6], #4
	cmp	r4, #0
	ldrne	r3, [r4, #4]
	beq	L62
	cmp	r2, r3
	bcs	L63
	cmp	r0, #0
	beq	L65
	ldr	r10, [r4]                @ array near
	ldr	r12, [r0]                @ object near
	ldr	r10, [r10]               @ array klass
	ldr	r12, [r12]               @ object klass
	ldr	r10, [r10, #36]          @ array element type
	ldr	r1, [r12, #20]           @ subtype cache 1
	ldr	r12, [r12, #24]          @ subtype cache 2
	cmp	r10, r1
	cmpne	r10, r12
	bne	L64
L65:
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	@  store array element
	add	r4, r4, r2, lsl #2
	str	r0, [r4, #8]!
	@  oop_write_barrier
	ldr	r2, [r5, #1076]          @ bitvector_base 
	ldr	r3, [r5, #1080]          @ bit_selector 
	ldrb	r9, [r2, +r4, lsr #5]!
	mvn	r4, r4, lsr #2
	orr	r9, r9, r3, ror r4
	strb	r9, [r2]
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	ldr	r0, [r6], #4
	mov	pc, r12                  @ go to next bytecode
L62:
	@  Handle null pointer exception
	str	r4, [r6, #-4]!
	str	r2, [r6, #-4]!
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	b	interpreter_throw_NullPointerException
L63:
	@  Handle array index out of bounds exception
	str	r4, [r6, #-4]!
	str	r2, [r6, #-4]!
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	b	interpreter_throw_ArrayIndexOutOfBoundsException
L64:
	@  Handle slow type checking
	str	r4, [r6, #-4]!
	str	r2, [r6, #-4]!
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	.extern	array_store_type_check
	ldr	r3, =array_store_type_check
	@  save interpreter state
	str	lr, [fp, #-16]
	bl	interpreter_call_vm
	@  restore interpreter state
	ldr	r7, [fp, #-12]           @ restore locals
	ldr	r8, [fp, #-8]            @ restore cpool
	ldr	lr, [fp, #-16]           @ restore bcp
	@  Restore stack state from tos_on_stack
	ldr	r0, [r6], #4             @ cache topmost element of stack
	@  Entry: Count = 3
	ldr	r2, [r6], #4
	ldr	r4, [r6], #4
	b	L65
.LLfe48:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_bastore:

	.global	bc_impl_bastore_internal
bc_impl_bastore_internal:
	@  Entry: Count = 3
	ldr	r2, [r6], #4
	ldr	r4, [r6], #4
	cmp	r4, #0
	ldrne	r3, [r4, #4]
	beq	L66
	cmp	r2, r3
	bcs	L67
L69:
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	@  store array element
	add	r4, r4, r2
	strb	r0, [r4, #8]
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	ldr	r0, [r6], #4
	mov	pc, r12                  @ go to next bytecode
L66:
	@  Handle null pointer exception
	str	r4, [r6, #-4]!
	str	r2, [r6, #-4]!
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	b	interpreter_throw_NullPointerException
L67:
	@  Handle array index out of bounds exception
	str	r4, [r6, #-4]!
	str	r2, [r6, #-4]!
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	b	interpreter_throw_ArrayIndexOutOfBoundsException
L68:
.LLfe49:
	.pool
	@  End segment: unnamed


	@  castore is a duplicate of sastore
	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_sastore:

	.global	bc_impl_castore
bc_impl_castore:

	.global	bc_impl_sastore_internal
bc_impl_sastore_internal:

	.global	bc_impl_castore_internal
bc_impl_castore_internal:
	@  Entry: Count = 3
	ldr	r2, [r6], #4
	ldr	r4, [r6], #4
	cmp	r4, #0
	ldrne	r3, [r4, #4]
	beq	L70
	cmp	r2, r3
	bcs	L71
L73:
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	@  store array element
	add	r4, r4, r2, lsl #1
	strh	r0, [r4, #8]
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	ldr	r0, [r6], #4
	mov	pc, r12                  @ go to next bytecode
L70:
	@  Handle null pointer exception
	str	r4, [r6, #-4]!
	str	r2, [r6, #-4]!
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	b	interpreter_throw_NullPointerException
L71:
	@  Handle array index out of bounds exception
	str	r4, [r6, #-4]!
	str	r2, [r6, #-4]!
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	b	interpreter_throw_ArrayIndexOutOfBoundsException
L72:
.LLfe50:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_pop:

	.global	bc_impl_pop_internal
bc_impl_pop_internal:
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	@  Entry: Count = -1
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	ldr	r0, [r6], #4
	mov	pc, r12                  @ go to next bytecode
.LLfe51:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_pop2:

	.global	bc_impl_pop2_internal
bc_impl_pop2_internal:
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	@  Entry: Count = -2
	add	r6, r6, #4
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	ldr	r0, [r6], #4
	mov	pc, r12                  @ go to next bytecode
.LLfe52:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_dup:

	.global	bc_impl_dup_internal
bc_impl_dup_internal:
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	@  Entry: Count = 1
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	str	r0, [r6, #-4]!
	mov	pc, r12                  @ go to next bytecode
.LLfe53:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_dup_x1:

	.global	bc_impl_dup_x1_internal
bc_impl_dup_x1_internal:
	@  Entry: Count = 2
	ldr	r2, [r6], #4
	@  dup_x1 (...ba => ...aba)
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	str	r0, [r6, #-4]!
	str	r2, [r6, #-4]!
	mov	pc, r12                  @ go to next bytecode
.LLfe54:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_dup_x2:

	.global	bc_impl_dup_x2_internal
bc_impl_dup_x2_internal:
	@  Entry: Count = 3
	ldr	r2, [r6], #4
	ldr	r4, [r6], #4
	@  dup_x2 (...cba => ...acba
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	str	r0, [r6, #-4]!
	str	r4, [r6, #-4]!
	str	r2, [r6, #-4]!
	mov	pc, r12                  @ go to next bytecode
.LLfe55:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_dup2:

	.global	bc_impl_dup2_internal
bc_impl_dup2_internal:
	@  Entry: Count = 1
	ldr	r2, [r6]                 @ Grab second element without popping it
	@  dup2 (...ba => ...baba
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	str	r0, [r6, #-4]!
	str	r2, [r6, #-4]!
	mov	pc, r12                  @ go to next bytecode
.LLfe56:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_dup2_x1:

	.global	bc_impl_dup2_x1_internal
bc_impl_dup2_x1_internal:
	@  Entry: Count = 3
	ldr	r2, [r6], #4
	ldr	r4, [r6], #4
	@  dup2_x1 (...cba => ...bacba)
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	str	r2, [r6, #-4]!
	str	r0, [r6, #-4]!
	str	r4, [r6, #-4]!
	str	r2, [r6, #-4]!
	mov	pc, r12                  @ go to next bytecode
.LLfe57:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_dup2_x2:

	.global	bc_impl_dup2_x2_internal
bc_impl_dup2_x2_internal:
	@  Entry: Count = 4
	ldr	r2, [r6], #4
	ldr	r4, [r6], #4
	ldr	r10, [r6], #4
	@  dup_x2 (...dcba => ...badcba)
	str	r2, [r6, #-4]!
	str	r0, [r6, #-4]!
	str	r10, [r6, #-4]!
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	str	r4, [r6, #-4]!
	str	r2, [r6, #-4]!
	mov	pc, r12                  @ go to next bytecode
.LLfe58:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_swap:

	.global	bc_impl_swap_internal
bc_impl_swap_internal:
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	@  Entry: Count = 2
	ldr	r2, [r6], #4
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	str	r0, [r6, #-4]!
	mov	r0, r2
	mov	pc, r12                  @ go to next bytecode
.LLfe59:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_iadd:

	.global	bc_impl_iadd_internal
bc_impl_iadd_internal:
	@  Entry: Count = 2
	ldr	r2, [r6], #4
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	add	r0, r2, r0
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe60:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_ladd:

	.global	bc_impl_ladd_internal
bc_impl_ladd_internal:
	@  Entry: Count = 4
	ldr	r2, [r6], #4
	ldr	r4, [r6], #4
	ldr	r10, [r6], #4
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	adds	r0, r4, r0
	adc	r2, r10, r2
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	str	r2, [r6, #-4]!
	mov	pc, r12                  @ go to next bytecode
.LLfe61:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_fadd:

	.global	bc_impl_fadd_internal
bc_impl_fadd_internal:
	@  load one argument
	@  load other argument
	ldr	r1, [r6], #4
	mov	r9, lr
	bl	jvm_fadd
	mov	lr, r9
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	@  store result
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe62:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_dadd:

	.global	bc_impl_dadd_internal
bc_impl_dadd_internal:
	@  load one argument
	ldr	r1, [r6], #4
	@  load other argument
	ldr	r2, [r6], #4
	ldr	r3, [r6], #4
	mov	r9, lr
	bl	jvm_dadd
	mov	lr, r9
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	@  store result
	str	r1, [r6, #-4]!
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe63:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_isub:

	.global	bc_impl_isub_internal
bc_impl_isub_internal:
	@  Entry: Count = 2
	ldr	r2, [r6], #4
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	sub	r0, r2, r0
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe64:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_lsub:

	.global	bc_impl_lsub_internal
bc_impl_lsub_internal:
	@  Entry: Count = 4
	ldr	r2, [r6], #4
	ldr	r4, [r6], #4
	ldr	r10, [r6], #4
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	subs	r0, r4, r0
	sbc	r2, r10, r2
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	str	r2, [r6, #-4]!
	mov	pc, r12                  @ go to next bytecode
.LLfe65:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_fsub:

	.global	bc_impl_fsub_internal
bc_impl_fsub_internal:
	@  load one argument
	mov	r1, r0
	@  load other argument
	ldr	r0, [r6], #4
	mov	r9, lr
	bl	jvm_fsub
	mov	lr, r9
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	@  store result
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe66:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_dsub:

	.global	bc_impl_dsub_internal
bc_impl_dsub_internal:
	@  load one argument
	mov	r2, r0
	ldr	r3, [r6], #4
	@  load other argument
	ldr	r0, [r6], #4
	ldr	r1, [r6], #4
	mov	r9, lr
	bl	jvm_dsub
	mov	lr, r9
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	@  store result
	str	r1, [r6, #-4]!
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe67:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_imul:

	.global	bc_impl_imul_internal
bc_impl_imul_internal:
	@  Entry: Count = 2
	ldr	r2, [r6], #4
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	mul	r0, r2, r0
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe68:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_lmul:

	.global	bc_impl_lmul_internal
bc_impl_lmul_internal:
	@  Entry: Count = 4
	ldr	r2, [r6], #4
	ldr	r4, [r6], #4
	ldr	r10, [r6], #4
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	umull	r3, r9, r0, r4
	mla	r9, r0, r10, r9
	mla	r9, r2, r4, r9
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	str	r9, [r6, #-4]!
	mov	r0, r3
	mov	pc, r12                  @ go to next bytecode
.LLfe69:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_fmul:

	.global	bc_impl_fmul_internal
bc_impl_fmul_internal:
	@  load one argument
	@  load other argument
	ldr	r1, [r6], #4
	mov	r9, lr
	bl	jvm_fmul
	mov	lr, r9
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	@  store result
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe70:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_dmul:

	.global	bc_impl_dmul_internal
bc_impl_dmul_internal:
	@  load one argument
	ldr	r1, [r6], #4
	@  load other argument
	ldr	r2, [r6], #4
	ldr	r3, [r6], #4
	mov	r9, lr
	bl	jvm_dmul
	mov	lr, r9
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	@  store result
	str	r1, [r6, #-4]!
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe71:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_idiv:

	.global	bc_impl_idiv_internal
bc_impl_idiv_internal:
	mov	r4, lr
	bl	interpreter_idiv_irem

	.global	called_from_bc_idiv
called_from_bc_idiv:
	mov	lr, r4
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	mov	r0, r1
	@  store result
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe72:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_ldiv:

	.global	bc_impl_ldiv_internal
bc_impl_ldiv_internal:
	@  load one argument
	mov	r2, r0
	ldr	r3, [r6], #4
	@  load other argument
	ldr	r0, [r6], #4
	ldr	r1, [r6], #4
	orrs	r4, r2, r3
	beq	long_divide_by_zero
	mov	r9, lr
	bl	jvm_ldiv
	mov	lr, r9
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	@  store result
	str	r1, [r6, #-4]!
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
long_divide_by_zero:
	str	r1, [r6, #-4]!
	str	r0, [r6, #-4]!
	str	r3, [r6, #-4]!
	mov	r0, r2
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	.extern	division_by_zero_exception
	ldr	r3, =division_by_zero_exception
	@  save interpreter state
	str	lr, [fp, #-16]
	bl	interpreter_call_vm
.LLfe73:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_fdiv:

	.global	bc_impl_fdiv_internal
bc_impl_fdiv_internal:
	@  load one argument
	mov	r1, r0
	@  load other argument
	ldr	r0, [r6], #4
	mov	r9, lr
	bl	jvm_fdiv
	mov	lr, r9
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	@  store result
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe74:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_ddiv:

	.global	bc_impl_ddiv_internal
bc_impl_ddiv_internal:
	@  load one argument
	mov	r2, r0
	ldr	r3, [r6], #4
	@  load other argument
	ldr	r0, [r6], #4
	ldr	r1, [r6], #4
	mov	r9, lr
	bl	jvm_ddiv
	mov	lr, r9
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	@  store result
	str	r1, [r6, #-4]!
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe75:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_irem:

	.global	bc_impl_irem_internal
bc_impl_irem_internal:
	mov	r4, lr
	bl	interpreter_idiv_irem

	.global	called_from_bc_irem
called_from_bc_irem:
	mov	lr, r4
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	@  store result
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe76:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_lrem:

	.global	bc_impl_lrem_internal
bc_impl_lrem_internal:
	@  load one argument
	mov	r2, r0
	ldr	r3, [r6], #4
	@  load other argument
	ldr	r0, [r6], #4
	ldr	r1, [r6], #4
	orrs	r4, r2, r3
	beq	long_divide_by_zero
	mov	r9, lr
	bl	jvm_lrem
	mov	lr, r9
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	@  store result
	str	r1, [r6, #-4]!
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe77:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_frem:

	.global	bc_impl_frem_internal
bc_impl_frem_internal:
	@  load one argument
	mov	r1, r0
	@  load other argument
	ldr	r0, [r6], #4
	mov	r9, lr
	bl	jvm_frem
	mov	lr, r9
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	@  store result
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe78:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_drem:

	.global	bc_impl_drem_internal
bc_impl_drem_internal:
	@  load one argument
	mov	r2, r0
	ldr	r3, [r6], #4
	@  load other argument
	ldr	r0, [r6], #4
	ldr	r1, [r6], #4
	mov	r9, lr
	bl	jvm_drem
	mov	lr, r9
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	@  store result
	str	r1, [r6, #-4]!
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe79:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_ineg:

	.global	bc_impl_ineg_internal
bc_impl_ineg_internal:
	@  Entry: Count = 1
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	rsb	r0, r0, #0               @ negate
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe80:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_lneg:

	.global	bc_impl_lneg_internal
bc_impl_lneg_internal:
	@  Entry: Count = 2
	ldr	r2, [r6], #4
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	rsbs	r0, r0, #0               @ negate
	rsc	r2, r2, #0
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	str	r2, [r6, #-4]!
	mov	pc, r12                  @ go to next bytecode
.LLfe81:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_fneg:

	.global	bc_impl_fneg_internal
bc_impl_fneg_internal:
	@  Entry: Count = 1
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	eor	r0, r0, #0x80000000      @ flip sign bit
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe82:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_dneg:

	.global	bc_impl_dneg_internal
bc_impl_dneg_internal:
	ldr	r2, [r6]                 @ get word containing sign bit
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	eor	r2, r2, #0x80000000      @ flip sign bit
	str	r2, [r6]                 @ and put it back
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe83:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_ishl:

	.global	bc_impl_ishl_internal
bc_impl_ishl_internal:
	@  Entry: Count = 2
	ldr	r2, [r6], #4
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	and	r0, r0, #31
	mov	r0, r2, lsl r0
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe84:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_lshl:

	.global	bc_impl_lshl_internal
bc_impl_lshl_internal:
	@  Entry: Count = 3
	ldr	r2, [r6], #4
	ldr	r4, [r6], #4
	@  Normalize shift amount
	ands	r0, r0, #63
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	moveq	r9, r4
	moveq	r3, r2
	beq	L74
	@  Get long argument from the stack
	@  Calculate 32-shift
	rsbs	r1, r0, #32
	@  condition code le means that shift is >= 32
	suble	r0, r0, #32
	movle	r9, r2, lsl r0
	movle	r3, #0
	@  condition code gt means that shift is < 32
	movgt	r9, r4, lsl r0
	orrgt	r9, r9, r2, lsr r1
	movgt	r3, r2, lsl r0
L74:
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	str	r9, [r6, #-4]!
	mov	r0, r3
	mov	pc, r12                  @ go to next bytecode
.LLfe85:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_ishr:

	.global	bc_impl_ishr_internal
bc_impl_ishr_internal:
	@  Entry: Count = 2
	ldr	r2, [r6], #4
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	and	r0, r0, #31
	mov	r0, r2, asr r0
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe86:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_lshr:

	.global	bc_impl_lshr_internal
bc_impl_lshr_internal:
	@  Entry: Count = 3
	ldr	r2, [r6], #4
	ldr	r4, [r6], #4
	@  Normalize shift amount
	ands	r0, r0, #63
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	moveq	r9, r4
	moveq	r3, r2
	beq	L75
	@  Calculate 32-shift
	rsbs	r1, r0, #32
	@  condition code le means that shift is >= 32
	suble	r0, r0, #32
	movle	r3, r4, asr r0
	movle	r9, r4, asr #31
	@  condition code gt means that shift is < 32
	movgt	r3, r2, lsr r0
	orrgt	r3, r3, r4, lsl r1
	movgt	r9, r4, asr r0
L75:
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	str	r9, [r6, #-4]!
	mov	r0, r3
	mov	pc, r12                  @ go to next bytecode
.LLfe87:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_iushr:

	.global	bc_impl_iushr_internal
bc_impl_iushr_internal:
	@  Entry: Count = 2
	ldr	r2, [r6], #4
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	and	r0, r0, #31
	mov	r0, r2, lsr r0
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe88:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_lushr:

	.global	bc_impl_lushr_internal
bc_impl_lushr_internal:
	@  Entry: Count = 3
	ldr	r2, [r6], #4
	ldr	r4, [r6], #4
	@  Normalize shift amount
	ands	r0, r0, #63
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	moveq	r9, r4
	moveq	r3, r2
	beq	L76
	@  Calculate 32-shift
	rsbs	r1, r0, #32
	@  condition code le means that shift is >= 32
	suble	r0, r0, #32
	movle	r3, r4, lsr r0
	movle	r9, #0
	@  condition code gt means that shift is < 32
	movgt	r3, r2, lsr r0
	orrgt	r3, r3, r4, lsl r1
	movgt	r9, r4, lsr r0
L76:
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	str	r9, [r6, #-4]!
	mov	r0, r3
	mov	pc, r12                  @ go to next bytecode
.LLfe89:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_iand:

	.global	bc_impl_iand_internal
bc_impl_iand_internal:
	@  Entry: Count = 2
	ldr	r2, [r6], #4
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	and	r0, r2, r0
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe90:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_land:

	.global	bc_impl_land_internal
bc_impl_land_internal:
	@  Entry: Count = 4
	ldr	r2, [r6], #4
	ldr	r4, [r6], #4
	ldr	r10, [r6], #4
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	and	r0, r4, r0
	and	r2, r10, r2
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	str	r2, [r6, #-4]!
	mov	pc, r12                  @ go to next bytecode
.LLfe91:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_ior:

	.global	bc_impl_ior_internal
bc_impl_ior_internal:
	@  Entry: Count = 2
	ldr	r2, [r6], #4
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	orr	r0, r2, r0
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe92:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_lor:

	.global	bc_impl_lor_internal
bc_impl_lor_internal:
	@  Entry: Count = 4
	ldr	r2, [r6], #4
	ldr	r4, [r6], #4
	ldr	r10, [r6], #4
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	orr	r0, r4, r0
	orr	r2, r10, r2
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	str	r2, [r6, #-4]!
	mov	pc, r12                  @ go to next bytecode
.LLfe93:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_ixor:

	.global	bc_impl_ixor_internal
bc_impl_ixor_internal:
	@  Entry: Count = 2
	ldr	r2, [r6], #4
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	eor	r0, r2, r0
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe94:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_lxor:

	.global	bc_impl_lxor_internal
bc_impl_lxor_internal:
	@  Entry: Count = 4
	ldr	r2, [r6], #4
	ldr	r4, [r6], #4
	ldr	r10, [r6], #4
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	eor	r0, r4, r0
	eor	r2, r10, r2
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	str	r2, [r6, #-4]!
	mov	pc, r12                  @ go to next bytecode
.LLfe95:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_wide_iinc:

	.global	bc_impl_wide_iinc_internal
bc_impl_wide_iinc_internal:
	ldrb	r2, [lr, #2]
	ldrb	r3, [lr, #3]
	ldrsb	r4, [lr, #4]
	ldrb	r9, [lr, #5]
	add	r2, r3, r2, lsl #8       @ register index
	ldr	r3, [r7, -r2, lsl #2]    @ get local
	ldrb	r12, [lr, #6]!           @ prefetch next bytecode
	add	r3, r3, r4, lsl #8       @ increment local
	add	r3, r3, r9
	str	r3, [r7, -r2, lsl #2]    @ store local
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe96:
	@  Data containing entry for wide bytecode
	.long	bc_impl_wide_iinc
bc_impl_iinc:

	.global	bc_impl_iinc_internal
bc_impl_iinc_internal:
	ldrb	r2, [lr, #1]             @ register index
	ldrsb	r4, [lr, #2]             @ increment
	ldr	r3, [r7, -r2, lsl #2]    @ get local
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	add	r3, r3, r4               @ increment local
	str	r3, [r7, -r2, lsl #2]    @ store local
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe97:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_i2l:

	.global	bc_impl_i2l_internal
bc_impl_i2l_internal:
	@  Entry: Count = 1
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	mov	r4, r0, asr #31
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	str	r4, [r6, #-4]!
	mov	pc, r12                  @ go to next bytecode
.LLfe98:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_i2f:

	.global	bc_impl_i2f_internal
bc_impl_i2f_internal:
	@  load one argument
	mov	r9, lr
	bl	jvm_i2f
	mov	lr, r9
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	@  store result
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe99:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_i2d:

	.global	bc_impl_i2d_internal
bc_impl_i2d_internal:
	@  load one argument
	mov	r9, lr
	bl	jvm_i2d
	mov	lr, r9
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	@  store result
	str	r1, [r6, #-4]!
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe100:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_l2i:

	.global	bc_impl_l2i_internal
bc_impl_l2i_internal:
	@  Entry: Count = 1
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	add	r6, r6, #4
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe101:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_l2f:

	.global	bc_impl_l2f_internal
bc_impl_l2f_internal:
	@  load one argument
	ldr	r1, [r6], #4
	mov	r9, lr
	bl	jvm_l2f
	mov	lr, r9
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	@  store result
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe102:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_l2d:

	.global	bc_impl_l2d_internal
bc_impl_l2d_internal:
	@  load one argument
	ldr	r1, [r6], #4
	mov	r9, lr
	bl	jvm_l2d
	mov	lr, r9
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	@  store result
	str	r1, [r6, #-4]!
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe103:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_f2i:

	.global	bc_impl_f2i_internal
bc_impl_f2i_internal:
	@  load one argument
	mov	r9, lr
	bl	jvm_f2i
	mov	lr, r9
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	@  store result
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe104:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_f2l:

	.global	bc_impl_f2l_internal
bc_impl_f2l_internal:
	@  load one argument
	mov	r9, lr
	bl	jvm_f2l
	mov	lr, r9
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	@  store result
	str	r1, [r6, #-4]!
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe105:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_f2d:

	.global	bc_impl_f2d_internal
bc_impl_f2d_internal:
	@  load one argument
	mov	r9, lr
	bl	jvm_f2d
	mov	lr, r9
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	@  store result
	str	r1, [r6, #-4]!
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe106:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_d2i:

	.global	bc_impl_d2i_internal
bc_impl_d2i_internal:
	@  load one argument
	ldr	r1, [r6], #4
	mov	r9, lr
	bl	jvm_d2i
	mov	lr, r9
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	@  store result
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe107:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_d2l:

	.global	bc_impl_d2l_internal
bc_impl_d2l_internal:
	@  load one argument
	ldr	r1, [r6], #4
	mov	r9, lr
	bl	jvm_d2l
	mov	lr, r9
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	@  store result
	str	r1, [r6, #-4]!
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe108:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_d2f:

	.global	bc_impl_d2f_internal
bc_impl_d2f_internal:
	@  load one argument
	ldr	r1, [r6], #4
	mov	r9, lr
	bl	jvm_d2f
	mov	lr, r9
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	@  store result
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe109:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_i2b:

	.global	bc_impl_i2b_internal
bc_impl_i2b_internal:
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	@  Entry: Count = 1
	mov	r0, r0, lsl #24
	mov	r0, r0, asr #24
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe110:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_i2c:

	.global	bc_impl_i2c_internal
bc_impl_i2c_internal:
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	@  Entry: Count = 1
	mov	r0, r0, lsl #16
	mov	r0, r0, lsr #16
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe111:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_i2s:

	.global	bc_impl_i2s_internal
bc_impl_i2s_internal:
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	@  Entry: Count = 1
	mov	r0, r0, lsl #16
	mov	r0, r0, asr #16
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe112:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_lcmp:

	.global	bc_impl_lcmp_internal
bc_impl_lcmp_internal:
	@  Entry: Count = 4
	ldr	r2, [r6], #4
	ldr	r4, [r6], #4
	ldr	r10, [r6], #4
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	subs	r4, r4, r0               @ subtract arguments
	sbcs	r10, r10, r2
	mvnlt	r0, #0                   @ handle *only* for < and >=
	movge	r0, #1
	orrs	r4, r4, r10              @ correction for ==
	moveq	r0, #0
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe113:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_fcmpl:

	.global	bc_impl_fcmpl_internal
bc_impl_fcmpl_internal:
	@  load one argument
	mov	r1, r0
	@  load other argument
	ldr	r0, [r6], #4
	mov	r9, lr
	bl	jvm_fcmpl
	mov	lr, r9
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	@  store result
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe114:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_fcmpg:

	.global	bc_impl_fcmpg_internal
bc_impl_fcmpg_internal:
	@  load one argument
	mov	r1, r0
	@  load other argument
	ldr	r0, [r6], #4
	mov	r9, lr
	bl	jvm_fcmpg
	mov	lr, r9
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	@  store result
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe115:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_dcmpl:

	.global	bc_impl_dcmpl_internal
bc_impl_dcmpl_internal:
	@  load one argument
	mov	r2, r0
	ldr	r3, [r6], #4
	@  load other argument
	ldr	r0, [r6], #4
	ldr	r1, [r6], #4
	mov	r9, lr
	bl	jvm_dcmpl
	mov	lr, r9
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	@  store result
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe116:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_dcmpg:

	.global	bc_impl_dcmpg_internal
bc_impl_dcmpg_internal:
	@  load one argument
	mov	r2, r0
	ldr	r3, [r6], #4
	@  load other argument
	ldr	r0, [r6], #4
	ldr	r1, [r6], #4
	mov	r9, lr
	bl	jvm_dcmpg
	mov	lr, r9
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	@  store result
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe117:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_ifeq:

	.global	bc_impl_ifnull
bc_impl_ifnull:

	.global	bc_impl_ifeq_internal
bc_impl_ifeq_internal:

	.global	bc_impl_ifnull_internal
bc_impl_ifnull_internal:
	ldr	r2, [r5, #1032]          @ rt_timer_ticks  @ Timer tick?
	cmp	r2, #0
	bne	interpreter_timer_tick
	@  Entry: Count = 1
	cmp	r0, #0
	bne	L77
	ldrsb	r3, [lr, #1]
	ldrb	r2, [lr, #2]
	add	lr, lr, r3, lsl #8
	ldrb	r12, [lr, +r2]!          @ prefetch next bytecode
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	ldr	r0, [r6], #4
	mov	pc, r12                  @ go to next bytecode
L77:
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	ldr	r0, [r6], #4
	mov	pc, r12                  @ go to next bytecode
.LLfe118:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_ifne:

	.global	bc_impl_ifnonnull
bc_impl_ifnonnull:

	.global	bc_impl_ifne_internal
bc_impl_ifne_internal:

	.global	bc_impl_ifnonnull_internal
bc_impl_ifnonnull_internal:
	ldr	r2, [r5, #1032]          @ rt_timer_ticks  @ Timer tick?
	cmp	r2, #0
	bne	interpreter_timer_tick
	@  Entry: Count = 1
	cmp	r0, #0
	beq	L78
	ldrsb	r3, [lr, #1]
	ldrb	r2, [lr, #2]
	add	lr, lr, r3, lsl #8
	ldrb	r12, [lr, +r2]!          @ prefetch next bytecode
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	ldr	r0, [r6], #4
	mov	pc, r12                  @ go to next bytecode
L78:
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	ldr	r0, [r6], #4
	mov	pc, r12                  @ go to next bytecode
.LLfe119:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_iflt:

	.global	bc_impl_iflt_internal
bc_impl_iflt_internal:
	ldr	r2, [r5, #1032]          @ rt_timer_ticks  @ Timer tick?
	cmp	r2, #0
	bne	interpreter_timer_tick
	@  Entry: Count = 1
	cmp	r0, #0
	bge	L79
	ldrsb	r3, [lr, #1]
	ldrb	r2, [lr, #2]
	add	lr, lr, r3, lsl #8
	ldrb	r12, [lr, +r2]!          @ prefetch next bytecode
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	ldr	r0, [r6], #4
	mov	pc, r12                  @ go to next bytecode
L79:
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	ldr	r0, [r6], #4
	mov	pc, r12                  @ go to next bytecode
.LLfe120:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_ifge:

	.global	bc_impl_ifge_internal
bc_impl_ifge_internal:
	ldr	r2, [r5, #1032]          @ rt_timer_ticks  @ Timer tick?
	cmp	r2, #0
	bne	interpreter_timer_tick
	@  Entry: Count = 1
	cmp	r0, #0
	blt	L80
	ldrsb	r3, [lr, #1]
	ldrb	r2, [lr, #2]
	add	lr, lr, r3, lsl #8
	ldrb	r12, [lr, +r2]!          @ prefetch next bytecode
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	ldr	r0, [r6], #4
	mov	pc, r12                  @ go to next bytecode
L80:
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	ldr	r0, [r6], #4
	mov	pc, r12                  @ go to next bytecode
.LLfe121:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_ifgt:

	.global	bc_impl_ifgt_internal
bc_impl_ifgt_internal:
	ldr	r2, [r5, #1032]          @ rt_timer_ticks  @ Timer tick?
	cmp	r2, #0
	bne	interpreter_timer_tick
	@  Entry: Count = 1
	cmp	r0, #0
	ble	L81
	ldrsb	r3, [lr, #1]
	ldrb	r2, [lr, #2]
	add	lr, lr, r3, lsl #8
	ldrb	r12, [lr, +r2]!          @ prefetch next bytecode
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	ldr	r0, [r6], #4
	mov	pc, r12                  @ go to next bytecode
L81:
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	ldr	r0, [r6], #4
	mov	pc, r12                  @ go to next bytecode
.LLfe122:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_ifle:

	.global	bc_impl_ifle_internal
bc_impl_ifle_internal:
	ldr	r2, [r5, #1032]          @ rt_timer_ticks  @ Timer tick?
	cmp	r2, #0
	bne	interpreter_timer_tick
	@  Entry: Count = 1
	cmp	r0, #0
	bgt	L82
	ldrsb	r3, [lr, #1]
	ldrb	r2, [lr, #2]
	add	lr, lr, r3, lsl #8
	ldrb	r12, [lr, +r2]!          @ prefetch next bytecode
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	ldr	r0, [r6], #4
	mov	pc, r12                  @ go to next bytecode
L82:
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	ldr	r0, [r6], #4
	mov	pc, r12                  @ go to next bytecode
.LLfe123:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_if_icmpeq:

	.global	bc_impl_if_acmpeq
bc_impl_if_acmpeq:

	.global	bc_impl_if_icmpeq_internal
bc_impl_if_icmpeq_internal:

	.global	bc_impl_if_acmpeq_internal
bc_impl_if_acmpeq_internal:
	ldr	r2, [r5, #1032]          @ rt_timer_ticks  @ Timer tick?
	cmp	r2, #0
	bne	interpreter_timer_tick
	@  Entry: Count = 2
	ldr	r2, [r6], #4
	cmp	r2, r0
	bne	L83
	ldrsb	r3, [lr, #1]
	ldrb	r2, [lr, #2]
	add	lr, lr, r3, lsl #8
	ldrb	r12, [lr, +r2]!          @ prefetch next bytecode
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	ldr	r0, [r6], #4
	mov	pc, r12                  @ go to next bytecode
L83:
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	ldr	r0, [r6], #4
	mov	pc, r12                  @ go to next bytecode
.LLfe124:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_if_icmpne:

	.global	bc_impl_if_acmpne
bc_impl_if_acmpne:

	.global	bc_impl_if_icmpne_internal
bc_impl_if_icmpne_internal:

	.global	bc_impl_if_acmpne_internal
bc_impl_if_acmpne_internal:
	ldr	r2, [r5, #1032]          @ rt_timer_ticks  @ Timer tick?
	cmp	r2, #0
	bne	interpreter_timer_tick
	@  Entry: Count = 2
	ldr	r2, [r6], #4
	cmp	r2, r0
	beq	L84
	ldrsb	r3, [lr, #1]
	ldrb	r2, [lr, #2]
	add	lr, lr, r3, lsl #8
	ldrb	r12, [lr, +r2]!          @ prefetch next bytecode
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	ldr	r0, [r6], #4
	mov	pc, r12                  @ go to next bytecode
L84:
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	ldr	r0, [r6], #4
	mov	pc, r12                  @ go to next bytecode
.LLfe125:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_if_icmplt:

	.global	bc_impl_if_icmplt_internal
bc_impl_if_icmplt_internal:
	ldr	r2, [r5, #1032]          @ rt_timer_ticks  @ Timer tick?
	cmp	r2, #0
	bne	interpreter_timer_tick
	@  Entry: Count = 2
	ldr	r2, [r6], #4
	cmp	r2, r0
	bge	L85
	ldrsb	r3, [lr, #1]
	ldrb	r2, [lr, #2]
	add	lr, lr, r3, lsl #8
	ldrb	r12, [lr, +r2]!          @ prefetch next bytecode
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	ldr	r0, [r6], #4
	mov	pc, r12                  @ go to next bytecode
L85:
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	ldr	r0, [r6], #4
	mov	pc, r12                  @ go to next bytecode
.LLfe126:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_if_icmpge:

	.global	bc_impl_if_icmpge_internal
bc_impl_if_icmpge_internal:
	ldr	r2, [r5, #1032]          @ rt_timer_ticks  @ Timer tick?
	cmp	r2, #0
	bne	interpreter_timer_tick
	@  Entry: Count = 2
	ldr	r2, [r6], #4
	cmp	r2, r0
	blt	L86
	ldrsb	r3, [lr, #1]
	ldrb	r2, [lr, #2]
	add	lr, lr, r3, lsl #8
	ldrb	r12, [lr, +r2]!          @ prefetch next bytecode
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	ldr	r0, [r6], #4
	mov	pc, r12                  @ go to next bytecode
L86:
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	ldr	r0, [r6], #4
	mov	pc, r12                  @ go to next bytecode
.LLfe127:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_if_icmpgt:

	.global	bc_impl_if_icmpgt_internal
bc_impl_if_icmpgt_internal:
	ldr	r2, [r5, #1032]          @ rt_timer_ticks  @ Timer tick?
	cmp	r2, #0
	bne	interpreter_timer_tick
	@  Entry: Count = 2
	ldr	r2, [r6], #4
	cmp	r2, r0
	ble	L87
	ldrsb	r3, [lr, #1]
	ldrb	r2, [lr, #2]
	add	lr, lr, r3, lsl #8
	ldrb	r12, [lr, +r2]!          @ prefetch next bytecode
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	ldr	r0, [r6], #4
	mov	pc, r12                  @ go to next bytecode
L87:
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	ldr	r0, [r6], #4
	mov	pc, r12                  @ go to next bytecode
.LLfe128:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_if_icmple:

	.global	bc_impl_if_icmple_internal
bc_impl_if_icmple_internal:
	ldr	r2, [r5, #1032]          @ rt_timer_ticks  @ Timer tick?
	cmp	r2, #0
	bne	interpreter_timer_tick
	@  Entry: Count = 2
	ldr	r2, [r6], #4
	cmp	r2, r0
	bgt	L88
	ldrsb	r3, [lr, #1]
	ldrb	r2, [lr, #2]
	add	lr, lr, r3, lsl #8
	ldrb	r12, [lr, +r2]!          @ prefetch next bytecode
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	ldr	r0, [r6], #4
	mov	pc, r12                  @ go to next bytecode
L88:
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	ldr	r0, [r6], #4
	mov	pc, r12                  @ go to next bytecode
.LLfe129:
	.pool
	@  End segment: unnamed


	@  if_acmpeq is a duplicate of if_icmpeq
	@  if_acmpne is a duplicate of if_icmpne
	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_goto:

	.global	bc_impl_goto_internal
bc_impl_goto_internal:
	ldr	r2, [r5, #1032]          @ rt_timer_ticks  @ Timer tick?
	cmp	r2, #0
	bne	interpreter_timer_tick
	@  Entry: Count = 0
	str	r0, [r6, #-4]!
	ldrsb	r3, [lr, #1]
	ldrb	r2, [lr, #2]
	add	lr, lr, r3, lsl #8
	ldrb	r12, [lr, +r2]!          @ prefetch next bytecode
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	ldr	r0, [r6], #4
	mov	pc, r12                  @ go to next bytecode
.LLfe130:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_tableswitch:

	.global	bc_impl_tableswitch_internal
bc_impl_tableswitch_internal:
	ldr	r2, [r5, #1032]          @ rt_timer_ticks  @ Timer tick?
	cmp	r2, #0
	bne	interpreter_timer_tick
	@  Entry: Count = 1
	@  Have tos_tag point to lower limit
	add	r1, lr, #8
	bic	r1, r1, #3
	@  tmp2 gets the lower limit
	@  tmp3 gets the upper limit
	@  tos_tag is updated to point to the start of the offsets
	ldmia	r1!, {r4, r9}
	cmp	r9, r0
	subges	r0, r0, r4
	@  Offset of branch target (swapped?) if value is in the table
	ldrge	r3, [r1, +r0, lsl #2]
	@  Offset of branch target (swapped?) if value is outside of table
	ldrlt	r3, [r1, #-12]
	ldrb	r12, [lr, +r3]!          @ prefetch next bytecode
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	ldr	r0, [r6], #4
	mov	pc, r12                  @ go to next bytecode
.LLfe131:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_lookupswitch:

	.global	bc_impl_lookupswitch_internal
bc_impl_lookupswitch_internal:
	ldr	r2, [r5, #1032]          @ rt_timer_ticks  @ Timer tick?
	cmp	r2, #0
	bne	interpreter_timer_tick
	@  Entry: Count = 1
	@  compute aligned table ptr
	bic	r2, lr, #3
	@  get number of table entries + 1
	ldr	r3, [r2, #8]
	add	r3, r3, #1
	@  set table ptr to default entry
	add	r2, r2, #4
	@  search the table
L89:
	ldr	r4, [r2, #8]!
	subs	r3, r3, #1
	cmpne	r0, r4
	bne	L89
	@  reset table ptr if not found
	cmp	r3, #0
	biceq	r2, lr, #3
	@  get table entry or default offset
	ldr	r3, [r2, #4]
	@  get bytecode at destination
	ldrb	r12, [lr, +r3]!
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	ldr	r0, [r6], #4
	mov	pc, r12                  @ go to next bytecode
.LLfe132:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_ireturn:

	.global	bc_impl_freturn
bc_impl_freturn:

	.global	bc_impl_areturn
bc_impl_areturn:

	.global	bc_impl_ireturn_internal
bc_impl_ireturn_internal:

	.global	bc_impl_freturn_internal
bc_impl_freturn_internal:

	.global	bc_impl_areturn_internal
bc_impl_areturn_internal:
	@  unlock_activation
	@  See if there are any locks on this frame
	ldr	r2, [fp, #4]             @ address of stack bottom pointer
	ldr	r4, [fp, #-4]            @ current method
	sub	r3, fp, #20              @ address of stack bottom pointer if no locks
	cmp	r2, r3
	beq	L90
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	@  save interpreter state
	str	lr, [fp, #-16]
	@  Is this a synchronized method?
	ldrh	r4, [r4, #24]            @ access flags
	tst	r4, #32                  @ ACC_SYNCHRONIZED
	beq	L92
	bl	shared_unlock_synchronized_method
	ldr	r2, [fp, #4]
	b	L92
L91:
	.extern	illegal_monitor_state_exception
	ldr	r3, =illegal_monitor_state_exception
	bl	interpreter_call_vm
L92:
	@  Check that all stack locks have been unlocked.
	ldr	r3, [r2, #24]!
	sub	r4, fp, #24
L93:
	cmp	r3, #0
	bne	L91
	cmp	r2, r4
	ldrne	r3, [r2, #28]!
	bne	L93
	@  Restore stack state from tos_on_stack
	ldr	r0, [r6], #4             @ cache topmost element of stack
	ldr	r7, [fp, #-12]
L90:
	@  load one argument
	ldr	lr, [fp, #-20]
	ldr	fp, [fp]
	add	r6, r7, #4
	mov	r2, #1
	mov	pc, lr
.LLfe133:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_lreturn:

	.global	bc_impl_dreturn
bc_impl_dreturn:

	.global	bc_impl_lreturn_internal
bc_impl_lreturn_internal:

	.global	bc_impl_dreturn_internal
bc_impl_dreturn_internal:
	@  unlock_activation
	@  See if there are any locks on this frame
	ldr	r2, [fp, #4]             @ address of stack bottom pointer
	ldr	r4, [fp, #-4]            @ current method
	sub	r3, fp, #20              @ address of stack bottom pointer if no locks
	cmp	r2, r3
	beq	L94
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	@  save interpreter state
	str	lr, [fp, #-16]
	@  Is this a synchronized method?
	ldrh	r4, [r4, #24]            @ access flags
	tst	r4, #32                  @ ACC_SYNCHRONIZED
	beq	L96
	bl	shared_unlock_synchronized_method
	ldr	r2, [fp, #4]
	b	L96
L95:
	.extern	illegal_monitor_state_exception
	ldr	r3, =illegal_monitor_state_exception
	bl	interpreter_call_vm
L96:
	@  Check that all stack locks have been unlocked.
	ldr	r3, [r2, #24]!
	sub	r4, fp, #24
L97:
	cmp	r3, #0
	bne	L95
	cmp	r2, r4
	ldrne	r3, [r2, #28]!
	bne	L97
	@  Restore stack state from tos_on_stack
	ldr	r0, [r6], #4             @ cache topmost element of stack
	ldr	r7, [fp, #-12]
L94:
	@  load one argument
	ldr	r1, [r6], #4
	ldr	lr, [fp, #-20]
	ldr	fp, [fp]
	add	r6, r7, #4
	mov	r2, #2
	mov	pc, lr
.LLfe134:
	.pool
	@  End segment: unnamed


	@  freturn is a duplicate of ireturn
	@  dreturn is a duplicate of lreturn
	@  areturn is a duplicate of ireturn
	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_return:

	.global	bc_impl_return_internal
bc_impl_return_internal:
	@  unlock_activation
	@  See if there are any locks on this frame
	ldr	r2, [fp, #4]             @ address of stack bottom pointer
	ldr	r4, [fp, #-4]            @ current method
	sub	r3, fp, #20              @ address of stack bottom pointer if no locks
	cmp	r2, r3
	beq	L98
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	@  save interpreter state
	str	lr, [fp, #-16]
	@  Is this a synchronized method?
	ldrh	r4, [r4, #24]            @ access flags
	tst	r4, #32                  @ ACC_SYNCHRONIZED
	beq	L100
	bl	shared_unlock_synchronized_method
	ldr	r2, [fp, #4]
	b	L100
L99:
	.extern	illegal_monitor_state_exception
	ldr	r3, =illegal_monitor_state_exception
	bl	interpreter_call_vm
L100:
	@  Check that all stack locks have been unlocked.
	ldr	r3, [r2, #24]!
	sub	r4, fp, #24
L101:
	cmp	r3, #0
	bne	L99
	cmp	r2, r4
	ldrne	r3, [r2, #28]!
	bne	L101
	@  Restore stack state from tos_on_stack
	ldr	r0, [r6], #4             @ cache topmost element of stack
	ldr	r7, [fp, #-12]
L98:
	ldr	lr, [fp, #-20]
	ldr	fp, [fp]
	add	r6, r7, #4
	mov	r2, #0
	mov	pc, lr
.LLfe135:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_getstatic:

	.global	bc_impl_getstatic_internal
bc_impl_getstatic_internal:
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	.extern	getstatic
	ldr	r3, =getstatic
	@  save interpreter state
	str	lr, [fp, #-16]
	bl	interpreter_call_vm
	@  restore interpreter state
	ldr	r7, [fp, #-12]           @ restore locals
	ldr	r8, [fp, #-8]            @ restore cpool
	ldr	lr, [fp, #-16]           @ restore bcp
	ldr	r12, [r5, +r0, lsl #2]
	@  Restore stack state from tos_on_stack
	ldr	r0, [r6], #4             @ cache topmost element of stack
	mov	pc, r12
.LLfe136:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_putstatic:

	.global	bc_impl_putstatic_internal
bc_impl_putstatic_internal:
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	.extern	putstatic
	ldr	r3, =putstatic
	@  save interpreter state
	str	lr, [fp, #-16]
	bl	interpreter_call_vm
	@  restore interpreter state
	ldr	r7, [fp, #-12]           @ restore locals
	ldr	r8, [fp, #-8]            @ restore cpool
	ldr	lr, [fp, #-16]           @ restore bcp
	ldr	r12, [r5, +r0, lsl #2]
	@  Restore stack state from tos_on_stack
	ldr	r0, [r6], #4             @ cache topmost element of stack
	mov	pc, r12
.LLfe137:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_getfield:

	.global	bc_impl_getfield_internal
bc_impl_getfield_internal:
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	.extern	getfield
	ldr	r3, =getfield
	@  save interpreter state
	str	lr, [fp, #-16]
	bl	interpreter_call_vm
	@  restore interpreter state
	ldr	r7, [fp, #-12]           @ restore locals
	ldr	r8, [fp, #-8]            @ restore cpool
	ldr	lr, [fp, #-16]           @ restore bcp
	ldrb	r12, [lr]                @ prefetch next bytecode
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	@  Restore stack state from tos_on_stack
	ldr	r0, [r6], #4             @ cache topmost element of stack
	mov	pc, r12                  @ go to next bytecode
.LLfe138:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_putfield:

	.global	bc_impl_putfield_internal
bc_impl_putfield_internal:
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	.extern	putfield
	ldr	r3, =putfield
	@  save interpreter state
	str	lr, [fp, #-16]
	bl	interpreter_call_vm
	@  restore interpreter state
	ldr	r7, [fp, #-12]           @ restore locals
	ldr	r8, [fp, #-8]            @ restore cpool
	ldr	lr, [fp, #-16]           @ restore bcp
	ldrb	r12, [lr]                @ prefetch next bytecode
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	@  Restore stack state from tos_on_stack
	ldr	r0, [r6], #4             @ cache topmost element of stack
	mov	pc, r12                  @ go to next bytecode
.LLfe139:
	.pool
	@  End segment: unnamed


	@  invokevirtual is a duplicate of ldc
	@  invokespecial is a duplicate of ldc
	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_invokestatic:

	.global	bc_impl_invokestatic_internal
bc_impl_invokestatic_internal:
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	.extern	quicken_invokestatic
	ldr	r3, =quicken_invokestatic
	@  save interpreter state
	str	lr, [fp, #-16]
	bl	interpreter_call_vm
	@  restore interpreter state
	ldr	r7, [fp, #-12]           @ restore locals
	ldr	r8, [fp, #-8]            @ restore cpool
	ldr	lr, [fp, #-16]           @ restore bcp
	ldr	r12, [r5, +r0, lsl #2]
	@  Restore stack state from tos_on_stack
	ldr	r0, [r6], #4             @ cache topmost element of stack
	mov	pc, r12
.LLfe140:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_invokeinterface:

	.global	bc_impl_invokeinterface_internal
bc_impl_invokeinterface_internal:
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	.extern	quicken
	ldr	r3, =quicken
	@  save interpreter state
	str	lr, [fp, #-16]
	bl	interpreter_call_vm
	@  restore interpreter state
	ldr	r7, [fp, #-12]           @ restore locals
	ldr	r8, [fp, #-8]            @ restore cpool
	ldr	lr, [fp, #-16]           @ restore bcp
	ldrb	r12, [lr]                @ prefetch next bytecode
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	@  Restore stack state from tos_on_stack
	ldr	r0, [r6], #4             @ cache topmost element of stack
	mov	pc, r12                  @ go to next bytecode
.LLfe141:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_new:

	.global	bc_impl_new_internal
bc_impl_new_internal:
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	.extern	newobject
	ldr	r3, =newobject
	@  save interpreter state
	str	lr, [fp, #-16]
	bl	interpreter_call_vm_oop
	@  restore interpreter state
	ldr	r7, [fp, #-12]           @ restore locals
	ldr	r8, [fp, #-8]            @ restore cpool
	ldr	lr, [fp, #-16]           @ restore bcp
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe142:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_newarray:

	.global	bc_impl_newarray_internal
bc_impl_newarray_internal:
	@  Entry: Count = 1
	mov	r2, r0
	@  get array type
	@  load unsigned byte at bcp + 1
	ldrb	r3, [lr, #1]
	@  Handle slowly if length too large or negative
	cmp	r2, #0x100000
	bhi	L102
	@  compute array size in words
	and	r1, r3, #3
	mov	r1, r2, lsl r1
	add	r1, r1, #11
	mov	r1, r1, lsr #2
	@  get _inline_allocation_top
	ldr	r0, [r5, #1060]          @ inline_allocation_top 
	@  get _inline_allocation_end, and check for overflow
	ldr	r9, [r5, #1064]          @ inline_allocation_end 
	adds	r4, r0, r1, lsl #2
	bcs	L102
	cmp	r4, r9
	bhi	L102
	.extern	persistent_handles
	ldr	r9, =persistent_handles + 48
	@  allocation succeeded, set _inline_allocation_top
	str	r4, [r5, #1060]          @ inline_allocation_top 
	@  Get array class
	ldr	r3, [r9, +r3, lsl #2]
	@  set array length
	str	r2, [r0, #4]
	@  Get array class near
	ldr	r3, [r3, #12]
	ldrb	r12, [lr, #2]!           @ prefetch next bytecode
	@  set prototypical near in array; no need for write barrier
	str	r3, [r0]
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
L102:
	mov	r1, r3
	.extern	_newarray
	ldr	r3, =_newarray
	@  save interpreter state
	str	lr, [fp, #-16]
	bl	interpreter_call_vm_oop
	@  restore interpreter state
	ldr	r7, [fp, #-12]           @ restore locals
	ldr	r8, [fp, #-8]            @ restore cpool
	ldr	lr, [fp, #-16]           @ restore bcp
	ldrb	r12, [lr, #2]!           @ prefetch next bytecode
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe143:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_anewarray:

	.global	bc_impl_anewarray_internal
bc_impl_anewarray_internal:
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	.extern	anewarray
	ldr	r3, =anewarray
	@  save interpreter state
	str	lr, [fp, #-16]
	bl	interpreter_call_vm_oop
	@  restore interpreter state
	ldr	r7, [fp, #-12]           @ restore locals
	ldr	r8, [fp, #-8]            @ restore cpool
	ldr	lr, [fp, #-16]           @ restore bcp
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	add	r6, r6, #4
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe144:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_arraylength:

	.global	bc_impl_arraylength_internal
bc_impl_arraylength_internal:
	@  Entry: Count = 1
	cmp	r0, #0
	beq	L103
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	ldr	r0, [r0, #4]
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
L103:
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	b	interpreter_throw_NullPointerException
.LLfe145:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_athrow:

	.global	bc_impl_athrow_internal
bc_impl_athrow_internal:
	@  Change state to tos_in_regs
	@  null check
	cmp	r0, #0                   @ is object zero?
	beq	interpreter_throw_NullPointerException_tos_cached
	@  save interpreter state
	str	lr, [fp, #-16]
	mov	r1, r0
	bl	interpreter_call_vm_exception
.LLfe146:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_checkcast:

	.global	bc_impl_checkcast_internal
bc_impl_checkcast_internal:
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	.extern	checkcast
	ldr	r3, =checkcast
	@  save interpreter state
	str	lr, [fp, #-16]
	bl	interpreter_call_vm
	@  restore interpreter state
	ldr	r7, [fp, #-12]           @ restore locals
	ldr	r8, [fp, #-8]            @ restore cpool
	ldr	lr, [fp, #-16]           @ restore bcp
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	@  Restore stack state from tos_on_stack
	ldr	r0, [r6], #4             @ cache topmost element of stack
	mov	pc, r12                  @ go to next bytecode
.LLfe147:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_instanceof:

	.global	bc_impl_instanceof_internal
bc_impl_instanceof_internal:
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	.extern	instanceof
	ldr	r3, =instanceof
	@  save interpreter state
	str	lr, [fp, #-16]
	bl	interpreter_call_vm
	@  restore interpreter state
	ldr	r7, [fp, #-12]           @ restore locals
	ldr	r8, [fp, #-8]            @ restore cpool
	ldr	lr, [fp, #-16]           @ restore bcp
	add	r6, r6, #4
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	mov	r1, #2
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe148:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_monitorenter:

	.global	bc_impl_monitorenter_internal
bc_impl_monitorenter_internal:
	@  Change state to tos_in_regs
	@  null check
	cmp	r0, #0                   @ is object zero?
	beq	interpreter_throw_NullPointerException_tos_cached
	add	lr, lr, #1
	@  save interpreter state
	str	lr, [fp, #-16]
	bl	shared_monitor_enter
	@  restore interpreter state
	ldr	r7, [fp, #-12]           @ restore locals
	ldr	r8, [fp, #-8]            @ restore cpool
	ldr	lr, [fp, #-16]           @ restore bcp
	@  Dispatch to the next bytecode
	ldrb	r12, [lr]                @ prefetch next bytecode
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	@  Restore stack state from tos_on_stack
	ldr	r0, [r6], #4             @ cache topmost element of stack
	mov	pc, r12                  @ go to next bytecode
.LLfe149:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_monitorexit:

	.global	bc_impl_monitorexit_internal
bc_impl_monitorexit_internal:
	@  Change state to tos_in_regs
	@  null check
	cmp	r0, #0                   @ is object zero?
	beq	interpreter_throw_NullPointerException_tos_cached
	@  save interpreter state
	str	lr, [fp, #-16]
	bl	shared_monitor_exit
	@  restore interpreter state
	ldr	r7, [fp, #-12]           @ restore locals
	ldr	r8, [fp, #-8]            @ restore cpool
	ldr	lr, [fp, #-16]           @ restore bcp
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	@  Restore stack state from tos_on_stack
	ldr	r0, [r6], #4             @ cache topmost element of stack
	mov	pc, r12                  @ go to next bytecode
.LLfe150:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_wide:

	.global	bc_impl_wide_internal
bc_impl_wide_internal:
	@  load unsigned byte at bcp + 1
	ldrb	r12, [lr, #1]
	ldr	r3, [r5, +r12, lsl #2]
	ldr	r4, [r3, #-4]
	mov	pc, r4
.LLfe151:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_multianewarray:

	.global	bc_impl_multianewarray_internal
bc_impl_multianewarray_internal:
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	.extern	multianewarray
	ldr	r3, =multianewarray
	@  save interpreter state
	str	lr, [fp, #-16]
	bl	interpreter_call_vm_oop
	@  restore interpreter state
	ldr	r7, [fp, #-12]           @ restore locals
	ldr	r8, [fp, #-8]            @ restore cpool
	ldr	lr, [fp, #-16]           @ restore bcp
	@  load unsigned byte at bcp + 3
	ldrb	r2, [lr, #3]
	ldrb	r12, [lr, #4]!           @ prefetch next bytecode
	add	r6, r6, r2, lsl #2
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe152:
	.pool
	@  End segment: unnamed


	@  ifnull is a duplicate of ifeq
	@  ifnonnull is a duplicate of ifne
	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_goto_w:

	.global	bc_impl_goto_w_internal
bc_impl_goto_w_internal:
	ldr	r2, [r5, #1032]          @ rt_timer_ticks  @ Timer tick?
	cmp	r2, #0
	bne	interpreter_timer_tick
	@  Entry: Count = 0
	str	r0, [r6, #-4]!
	ldrb	r2, [lr, #1]
	ldrb	r3, [lr, #2]
	ldrb	r4, [lr, #3]
	ldrb	r9, [lr, #4]
	add	r2, r3, r2, lsl #8
	add	r2, r4, r2, lsl #8
	add	r2, r9, r2, lsl #8
	ldrb	r12, [lr, +r2]!          @ prefetch next bytecode
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	ldr	r0, [r6], #4
	mov	pc, r12                  @ go to next bytecode
.LLfe153:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_breakpoint:

	.global	bc_impl_breakpoint_internal
bc_impl_breakpoint_internal:
	@  unimplemented bytecode
	.extern	print_fatal
	swi	0x9f0001
	adr	r0, _D0
	bl	print_fatal
.LLfe154:
	.pool
	@  literals
_D0:	.ascii	"trying to execute unimplemented bytecode\0\0\0\0"
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_fast_1_ldc:

	.global	bc_impl_fast_1_ldc_internal
bc_impl_fast_1_ldc_internal:
	@  Entry: Count = 0
	str	r0, [r6, #-4]!
	@  Get byte of constant pool entry
	ldrb	r0, [lr, #1]
	ldrb	r12, [lr, #2]!           @ prefetch next bytecode
	ldr	r0, [r8, +r0, lsl #2]    @ load constant pool entry
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe155:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_fast_1_ldc_w:

	.global	bc_impl_fast_1_ldc_w_internal
bc_impl_fast_1_ldc_w_internal:
	@  Entry: Count = 0
	str	r0, [r6, #-4]!
	@  Get two bytes of constant pool entry
	ldrb	r2, [lr, #1]
	ldrb	r0, [lr, #2]
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	add	r2, r8, r2, lsl #10
	ldr	r0, [r2, +r0, lsl #2]!
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe156:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_fast_2_ldc_w:

	.global	bc_impl_fast_2_ldc_w_internal
bc_impl_fast_2_ldc_w_internal:
	@  Entry: Count = 0
	str	r0, [r6, #-4]!
	@  Get two bytes of constant pool entry
	ldrb	r9, [lr, #1]
	ldrb	r0, [lr, #2]
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	add	r9, r8, r9, lsl #10
	ldr	r0, [r9, +r0, lsl #2]!
	ldr	r2, [r9, #4]
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	str	r2, [r6, #-4]!
	mov	pc, r12                  @ go to next bytecode
.LLfe157:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_fast_1_putstatic:

	.global	bc_impl_fast_1_putstatic_internal
bc_impl_fast_1_putstatic_internal:
	@  Entry: Count = 1
L105:
	@  Get two bytes of constant pool entry
	ldrb	r4, [lr, #1]
	ldrb	r1, [lr, #2]
	ldr	r9, [r5, #1204]          @ class_list_base 
	add	r4, r8, r4, lsl #10
	ldr	r1, [r4, +r1, lsl #2]!
	@  Get field offset from high sixteen bits of entry
	mov	r3, r1, lsr #16
	@  Get class_id from low sixteen bits of entry
	sub	r1, r1, r3, lsl #16
	ldr	r4, [r9, +r1, lsl #2]
	@  Get TaskMirror from class_id
	ldr	r9, [r5, #1208]          @ mirror_list_base 
	ldr	r1, [r9, +r1, lsl #2]
	cmp	r1, #0
	@  It will almost never be zero, so preload tmp3
	ldr	r9, [r5, #1808]          @ task_class_init_marker 
	cmpne	r1, r9
	bne	L106
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	@  move instanceclass obj into extra arg
	mov	r1, r4
	.extern	task_barrier
	ldr	r3, =task_barrier
	@  save interpreter state
	str	lr, [fp, #-16]
	bl	interpreter_call_vm_oop
	@  restore interpreter state
	ldr	r7, [fp, #-12]           @ restore locals
	ldr	r8, [fp, #-8]            @ restore cpool
	ldr	lr, [fp, #-16]           @ restore bcp
	@  reload registers
	@  Get two bytes of constant pool entry
	ldrb	r4, [lr, #1]
	ldrb	r1, [lr, #2]
	add	r4, r8, r4, lsl #10
	ldr	r1, [r4, +r1, lsl #2]!
	@  Get field offset from high sixteen bits of entry
	mov	r3, r1, lsr #16
	@  get pointer to task mirror into object
	mov	r1, r0
	@  Restore stack state from tos_on_stack
	ldr	r0, [r6], #4             @ cache topmost element of stack
	@  Entry: Count = 1
L106:
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	str	r0, [r1, +r3]            @ r1[r3] := r0
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	ldr	r0, [r6], #4
	mov	pc, r12                  @ go to next bytecode
L104:
.LLfe158:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_fast_2_putstatic:

	.global	bc_impl_fast_2_putstatic_internal
bc_impl_fast_2_putstatic_internal:
	@  Entry: Count = 2
	ldr	r2, [r6], #4
L108:
	@  Get two bytes of constant pool entry
	ldrb	r4, [lr, #1]
	ldrb	r1, [lr, #2]
	ldr	r9, [r5, #1204]          @ class_list_base 
	add	r4, r8, r4, lsl #10
	ldr	r1, [r4, +r1, lsl #2]!
	@  Get field offset from high sixteen bits of entry
	mov	r3, r1, lsr #16
	@  Get class_id from low sixteen bits of entry
	sub	r1, r1, r3, lsl #16
	ldr	r4, [r9, +r1, lsl #2]
	@  Get TaskMirror from class_id
	ldr	r9, [r5, #1208]          @ mirror_list_base 
	ldr	r1, [r9, +r1, lsl #2]
	cmp	r1, #0
	@  It will almost never be zero, so preload tmp3
	ldr	r9, [r5, #1808]          @ task_class_init_marker 
	cmpne	r1, r9
	bne	L109
	str	r2, [r6, #-4]!
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	@  move instanceclass obj into extra arg
	mov	r1, r4
	.extern	task_barrier
	ldr	r3, =task_barrier
	@  save interpreter state
	str	lr, [fp, #-16]
	bl	interpreter_call_vm_oop
	@  restore interpreter state
	ldr	r7, [fp, #-12]           @ restore locals
	ldr	r8, [fp, #-8]            @ restore cpool
	ldr	lr, [fp, #-16]           @ restore bcp
	@  reload registers
	@  Get two bytes of constant pool entry
	ldrb	r4, [lr, #1]
	ldrb	r1, [lr, #2]
	add	r4, r8, r4, lsl #10
	ldr	r1, [r4, +r1, lsl #2]!
	@  Get field offset from high sixteen bits of entry
	mov	r3, r1, lsr #16
	@  get pointer to task mirror into object
	mov	r1, r0
	@  Restore stack state from tos_on_stack
	ldr	r0, [r6], #4             @ cache topmost element of stack
	@  Entry: Count = 2
	ldr	r2, [r6], #4
L109:
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	str	r0, [r1, +r3]!           @ r1[r3] := r0
	str	r2, [r1, #4]
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	ldr	r0, [r6], #4
	mov	pc, r12                  @ go to next bytecode
L107:
.LLfe159:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_fast_a_putstatic:

	.global	bc_impl_fast_a_putstatic_internal
bc_impl_fast_a_putstatic_internal:
	@  Entry: Count = 1
L111:
	@  Get two bytes of constant pool entry
	ldrb	r4, [lr, #1]
	ldrb	r1, [lr, #2]
	ldr	r9, [r5, #1204]          @ class_list_base 
	add	r4, r8, r4, lsl #10
	ldr	r1, [r4, +r1, lsl #2]!
	@  Get field offset from high sixteen bits of entry
	mov	r3, r1, lsr #16
	@  Get class_id from low sixteen bits of entry
	sub	r1, r1, r3, lsl #16
	ldr	r4, [r9, +r1, lsl #2]
	@  Get TaskMirror from class_id
	ldr	r9, [r5, #1208]          @ mirror_list_base 
	ldr	r1, [r9, +r1, lsl #2]
	cmp	r1, #0
	@  It will almost never be zero, so preload tmp3
	ldr	r9, [r5, #1808]          @ task_class_init_marker 
	cmpne	r1, r9
	bne	L112
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	@  move instanceclass obj into extra arg
	mov	r1, r4
	.extern	task_barrier
	ldr	r3, =task_barrier
	@  save interpreter state
	str	lr, [fp, #-16]
	bl	interpreter_call_vm_oop
	@  restore interpreter state
	ldr	r7, [fp, #-12]           @ restore locals
	ldr	r8, [fp, #-8]            @ restore cpool
	ldr	lr, [fp, #-16]           @ restore bcp
	@  reload registers
	@  Get two bytes of constant pool entry
	ldrb	r4, [lr, #1]
	ldrb	r1, [lr, #2]
	add	r4, r8, r4, lsl #10
	ldr	r1, [r4, +r1, lsl #2]!
	@  Get field offset from high sixteen bits of entry
	mov	r3, r1, lsr #16
	@  get pointer to task mirror into object
	mov	r1, r0
	@  Restore stack state from tos_on_stack
	ldr	r0, [r6], #4             @ cache topmost element of stack
	@  Entry: Count = 1
L112:
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	str	r0, [r1, +r3]!           @ r1[r3] := r0
	@  oop_write_barrier
	ldr	r0, [r5, #1076]          @ bitvector_base 
	ldr	r3, [r5, #1080]          @ bit_selector 
	ldrb	r9, [r0, +r1, lsr #5]!
	mvn	r1, r1, lsr #2
	orr	r9, r9, r3, ror r1
	strb	r9, [r0]
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	ldr	r0, [r6], #4
	mov	pc, r12                  @ go to next bytecode
L110:
.LLfe160:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_fast_1_getstatic:

	.global	bc_impl_fast_1_getstatic_internal
bc_impl_fast_1_getstatic_internal:
	@  Entry: Count = 0
	str	r0, [r6, #-4]!
L114:
	@  Get two bytes of constant pool entry
	ldrb	r4, [lr, #1]
	ldrb	r1, [lr, #2]
	ldr	r9, [r5, #1204]          @ class_list_base 
	add	r4, r8, r4, lsl #10
	ldr	r1, [r4, +r1, lsl #2]!
	@  Get field offset from high sixteen bits of entry
	mov	r3, r1, lsr #16
	@  Get class_id from low sixteen bits of entry
	sub	r1, r1, r3, lsl #16
	ldr	r4, [r9, +r1, lsl #2]
	@  Get TaskMirror from class_id
	ldr	r9, [r5, #1208]          @ mirror_list_base 
	ldr	r1, [r9, +r1, lsl #2]
	cmp	r1, #0
	@  It will almost never be zero, so preload tmp3
	ldr	r9, [r5, #1808]          @ task_class_init_marker 
	cmpne	r1, r9
	bne	L115
	ldr	r0, [r6], #4
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	@  move instanceclass obj into extra arg
	mov	r1, r4
	.extern	task_barrier
	ldr	r3, =task_barrier
	@  save interpreter state
	str	lr, [fp, #-16]
	bl	interpreter_call_vm_oop
	@  restore interpreter state
	ldr	r7, [fp, #-12]           @ restore locals
	ldr	r8, [fp, #-8]            @ restore cpool
	ldr	lr, [fp, #-16]           @ restore bcp
	@  reload registers
	@  Get two bytes of constant pool entry
	ldrb	r4, [lr, #1]
	ldrb	r1, [lr, #2]
	add	r4, r8, r4, lsl #10
	ldr	r1, [r4, +r1, lsl #2]!
	@  Get field offset from high sixteen bits of entry
	mov	r3, r1, lsr #16
	@  get pointer to task mirror into object
	mov	r1, r0
	@  Restore stack state from tos_on_stack
	ldr	r0, [r6], #4             @ cache topmost element of stack
	@  Entry: Count = 0
	str	r0, [r6, #-4]!
L115:
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	ldr	r0, [r1, +r3]            @ r0 := r1[r3]
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
L113:
.LLfe161:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_fast_2_getstatic:

	.global	bc_impl_fast_2_getstatic_internal
bc_impl_fast_2_getstatic_internal:
	@  Entry: Count = 0
	str	r0, [r6, #-4]!
L117:
	@  Get two bytes of constant pool entry
	ldrb	r4, [lr, #1]
	ldrb	r1, [lr, #2]
	ldr	r9, [r5, #1204]          @ class_list_base 
	add	r4, r8, r4, lsl #10
	ldr	r1, [r4, +r1, lsl #2]!
	@  Get field offset from high sixteen bits of entry
	mov	r3, r1, lsr #16
	@  Get class_id from low sixteen bits of entry
	sub	r1, r1, r3, lsl #16
	ldr	r4, [r9, +r1, lsl #2]
	@  Get TaskMirror from class_id
	ldr	r9, [r5, #1208]          @ mirror_list_base 
	ldr	r1, [r9, +r1, lsl #2]
	cmp	r1, #0
	@  It will almost never be zero, so preload tmp3
	ldr	r9, [r5, #1808]          @ task_class_init_marker 
	cmpne	r1, r9
	bne	L118
	ldr	r0, [r6], #4
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	@  move instanceclass obj into extra arg
	mov	r1, r4
	.extern	task_barrier
	ldr	r3, =task_barrier
	@  save interpreter state
	str	lr, [fp, #-16]
	bl	interpreter_call_vm_oop
	@  restore interpreter state
	ldr	r7, [fp, #-12]           @ restore locals
	ldr	r8, [fp, #-8]            @ restore cpool
	ldr	lr, [fp, #-16]           @ restore bcp
	@  reload registers
	@  Get two bytes of constant pool entry
	ldrb	r4, [lr, #1]
	ldrb	r1, [lr, #2]
	add	r4, r8, r4, lsl #10
	ldr	r1, [r4, +r1, lsl #2]!
	@  Get field offset from high sixteen bits of entry
	mov	r3, r1, lsr #16
	@  get pointer to task mirror into object
	mov	r1, r0
	@  Restore stack state from tos_on_stack
	ldr	r0, [r6], #4             @ cache topmost element of stack
	@  Entry: Count = 0
	str	r0, [r6, #-4]!
L118:
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	ldr	r0, [r1, +r3]!           @ r0 := r1[r3]
	ldr	r2, [r1, #4]
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	str	r2, [r6, #-4]!
	mov	pc, r12                  @ go to next bytecode
L116:
.LLfe162:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_fast_bputfield:

	.global	bc_impl_fast_bputfield_internal
bc_impl_fast_bputfield_internal:
	@  Entry: Count = 2
	ldr	r2, [r6], #4
	ldrb	r9, [lr, #1]
	ldrb	r10, [lr, #2]
	cmp	r2, #0
	beq	L119
	orr	r3, r9, r10, lsl #8
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	strb	r0, [r2, +r3]            @ r2[r3] := r0
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	ldr	r0, [r6], #4
	mov	pc, r12                  @ go to next bytecode
L119:
	@  Handle null pointer exception
	str	r2, [r6, #-4]!
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	b	interpreter_throw_NullPointerException
.LLfe163:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_fast_sputfield:

	.global	bc_impl_fast_sputfield_internal
bc_impl_fast_sputfield_internal:
	@  Entry: Count = 2
	ldr	r2, [r6], #4
	ldrb	r9, [lr, #1]
	ldrb	r10, [lr, #2]
	cmp	r2, #0
	beq	L120
	orr	r3, r9, r10, lsl #8
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	strh	r0, [r2, +r3]            @ r2[r3] := r0
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	ldr	r0, [r6], #4
	mov	pc, r12                  @ go to next bytecode
L120:
	@  Handle null pointer exception
	str	r2, [r6, #-4]!
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	b	interpreter_throw_NullPointerException
.LLfe164:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_fast_iputfield:

	.global	bc_impl_fast_fputfield
bc_impl_fast_fputfield:

	.global	bc_impl_fast_iputfield_internal
bc_impl_fast_iputfield_internal:

	.global	bc_impl_fast_fputfield_internal
bc_impl_fast_fputfield_internal:
	@  Entry: Count = 2
	ldr	r2, [r6], #4
	ldrb	r9, [lr, #1]
	ldrb	r10, [lr, #2]
	cmp	r2, #0
	beq	L121
	orr	r3, r9, r10, lsl #8
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	str	r0, [r2, +r3, lsl #2]    @ r2[r3] := r0
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	ldr	r0, [r6], #4
	mov	pc, r12                  @ go to next bytecode
L121:
	@  Handle null pointer exception
	str	r2, [r6, #-4]!
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	b	interpreter_throw_NullPointerException
.LLfe165:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_fast_lputfield:

	.global	bc_impl_fast_dputfield
bc_impl_fast_dputfield:

	.global	bc_impl_fast_lputfield_internal
bc_impl_fast_lputfield_internal:

	.global	bc_impl_fast_dputfield_internal
bc_impl_fast_dputfield_internal:
	@  Entry: Count = 3
	ldr	r2, [r6], #4
	ldr	r4, [r6], #4
	ldrb	r9, [lr, #1]
	ldrb	r10, [lr, #2]
	cmp	r4, #0
	beq	L122
	orr	r3, r9, r10, lsl #8
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	str	r0, [r4, +r3, lsl #2]!   @ r4[r3] := r0
	str	r2, [r4, #4]
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	ldr	r0, [r6], #4
	mov	pc, r12                  @ go to next bytecode
L122:
	@  Handle null pointer exception
	str	r4, [r6, #-4]!
	str	r2, [r6, #-4]!
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	b	interpreter_throw_NullPointerException
.LLfe166:
	.pool
	@  End segment: unnamed


	@  fast_fputfield is a duplicate of fast_iputfield
	@  fast_dputfield is a duplicate of fast_lputfield
	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_fast_aputfield:

	.global	bc_impl_fast_aputfield_internal
bc_impl_fast_aputfield_internal:
	@  Entry: Count = 2
	ldr	r2, [r6], #4
	ldrb	r9, [lr, #1]
	ldrb	r10, [lr, #2]
	cmp	r2, #0
	beq	L123
	orr	r3, r9, r10, lsl #8
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	str	r0, [r2, +r3, lsl #2]!   @ r2[r3] := r0
	@  oop_write_barrier
	ldr	r0, [r5, #1076]          @ bitvector_base 
	ldr	r3, [r5, #1080]          @ bit_selector 
	ldrb	r9, [r0, +r2, lsr #5]!
	mvn	r2, r2, lsr #2
	orr	r9, r9, r3, ror r2
	strb	r9, [r0]
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	ldr	r0, [r6], #4
	mov	pc, r12                  @ go to next bytecode
L123:
	@  Handle null pointer exception
	str	r2, [r6, #-4]!
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	b	interpreter_throw_NullPointerException
.LLfe167:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_fast_bgetfield:

	.global	bc_impl_fast_bgetfield_internal
bc_impl_fast_bgetfield_internal:
	@  Entry: Count = 1
	ldrb	r9, [lr, #1]
	ldrb	r10, [lr, #2]
	cmp	r0, #0
	beq	L124
	orr	r3, r9, r10, lsl #8
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	ldrsb	r0, [r0, +r3]            @ r0 := r0[r3]
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
L124:
	@  Handle null pointer exception
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	b	interpreter_throw_NullPointerException
.LLfe168:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_fast_sgetfield:

	.global	bc_impl_fast_sgetfield_internal
bc_impl_fast_sgetfield_internal:
	@  Entry: Count = 1
	ldrb	r9, [lr, #1]
	ldrb	r10, [lr, #2]
	cmp	r0, #0
	beq	L125
	orr	r3, r9, r10, lsl #8
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	ldrsh	r0, [r0, +r3]            @ r0 := r0[r3]
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
L125:
	@  Handle null pointer exception
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	b	interpreter_throw_NullPointerException
.LLfe169:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_fast_igetfield:

	.global	bc_impl_fast_fgetfield
bc_impl_fast_fgetfield:

	.global	bc_impl_fast_agetfield
bc_impl_fast_agetfield:

	.global	bc_impl_fast_igetfield_internal
bc_impl_fast_igetfield_internal:

	.global	bc_impl_fast_fgetfield_internal
bc_impl_fast_fgetfield_internal:

	.global	bc_impl_fast_agetfield_internal
bc_impl_fast_agetfield_internal:
	@  Entry: Count = 1
	ldrb	r9, [lr, #1]
	ldrb	r10, [lr, #2]
	cmp	r0, #0
	beq	L126
	orr	r3, r9, r10, lsl #8
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	ldr	r0, [r0, +r3, lsl #2]    @ r0 := r0[r3]
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
L126:
	@  Handle null pointer exception
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	b	interpreter_throw_NullPointerException
.LLfe170:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_fast_lgetfield:

	.global	bc_impl_fast_dgetfield
bc_impl_fast_dgetfield:

	.global	bc_impl_fast_lgetfield_internal
bc_impl_fast_lgetfield_internal:

	.global	bc_impl_fast_dgetfield_internal
bc_impl_fast_dgetfield_internal:
	@  Entry: Count = 1
	ldrb	r9, [lr, #1]
	ldrb	r10, [lr, #2]
	cmp	r0, #0
	beq	L127
	orr	r3, r9, r10, lsl #8
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	add	r3, r0, r3, lsl #2       @ r0 := r0[r3]
	ldr	r0, [r3]
	ldr	r2, [r3, #4]
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	str	r2, [r6, #-4]!
	mov	pc, r12                  @ go to next bytecode
L127:
	@  Handle null pointer exception
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	b	interpreter_throw_NullPointerException
.LLfe171:
	.pool
	@  End segment: unnamed


	@  fast_fgetfield is a duplicate of fast_igetfield
	@  fast_dgetfield is a duplicate of fast_lgetfield
	@  fast_agetfield is a duplicate of fast_igetfield
	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_fast_cgetfield:

	.global	bc_impl_fast_cgetfield_internal
bc_impl_fast_cgetfield_internal:
	@  Entry: Count = 1
	ldrb	r9, [lr, #1]
	ldrb	r10, [lr, #2]
	cmp	r0, #0
	beq	L128
	orr	r3, r9, r10, lsl #8
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	ldrh	r0, [r0, +r3]            @ r0 := r0[r3]
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
L128:
	@  Handle null pointer exception
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	b	interpreter_throw_NullPointerException
.LLfe172:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_fast_invokevirtual:

	.global	bc_impl_fast_invokevirtual_internal
bc_impl_fast_invokevirtual_internal:
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	@  save interpreter state
	str	lr, [fp, #-16]
	@  Get two bytes of constant pool entry
	ldrb	r3, [lr, #1]
	ldrb	r9, [lr, #2]
	ldr	r4, [r5, #1204]          @ class_list_base 
	add	r3, r8, r3, lsl #10
	ldr	r9, [r3, +r9, lsl #2]!
	mov	r10, r9, lsl #16         @ vtable index is low bits of cpool entry
	mov	r10, r10, lsr #16
	mov	r9, r9, asr #16          @ class id is high bits of cpool entry
	ldr	r9, [r4, +r9, lsl #2]
	@  Get fixed method from vtable of the class
	ldr	r9, [r9, #16]
	add	r3, r9, r10, lsl #2
	ldr	r3, [r3, #40]
	@  Get the number of parameters
	ldrh	r4, [r3, #32]
	mov	r4, r4, lsl #23
	mov	r4, r4, lsr #23          @ r4 = parameter size in words
	@  Get receiver object
	add	r9, r6, r4, lsl #2       @ address of stack pointer just past receiver
	ldr	r9, [r9, #-4]            @ receiver
	@  Check if it is null
	@  null check
	cmp	r9, #0                   @ is object zero?
	beq	interpreter_throw_NullPointerException
	@  Get java near
	ldr	r9, [r9]
	@  Get classinfo of receiver object
	ldr	r9, [r9, #4]
	@  Get method from vtable of the class
	add	r0, r9, r10, lsl #2
	ldr	r0, [r0, #40]
	@  Call method
	ldr	r2, [r0, #20]
	ldr	r2, [r2]
	ldrh	r4, [r0, #32]
	mov	r4, r4, lsr #9           @ r4 = result type
	ldr	pc, [pc, +r4, lsl #2]
	nop
	.long	L129
	.long	L130
	.long	L131
	.long	L132
	.long	L133
L129:
	@  invoke method with result type == 0
	mov	lr, pc
	mov	pc, r2
	@  restore interpreter state
	ldr	r7, [fp, #-12]           @ restore locals
	ldr	r8, [fp, #-8]            @ restore cpool
	ldr	lr, [fp, #-16]           @ restore bcp
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	@  return type is void
	ldr	r0, [r6], #4
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
L130:
L131:
	@  invoke method with result type == 2
	mov	lr, pc
	mov	pc, r2
	@  restore interpreter state
	ldr	r7, [fp, #-12]           @ restore locals
	ldr	r8, [fp, #-8]            @ restore cpool
	ldr	lr, [fp, #-16]           @ restore bcp
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	@  result is in r0: do nothing
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
L132:
L133:
	@  invoke method with result type == 4
	mov	lr, pc
	mov	pc, r2
	@  restore interpreter state
	ldr	r7, [fp, #-12]           @ restore locals
	ldr	r8, [fp, #-8]            @ restore cpool
	ldr	lr, [fp, #-16]           @ restore bcp
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	@  result is in r0/r1
	str	r1, [r6, #-4]!
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe173:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_fast_invokestatic:

	.global	bc_impl_fast_invokestatic_internal
bc_impl_fast_invokestatic_internal:
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	@  save interpreter state
	str	lr, [fp, #-16]
L134:
	@  Get two bytes of constant pool entry
	ldrb	r2, [lr, #1]
	ldrb	r0, [lr, #2]
	add	r2, r8, r2, lsl #10
	ldr	r0, [r2, +r0, lsl #2]!
	@  get holder of this method
	ldr	r3, [r5, #1204]          @ class_list_base 
	ldrh	r2, [r0, #26]
	ldr	r2, [r3, +r2, lsl #2]
	ldr	r3, [r5, #1208]          @ mirror_list_base 
	ldrh	r4, [r0, #26]
	ldr	r3, [r3, +r4, lsl #2]
	cmp	r3, #0
	@  It will almost never be zero, so preload tmp2
	ldr	r4, [r5, #1808]          @ task_class_init_marker 
	cmpne	r3, r4
	bne	L135
	@  move instanceclass obj into extra arg
	mov	r1, r2
	.extern	task_barrier
	ldr	r3, =task_barrier
	@  save interpreter state
	str	lr, [fp, #-16]
	bl	interpreter_call_vm_oop
	@  restore interpreter state
	ldr	r7, [fp, #-12]           @ restore locals
	ldr	r8, [fp, #-8]            @ restore cpool
	ldr	lr, [fp, #-16]           @ restore bcp
	@  reload registers
	@  Get two bytes of constant pool entry
	ldrb	r2, [lr, #1]
	ldrb	r0, [lr, #2]
	add	r2, r8, r2, lsl #10
	ldr	r0, [r2, +r0, lsl #2]!
L135:
	@  Call method
	ldr	r2, [r0, #20]
	ldr	r2, [r2]
	ldrh	r4, [r0, #32]
	mov	r4, r4, lsr #9           @ r4 = result type
	ldr	pc, [pc, +r4, lsl #2]
	nop
	.long	L136
	.long	L137
	.long	L138
	.long	L139
	.long	L140
L136:
	@  invoke method with result type == 0
	mov	lr, pc
	mov	pc, r2
	@  restore interpreter state
	ldr	r7, [fp, #-12]           @ restore locals
	ldr	r8, [fp, #-8]            @ restore cpool
	ldr	lr, [fp, #-16]           @ restore bcp
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	@  return type is void
	ldr	r0, [r6], #4
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
L137:
L138:
	@  invoke method with result type == 2
	mov	lr, pc
	mov	pc, r2
	@  restore interpreter state
	ldr	r7, [fp, #-12]           @ restore locals
	ldr	r8, [fp, #-8]            @ restore cpool
	ldr	lr, [fp, #-16]           @ restore bcp
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	@  result is in r0: do nothing
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
L139:
L140:
	@  invoke method with result type == 4
	mov	lr, pc
	mov	pc, r2
	@  restore interpreter state
	ldr	r7, [fp, #-12]           @ restore locals
	ldr	r8, [fp, #-8]            @ restore cpool
	ldr	lr, [fp, #-16]           @ restore bcp
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	@  result is in r0/r1
	str	r1, [r6, #-4]!
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe174:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_fast_invokeinterface:

	.global	bc_impl_fast_invokeinterface_internal
bc_impl_fast_invokeinterface_internal:
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	@  load unsigned 16bit word at bcp + 1
	ldrb	r2, [lr, #1]             @ get bytes of constant pool index
	ldrb	r9, [lr, #2]
	@  load unsigned byte at bcp + 3
	ldrb	r3, [lr, #3]             @ get count
	orr	r2, r9, r2, lsl #8       @ combine bytes
	@  save interpreter state
	str	lr, [fp, #-16]
	@  Get the class id from resolved constant pool entry
	ldr	r4, [r8, +r2, lsl #2]
	@  Get receiver object
	add	r0, r6, r3, lsl #2       @ address of stack pointer just past receiver
	ldr	r0, [r0, #-4]            @ receiver
	mov	r9, r4, lsr #16
	@  null check
	cmp	r0, #0                   @ is object zero?
	beq	interpreter_throw_NullPointerException
	@  Get class of receiver object
	ldr	r0, [r0]
	sub	r4, r4, r9, lsl #16
	ldr	r0, [r0]
	@  Get ClassInfo of receiver class
	ldr	r0, [r0, #16]
	@  Get the itable from the ClassInfo of the receiver object
	ldrh	r3, [r0, #6]
	ldrh	r2, [r0, #8]
	add	r1, r0, #32
	add	r1, r1, r3, lsl #2
	@  Lookup interface method table by linear search
L141:
	subs	r2, r2, #1
	ldrge	r10, [r1, #8]!
	blt	interpreter_throw_IncompatibleClassChangeError
	cmp	r9, r10
	bne	L141
	@  Found the itable entry; now get the method table offset from there
	ldr	r3, [r1, #4]
	@  Get the method from the method table
	add	r0, r0, r4, lsl #2
	ldr	r0, [r0, +r3]
	@  Call method
	ldr	r2, [r0, #20]
	ldr	r2, [r2]
	ldrh	r4, [r0, #32]
	mov	r4, r4, lsr #9           @ r4 = result type
	ldr	pc, [pc, +r4, lsl #2]
	nop
	.long	L142
	.long	L143
	.long	L144
	.long	L145
	.long	L146
L142:
	@  invoke method with result type == 0
	mov	lr, pc
	mov	pc, r2

	.global	invoke5_deoptimization_entry_0
invoke5_deoptimization_entry_0:
	@  restore interpreter state
	ldr	r7, [fp, #-12]           @ restore locals
	ldr	r8, [fp, #-8]            @ restore cpool
	ldr	lr, [fp, #-16]           @ restore bcp
	ldrb	r12, [lr, #5]!           @ prefetch next bytecode
	@  return type is void
	ldr	r0, [r6], #4
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
L143:
L144:
	@  invoke method with result type == 2
	mov	lr, pc
	mov	pc, r2

	.global	invoke5_deoptimization_entry_1
invoke5_deoptimization_entry_1:

	.global	invoke5_deoptimization_entry_2
invoke5_deoptimization_entry_2:
	@  restore interpreter state
	ldr	r7, [fp, #-12]           @ restore locals
	ldr	r8, [fp, #-8]            @ restore cpool
	ldr	lr, [fp, #-16]           @ restore bcp
	ldrb	r12, [lr, #5]!           @ prefetch next bytecode
	@  result is in r0: do nothing
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
L145:
L146:
	@  invoke method with result type == 4
	mov	lr, pc
	mov	pc, r2

	.global	invoke5_deoptimization_entry_3
invoke5_deoptimization_entry_3:

	.global	invoke5_deoptimization_entry_4
invoke5_deoptimization_entry_4:
	@  restore interpreter state
	ldr	r7, [fp, #-12]           @ restore locals
	ldr	r8, [fp, #-8]            @ restore cpool
	ldr	lr, [fp, #-16]           @ restore bcp
	ldrb	r12, [lr, #5]!           @ prefetch next bytecode
	@  result is in r0/r1
	str	r1, [r6, #-4]!
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe175:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_fast_invokenative:

	.global	bc_impl_fast_invokenative_internal
bc_impl_fast_invokenative_internal:
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
L147:
	@  Get address of first paramater
	ldr	r2, [fp, #-4]            @ current method
	ldrh	r2, [r2, #24]
	mov	r3, r7
	@  load unsigned byte at bcp + 1
	ldrb	r4, [lr, #1]
	@  Set space for fake parameter for static method (KNI-ism)
	tst	r2, #8
	addne	r3, r3, #4
	@  Point _kni_parameter_base to the first parameter
	str	r3, [r5, #1264]          @ kni_parameter_base 
	ldr	r3, [lr, #4]             @ get the pointer from the bytecode
	@  call the native code
	@  save interpreter state
	str	lr, [fp, #-16]
	cmp	r4, #12
	adr	lr, L148
	beq	shared_call_vm_oop
	bne	shared_call_vm
L148:
	@  restore interpreter state
	ldr	r7, [fp, #-12]           @ restore locals
	ldr	r8, [fp, #-8]            @ restore cpool
	ldr	lr, [fp, #-16]           @ restore bcp
	ldr	r3, [r5, #1036]          @ current_thread 
	ldr	r4, [r3, #76]
	@  Clear Thread.async_redo so that we won't loop indefinitely
	mov	r9, #0
	str	r9, [r3, #76]
	@  loop if need to redo the native method invocation
	cmp	r4, #0
	bne	L147
	@  load unsigned byte at bcp + 1
	ldrb	r4, [lr, #1]             @ Get kind
	str	r9, [r3, #52]            @ Clear Thread.async_info
	sub	r4, r4, #7
	ldr	pc, [pc, +r4, lsl #2]
	.long	L154
	.long	L155
	.long	L153
	.long	L153
	.long	L149
	.long	L150
	.long	L152
	.long	L153
	.long	L151
L149:
	@  store result
	b	bc_impl_ireturn_internal
L150:
	@  store result
	str	r1, [r6, #-4]!
	b	bc_impl_lreturn_internal
L152:
	@  store result
	b	bc_impl_areturn_internal
L151:
	ldr	r0, [r6], #4
	b	bc_impl_return_internal
L154:
	@  store result
	b	bc_impl_freturn_internal
L155:
	@  store result
	str	r1, [r6, #-4]!
	b	bc_impl_dreturn_internal
L153:
	swi	0x9f0001
.LLfe176:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_fast_invokevirtual_final:

	.global	bc_impl_fast_invokevirtual_final_internal
bc_impl_fast_invokevirtual_final_internal:
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	@  save interpreter state
	str	lr, [fp, #-16]
L156:
	@  Get two bytes of constant pool entry
	ldrb	r2, [lr, #1]
	ldrb	r0, [lr, #2]
	add	r2, r8, r2, lsl #10
	ldr	r0, [r2, +r0, lsl #2]!
	ldrh	r4, [r0, #32]            @ number of parameters
	mov	r4, r4, lsl #23
	mov	r4, r4, lsr #23          @ r4 = parameter size in words
	add	r9, r6, r4, lsl #2       @ get receiver object
	ldr	r9, [r9, #-4]            @ receiver
	@  null check
	cmp	r9, #0                   @ is object zero?
	beq	interpreter_throw_NullPointerException
	@  Call method
	ldr	r2, [r0, #20]
	ldr	r2, [r2]
	ldrh	r4, [r0, #32]
	mov	r4, r4, lsr #9           @ r4 = result type
	ldr	pc, [pc, +r4, lsl #2]
	nop
	.long	L157
	.long	L158
	.long	L159
	.long	L160
	.long	L161
L157:
	@  invoke method with result type == 0
	mov	lr, pc
	mov	pc, r2
	@  restore interpreter state
	ldr	r7, [fp, #-12]           @ restore locals
	ldr	r8, [fp, #-8]            @ restore cpool
	ldr	lr, [fp, #-16]           @ restore bcp
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	@  return type is void
	ldr	r0, [r6], #4
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
L158:
L159:
	@  invoke method with result type == 2
	mov	lr, pc
	mov	pc, r2
	@  restore interpreter state
	ldr	r7, [fp, #-12]           @ restore locals
	ldr	r8, [fp, #-8]            @ restore cpool
	ldr	lr, [fp, #-16]           @ restore bcp
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	@  result is in r0: do nothing
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
L160:
L161:
	@  invoke method with result type == 4
	mov	lr, pc
	mov	pc, r2
	@  restore interpreter state
	ldr	r7, [fp, #-12]           @ restore locals
	ldr	r8, [fp, #-8]            @ restore cpool
	ldr	lr, [fp, #-16]           @ restore bcp
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	@  result is in r0/r1
	str	r1, [r6, #-4]!
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe177:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_fast_invokespecial:

	.global	bc_impl_fast_invokespecial_internal
bc_impl_fast_invokespecial_internal:
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	@  save interpreter state
	str	lr, [fp, #-16]
	@  Get two bytes of constant pool entry
	ldrb	r3, [lr, #1]
	ldrb	r9, [lr, #2]
	ldr	r4, [r5, #1204]          @ class_list_base 
	add	r3, r8, r3, lsl #10
	ldr	r9, [r3, +r9, lsl #2]!
	mov	r10, r9, lsl #16         @ vtable index is low 16 bits of the cpool entry
	mov	r10, r10, lsr #16
	mov	r9, r9, asr #16          @ class id is high 16 bits of the cpool entry
	ldr	r9, [r4, +r9, lsl #2]
	@  Get ClassInfo from class
	ldr	r9, [r9, #16]
	@  Get method from vtable of the ClassInfo
	add	r0, r9, r10, lsl #2
	ldr	r0, [r0, #40]
	@  Get the number of parameters
	ldrh	r4, [r0, #32]
	mov	r4, r4, lsl #23
	mov	r4, r4, lsr #23          @ r4 = parameter size in words
	@  Get receiver object
	add	r9, r6, r4, lsl #2       @ address of stack pointer just past receiver
	ldr	r9, [r9, #-4]            @ receiver
	@  null check
	cmp	r9, #0                   @ is object zero?
	beq	interpreter_throw_NullPointerException
	@  Call method
	ldr	r2, [r0, #20]
	ldr	r2, [r2]
	ldrh	r4, [r0, #32]
	mov	r4, r4, lsr #9           @ r4 = result type
	ldr	pc, [pc, +r4, lsl #2]
	nop
	.long	L162
	.long	L163
	.long	L164
	.long	L165
	.long	L166
L162:
	@  invoke method with result type == 0
	mov	lr, pc
	mov	pc, r2

	.global	invoke3_deoptimization_entry_0
invoke3_deoptimization_entry_0:
	@  restore interpreter state
	ldr	r7, [fp, #-12]           @ restore locals
	ldr	r8, [fp, #-8]            @ restore cpool
	ldr	lr, [fp, #-16]           @ restore bcp
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	@  return type is void
	ldr	r0, [r6], #4
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
L163:
L164:
	@  invoke method with result type == 2
	mov	lr, pc
	mov	pc, r2

	.global	invoke3_deoptimization_entry_1
invoke3_deoptimization_entry_1:

	.global	invoke3_deoptimization_entry_2
invoke3_deoptimization_entry_2:
	@  restore interpreter state
	ldr	r7, [fp, #-12]           @ restore locals
	ldr	r8, [fp, #-8]            @ restore cpool
	ldr	lr, [fp, #-16]           @ restore bcp
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	@  result is in r0: do nothing
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
L165:
L166:
	@  invoke method with result type == 4
	mov	lr, pc
	mov	pc, r2

	.global	invoke3_deoptimization_entry_3
invoke3_deoptimization_entry_3:

	.global	invoke3_deoptimization_entry_4
invoke3_deoptimization_entry_4:
	@  restore interpreter state
	ldr	r7, [fp, #-12]           @ restore locals
	ldr	r8, [fp, #-8]            @ restore cpool
	ldr	lr, [fp, #-16]           @ restore bcp
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	@  result is in r0/r1
	str	r1, [r6, #-4]!
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe178:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_fast_new:

	.global	bc_impl_fast_new_internal
bc_impl_fast_new_internal:
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
L168:
	@  Get two bytes of constant pool entry
	ldrb	r4, [lr, #1]
	ldrb	r1, [lr, #2]
	ldr	r3, [r5, #1204]          @ class_list_base 
	add	r4, r8, r4, lsl #10
	ldr	r1, [r4, +r1, lsl #2]!
	ldr	r2, [r3, +r1, lsl #2]
	ldr	r4, [r5, #1208]          @ mirror_list_base 
	ldr	r3, [r4, +r1, lsl #2]
	ldr	r4, [r5, #1808]          @ task_class_init_marker 
	cmp	r3, r4
	bne	L169
	@  move instanceclass obj into extra arg
	mov	r1, r2
	.extern	task_barrier
	ldr	r3, =task_barrier
	@  save interpreter state
	str	lr, [fp, #-16]
	bl	interpreter_call_vm_oop
	@  restore interpreter state
	ldr	r7, [fp, #-12]           @ restore locals
	ldr	r8, [fp, #-8]            @ restore cpool
	ldr	lr, [fp, #-16]           @ restore bcp
	@  reload registers
	@  Get two bytes of constant pool entry
	ldrb	r4, [lr, #1]
	ldrb	r2, [lr, #2]
	ldr	r3, [r5, #1204]          @ class_list_base 
	add	r4, r8, r4, lsl #10
	ldr	r2, [r4, +r2, lsl #2]!
	ldr	r2, [r3, +r2, lsl #2]
L169:
	@  get _inline_allocation_top
	ldr	r0, [r5, #1060]          @ inline_allocation_top 
	@  get instance size (in bytes) from class
	ldrsh	r1, [r2, #6]
	@  get _inline_allocation_end, and check for overflow
	ldr	r9, [r5, #1064]          @ inline_allocation_end 
	adds	r4, r0, r1
	bcs	L167
	cmp	r4, r9
	bhi	L167
	@  allocation succeeded, set _inline_allocation_top
	str	r4, [r5, #1060]          @ inline_allocation_top 
	@  get prototypical near from class
	ldr	r2, [r2, #12]
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	@  set prototypical near in object; no need for write barrier
	str	r2, [r0]
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
L167:
	@  Restore stack state from tos_on_stack
	ldr	r0, [r6], #4             @ cache topmost element of stack
	b	bc_impl_new_internal
.LLfe179:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_fast_anewarray:

	.global	bc_impl_fast_anewarray_internal
bc_impl_fast_anewarray_internal:
	@  Entry: Count = 1
	mov	r2, r0
	@  go slow case if array length is larger than maximum safe array length or below zero
	cmp	r2, #0x100000
	bhi	L170
	@  compute array size in bytes
	mov	r1, r2, lsl #2
	add	r1, r1, #8
	@  get _inline_allocation_top
	ldr	r0, [r5, #1060]          @ inline_allocation_top 
	@  get _inline_allocation_end, and check for overflow
	ldr	r9, [r5, #1064]          @ inline_allocation_end 
	adds	r4, r0, r1
	bcs	L170
	cmp	r4, r9
	bhi	L170
	@  allocation succeeded, set _inline_allocation_top
	str	r4, [r5, #1060]          @ inline_allocation_top 
	@  get constant pool entry
	@  Get two bytes of constant pool entry
	ldrb	r4, [lr, #1]
	ldrb	r3, [lr, #2]
	add	r4, r8, r4, lsl #10
	ldr	r3, [r4, +r3, lsl #2]!
	@  If arrayclass pointer is non-zero we're done
	ldr	r9, [r5, #1204]          @ class_list_base 
	ldr	r4, [r9, +r3, lsl #2]
	ldr	r4, [r4, #28]
	cmp	r4, #0
	movne	r3, r4
	bne	L171
	@  get task mirror
	ldr	r9, [r5, #1208]          @ mirror_list_base 
	ldr	r3, [r9, +r3, lsl #2]
	@  get array class from class
	ldr	r3, [r3, #20]
	cmp	r3, #0
	beq	L170
L171:
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	@  set array length
	str	r2, [r0, #4]
	@  get prototypical near from array class
	ldr	r3, [r3, #12]
	@  set prototypical near in array; no need for write barrier
	str	r3, [r0]
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
L170:
	mov	r0, r2
	b	bc_impl_anewarray
.LLfe180:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_fast_checkcast:

	.global	bc_impl_fast_checkcast_internal
bc_impl_fast_checkcast_internal:
	@  Entry: Count = 1
	@  Check for NULL object
	cmp	r0, #0
	beq	L172
	@  Get the class for the object, and the constant pool entry
	ldr	r2, [r0]
	@  Get two bytes of constant pool entry
	ldrb	r4, [lr, #1]
	ldrb	r3, [lr, #2]
	ldr	r9, [r5, #1204]          @ class_list_base 
	add	r4, r8, r4, lsl #10
	ldr	r3, [r4, +r3, lsl #2]!
	ldr	r2, [r2]
	ldr	r3, [r9, +r3, lsl #2]
	@  Compare the classes
	cmp	r3, r2
	@  Check the subtype caches
	ldrne	r4, [r2, #20]
	ldrne	r9, [r2, #24]
	cmpne	r3, r4
	cmpne	r3, r9
	bne	L173
L172:
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
L173:
	b	bc_impl_checkcast
.LLfe181:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_fast_instanceof:

	.global	bc_impl_fast_instanceof_internal
bc_impl_fast_instanceof_internal:
	@  Entry: Count = 1
	@  Get the object to check cast for
	@  Check for NULL object
	cmp	r0, #0
	beq	L174
	@  Get the class for the object, and the constant pool entry
	ldr	r2, [r0]
	@  Get two bytes of constant pool entry
	ldrb	r4, [lr, #1]
	ldrb	r3, [lr, #2]
	ldr	r9, [r5, #1204]          @ class_list_base 
	add	r4, r8, r4, lsl #10
	ldr	r3, [r4, +r3, lsl #2]!
	ldr	r2, [r2]
	ldr	r3, [r9, +r3, lsl #2]
	@  Compare the classes
	cmp	r3, r2
	@  Check the subtype caches
	ldrne	r4, [r2, #20]
	ldrne	r9, [r2, #24]
	cmpne	r3, r4
	cmpne	r3, r9
	bne	L175
	@  Set the result to 1
	mov	r0, #1
L174:
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
L175:
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	mov	r1, r2
	mov	r2, r3
	.extern	_instanceof
	ldr	r3, =_instanceof
	@  save interpreter state
	str	lr, [fp, #-16]
	bl	interpreter_call_vm
	@  restore interpreter state
	ldr	r7, [fp, #-12]           @ restore locals
	ldr	r8, [fp, #-8]            @ restore cpool
	ldr	lr, [fp, #-16]           @ restore bcp
	add	r6, r6, #4
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	mov	r1, #2
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe182:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_fast_igetfield_1:

	.global	bc_impl_fast_agetfield_1
bc_impl_fast_agetfield_1:

	.global	bc_impl_fast_igetfield_1_internal
bc_impl_fast_igetfield_1_internal:

	.global	bc_impl_fast_agetfield_1_internal
bc_impl_fast_agetfield_1_internal:
	@  Entry: Count = 1
	ldrb	r3, [lr, #1]
	cmp	r0, #0
	beq	L176
	ldrb	r12, [lr, #2]!           @ prefetch next bytecode
	ldr	r0, [r0, +r3, lsl #2]    @ r0 := r0[r3]
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
L176:
	@  Handle null pointer exception
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	b	interpreter_throw_NullPointerException
.LLfe183:
	.pool
	@  End segment: unnamed


	@  fast_agetfield_1 is a duplicate of fast_igetfield_1
	@  aload_0_fast_agetfield_1 is a duplicate of aload_0_fast_igetfield_1
	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_aload_0_fast_igetfield_1:

	.global	bc_impl_aload_0_fast_agetfield_1
bc_impl_aload_0_fast_agetfield_1:

	.global	bc_impl_aload_0_fast_igetfield_1_internal
bc_impl_aload_0_fast_igetfield_1_internal:

	.global	bc_impl_aload_0_fast_agetfield_1_internal
bc_impl_aload_0_fast_agetfield_1_internal:
	@  Entry: Count = 0
	str	r0, [r6, #-4]!
	ldr	r0, [r7]
	ldrb	r3, [lr, #1]
	cmp	r0, #0
	beq	L177
	ldrb	r12, [lr, #2]!           @ prefetch next bytecode
	ldr	r0, [r0, +r3, lsl #2]    @ r0 := r0[r3]
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
L177:
	@  Handle null pointer exception
	ldr	r0, [r6], #4
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	b	interpreter_throw_NullPointerException
.LLfe184:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_pop_and_npe_if_null:

	.global	bc_impl_pop_and_npe_if_null_internal
bc_impl_pop_and_npe_if_null_internal:
	@  Change state to tos_in_regs
	@  null check
	cmp	r0, #0                   @ is object zero?
	beq	interpreter_throw_NullPointerException_tos_cached
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	@  Restore stack state from tos_on_stack
	ldr	r0, [r6], #4             @ cache topmost element of stack
	mov	pc, r12                  @ go to next bytecode
.LLfe185:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_init_static_array:

	.global	bc_impl_init_static_array_internal
bc_impl_init_static_array_internal:
	ldrb	r3, [lr, #1]!
	ldrb	r10, [lr, #1]!
	ldrb	r4, [lr, #1]!
	cmp	r0, #0
	beq	interpreter_throw_NullPointerException
	ldr	r9, [r0, #4]
	add	r2, r0, #8
	orr	r4, r10, r4, lsl #8
	cmp	r4, r9
	bgt	interpreter_throw_ArrayIndexOutOfBoundsException
	mov	r4, r4, lsl r3
	add	r4, r4, lr
L178:
	ldrb	r3, [lr, #1]!
	strb	r3, [r2], #1
	cmp	r4, lr
	bgt	L178
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe186:
	.pool
	@  End segment: unnamed


	@  aload_0_fast_agetfield_4 is a duplicate of aload_0_fast_igetfield_4
	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_aload_0_fast_igetfield_4:

	.global	bc_impl_aload_0_fast_agetfield_4
bc_impl_aload_0_fast_agetfield_4:

	.global	bc_impl_aload_0_fast_igetfield_4_internal
bc_impl_aload_0_fast_igetfield_4_internal:

	.global	bc_impl_aload_0_fast_agetfield_4_internal
bc_impl_aload_0_fast_agetfield_4_internal:
	@  Entry: Count = 0
	str	r0, [r6, #-4]!
	ldr	r0, [r7]
	@  Implicit offset = 4
	mov	r3, #1
	cmp	r0, #0
	beq	L179
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	ldr	r0, [r0, +r3, lsl #2]    @ r0 := r0[r3]
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
L179:
	@  Handle null pointer exception
	ldr	r0, [r6], #4
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	b	interpreter_throw_NullPointerException
.LLfe187:
	.pool
	@  End segment: unnamed


	@  aload_0_fast_agetfield_8 is a duplicate of aload_0_fast_igetfield_8
	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_aload_0_fast_igetfield_8:

	.global	bc_impl_aload_0_fast_agetfield_8
bc_impl_aload_0_fast_agetfield_8:

	.global	bc_impl_aload_0_fast_igetfield_8_internal
bc_impl_aload_0_fast_igetfield_8_internal:

	.global	bc_impl_aload_0_fast_agetfield_8_internal
bc_impl_aload_0_fast_agetfield_8_internal:
	@  Entry: Count = 0
	str	r0, [r6, #-4]!
	ldr	r0, [r7]
	@  Implicit offset = 8
	mov	r3, #2
	cmp	r0, #0
	beq	L180
	ldrb	r12, [lr, #1]!           @ prefetch next bytecode
	ldr	r0, [r0, +r3, lsl #2]    @ r0 := r0[r3]
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
L180:
	@  Handle null pointer exception
	ldr	r0, [r6], #4
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	b	interpreter_throw_NullPointerException
.LLfe188:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_fast_init_1_putstatic:

	.global	bc_impl_fast_init_1_putstatic_internal
bc_impl_fast_init_1_putstatic_internal:
	@  Entry: Count = 1
L182:
	@  Get two bytes of constant pool entry
	ldrb	r4, [lr, #1]
	ldrb	r1, [lr, #2]
	ldr	r9, [r5, #1204]          @ class_list_base 
	add	r4, r8, r4, lsl #10
	ldr	r1, [r4, +r1, lsl #2]!
	@  Get field offset from high sixteen bits of entry
	mov	r3, r1, lsr #16
	@  Get class_id from low sixteen bits of entry
	sub	r1, r1, r3, lsl #16
	ldr	r4, [r9, +r1, lsl #2]
	@  Get TaskMirror from class_id
	ldr	r9, [r5, #1208]          @ mirror_list_base 
	ldr	r1, [r9, +r1, lsl #2]
	cmp	r1, #0
	@  It will almost never be zero, so preload tmp3
	ldr	r9, [r5, #1808]          @ task_class_init_marker 
	cmpne	r1, r9
	bne	L183
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	@  move instanceclass obj into extra arg
	mov	r1, r4
	.extern	task_barrier
	ldr	r3, =task_barrier
	@  save interpreter state
	str	lr, [fp, #-16]
	bl	interpreter_call_vm_oop
	@  restore interpreter state
	ldr	r7, [fp, #-12]           @ restore locals
	ldr	r8, [fp, #-8]            @ restore cpool
	ldr	lr, [fp, #-16]           @ restore bcp
	@  reload registers
	@  Get two bytes of constant pool entry
	ldrb	r4, [lr, #1]
	ldrb	r1, [lr, #2]
	add	r4, r8, r4, lsl #10
	ldr	r1, [r4, +r1, lsl #2]!
	@  Get field offset from high sixteen bits of entry
	mov	r3, r1, lsr #16
	@  get pointer to task mirror into object
	mov	r1, r0
	@  Restore stack state from tos_on_stack
	ldr	r0, [r6], #4             @ cache topmost element of stack
	@  Entry: Count = 1
L183:
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	str	r0, [r1, +r3]            @ r1[r3] := r0
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	ldr	r0, [r6], #4
	mov	pc, r12                  @ go to next bytecode
L181:
.LLfe189:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_fast_init_2_putstatic:

	.global	bc_impl_fast_init_2_putstatic_internal
bc_impl_fast_init_2_putstatic_internal:
	@  Entry: Count = 2
	ldr	r2, [r6], #4
L185:
	@  Get two bytes of constant pool entry
	ldrb	r4, [lr, #1]
	ldrb	r1, [lr, #2]
	ldr	r9, [r5, #1204]          @ class_list_base 
	add	r4, r8, r4, lsl #10
	ldr	r1, [r4, +r1, lsl #2]!
	@  Get field offset from high sixteen bits of entry
	mov	r3, r1, lsr #16
	@  Get class_id from low sixteen bits of entry
	sub	r1, r1, r3, lsl #16
	ldr	r4, [r9, +r1, lsl #2]
	@  Get TaskMirror from class_id
	ldr	r9, [r5, #1208]          @ mirror_list_base 
	ldr	r1, [r9, +r1, lsl #2]
	cmp	r1, #0
	@  It will almost never be zero, so preload tmp3
	ldr	r9, [r5, #1808]          @ task_class_init_marker 
	cmpne	r1, r9
	bne	L186
	str	r2, [r6, #-4]!
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	@  move instanceclass obj into extra arg
	mov	r1, r4
	.extern	task_barrier
	ldr	r3, =task_barrier
	@  save interpreter state
	str	lr, [fp, #-16]
	bl	interpreter_call_vm_oop
	@  restore interpreter state
	ldr	r7, [fp, #-12]           @ restore locals
	ldr	r8, [fp, #-8]            @ restore cpool
	ldr	lr, [fp, #-16]           @ restore bcp
	@  reload registers
	@  Get two bytes of constant pool entry
	ldrb	r4, [lr, #1]
	ldrb	r1, [lr, #2]
	add	r4, r8, r4, lsl #10
	ldr	r1, [r4, +r1, lsl #2]!
	@  Get field offset from high sixteen bits of entry
	mov	r3, r1, lsr #16
	@  get pointer to task mirror into object
	mov	r1, r0
	@  Restore stack state from tos_on_stack
	ldr	r0, [r6], #4             @ cache topmost element of stack
	@  Entry: Count = 2
	ldr	r2, [r6], #4
L186:
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	str	r0, [r1, +r3]!           @ r1[r3] := r0
	str	r2, [r1, #4]
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	ldr	r0, [r6], #4
	mov	pc, r12                  @ go to next bytecode
L184:
.LLfe190:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_fast_init_a_putstatic:

	.global	bc_impl_fast_init_a_putstatic_internal
bc_impl_fast_init_a_putstatic_internal:
	@  Entry: Count = 1
L188:
	@  Get two bytes of constant pool entry
	ldrb	r4, [lr, #1]
	ldrb	r1, [lr, #2]
	ldr	r9, [r5, #1204]          @ class_list_base 
	add	r4, r8, r4, lsl #10
	ldr	r1, [r4, +r1, lsl #2]!
	@  Get field offset from high sixteen bits of entry
	mov	r3, r1, lsr #16
	@  Get class_id from low sixteen bits of entry
	sub	r1, r1, r3, lsl #16
	ldr	r4, [r9, +r1, lsl #2]
	@  Get TaskMirror from class_id
	ldr	r9, [r5, #1208]          @ mirror_list_base 
	ldr	r1, [r9, +r1, lsl #2]
	cmp	r1, #0
	@  It will almost never be zero, so preload tmp3
	ldr	r9, [r5, #1808]          @ task_class_init_marker 
	cmpne	r1, r9
	bne	L189
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	@  move instanceclass obj into extra arg
	mov	r1, r4
	.extern	task_barrier
	ldr	r3, =task_barrier
	@  save interpreter state
	str	lr, [fp, #-16]
	bl	interpreter_call_vm_oop
	@  restore interpreter state
	ldr	r7, [fp, #-12]           @ restore locals
	ldr	r8, [fp, #-8]            @ restore cpool
	ldr	lr, [fp, #-16]           @ restore bcp
	@  reload registers
	@  Get two bytes of constant pool entry
	ldrb	r4, [lr, #1]
	ldrb	r1, [lr, #2]
	add	r4, r8, r4, lsl #10
	ldr	r1, [r4, +r1, lsl #2]!
	@  Get field offset from high sixteen bits of entry
	mov	r3, r1, lsr #16
	@  get pointer to task mirror into object
	mov	r1, r0
	@  Restore stack state from tos_on_stack
	ldr	r0, [r6], #4             @ cache topmost element of stack
	@  Entry: Count = 1
L189:
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	str	r0, [r1, +r3]!           @ r1[r3] := r0
	@  oop_write_barrier
	ldr	r0, [r5, #1076]          @ bitvector_base 
	ldr	r3, [r5, #1080]          @ bit_selector 
	ldrb	r9, [r0, +r1, lsr #5]!
	mvn	r1, r1, lsr #2
	orr	r9, r9, r3, ror r1
	strb	r9, [r0]
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	ldr	r0, [r6], #4
	mov	pc, r12                  @ go to next bytecode
L187:
.LLfe191:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_fast_init_1_getstatic:

	.global	bc_impl_fast_init_1_getstatic_internal
bc_impl_fast_init_1_getstatic_internal:
	@  Entry: Count = 0
	str	r0, [r6, #-4]!
L191:
	@  Get two bytes of constant pool entry
	ldrb	r4, [lr, #1]
	ldrb	r1, [lr, #2]
	ldr	r9, [r5, #1204]          @ class_list_base 
	add	r4, r8, r4, lsl #10
	ldr	r1, [r4, +r1, lsl #2]!
	@  Get field offset from high sixteen bits of entry
	mov	r3, r1, lsr #16
	@  Get class_id from low sixteen bits of entry
	sub	r1, r1, r3, lsl #16
	ldr	r4, [r9, +r1, lsl #2]
	@  Get TaskMirror from class_id
	ldr	r9, [r5, #1208]          @ mirror_list_base 
	ldr	r1, [r9, +r1, lsl #2]
	cmp	r1, #0
	@  It will almost never be zero, so preload tmp3
	ldr	r9, [r5, #1808]          @ task_class_init_marker 
	cmpne	r1, r9
	bne	L192
	ldr	r0, [r6], #4
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	@  move instanceclass obj into extra arg
	mov	r1, r4
	.extern	task_barrier
	ldr	r3, =task_barrier
	@  save interpreter state
	str	lr, [fp, #-16]
	bl	interpreter_call_vm_oop
	@  restore interpreter state
	ldr	r7, [fp, #-12]           @ restore locals
	ldr	r8, [fp, #-8]            @ restore cpool
	ldr	lr, [fp, #-16]           @ restore bcp
	@  reload registers
	@  Get two bytes of constant pool entry
	ldrb	r4, [lr, #1]
	ldrb	r1, [lr, #2]
	add	r4, r8, r4, lsl #10
	ldr	r1, [r4, +r1, lsl #2]!
	@  Get field offset from high sixteen bits of entry
	mov	r3, r1, lsr #16
	@  get pointer to task mirror into object
	mov	r1, r0
	@  Restore stack state from tos_on_stack
	ldr	r0, [r6], #4             @ cache topmost element of stack
	@  Entry: Count = 0
	str	r0, [r6, #-4]!
L192:
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	ldr	r0, [r1, +r3]            @ r0 := r1[r3]
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
L190:
.LLfe192:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_fast_init_2_getstatic:

	.global	bc_impl_fast_init_2_getstatic_internal
bc_impl_fast_init_2_getstatic_internal:
	@  Entry: Count = 0
	str	r0, [r6, #-4]!
L194:
	@  Get two bytes of constant pool entry
	ldrb	r4, [lr, #1]
	ldrb	r1, [lr, #2]
	ldr	r9, [r5, #1204]          @ class_list_base 
	add	r4, r8, r4, lsl #10
	ldr	r1, [r4, +r1, lsl #2]!
	@  Get field offset from high sixteen bits of entry
	mov	r3, r1, lsr #16
	@  Get class_id from low sixteen bits of entry
	sub	r1, r1, r3, lsl #16
	ldr	r4, [r9, +r1, lsl #2]
	@  Get TaskMirror from class_id
	ldr	r9, [r5, #1208]          @ mirror_list_base 
	ldr	r1, [r9, +r1, lsl #2]
	cmp	r1, #0
	@  It will almost never be zero, so preload tmp3
	ldr	r9, [r5, #1808]          @ task_class_init_marker 
	cmpne	r1, r9
	bne	L195
	ldr	r0, [r6], #4
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	@  move instanceclass obj into extra arg
	mov	r1, r4
	.extern	task_barrier
	ldr	r3, =task_barrier
	@  save interpreter state
	str	lr, [fp, #-16]
	bl	interpreter_call_vm_oop
	@  restore interpreter state
	ldr	r7, [fp, #-12]           @ restore locals
	ldr	r8, [fp, #-8]            @ restore cpool
	ldr	lr, [fp, #-16]           @ restore bcp
	@  reload registers
	@  Get two bytes of constant pool entry
	ldrb	r4, [lr, #1]
	ldrb	r1, [lr, #2]
	add	r4, r8, r4, lsl #10
	ldr	r1, [r4, +r1, lsl #2]!
	@  Get field offset from high sixteen bits of entry
	mov	r3, r1, lsr #16
	@  get pointer to task mirror into object
	mov	r1, r0
	@  Restore stack state from tos_on_stack
	ldr	r0, [r6], #4             @ cache topmost element of stack
	@  Entry: Count = 0
	str	r0, [r6, #-4]!
L195:
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	ldr	r0, [r1, +r3]!           @ r0 := r1[r3]
	ldr	r2, [r1, #4]
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	str	r2, [r6, #-4]!
	mov	pc, r12                  @ go to next bytecode
L193:
.LLfe193:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_fast_init_invokestatic:

	.global	bc_impl_fast_init_invokestatic_internal
bc_impl_fast_init_invokestatic_internal:
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	@  save interpreter state
	str	lr, [fp, #-16]
L196:
	@  Get two bytes of constant pool entry
	ldrb	r2, [lr, #1]
	ldrb	r0, [lr, #2]
	add	r2, r8, r2, lsl #10
	ldr	r0, [r2, +r0, lsl #2]!
	@  get holder of this method
	ldr	r3, [r5, #1204]          @ class_list_base 
	ldrh	r2, [r0, #26]
	ldr	r2, [r3, +r2, lsl #2]
	ldr	r3, [r5, #1208]          @ mirror_list_base 
	ldrh	r4, [r0, #26]
	ldr	r3, [r3, +r4, lsl #2]
	cmp	r3, #0
	@  It will almost never be zero, so preload tmp2
	ldr	r4, [r5, #1808]          @ task_class_init_marker 
	cmpne	r3, r4
	bne	L197
	@  move instanceclass obj into extra arg
	mov	r1, r2
	.extern	task_barrier
	ldr	r3, =task_barrier
	@  save interpreter state
	str	lr, [fp, #-16]
	bl	interpreter_call_vm_oop
	@  restore interpreter state
	ldr	r7, [fp, #-12]           @ restore locals
	ldr	r8, [fp, #-8]            @ restore cpool
	ldr	lr, [fp, #-16]           @ restore bcp
	@  reload registers
	@  Get two bytes of constant pool entry
	ldrb	r2, [lr, #1]
	ldrb	r0, [lr, #2]
	add	r2, r8, r2, lsl #10
	ldr	r0, [r2, +r0, lsl #2]!
L197:
	@  Call method
	ldr	r2, [r0, #20]
	ldr	r2, [r2]
	ldrh	r4, [r0, #32]
	mov	r4, r4, lsr #9           @ r4 = result type
	ldr	pc, [pc, +r4, lsl #2]
	nop
	.long	L198
	.long	L199
	.long	L200
	.long	L201
	.long	L202
L198:
	@  invoke method with result type == 0
	mov	lr, pc
	mov	pc, r2
	@  restore interpreter state
	ldr	r7, [fp, #-12]           @ restore locals
	ldr	r8, [fp, #-8]            @ restore cpool
	ldr	lr, [fp, #-16]           @ restore bcp
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	@  return type is void
	ldr	r0, [r6], #4
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
L199:
L200:
	@  invoke method with result type == 2
	mov	lr, pc
	mov	pc, r2
	@  restore interpreter state
	ldr	r7, [fp, #-12]           @ restore locals
	ldr	r8, [fp, #-8]            @ restore cpool
	ldr	lr, [fp, #-16]           @ restore bcp
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	@  result is in r0: do nothing
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
L201:
L202:
	@  invoke method with result type == 4
	mov	lr, pc
	mov	pc, r2
	@  restore interpreter state
	ldr	r7, [fp, #-12]           @ restore locals
	ldr	r8, [fp, #-8]            @ restore cpool
	ldr	lr, [fp, #-16]           @ restore bcp
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	@  result is in r0/r1
	str	r1, [r6, #-4]!
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
.LLfe194:
	.pool
	@  End segment: unnamed


	@  ------------------------------------------------------
	@  Start segment:  unnamed
bc_impl_fast_init_new:

	.global	bc_impl_fast_init_new_internal
bc_impl_fast_init_new_internal:
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
L204:
	@  Get two bytes of constant pool entry
	ldrb	r4, [lr, #1]
	ldrb	r1, [lr, #2]
	ldr	r3, [r5, #1204]          @ class_list_base 
	add	r4, r8, r4, lsl #10
	ldr	r1, [r4, +r1, lsl #2]!
	ldr	r2, [r3, +r1, lsl #2]
	ldr	r4, [r5, #1208]          @ mirror_list_base 
	ldr	r3, [r4, +r1, lsl #2]
	ldr	r4, [r5, #1808]          @ task_class_init_marker 
	cmp	r3, r4
	bne	L205
	@  move instanceclass obj into extra arg
	mov	r1, r2
	.extern	task_barrier
	ldr	r3, =task_barrier
	@  save interpreter state
	str	lr, [fp, #-16]
	bl	interpreter_call_vm_oop
	@  restore interpreter state
	ldr	r7, [fp, #-12]           @ restore locals
	ldr	r8, [fp, #-8]            @ restore cpool
	ldr	lr, [fp, #-16]           @ restore bcp
	@  reload registers
	@  Get two bytes of constant pool entry
	ldrb	r4, [lr, #1]
	ldrb	r2, [lr, #2]
	ldr	r3, [r5, #1204]          @ class_list_base 
	add	r4, r8, r4, lsl #10
	ldr	r2, [r4, +r2, lsl #2]!
	ldr	r2, [r3, +r2, lsl #2]
L205:
	@  get _inline_allocation_top
	ldr	r0, [r5, #1060]          @ inline_allocation_top 
	@  get instance size (in bytes) from class
	ldrsh	r1, [r2, #6]
	@  get _inline_allocation_end, and check for overflow
	ldr	r9, [r5, #1064]          @ inline_allocation_end 
	adds	r4, r0, r1
	bcs	L203
	cmp	r4, r9
	bhi	L203
	@  allocation succeeded, set _inline_allocation_top
	str	r4, [r5, #1060]          @ inline_allocation_top 
	@  get prototypical near from class
	ldr	r2, [r2, #12]
	ldrb	r12, [lr, #3]!           @ prefetch next bytecode
	@  set prototypical near in object; no need for write barrier
	str	r2, [r0]
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
L203:
	@  Restore stack state from tos_on_stack
	ldr	r0, [r6], #4             @ cache topmost element of stack
	b	bc_impl_new_internal
.LLfe195:
	.pool
	@  End segment: unnamed


.bss
	@  ------------------------------------------------------
	@  Start segment: Interpreter bytecode counters table

	.global	interpreter_bytecode_counters
interpreter_bytecode_counters:
	@  The bytecode counters table is intentionally left empty.
	@  To use bytecode counters, turn on +PrintBytecodeHistogram
	@  during interpreter loop generation.
	@  End segment: Interpreter bytecode counters table


	@  ------------------------------------------------------
	@  Start segment: Interpreter pair counters table

	.global	interpreter_pair_counters
interpreter_pair_counters:
	@  The pair counters table is intentionally left empty.
	@  To use pair counters, turn on +PrintPairHistogram
	@  during interpreter loop generation.
	@  End segment: Interpreter pair counters table


.text
	@  ------------------------------------------------------
	@  Start segment: Shared entry

	.global	shared_entry
shared_entry:
	@  Get information
	ldr	r3, [r5, #1036]          @ current_thread 
	ldr	r10, [r5, #1040]         @ current_pending_exception 
	ldr	r4, [r3, #72]            @ Thread::last_java_fp
	ldr	r9, [r3, #68]            @ Thread::last_java_sp
	ldr	r12, =0xdeadbeef         @ EntryFrame::FakeReturnAddress
	sub	r6, r6, #36              @ make room on stack for EntryFrame
	add	fp, r6, #28              @ set the fp of the EntryFrame
	@  Store information in the frame
	str	r10, [fp, #-24]          @ current pending exception
	str	r2, [fp, #4]             @ stored obj value
	str	r1, [fp, #-12]           @ stored int value #1
	str	r0, [fp, #-16]           @ stored int value #2
	str	r9, [fp, #-8]            @ stored_last_sp
	str	r4, [fp, #-4]            @ stored_last_fp
	str	lr, [fp]                 @ real_return_address
	str	r12, [fp, #-20]          @ fake_return_address
	@  reset last java frame
	mov	r0, #0
	str	r0, [r3, #68]            @ last_java_sp
	str	r0, [r3, #72]            @ last_java_fp
	@  get pending activation list
	ldr	r4, [r3, #36]
	@  reset pending exception and activation
	str	r0, [r3, #36]
	str	r0, [r5, #1040]          @ current_pending_exception 
L206:
	@  Handle one activation
	ldr	r0, [r4, #12]            @ get next pending activation
	ldr	r10, [r4, #4]            @ argument length
	add	r9, r4, #16              @ pointer to first argument
	cmp	r10, #0
	str	r0, [fp, #-28]           @ save next pending activation
	@  push parameter from entry activation
L207:
	ldrgt	r0, [r9], #8
	strgt	r0, [r6, #-4]!
	subs	r10, r10, #1             @ Decrement the length and check for more parameters
	bgt	L207
	ldr	r0, [r4, #8]             @ method from entry activation
	ldr	r2, [r0, #20]            @ variable_part_offset
	ldr	r2, [r2]                 @ Code to call
	mov	lr, pc
	mov	pc, r2
shared_entry_return_point:
	ldr	r4, [fp, #-28]           @ restore the next pending activation
	ldr	r3, [fp, #-24]           @ speculatively restore pending exception
	cmp	r4, #0                   @ check for more pending activations
	bne	L206
	ldr	r2, [fp, #4]             @ restore return value(s)
	ldr	r1, [fp, #-12]
	ldr	r0, [fp, #-16]
	@  restore pending exception
	str	r3, [r5, #1040]          @ current_pending_exception 
shared_entry_return:
	@  Remove information from the entry frame
	ldr	r3, [r5, #1036]          @ current_thread 
	ldr	r9, [fp, #-8]            @ stored_last_fp
	ldr	r4, [fp, #-4]            @ stored_last_sp
	ldr	lr, [fp]                 @ real_return_address
	add	r6, fp, #8               @ pop entry frame
	mov	fp, r4                   @ restore fp
	str	r9, [r3, #68]            @ restore Thread::last_java_sp
	str	r4, [r3, #72]            @ restore Thread::last_java_fp
	mov	pc, lr
	.pool
	@  End segment: Shared entry


	@  ------------------------------------------------------
	@  Start segment: Shared fast accessors

	.global	shared_fast_getbyte_accessor
shared_fast_getbyte_accessor:
	ldr	r4, [r6], #4             @ pop 'this'
	ldrh	r9, [r0, #30]            @ get field offset (is 2-byte aligned!)
	mov	r2, #1
	ldrsb	r0, [r4, +r9]            @ get field from 'this' object
	mov	pc, lr
.LLfe196:

	.global	shared_fast_getshort_accessor
shared_fast_getshort_accessor:
	ldr	r4, [r6], #4             @ pop 'this'
	ldrh	r9, [r0, #30]            @ get field offset (is 2-byte aligned!)
	mov	r2, #1
	ldrsh	r0, [r4, +r9]            @ get field from 'this' object
	mov	pc, lr
.LLfe197:

	.global	shared_fast_getchar_accessor
shared_fast_getchar_accessor:
	ldr	r4, [r6], #4             @ pop 'this'
	ldrh	r9, [r0, #30]            @ get field offset (is 2-byte aligned!)
	mov	r2, #1
	ldrh	r0, [r4, +r9]            @ get field from 'this' object
	mov	pc, lr
.LLfe198:

	.global	shared_fast_getfloat_accessor
shared_fast_getfloat_accessor:
.LLfe199:

	.global	shared_fast_getint_accessor
shared_fast_getint_accessor:
	ldr	r4, [r6], #4             @ pop 'this'
	ldrh	r9, [r0, #30]            @ get field offset (is 2-byte aligned!)
	mov	r2, #1
	ldr	r0, [r4, +r9]            @ get field from 'this' object
	mov	pc, lr
.LLfe200:

	.global	shared_fast_getdouble_accessor
shared_fast_getdouble_accessor:
.LLfe201:

	.global	shared_fast_getlong_accessor
shared_fast_getlong_accessor:
	ldr	r4, [r6], #4             @ pop 'this'
	ldrh	r9, [r0, #30]            @ get field offset (is 2-byte aligned!)
	mov	r2, #2
	ldr	r0, [r4, +r9]!           @ get field from 'this' object
	ldr	r1, [r4, #4]
	mov	pc, lr
.LLfe202:

	.global	shared_fast_getbyte_static_accessor
shared_fast_getbyte_static_accessor:
	ldr	r4, [r6], #4             @ pop 'this'
	ldrh	r9, [r0, #30]            @ get field offset (is 2-byte aligned!)
	cmp	r4, #0
	beq	L208
	mov	r2, #1
	ldrsb	r0, [r4, +r9]            @ get field from 'this' object
	mov	pc, lr
.LLfe203:

	.global	shared_fast_getshort_static_accessor
shared_fast_getshort_static_accessor:
	ldr	r4, [r6], #4             @ pop 'this'
	ldrh	r9, [r0, #30]            @ get field offset (is 2-byte aligned!)
	cmp	r4, #0
	beq	L208
	mov	r2, #1
	ldrsh	r0, [r4, +r9]            @ get field from 'this' object
	mov	pc, lr
.LLfe204:

	.global	shared_fast_getchar_static_accessor
shared_fast_getchar_static_accessor:
	ldr	r4, [r6], #4             @ pop 'this'
	ldrh	r9, [r0, #30]            @ get field offset (is 2-byte aligned!)
	cmp	r4, #0
	beq	L208
	mov	r2, #1
	ldrh	r0, [r4, +r9]            @ get field from 'this' object
	mov	pc, lr
.LLfe205:

	.global	shared_fast_getfloat_static_accessor
shared_fast_getfloat_static_accessor:
.LLfe206:

	.global	shared_fast_getint_static_accessor
shared_fast_getint_static_accessor:
	ldr	r4, [r6], #4             @ pop 'this'
	ldrh	r9, [r0, #30]            @ get field offset (is 2-byte aligned!)
	cmp	r4, #0
	beq	L208
	mov	r2, #1
	ldr	r0, [r4, +r9]            @ get field from 'this' object
	mov	pc, lr
.LLfe207:

	.global	shared_fast_getdouble_static_accessor
shared_fast_getdouble_static_accessor:
.LLfe208:

	.global	shared_fast_getlong_static_accessor
shared_fast_getlong_static_accessor:
	ldr	r4, [r6], #4             @ pop 'this'
	ldrh	r9, [r0, #30]            @ get field offset (is 2-byte aligned!)
	cmp	r4, #0
	beq	L208
	mov	r2, #2
	ldr	r0, [r4, +r9]!           @ get field from 'this' object
	ldr	r1, [r4, #4]
	mov	pc, lr
.LLfe209:
L208:
	@  Restore stack state and continue with exception throwing
	str	r4, [r6, #-4]!
	b	interpreter_throw_NullPointerException
	.pool
	@  End segment: Shared fast accessors


	@  ------------------------------------------------------
	@  Start segment: Shared call VM
interpreter_call_vm:

	.global	shared_call_vm
shared_call_vm:
	@  Save value of java stack pointer
	mov	r10, r6
	str	lr, [r6, #-4]!
	@  Get the current thread
	ldr	r12, [r5, #1036]         @ current_thread 
	@  LWT Switch stack to primordial thread
	adr	r0, L209
	str	r0, [r6, #-4]!
	str	fp, [r6, #-4]!
	@  Save last java stack pointer and last java frame pointer in thread
	str	r6, [r12, #100]
	str	fp, [r12, #72]
	str	r10, [r12, #68]
	add	r0, r5, #0x400           @ slow add_gp_imm current_thread 1036
	add	r0, r0, #12
	@  call the native entry
	mov	lr, pc
	mov	pc, r3
	@  Save the return values in the thread
	ldr	r2, [r5, #1036]          @ current_thread 
	str	r0, [r2, #112]
	str	r1, [r2, #116]
	@  check for pending entries and if so, check for enough stack space
	ldr	r0, [r2, #36]
	cmp	r0, #0
	beq	L212
	ldr	r3, [r0, #4]
	b	L211
L210:
	@  check parameter size
	ldr	r1, [r0, #4]
	cmp	r3, r1                   @ r3 = max(r3, r1)
	movlt	r3, r1
L211:
	@  get next activation
	ldr	r0, [r0, #12]
	cmp	r0, #0
	bne	L210
	@  convert count to bytes plus a fudge factor
	mov	r3, r3, lsl #3
	@  add in a stack lock for good measure
	add	r3, r3, #28
	ldr	r0, [r2, #100]
	sub	r3, r0, r3
	ldr	r0, [r5, #1024]          @ current_stack_limit 
	cmp	r3, r0
	bgt	L212
	@  need more stack, call into the VM
	add	r0, r5, #0x400           @ slow add_gp_imm current_thread 1036
	add	r0, r0, #12
	mov	r1, r3
	bl	stack_overflow
L212:
	@  Call switch_thread(JVM_TRAPS)
	add	r0, r5, #0x400           @ slow add_gp_imm current_thread 1036
	add	r0, r0, #12
	.extern	switch_thread
	bl	switch_thread
	@  Switch back to the java stack
	ldr	r2, [r5, #1036]          @ current_thread 
	ldr	r6, [r2, #100]
	ldr	fp, [r6], #4
	ldr	lr, [r6], #4
	cmp	lr, pc
	movne	pc, lr
L209:
	ldr	r0, [r5, #1036]          @ current_thread 
	@  Load pending offset activations here and check below
	ldr	r3, [r0, #36]
	@  restore the return values in the thread
	ldr	r1, [r0, #116]
	ldr	r0, [r0, #112]
	mov	r2, #0
	cmp	r3, #0                   @ check for pending activations
	beq	L213                     @ invoke the pending activations
	bl	shared_entry
L213:
	@  reset last java frame in thread
	ldr	r2, [r5, #1036]          @ current_thread 
	mov	r3, #0
	str	r3, [r2, #68]
	str	r3, [r2, #72]
	@  Get pending exceptions
	ldr	r3, [r5, #1040]          @ current_pending_exception 
	ldr	lr, [r6], #4
	@  check for pending exceptions
	cmp	r3, #0
	@  return if no pending exceptions
	moveq	pc, lr
	@  Call shared_call_vm_exception
	mov	r1, r3                   @ argument goes into r1
	mov	r3, #0                   @ clear pending exception
	str	r3, [r5, #1040]          @ current_pending_exception 
	b	shared_call_vm_exception
	.pool
	@  End segment: Shared call VM


	@  ------------------------------------------------------
	@  Start segment: Shared call VM
interpreter_call_vm_oop:

	.global	shared_call_vm_oop
shared_call_vm_oop:
	@  Save value of java stack pointer
	mov	r10, r6
	str	lr, [r6, #-4]!
	@  Get the current thread
	ldr	r12, [r5, #1036]         @ current_thread 
	@  LWT Switch stack to primordial thread
	adr	r0, L214
	str	r0, [r6, #-4]!
	str	fp, [r6, #-4]!
	@  Save last java stack pointer and last java frame pointer in thread
	str	r6, [r12, #100]
	str	fp, [r12, #72]
	str	r10, [r12, #68]
	add	r0, r5, #0x400           @ slow add_gp_imm current_thread 1036
	add	r0, r0, #12
	@  call the native entry
	mov	lr, pc
	mov	pc, r3
	@  Save the return values in the thread
	ldr	r2, [r5, #1036]          @ current_thread 
	str	r0, [r2, #48]!
	@  oop_write_barrier
	ldr	r0, [r5, #1076]          @ bitvector_base 
	ldr	r1, [r5, #1080]          @ bit_selector 
	ldrb	r3, [r0, +r2, lsr #5]!
	mvn	r2, r2, lsr #2
	orr	r3, r3, r1, ror r2
	strb	r3, [r0]
	ldr	r2, [r5, #1036]          @ current_thread 
	str	r0, [r2, #112]
	@  check for pending entries and if so, check for enough stack space
	ldr	r0, [r2, #36]
	cmp	r0, #0
	beq	L217
	ldr	r3, [r0, #4]
	b	L216
L215:
	@  check parameter size
	ldr	r1, [r0, #4]
	cmp	r3, r1                   @ r3 = max(r3, r1)
	movlt	r3, r1
L216:
	@  get next activation
	ldr	r0, [r0, #12]
	cmp	r0, #0
	bne	L215
	@  convert count to bytes plus a fudge factor
	mov	r3, r3, lsl #3
	@  add in a stack lock for good measure
	add	r3, r3, #28
	ldr	r0, [r2, #100]
	sub	r3, r0, r3
	ldr	r0, [r5, #1024]          @ current_stack_limit 
	cmp	r3, r0
	bgt	L217
	@  need more stack, call into the VM
	add	r0, r5, #0x400           @ slow add_gp_imm current_thread 1036
	add	r0, r0, #12
	mov	r1, r3
	bl	stack_overflow
L217:
	@  Call switch_thread(JVM_TRAPS)
	add	r0, r5, #0x400           @ slow add_gp_imm current_thread 1036
	add	r0, r0, #12
	bl	switch_thread
	@  Switch back to the java stack
	ldr	r2, [r5, #1036]          @ current_thread 
	ldr	r6, [r2, #100]
	ldr	fp, [r6], #4
	ldr	lr, [r6], #4
	cmp	lr, pc
	movne	pc, lr
L214:

	.global	shared_call_vm_oop_return
shared_call_vm_oop_return:
	ldr	r0, [r5, #1036]          @ current_thread 
	@  Load pending offset activations here and check below
	ldr	r3, [r0, #36]
	@  restore the return values in the thread
	ldr	r2, [r0, #48]
	mov	r1, #0
	str	r1, [r0, #48]
	cmp	r3, #0                   @ check for pending activations
	beq	L218                     @ invoke the pending activations
	bl	shared_entry
L218:
	mov	r0, r2                   @ move oop return value to r0
	@  reset last java frame in thread
	ldr	r2, [r5, #1036]          @ current_thread 
	mov	r3, #0
	str	r3, [r2, #68]
	str	r3, [r2, #72]
	@  Get pending exceptions
	ldr	r3, [r5, #1040]          @ current_pending_exception 
	ldr	lr, [r6], #4
	@  check for pending exceptions
	cmp	r3, #0
	@  return if no pending exceptions
	moveq	pc, lr
	@  Call shared_call_vm_exception
	mov	r1, r3                   @ argument goes into r1
	mov	r3, #0                   @ clear pending exception
	str	r3, [r5, #1040]          @ current_pending_exception 
	b	shared_call_vm_exception
	.pool
	@  End segment: Shared call VM


	@  ------------------------------------------------------
	@  Start segment: Shared call VM
interpreter_call_vm_exception:

	.global	shared_call_vm_exception
shared_call_vm_exception:
	@  Save value of java stack pointer
	mov	r10, r6
	str	lr, [r6, #-4]!
	sub	r6, r6, #4               @ Leave empty space on stack
	@  Get the current thread
	ldr	r12, [r5, #1036]         @ current_thread 
	@  LWT Switch stack to primordial thread
	adr	r0, L219
	str	r0, [r6, #-4]!
	str	fp, [r6, #-4]!
	@  Save last java stack pointer and last java frame pointer in thread
	str	r6, [r12, #100]
	str	fp, [r12, #72]
	str	r10, [r12, #68]
	add	r0, r5, #0x400           @ slow add_gp_imm current_thread 1036
	add	r0, r0, #12
	@  call the native entry
	.extern	find_exception_frame
	bl	find_exception_frame
	@  Save the return values in the thread
	ldr	r2, [r5, #1036]          @ current_thread 
	str	r0, [r2, #112]
	@  check for pending entries and if so, check for enough stack space
	ldr	r0, [r2, #36]
	cmp	r0, #0
	beq	L222
	ldr	r3, [r0, #4]
	b	L221
L220:
	@  check parameter size
	ldr	r1, [r0, #4]
	cmp	r3, r1                   @ r3 = max(r3, r1)
	movlt	r3, r1
L221:
	@  get next activation
	ldr	r0, [r0, #12]
	cmp	r0, #0
	bne	L220
	@  convert count to bytes plus a fudge factor
	mov	r3, r3, lsl #3
	@  add in a stack lock for good measure
	add	r3, r3, #28
	ldr	r0, [r2, #100]
	sub	r3, r0, r3
	ldr	r0, [r5, #1024]          @ current_stack_limit 
	cmp	r3, r0
	bgt	L222
	@  need more stack, call into the VM
	add	r0, r5, #0x400           @ slow add_gp_imm current_thread 1036
	add	r0, r0, #12
	mov	r1, r3
	bl	stack_overflow
L222:
	@  Call switch_thread(JVM_TRAPS)
	add	r0, r5, #0x400           @ slow add_gp_imm current_thread 1036
	add	r0, r0, #12
	bl	switch_thread
	@  Switch back to the java stack
	ldr	r2, [r5, #1036]          @ current_thread 
	ldr	r6, [r2, #100]
	ldr	fp, [r6], #4
	ldr	lr, [r6], #4
	cmp	lr, pc
	movne	pc, lr
L219:
	ldr	r0, [r5, #1036]          @ current_thread 
	ldr	fp, [r0, #72]
	ldr	r1, [r0, #68]
	sub	r6, r1, #4
	@  Load pending offset activations here and check below
	ldr	r3, [r0, #36]
	@  restore the return values in the thread
	ldr	r0, [r0, #112]
	mov	r2, #0
	cmp	r3, #0                   @ check for pending activations
	beq	L223                     @ invoke the pending activations
	bl	shared_entry
L223:
	cmp	r0, #0
	@  reset last java frame in thread
	ldr	r2, [r5, #1036]          @ current_thread 
	mov	r3, #0
	str	r3, [r2, #68]
	str	r3, [r2, #72]
	@  A return value of zero means the initial entry frame
	bne	L224
	ldr	r1, [fp, #-24]           @ exception is stored in frame
	ldr	lr, [fp]                 @ return address
	add	r6, fp, #8               @ pop entry frame off stack
	str	r1, [r5, #1040]          @ current_pending_exception 
	mov	pc, lr
L224:
	@  Get pending exceptions
	ldr	r3, [r5, #1040]          @ current_pending_exception 
	ldr	lr, [r6], #4
	@  check for pending exceptions
	cmp	r3, #0
	@  return if no pending exceptions
	moveq	pc, lr
	@  Call shared_call_vm_exception
	mov	r1, r3                   @ argument goes into r1
	mov	r3, #0                   @ clear pending exception
	str	r3, [r5, #1040]          @ current_pending_exception 
	b	shared_call_vm_exception
	.pool
	@  End segment: Shared call VM


	@  ------------------------------------------------------
	@  Start segment: Shared monitor enter

	.global	shared_monitor_enter
shared_monitor_enter:
	@  Are there any monitors at all?
	ldr	r2, [fp, #4]
	mov	r1, #0
	sub	r3, fp, #20
	cmp	r3, r2
	beq	L225
	@  Search for empty lock
	@  point at object field of stack lock
	add	r2, r2, #24
	@  object field just before first lock
	add	r3, fp, #4
L227:
	@  Start the loop by checking if the current stack lock is empty
	ldr	r4, [r2], #28
	cmp	r4, #0
	subeq	r1, r2, #52              @ point at beginning of previous lock if empty
	@  Current stack lock object equal to the object from the stack?
	cmp	r4, r0
	beq	L228
	@  Go to next stack lock in monitor block
	cmp	r2, r3
	bne	L227
L228:
	@  Have we found an entry?
	cmp	r1, #0
	bne	L226
	beq	L225

	.global	shared_lock_synchronized_method
shared_lock_synchronized_method:
	sub	r6, r6, #28
	str	r6, [fp, #4]
	add	r1, r6, #0
L226:
	@  r0 contains the object to lock
	@  r1 contains a stack lock in the monitor block
	ldr	r3, [r0]                 @ Get the near object
	str	r0, [r1, #24]            @ Store the object in the stack lock and lock it
	ldr	r4, [r5, #1796]          @ interned_string_near_addr  @ See if this is an interned string
	cmp	r4, r3
	moveq	r4, #0
	@  If going the slow way, zero out real java near for GC 
	streq	r4, [r1, #4]
	beq	L230
	ldr	r4, [r3, #8]             @ object already locked?
	tst	r4, #1
	bne	L229
	@  We have the fast case
	ldr	r2, [r5, #1036]          @ current_thread 
	orr	r4, r4, #1
	str	r4, [r1, #20]
	str	r3, [r1, #4]
	str	r2, [r1]
	@  Clear waiters
	mov	r2, #0
	str	r2, [r1, #8]
	@  Update the near pointer in the object
	add	r2, r1, #12
	str	r2, [r0]
	@  Copy the locked near object to the stack and set lock bit
	@  Copy the first two fields of the near
	ldmia	r3, {r2, r4}
	str	r2, [r1, #12]
	str	r4, [r1, #16]
	mov	pc, lr
L229:
	@  maybe_slow_case
	ldr	r2, [r5, #1036]          @ current_thread 
	ldr	r3, [r3, #-12]           @ thread of object's lock
	mov	r4, #0                   @ set our own real java near field to nul
	str	r4, [r1, #4]
	cmp	r2, r3                   @ is this a recursive lock?
	moveq	pc, lr
L230:
	@  slow_case
	.extern	lock_stack_lock
	ldr	r3, =lock_stack_lock
	b	shared_call_vm
L225:
	@  Allocate a stack lock in the monitor block
	@  Set lock to old expression stack bottom
	ldr	r1, [fp, #4]
	@  Move expression stack top and bottom
	sub	r6, r6, #28
	sub	r1, r1, #28
	str	r1, [fp, #4]
	@  Move the expression stack contents
	mov	r2, r6
	b	L231
L232:
	ldr	r3, [r2, #28]
	str	r3, [r2], #4
L231:
	@  Check if bottom is reached
	cmp	r2, r1
	bne	L232
	b	L226
	.pool
	@  End segment: Shared monitor enter


	@  ------------------------------------------------------
	@  Start segment: Shared monitor exit

	.global	shared_monitor_exit
shared_monitor_exit:
	ldr	r3, [r0]                 @ Get the near object
	ldr	r4, [r5, #1796]          @ interned_string_near_addr  @ See if this is an interned string
	cmp	r4, r3
	bne	L235
	mov	r1, r0
	.extern	unlock_special_stack_lock
	ldr	r3, =unlock_special_stack_lock
	b	shared_call_vm
L235:
	@  Find matching slot in monitor block
	ldr	r1, [fp, #4]             @ end of locks
	sub	r3, fp, #20              @ address of lock just beyond the first
	cmp	r1, r3
	beq	L234
L236:
	@  Is monitor pointed at by lock the right one?
	ldr	r2, [r1, #24]            @ get object field
	cmp	r0, r2                   @ is this the right object?
	beq	L233
	add	r1, r1, #28              @ Go to next lock
	cmp	r1, r3                   @ at the end of the monitor block?
	bne	L236
L234:
	@  Throw an IllegalMonitorStateException
	.extern	illegal_monitor_state_exception
	ldr	r3, =illegal_monitor_state_exception
	b	shared_call_vm

	.global	shared_unlock_synchronized_method
shared_unlock_synchronized_method:
	@  get object field of last lock
	ldr	r0, [fp, #-24]
	@  get address of last lock
	sub	r1, fp, #48
	@  lock better be locked
	cmp	r0, #0
	beq	L234
L233:
	@  r0 contains the object that is being unlocked
	@  r1 contains the stack lock

	@  Get the real java near pointer from the stack lock
	ldr	r3, [r1, #4]
	@  Zero out the object field so that lock is free
	mov	r2, #0
	str	r2, [r1, #24]
	@  Is this the reentrant case?
	cmp	r3, #0
	moveq	pc, lr
	@  Set the object near pointer
	str	r3, [r0]
	cmp	r3, r0
	ldrcc	r2, [r1, #20]
	bcc	L237
	@  oop_write_barrier
	ldr	r2, [r5, #1052]          @ heap_start 
	ldr	r3, [r5, #1256]          @ old_generation_end 
	cmp	r0, r2
	cmpcs	r3, r0
	ldrcs	r2, [r5, #1076]          @ bitvector_base 
	ldrcs	r3, [r5, #1080]          @ bit_selector 
	ldrcsb	r4, [r2, +r0, lsr #5]!
	mvncs	r0, r0, lsr #2
	orrcs	r4, r4, r3, ror r0
	strcsb	r4, [r2]
	ldr	r2, [r1, #20]
L237:
	tst	r2, #2
	moveq	pc, lr
	@  Call the runtime system to signal the waiters
	.extern	signal_waiters
	ldr	r3, =signal_waiters
	b	shared_call_vm
	.pool
	@  End segment: Shared monitor exit


	@  ------------------------------------------------------
	@  Start segment: Invoke pending entries

	.global	invoke_pending_entries
invoke_pending_entries:
	@  Get current thread (may not have gp yet)
	ldr	r0, =_current_thread
	ldr	r0, [r0]
	@  Check if there's pending entries at all
	ldr	r1, [r0, #36]
	cmp	r1, #0
	moveq	pc, lr
	@  Save permanent registers
	sub	fp, fp, r6
	stmfd	r6!, {r4, r5, r7 - r12, lr}
	@  Invoke the pending entries
	mov	r2, #0
	bl	shared_entry
	@  Restore permanent registers, frame pointer and link register
	ldmfd	r6!, {r4, r5, r7 - r12, lr}
	add	fp, fp, r6
	mov	pc, lr
	.pool
	@  End segment: Invoke pending entries


	@  ------------------------------------------------------
	@  Start segment: Call on primoridal stack

	.global	call_on_primordial_stack
call_on_primordial_stack:
	@  Get target method
	mov	r2, r0
	ldr	r1, [r5, #1036]          @ current_thread 
	@  Save return address and fp on java stack
	str	lr, [r6, #-4]!
	str	fp, [r6, #-4]!
	str	r6, [r1, #100]
	@  Call r2(THREAD)
	add	r0, r5, #0x400           @ slow add_gp_imm current_thread 1036
	add	r0, r0, #12
	mov	lr, pc
	mov	pc, r2
	@  When we return, we may be on a different thread
	ldr	r1, [r5, #1036]          @ current_thread 
	ldr	r6, [r1, #100]
	ldr	lr, [r6, #4]
	ldr	fp, [r6], #8
	mov	pc, lr
	.pool
	@  End segment: Call on primoridal stack


	@  ------------------------------------------------------
	@  Start segment: Clears a large range of memory

	.global	fast_memclear
fast_memclear:
	stmfd	sp!, {r4 - r7, lr}
	subs	r1, r1, #0x400
	mov	r2, #0
	mov	r3, #0
	mov	r4, #0
	mov	r5, #0
	mov	r6, #0
	mov	r7, #0
	mov	r12, #0
	mov	lr, #0
	bcc	L239
L238:
	subs	r1, r1, #0x400
	stmia	r0!, {r2 - r7, r12, lr}
	stmia	r0!, {r2 - r7, r12, lr}
	stmia	r0!, {r2 - r7, r12, lr}
	stmia	r0!, {r2 - r7, r12, lr}
	stmia	r0!, {r2 - r7, r12, lr}
	stmia	r0!, {r2 - r7, r12, lr}
	stmia	r0!, {r2 - r7, r12, lr}
	stmia	r0!, {r2 - r7, r12, lr}
	stmia	r0!, {r2 - r7, r12, lr}
	stmia	r0!, {r2 - r7, r12, lr}
	stmia	r0!, {r2 - r7, r12, lr}
	stmia	r0!, {r2 - r7, r12, lr}
	stmia	r0!, {r2 - r7, r12, lr}
	stmia	r0!, {r2 - r7, r12, lr}
	stmia	r0!, {r2 - r7, r12, lr}
	stmia	r0!, {r2 - r7, r12, lr}
	stmia	r0!, {r2 - r7, r12, lr}
	stmia	r0!, {r2 - r7, r12, lr}
	stmia	r0!, {r2 - r7, r12, lr}
	stmia	r0!, {r2 - r7, r12, lr}
	stmia	r0!, {r2 - r7, r12, lr}
	stmia	r0!, {r2 - r7, r12, lr}
	stmia	r0!, {r2 - r7, r12, lr}
	stmia	r0!, {r2 - r7, r12, lr}
	stmia	r0!, {r2 - r7, r12, lr}
	stmia	r0!, {r2 - r7, r12, lr}
	stmia	r0!, {r2 - r7, r12, lr}
	stmia	r0!, {r2 - r7, r12, lr}
	stmia	r0!, {r2 - r7, r12, lr}
	stmia	r0!, {r2 - r7, r12, lr}
	stmia	r0!, {r2 - r7, r12, lr}
	stmia	r0!, {r2 - r7, r12, lr}
	bcs	L238
L239:
	tst	r1, #0x200
	beq	L240
	stmia	r0!, {r2 - r7, r12, lr}
	stmia	r0!, {r2 - r7, r12, lr}
	stmia	r0!, {r2 - r7, r12, lr}
	stmia	r0!, {r2 - r7, r12, lr}
	stmia	r0!, {r2 - r7, r12, lr}
	stmia	r0!, {r2 - r7, r12, lr}
	stmia	r0!, {r2 - r7, r12, lr}
	stmia	r0!, {r2 - r7, r12, lr}
	stmia	r0!, {r2 - r7, r12, lr}
	stmia	r0!, {r2 - r7, r12, lr}
	stmia	r0!, {r2 - r7, r12, lr}
	stmia	r0!, {r2 - r7, r12, lr}
	stmia	r0!, {r2 - r7, r12, lr}
	stmia	r0!, {r2 - r7, r12, lr}
	stmia	r0!, {r2 - r7, r12, lr}
	stmia	r0!, {r2 - r7, r12, lr}
L240:
	tst	r1, #0x100
	beq	L241
	stmia	r0!, {r2 - r7, r12, lr}
	stmia	r0!, {r2 - r7, r12, lr}
	stmia	r0!, {r2 - r7, r12, lr}
	stmia	r0!, {r2 - r7, r12, lr}
	stmia	r0!, {r2 - r7, r12, lr}
	stmia	r0!, {r2 - r7, r12, lr}
	stmia	r0!, {r2 - r7, r12, lr}
	stmia	r0!, {r2 - r7, r12, lr}
L241:
	tst	r1, #128
	beq	L242
	stmia	r0!, {r2 - r7, r12, lr}
	stmia	r0!, {r2 - r7, r12, lr}
	stmia	r0!, {r2 - r7, r12, lr}
	stmia	r0!, {r2 - r7, r12, lr}
L242:
	tst	r1, #64
	stmneia	r0!, {r2 - r7, r12, lr}
	stmneia	r0!, {r2 - r7, r12, lr}
	tst	r1, #32
	stmneia	r0!, {r2 - r7, r12, lr}
	tst	r1, #16
	stmneia	r0!, {r4 - r7}
	movs	r1, r1, lsl #29
	stmcsia	r0!, {r2, r3}
	strne	r2, [r0]
	ldmfd	sp!, {r4 - r7, pc}
	.pool
	@  End segment: Clears a large range of memory


	@  ------------------------------------------------------
	@  Start segment: Brute force icache flushing

	.global	arm_flush_icache
arm_flush_icache:
	mrs	r2, cpsr
	add	r1, r0, r1
	bic	r0, r0, #63
	add	r1, r1, #63
	bic	r1, r1, #63
	@  Are we in user mode?
	tst	r2, #15
	bne	non_user_mode_flush
user_mode_flush:
	mov	r2, #0
	swi	0x9f0002
	mov	pc, lr
L243:
	mcr	p15, 0, r0, c7, c10, 1   @ clean data cache line
	mcr	p15, 0, r0, c7, c5, 1    @ invalidate instruction line cache
	add	r0, r0, #32
	mcr	p15, 0, r0, c7, c14, 1   @ clean and invalidate data line cache
	mcr	p15, 0, r0, c7, c5, 1    @ invalidate instruction line cache
	add	r0, r0, #32
non_user_mode_flush:
	cmp	r0, r1
	bcc	L243
	mov	r0, #0
	mcr	p15, 0, r0, c7, c10, 4   @ drain write buffer
	@  CPUWAIT
	mrc	p15, 0, r0, c2, c0, 0    @ wait until p15 completes operation
	mov	r0, r0
	sub	pc, pc, #4
	mov	pc, lr
	.pool
	@  End segment: Brute force icache flushing


.text
	@  ------------------------------------------------------
	@  Start segment: Interpreter exception throwers

	.global	interpreter_throw_ArrayIndexOutOfBoundsException
interpreter_throw_ArrayIndexOutOfBoundsException:
	.extern	array_index_out_of_bounds_exception
	ldr	r3, =array_index_out_of_bounds_exception
	@  save interpreter state
	str	lr, [fp, #-16]
	bl	interpreter_call_vm

	.global	interpreter_throw_NullPointerException_tos_cached
interpreter_throw_NullPointerException_tos_cached:
	str	r0, [r6, #-4]!

	.global	interpreter_throw_NullPointerException
interpreter_throw_NullPointerException:
	.extern	null_pointer_exception
	ldr	r3, =null_pointer_exception
	@  save interpreter state
	str	lr, [fp, #-16]
	bl	interpreter_call_vm

	.global	interpreter_throw_IncompatibleClassChangeError
interpreter_throw_IncompatibleClassChangeError:
	.extern	incompatible_class_change_error
	ldr	r3, =incompatible_class_change_error
	@  save interpreter state
	str	lr, [fp, #-16]
	bl	interpreter_call_vm
	.pool
	@  End segment: Interpreter exception throwers


	@  ------------------------------------------------------
	@  Start segment: Interpreter deoptimization entry

	.global	interpreter_deoptimization_entry
interpreter_deoptimization_entry:
	@  restore interpreter state
	ldr	r7, [fp, #-12]           @ restore locals
	ldr	r8, [fp, #-8]            @ restore cpool
	ldr	lr, [fp, #-16]           @ restore bcp
	ldrb	r12, [lr]                @ prefetch next bytecode
	@  dispatch
	ldr	r12, [r5, +r12, lsl #2]  @ get implementation of bytecode
	@  Restore stack state from tos_on_stack
	ldr	r0, [r6], #4             @ cache topmost element of stack
	mov	pc, r12                  @ go to next bytecode
	.pool
	@  End segment: Interpreter deoptimization entry


	@  ------------------------------------------------------
	@  Start segment: Interpreter call timer_tick

	.global	interpreter_timer_tick
interpreter_timer_tick:
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	.extern	timer_tick
	ldr	r3, =timer_tick
	@  save interpreter state
	str	lr, [fp, #-16]
	bl	interpreter_call_vm
	@  restore interpreter state
	ldr	r7, [fp, #-12]           @ restore locals
	ldr	r8, [fp, #-8]            @ restore cpool
	ldr	lr, [fp, #-16]           @ restore bcp
	ldrb	r12, [lr]                @ prefetch next bytecode
	@  Restore stack state from tos_on_stack
	ldr	r0, [r6], #4             @ cache topmost element of stack
	@  dispatch
	ldr	pc, [r5, +r12, lsl #2]
	.pool
	@  End segment: Interpreter call timer_tick


	@  ------------------------------------------------------
	@  Start segment: Primordial to current thread

	.global	primordial_to_current_thread
primordial_to_current_thread:
	@  save permanent registers (including return address)
	stmfd	sp!, {r3 - fp}
	str	lr, [sp, #-4]!
	@  Set up global pointer
	ldr	r5, =gp_base_label
	@  Get current thread
	ldr	r1, [r5, #1036]          @ current_thread 
	@  Save primordial stack pointer
	str	sp, [r5, #1252]          @ primordial_sp 
	@  Get new stack pointer
	ldr	r6, [r1, #100]
	@  Go to code
	ldr	lr, [r6, #4]
	ldr	fp, [r6], #8
	mov	pc, lr

	.global	start_lightweight_thread_asm
start_lightweight_thread_asm:
	@  Set up global pointer
	ldr	r5, =gp_base_label
	@  Invoke pending entries unless the thread is being terminated
	ldr	r0, [r5, #1036]          @ current_thread 
	@  r1 = THREAD->status();
	ldr	r1, [r0, #104]
	mov	r2, #0
	@  if ((r1 & THREAD_TERMINATING) != 0) {
	tst	r1, #0x40000000
	@    THREAD->pending_entries = NULL;
	strne	r2, [r0, #36]
	@  } else {
	@    invoke_pending_entries(THREAD)
	bne	L245
	bl	invoke_pending_entries
L245:
	@  }
	@  if (!TestCompiler) {
	ldr	r0, [r6, #16]
	ldr	r1, [r5, #1040]          @ current_pending_exception 
	cmp	r0, #0
	bne	L244
	@    if (Thread::current_has_pending_exception()) {
	@      call_on_primordial_stack(lightweight_thread_uncaught_exception);
	@    }
	cmp	r1, #0
	beq	L246
	ldr	r0, [r6, #4]
	bl	call_on_primordial_stack
L246:
	@    call_on_primordial_stack(finish);
	ldr	r0, [r6, #8]
	bl	call_on_primordial_stack
	@    invoke_pending_entries(THREAD);
	ldr	r0, [r5, #1036]          @ current_thread 
	bl	invoke_pending_entries
	@    force_terminated()
	ldr	r0, [r6, #12]
	bl	call_on_primordial_stack
	@  }
	@  Terminate the task if no other threads on it
	.extern	thread_task_cleanup
	ldr	r0, =thread_task_cleanup
	bl	call_on_primordial_stack
	@    invoke_pending_entries(THREAD);
	ldr	r0, [r5, #1036]          @ current_thread 
	bl	invoke_pending_entries
L244:
	@  call_on_primordial_stack(lightweight_thread_exit);
	ldr	r0, [r6]
	bl	call_on_primordial_stack
	@  GUARANTEE(Scheduler::_next_runnable_thread == NULL
	.extern	_next_runnable_thread
	ldr	r0, =_next_runnable_thread
	ldr	r0, [r0]
	cmp	r0, #0
	swine	0x9f0001
	@  current_thread_to_primordial();
	bl	current_thread_to_primordial_fast
	swi	0x9f0001
	.pool
	@  End segment: Primordial to current thread


	@  ------------------------------------------------------
	@  Start segment: Current thread to primordial

	.global	current_thread_to_primordial
current_thread_to_primordial:
	@  Set up global pointer, as we can be called from C code
	ldr	r5, =gp_base_label

	.global	current_thread_to_primordial_fast
current_thread_to_primordial_fast:
	ldr	sp, [r5, #1252]          @ primordial_sp 
	@  restore permanent registers (including return address)
	ldr	lr, [sp], #4
	ldmfd	sp!, {r3 - fp}
	mov	pc, lr
	.pool
	@  End segment: Current thread to primordial


	.extern	_newobject
.text
	@  ------------------------------------------------------
	@  Start segment: Compiler idiv irem

	.global	interpreter_idiv_irem
interpreter_idiv_irem:
	@  load one argument
	mov	r1, r0
	@  load other argument
	ldr	r0, [r6], #4
	@  Register r0 holds the dividend, r1 the divisor

	.global	compiler_idiv_irem
compiler_idiv_irem:
	@  Copy Divisor to another register and free r1
	@  This instruction also resets V flag, which is used below by BLE
	adds	r12, r1, #0
	@  Check for special cases: Dividend < 0 or Divisor <= 0
	orrs	r3, r12, r0, asr #32
	ble	L282
L281:
	mov	r1, #0
	@  Count quotient bits
	cmp	r0, r12
	bls	L247
	cmp	r0, r12, lsl #1
	bls	L248
	cmp	r0, r12, lsl #2
	bls	L249
	cmp	r0, r12, lsl #3
	bls	L250
	cmp	r0, r12, lsl #4
	bls	L251
	cmp	r0, r12, lsl #5
	bls	L252
	cmp	r0, r12, lsl #6
	bls	L253
	cmp	r0, r12, lsl #7
	bls	L254
	cmp	r0, r12, lsl #8
	bls	L255
	cmp	r0, r12, lsl #9
	bls	L256
	cmp	r0, r12, lsl #10
	bls	L257
	cmp	r0, r12, lsl #11
	bls	L258
	cmp	r0, r12, lsl #12
	bls	L259
	cmp	r0, r12, lsl #13
	bls	L260
	cmp	r0, r12, lsl #14
	bls	L261
	cmp	r0, r12, lsl #15
	bls	L262
	cmp	r0, r12, lsl #16
	bls	L263
	cmp	r0, r12, lsl #17
	bls	L264
	cmp	r0, r12, lsl #18
	bls	L265
	cmp	r0, r12, lsl #19
	bls	L266
	cmp	r0, r12, lsl #20
	bls	L267
	cmp	r0, r12, lsl #21
	bls	L268
	cmp	r0, r12, lsl #22
	bls	L269
	cmp	r0, r12, lsl #23
	bls	L270
	cmp	r0, r12, lsl #24
	bls	L271
	cmp	r0, r12, lsl #25
	bls	L272
	cmp	r0, r12, lsl #26
	bls	L273
	cmp	r0, r12, lsl #27
	bls	L274
	cmp	r0, r12, lsl #28
	bls	L275
	cmp	r0, r12, lsl #29
	bls	L276
	cmp	r0, r12, lsl #30
	bls	L277
	@  Unrolled loop of 32 division steps
L278:
	cmp	r0, r12, lsl #31
	subcs	r0, r0, r12, lsl #31
	addcs	r1, r1, #0x80000000
L277:
	cmp	r0, r12, lsl #30
	subcs	r0, r0, r12, lsl #30
	addcs	r1, r1, #0x40000000
L276:
	cmp	r0, r12, lsl #29
	subcs	r0, r0, r12, lsl #29
	addcs	r1, r1, #0x20000000
L275:
	cmp	r0, r12, lsl #28
	subcs	r0, r0, r12, lsl #28
	addcs	r1, r1, #0x10000000
L274:
	cmp	r0, r12, lsl #27
	subcs	r0, r0, r12, lsl #27
	addcs	r1, r1, #0x8000000
L273:
	cmp	r0, r12, lsl #26
	subcs	r0, r0, r12, lsl #26
	addcs	r1, r1, #0x4000000
L272:
	cmp	r0, r12, lsl #25
	subcs	r0, r0, r12, lsl #25
	addcs	r1, r1, #0x2000000
L271:
	cmp	r0, r12, lsl #24
	subcs	r0, r0, r12, lsl #24
	addcs	r1, r1, #0x1000000
L270:
	cmp	r0, r12, lsl #23
	subcs	r0, r0, r12, lsl #23
	addcs	r1, r1, #0x800000
L269:
	cmp	r0, r12, lsl #22
	subcs	r0, r0, r12, lsl #22
	addcs	r1, r1, #0x400000
L268:
	cmp	r0, r12, lsl #21
	subcs	r0, r0, r12, lsl #21
	addcs	r1, r1, #0x200000
L267:
	cmp	r0, r12, lsl #20
	subcs	r0, r0, r12, lsl #20
	addcs	r1, r1, #0x100000
L266:
	cmp	r0, r12, lsl #19
	subcs	r0, r0, r12, lsl #19
	addcs	r1, r1, #0x80000
L265:
	cmp	r0, r12, lsl #18
	subcs	r0, r0, r12, lsl #18
	addcs	r1, r1, #0x40000
L264:
	cmp	r0, r12, lsl #17
	subcs	r0, r0, r12, lsl #17
	addcs	r1, r1, #0x20000
L263:
	cmp	r0, r12, lsl #16
	subcs	r0, r0, r12, lsl #16
	addcs	r1, r1, #0x10000
L262:
	cmp	r0, r12, lsl #15
	subcs	r0, r0, r12, lsl #15
	addcs	r1, r1, #0x8000
L261:
	cmp	r0, r12, lsl #14
	subcs	r0, r0, r12, lsl #14
	addcs	r1, r1, #0x4000
L260:
	cmp	r0, r12, lsl #13
	subcs	r0, r0, r12, lsl #13
	addcs	r1, r1, #0x2000
L259:
	cmp	r0, r12, lsl #12
	subcs	r0, r0, r12, lsl #12
	addcs	r1, r1, #0x1000
L258:
	cmp	r0, r12, lsl #11
	subcs	r0, r0, r12, lsl #11
	addcs	r1, r1, #0x800
L257:
	cmp	r0, r12, lsl #10
	subcs	r0, r0, r12, lsl #10
	addcs	r1, r1, #0x400
L256:
	cmp	r0, r12, lsl #9
	subcs	r0, r0, r12, lsl #9
	addcs	r1, r1, #0x200
L255:
	cmp	r0, r12, lsl #8
	subcs	r0, r0, r12, lsl #8
	addcs	r1, r1, #0x100
L254:
	cmp	r0, r12, lsl #7
	subcs	r0, r0, r12, lsl #7
	addcs	r1, r1, #128
L253:
	cmp	r0, r12, lsl #6
	subcs	r0, r0, r12, lsl #6
	addcs	r1, r1, #64
L252:
	cmp	r0, r12, lsl #5
	subcs	r0, r0, r12, lsl #5
	addcs	r1, r1, #32
L251:
	cmp	r0, r12, lsl #4
	subcs	r0, r0, r12, lsl #4
	addcs	r1, r1, #16
L250:
	cmp	r0, r12, lsl #3
	subcs	r0, r0, r12, lsl #3
	addcs	r1, r1, #8
L249:
	cmp	r0, r12, lsl #2
	subcs	r0, r0, r12, lsl #2
	addcs	r1, r1, #4
L248:
	cmp	r0, r12, lsl #1
	subcs	r0, r0, r12, lsl #1
	addcs	r1, r1, #2
L247:
	cmp	r0, r12
	subcs	r0, r0, r12
	addcs	r1, r1, #1
	mov	pc, lr
L282:
	@  set bit 0 of temp register if Divisor < 0
	cmp	r12, #0
	beq	L279
	mov	r3, r12, lsr #31
	rsblt	r12, r12, #0
	@  set bit 1 of temp register if Dividend < 0
	cmp	r0, #0
	orrlt	r3, r3, #2
	rsblt	r0, r0, #0
	@  Save the original return address and adjust lr to point to
	@  the appropriate sign-correction routine
	mov	r2, lr
	add	lr, pc, r3, lsl #4
	b	L281
	@  Should not reach here - for alignment purpose only
	swi	0x9f0001
	swi	0x9f0001
	swi	0x9f0001
	swi	0x9f0001
	@  Divisor < 0 return point
	rsb	r1, r1, #0
	mov	pc, r2
	nop
	nop
	@  Dividend < 0 return point
	rsb	r0, r0, #0
	rsb	r1, r1, #0
	mov	pc, r2
	nop
	@  Dividend < 0 && Divisor < 0 return point
	rsb	r0, r0, #0
	mov	pc, r2
	nop
	nop
L279:
	@  We must check here if we were called from interpreter
	ldr	r2, =called_from_bc_idiv
	ldr	r3, =called_from_bc_irem
	cmp	lr, r2
	cmpne	lr, r3
	bne	L280
	@  Put the arguments back on the stack
	str	r12, [r6, #-4]!
	str	r4, [fp, #-16]           @ store bcp in frame
	@  Change state to tos_on_stack
	str	r0, [r6, #-4]!           @ uncache topmost element of stack
	@  Fall through to compiler code
L280:
	@  Division by zero from the compiler
	.extern	division_by_zero_exception
	ldr	r3, =division_by_zero_exception
	b	shared_call_vm
	.pool
	@  End segment: Compiler idiv irem


.text
	@  ------------------------------------------------------
	@  Start segment: Native entry points for math functions
	.extern	jvm_sin
	.extern	jvm_cos
	.extern	jvm_tan
	.extern	jvm_sqrt
	.extern	jvm_floor
	.extern	jvm_ceil

	.global	native_math_sin_entry
native_math_sin_entry:
	ldr	r0, [r6], #4
	ldr	r1, [r6], #4
	mov	r7, lr
	bl	jvm_sin
	mov	r2, #2
	mov	pc, r7

	.global	native_math_cos_entry
native_math_cos_entry:
	ldr	r0, [r6], #4
	ldr	r1, [r6], #4
	mov	r7, lr
	bl	jvm_cos
	mov	r2, #2
	mov	pc, r7

	.global	native_math_tan_entry
native_math_tan_entry:
	ldr	r0, [r6], #4
	ldr	r1, [r6], #4
	mov	r7, lr
	bl	jvm_tan
	mov	r2, #2
	mov	pc, r7

	.global	native_math_sqrt_entry
native_math_sqrt_entry:
	ldr	r0, [r6], #4
	ldr	r1, [r6], #4
	mov	r7, lr
	bl	jvm_sqrt
	mov	r2, #2
	mov	pc, r7

	.global	native_math_ceil_entry
native_math_ceil_entry:
	ldr	r0, [r6], #4
	ldr	r1, [r6], #4
	mov	r7, lr
	bl	jvm_ceil
	mov	r2, #2
	mov	pc, r7

	.global	native_math_floor_entry
native_math_floor_entry:
	ldr	r0, [r6], #4
	ldr	r1, [r6], #4
	mov	r7, lr
	bl	jvm_floor
	mov	r2, #2
	mov	pc, r7
	.pool
	@  End segment: Native entry points for math functions


	@  ------------------------------------------------------
	@  Start segment: java.lang.String.charAt

	.global	native_string_charAt_entry
native_string_charAt_entry:
	@  r2 = string
	ldr	r2, [r6, #4]
	@  r3 = index
	ldr	r3, [r6]
	@  r4 = count
	ldr	r4, [r2, #12]
	@  r9 = array
	ldr	r9, [r2, #4]
	@  r10 = offset
	ldr	r10, [r2, #8]
	@  if ((unsigned int) index >= (unsigned int) count) goto bailout;
	cmp	r3, r4
	bcs	L283
	@  return: r0 = array[index + offset]
	add	r3, r3, r10
	add	r9, r9, r3, lsl #1
	ldrh	r0, [r9, #8]
	mov	r2, #1
	@  remove arguments from the stack
	add	r6, r6, #8
	mov	pc, lr
L283:
	@  Bail out to the general charAt() implementation
	b	interpreter_method_entry
	.pool
	@  End segment: java.lang.String.charAt


	@  ------------------------------------------------------
	@  Start segment: java.lang.String(java.lang.StringBuffer)

	.global	native_string_init_entry
native_string_init_entry:
	@  Get string, buffer from stack
	ldmfd	r6!, {r1, r2}
	@  Receiver or str are null?
	tst	r2, r1
	beq	L284
	ldmia	r1, {r3, r4, r10}
	ldr	r9, [r3, #8]
	mov	r12, #1
	@  Check the buffer lock bit.
	ands	r9, r9, #1
	bne	L284
	@  Note: r9 is zero at this point
	strb	r12, [r1, #12]
	stmib	r2, {r4, r9, r10}
	mov	r2, #0
	mov	pc, lr
L284:
	@  Bail out to the general <init>() implementation
	stmfd	r6!, {r1, r2}
	b	interpreter_method_entry
	.pool
	@  End segment: java.lang.String(java.lang.StringBuffer)


	@  ------------------------------------------------------
	@  Start segment: java.lang.String.equals(java.lang.Object)

	.global	native_string_equals_entry
native_string_equals_entry:
	@  get strings to compare
	ldr	r4, [r6], #4
	ldr	r2, [r6], #4
	@  Null receiver?
	cmp	r2, #0
	beq	L290
	@  Identity?
	cmp	r2, r4
	beq	L291
	@  Null argument?
	cmp	r4, #0
	@  Get str0.class, str1.class
	ldrne	r10, [r2]
	ldrne	r12, [r4]
	beq	L292
	cmp	r10, r12
	@  Get str0.blueprint, str1.blueprint
	ldrne	r10, [r10]
	ldrne	r12, [r12]
	@  Different classes?
	cmpne	r10, r12
	@  Get str0.count, str1.count
	ldreq	r10, [r2, #12]
	ldreq	r12, [r4, #12]
	@  Different length?
	cmpeq	r10, r12
	bne	L292
	@  Get str0.value[], str1.value[]
	ldr	r3, [r2, #4]
	ldr	r9, [r4, #4]
	@  Get str0.offset, str1.offset
	ldr	r2, [r2, #8]
	ldr	r4, [r4, #8]
	@  Compute start of character data
	add	r3, r3, #8
	add	r9, r9, #8
	add	r3, r3, r2, lsl #1
	add	r9, r9, r4, lsl #1
	@  Small string?
	cmp	r10, #2
	ble	L289
	@  Both strings are aligned?
	orr	r12, r3, r9
	tst	r12, #3
	@  Unaligned strings are uncommon - go out of the fast path ASAP
	bne	L288
L287:
	@  Get number of words
	movs	r10, r10, lsr #1
	@  Count word-aligned?
	ldrcc	r2, [r3], #4
	ldrcc	r4, [r9], #4
	bcc	L285
	@  Get offset of the last char
	mov	r12, r10, lsl #2
	@  Compare last char
	ldrh	r2, [r3, +r12]
	ldrh	r4, [r9, +r12]
	cmp	r2, r4
	ldreq	r2, [r3], #4
	ldreq	r4, [r9], #4
	bne	L292
L285:
	cmp	r2, r4
	bne	L292
	subs	r10, r10, #1
	ldrne	r2, [r3], #4
	ldrne	r4, [r9], #4
	beq	L291
	cmp	r2, r4
	bne	L292
	subs	r10, r10, #1
	ldrne	r2, [r3], #4
	ldrne	r4, [r9], #4
	bne	L285
L291:
	@  Return true
	mov	r0, #1
	mov	r2, #1
	@  continue in caller
	mov	pc, lr
L288:
	@  Check relative alignment
	eor	r12, r3, r9
	tst	r12, #3
	bne	L286
	@  Strings are both unaligned
	@  Compare first char and become word-aligned
	ldrh	r2, [r3], #2
	ldrh	r4, [r9], #2
	cmp	r2, r4
	subeq	r10, r10, #1
	beq	L287
L292:
	@  Return false
	mov	r0, #0
	mov	r2, #1
	@  continue in caller
	mov	pc, lr
L289:
	@  Empty string?
	cmp	r10, #0
	beq	L291
L286:
	@  Char-aligned loop
	ldrh	r2, [r3], #2
	ldrh	r4, [r9], #2
	cmp	r2, r4
	bne	L292
	subs	r10, r10, #1
	bne	L286
	@  Return true
	mov	r0, #1
	mov	r2, #1
	@  continue in caller
	mov	pc, lr
L290:
	@  We have some sort of error
	str	r2, [r6, #-4]!
	str	r4, [r6, #-4]!
	b	interpreter_method_entry
	.pool
	@  End segment: java.lang.String.equals(java.lang.Object)


	@  ------------------------------------------------------
	@  Start segment: java.lang.String.indexOf(java.lang.String)

	.global	native_string_indexof0_string_entry
native_string_indexof0_string_entry:
	@  Get receiver, str from stack
	ldmfd	r6!, {r2, r3}
	@  Set fromIndex to zero
	mov	r1, #0
	@  Neither receiver nor str are null?
	tst	r3, r2
	bne	L293
	@  Bailout to interpreter
	stmfd	r6!, {r2, r3}
	b	interpreter_method_entry

	.global	native_string_indexof_string_entry
native_string_indexof_string_entry:
	@  Get receiver, str, fromIndex from stack
	ldmfd	r6!, {r1 - r3}
	@  if (fromIndex < 0) { fromIndex = 0; }
	cmp	r1, #0
	movlt	r1, #0
	@  Receiver or str are null?
	tst	r3, r2
	beq	L294
L293:
	@  Get this.count, str.count
	ldr	r4, [r3, #12]
	ldr	r9, [r2, #12]
	@  fromIndex >= this.count ? - uncommon case
	cmp	r1, r4
	bge	L295
	@  if (str.count <= 0) { return fromIndex; }
	cmp	r9, #0
	beq	L296
	@  Get this.value[], str.value[]
	ldr	r10, [r3, #4]
	ldr	r12, [r2, #4]
	@  Get this.offset, str.offset
	ldr	r3, [r3, #8]
	ldr	r2, [r2, #8]
	@  Compute start of character data
	add	r10, r10, #8
	add	r12, r12, #8
	add	r10, r10, r3, lsl #1
	add	r12, r12, r2, lsl #1
	@  Compute max pointer to start of a substring = 
	@      = this.value + this.offset + this.count - str.count
	sub	r0, r4, r9
	add	r0, r10, r0, lsl #1
	@  Compute start of a substring = 
	@      = this.value + this.offset + fromIndex
	add	r1, r10, r1, lsl #1
	@  Compute pointer after the end of substring = 
	@      = str.value + str.offset + str.count * 2
	add	r9, r12, r9, lsl #1
	@  Load first character of a substring
	@   and increment pointer to substring start
	ldrh	r4, [r12], #2
	stmfd	sp!, {r5 - r7, lr}
	b	L297
L295:
	@  if (fromIndex >= count) {
	@      if (count == 0 && fromIndex == 0 && str.count == 0) {
	@          return 0;
	@      }
	@      return -1;
	@  }
	orrs	r1, r1, r9
	mvnne	r1, #0
L296:
	@  Subtring found at fromIndex
	mov	r0, r1
	mov	r2, #1
	@  continue in caller
	mov	pc, lr
L298:
	ldrh	r5, [r1], #2
	cmp	r5, r4
	beq	L299
L297:
	cmp	r1, r0
	ble	L298
	@  Substring not found - return -1;
	mvn	r0, #0
	mov	r2, #1
	@  continue in caller
	ldmfd	sp!, {r5 - r7, pc}
L299:
	mov	r7, r1
	mov	lr, r12
	b	L300
L301:
	ldrh	r5, [r7], #2
	ldrh	r6, [lr], #2
	cmp	r5, r6
	bne	L297
L300:
	cmp	lr, r9
	blt	L301
	@  Substring found - return index;
	sub	r0, r1, r10
	sub	r0, r0, #2
	mov	r0, r0, lsr #1
	mov	r2, #1
	@  continue in caller
	ldmfd	sp!, {r5 - r7, pc}
L294:
	@  Bailout to interpreter
	stmfd	r6!, {r1 - r3}
	b	interpreter_method_entry
	.pool
	@  End segment: java.lang.String.indexOf(java.lang.String)


	@  ------------------------------------------------------
	@  Start segment: java.lang.String.indexOf

	.global	native_string_indexof_entry
native_string_indexof_entry:
	@  [[[Hand coded method from java.lang.String]]]
	@      public int indexOf(int ch) {
	@          return indexOf(ch, 0);
	@      }
	@  return address is in lr
	ldr	r0, [r6], #4
	cmp	r0, #0
	movlt	r0, #0
	b	L302

	.global	native_string_indexof0_entry
native_string_indexof0_entry:
	@  [[[Hand coded method from java.lang.String]]]
	@      public int indexOf(int ch, int fromIndex) {
	@          int max = offset + count;
	@          char v[] = value;

	@          if (fromIndex < 0) {
	@              fromIndex = 0;
	@          } else if (fromIndex >= count) {
	@              // Note: fromIndex might be near -1>>>1.
	@              return -1;
	@          }
	@          for (int i = offset + fromIndex ; i < max ; i++) {
	@              if (v[i] == ch) {
	@                  return i - offset;
	@              }
	@          }
	@          return -1;
	@      }
	@  return address is in lr
	@  Get fromIndex, ch, string
	mov	r0, #0
L302:
	ldr	r2, [r6], #4
	ldr	r3, [r6], #4
	ldr	r10, [r3, #12]
	ldr	r1, [r3, #4]
	ldr	r4, [r3, #8]
	cmp	r0, r10
	bge	L305
	add	r1, r1, #8
	add	r1, r1, r4, lsl #1
	add	r4, r1, r10, lsl #1
	mov	r10, r1
	add	r1, r1, r0, lsl #1
L303:
	ldrh	r0, [r1]
	add	r1, r1, #2
	cmp	r0, r2
	beq	L306
L304:
	cmp	r1, r4
	blt	L303
L305:
	@  return -1;
	mvn	r0, #0
	mov	r2, #1
	@  continue in caller
	mov	pc, lr
L306:
	@  return i - offset;
	sub	r0, r1, r10
	mov	r0, r0, lsr #1
	sub	r0, r0, #1
	mov	r2, #1
	@  continue in caller
	mov	pc, lr
	.pool
	@  End segment: java.lang.String.indexOf


	@  ------------------------------------------------------
	@  Start segment: java.lang.String.compareTo

	.global	native_string_compareTo_entry
native_string_compareTo_entry:
	@  get strings to compare
	ldr	r4, [r6], #4
	ldr	r2, [r6], #4
	cmp	r4, #0
	beq	L309
	@  Get str0.value[], str1.value[]
	ldr	r3, [r2, #4]
	ldr	r9, [r4, #4]
	@  Get str0.offset[], str1.offset[]
	ldr	r10, [r2, #8]
	ldr	r12, [r4, #8]
	@  Get str0.count[], str1.count[]
	ldr	r2, [r2, #12]
	ldr	r4, [r4, #12]
	@  Compute start of character data
	add	r3, r3, #8
	add	r9, r9, #8
	add	r3, r3, r10, lsl #1
	add	r9, r9, r12, lsl #1
	@  Compute min(str0_count, str1_count)
	subs	r0, r2, r4
	movlt	r1, r2
	movge	r1, r4
	@  Is at least one string empty?
	cmp	r1, #0
	beq	L308
L307:
	ldrh	r10, [r3], #2
	ldrh	r12, [r9], #2
	subs	r0, r10, r12
	bne	L308
	subs	r1, r1, #1
	bne	L307
	sub	r0, r2, r4
L308:
	mov	r2, #1
	@  continue in caller
	mov	pc, lr
L309:
	@  We have some sort of error. Redo in Java version of compareTo()
	str	r2, [r6, #-4]!
	str	r4, [r6, #-4]!
	b	interpreter_method_entry
	.pool
	@  End segment: java.lang.String.compareTo


	@  ------------------------------------------------------
	@  Start segment: java.lang.String.{starts,ends}With

	.global	native_string_endsWith_entry
native_string_endsWith_entry:
	@  [[[Hand coded method from java.lang.String]]]
	@      public boolean endsWith(String suffix) {
	@          return startsWith(suffix, count - suffix.count);
	@      }
	@  return address is in lr
	@  get suffix and this String
	ldr	r4, [r6], #4
	ldr	r3, [r6], #4
	cmp	r4, #0
	beq	L311
	@  Get this.count[], suffix.count[]
	ldr	r9, [r3, #12]
	ldr	r2, [r4, #12]
	@  Preserve return address
	mov	r10, lr
	@  Calculate (this.count - suffix.count) for toffset
	sub	r12, r9, r2
	b	L310
L311:
	@  Bail out to the general endsWith implementation
	str	r3, [r6, #-4]!
	str	r4, [r6, #-4]!
	b	interpreter_method_entry

	.global	native_string_startsWith0_entry
native_string_startsWith0_entry:
	@  [[[Hand coded method from java.lang.String]]]
	@      public boolean startsWith(String prefix) {
	@          return startsWith(prefix, 0);
	@      }
	@  return address is in lr
	@  get prefix and this String
	ldr	r4, [r6], #4
	ldr	r3, [r6], #4
	cmp	r4, #0
	@  zero for toffset
	movne	r12, #0
	bne	L312
	@  Bail out to the general startsWith implementation
	str	r3, [r6, #-4]!
	str	r4, [r6, #-4]!
	b	interpreter_method_entry

	.global	native_string_startsWith_entry
native_string_startsWith_entry:
	@  get toffset, prefix, this String
	ldr	r12, [r6], #4
	ldr	r4, [r6], #4
	ldr	r3, [r6], #4
	cmp	r4, #0
	beq	L313
L312:
	@  Get this.count[], prefix.count[]
	ldr	r9, [r3, #12]
	ldr	r2, [r4, #12]
	@  Preserve return address
	mov	r10, lr
	@  if (toffset > this.count - prefix.count) return false;
	sub	r9, r9, r2
	cmp	r9, r12
	blt	L314
L310:
	cmp	r12, #0
	blt	L314
	@  Get this.value[], prefix.value[]
	ldr	r0, [r3, #4]
	ldr	r1, [r4, #4]
	@  Get this.offset[], prefix.offset[]
	ldr	r9, [r3, #8]
	ldr	lr, [r4, #8]
	@  Compute start of character data
	add	r0, r0, #8
	add	r1, r1, #8
	add	r0, r0, r9, lsl #1
	add	r1, r1, lr, lsl #1
	@  Add toffset
	add	r0, r0, r12, lsl #1
	@  Compute the number of bytes to compare
	mov	r2, r2, lsl #1
	bl	jvm_memcmp
	cmp	r0, #0
	moveq	r0, #1
L314:
	movne	r0, #0
	mov	r2, #1
	@  continue in caller
	mov	pc, r10
L313:
	@  Bail out to the general startsWith implementation
	str	r3, [r6, #-4]!
	str	r4, [r6, #-4]!
	str	r12, [r6, #-4]!
	b	interpreter_method_entry
	.pool
	@  End segment: java.lang.String.{starts,ends}With


	@  ------------------------------------------------------
	@  Start segment: other java.lang.String entries

	.global	native_string_substringI_entry
native_string_substringI_entry:
	ldr	r2, [r6, #4]             @ r2 = string
	ldr	r3, [r6]                 @ r3 = beginIndex
	ldr	r4, [r2, #12]            @ r4 = endIndex
	ldr	r9, [r2, #12]            @ r9 = count
	@  if (beginIndex < 0)          goto bailout;
	@  if (endIndex   < beginIndex) goto bailout;
	@  if (count      < endIndex)   goto bailout;
	cmp	r3, #0
	cmpge	r4, r3
	cmpge	r9, r4
	blt	L315
	ldr	r10, [r5, #1060]         @ inline_allocation_top 
	ldr	r12, [r5, #1064]         @ inline_allocation_end 
	cmp	r3, #0
	cmpeq	r4, r9
	beq	L316
	subs	r12, r12, #16
	bmi	L315
	cmp	r12, r10
	ldr	r7, [r2]                 @ r7 = string.near()
	ldr	r12, [r2, #8]            @ r12 = string.offset
	bcc	L315
	ldr	r7, [r7]                 @ r7 = string class
	sub	r4, r4, r3
	str	r4, [r10, #12]           @ result.count = endIndex - beginIndex
	ldr	r7, [r7, #12]            @ r7 = string proto near
	ldr	r4, [r2, #4]             @ r4 = string.value
	add	r12, r12, r3
	str	r12, [r10, #8]           @ result.offset = offset + beginIndex
	str	r7, [r10]                @ result.klass = String proto near
	str	r4, [r10, #4]            @ result.value = string.value
	mov	r0, r10                  @ return new string in r0
	add	r10, r10, #16
	str	r10, [r5, #1060]         @ inline_allocation_top 
	mov	r2, #1
	@  remove arguments from the stack
	add	r6, r6, #8
	mov	pc, lr
L316:
	@  return <this>
	mov	r0, r2
	mov	r2, #1
	@  remove arguments from the stack
	add	r6, r6, #8
	mov	pc, lr
L315:
	@  Bail out to the general startsWith implementation
	b	interpreter_method_entry

	.global	native_string_substringII_entry
native_string_substringII_entry:
	ldr	r2, [r6, #8]             @ r2 = string
	ldr	r3, [r6, #4]             @ r3 = beginIndex
	ldr	r4, [r6]                 @ r4 = endIndex
	ldr	r9, [r2, #12]            @ r9 = count
	@  if (beginIndex < 0)          goto bailout;
	@  if (endIndex   < beginIndex) goto bailout;
	@  if (count      < endIndex)   goto bailout;
	cmp	r3, #0
	cmpge	r4, r3
	cmpge	r9, r4
	blt	L317
	ldr	r10, [r5, #1060]         @ inline_allocation_top 
	ldr	r12, [r5, #1064]         @ inline_allocation_end 
	cmp	r3, #0
	cmpeq	r4, r9
	beq	L318
	subs	r12, r12, #16
	bmi	L317
	cmp	r12, r10
	ldr	r7, [r2]                 @ r7 = string.near()
	ldr	r12, [r2, #8]            @ r12 = string.offset
	bcc	L317
	ldr	r7, [r7]                 @ r7 = string class
	sub	r4, r4, r3
	str	r4, [r10, #12]           @ result.count = endIndex - beginIndex
	ldr	r7, [r7, #12]            @ r7 = string proto near
	ldr	r4, [r2, #4]             @ r4 = string.value
	add	r12, r12, r3
	str	r12, [r10, #8]           @ result.offset = offset + beginIndex
	str	r7, [r10]                @ result.klass = String proto near
	str	r4, [r10, #4]            @ result.value = string.value
	mov	r0, r10                  @ return new string in r0
	add	r10, r10, #16
	str	r10, [r5, #1060]         @ inline_allocation_top 
	mov	r2, #1
	@  remove arguments from the stack
	add	r6, r6, #12
	mov	pc, lr
L318:
	@  return <this>
	mov	r0, r2
	mov	r2, #1
	@  remove arguments from the stack
	add	r6, r6, #12
	mov	pc, lr
L317:
	@  Bail out to the general startsWith implementation
	b	interpreter_method_entry
	.pool
	@  End segment: other java.lang.String entries


	@  ------------------------------------------------------
	@  Start segment: Native entry points for system functions

	.global	native_system_arraycopy_entry
native_system_arraycopy_entry:
	@  load arguments to registers
	@  r1 = src
	ldr	r1, [r6, #16]
	@  r7 = dst
	ldr	r7, [r6, #8]
	@  r2 = length
	ldr	r2, [r6]
	@  r4 = dst_pos
	ldr	r4, [r6, #4]
	@  r3 = src_pos
	ldr	r3, [r6, #12]
	@  if (src == NULL || dst == NULL) goto bailout;
	cmp	r1, #0
	cmpne	r7, #0
	beq	L319
	@  Get src.klass() and dst.klass()
	ldr	r9, [r1]
	ldr	r10, [r7]
	ldr	r9, [r9]
	ldr	r10, [r10]
	@  if (length < 0 || src_pos < 0 || dst_pos < 0) goto bailout;
	orr	r12, r2, r3
	orrs	r12, r12, r4
	bmi	L319
	@  if (src.klass() != dst.klass()) goto bailout;
	cmp	r9, r10
	bne	L319
	@  Make sure we really have an array.
	@  Java objects have an instance size >= 0
	ldrsh	r12, [r9, #6]
	cmp	r12, #0
	bge	L319
	@  if ((unsigned int) src.length < (unsigned int) src_pos + (unsigned int) length) goto bailout;
	ldr	r9, [r1, #4]
	add	r10, r3, r2
	cmp	r9, r10
	bcc	L319
	@  if ((unsigned int) dst.length < (unsigned int) dst_pos + (unsigned int) length) goto bailout;
	ldr	r9, [r7, #4]
	add	r10, r4, r2
	cmp	r9, r10
	bcc	L319
	@  remove arguments from the stack
	add	r6, r6, #20
	@  Point at actual data
	add	r1, r1, #8
	add	r7, r7, #8
	@  Jump to appropriate copy routine if length > 0
	subs	r2, r2, #1
	@  Length > 0.  Jump to appropriate routine;
	ldrge	pc, [pc, -r12, lsl #2]
	@  Length == 0.  Just return
	blt	L321
	.long	L328
	.long	L326
	.long	L322
	.long	L323
	.long	L324
	.long	L325
	@  No fallthrough
	swi	0x9f0001

	.global	native_jvm_unchecked_byte_arraycopy_entry
native_jvm_unchecked_byte_arraycopy_entry:
	@  Pop arguments from stack
	ldr	r2, [r6], #4
	ldr	r4, [r6], #4
	ldr	r7, [r6], #4
	ldr	r3, [r6], #4
	ldr	r1, [r6], #4
	subs	r2, r2, #1
	@  Length == 0.  Just return
	blt	L321
	@  Point at actual data
	add	r7, r7, #8
	add	r1, r1, #8
L322:
	@  Each element of the array is 1 byte(s)
	add	r0, r7, r4
	add	r1, r1, r3
	add	r2, r2, #1
	mov	r7, lr
	bl	jvm_memcpy
	@  fill the top of stack cache and return
	mov	r2, #0
	mov	pc, r7
	@  No fallthrough
	swi	0x9f0001

	.global	native_jvm_unchecked_char_arraycopy_entry
native_jvm_unchecked_char_arraycopy_entry:
	@  Pop arguments from stack
	ldr	r2, [r6], #4
	ldr	r4, [r6], #4
	ldr	r7, [r6], #4
	ldr	r3, [r6], #4
	ldr	r1, [r6], #4
	subs	r2, r2, #1
	@  Length == 0.  Just return
	blt	L321
	@  Point at actual data
	add	r7, r7, #8
	add	r1, r1, #8
L323:
	@  Each element of the array is 2 byte(s)
	add	r0, r7, r4, lsl #1
	add	r1, r1, r3, lsl #1
	add	r2, r2, #1
	mov	r2, r2, lsl #1
	mov	r7, lr
	bl	jvm_memcpy
	@  fill the top of stack cache and return
	mov	r2, #0
	mov	pc, r7
	@  No fallthrough
	swi	0x9f0001

	.global	native_jvm_unchecked_int_arraycopy_entry
native_jvm_unchecked_int_arraycopy_entry:
	@  Pop arguments from stack
	ldr	r2, [r6], #4
	ldr	r4, [r6], #4
	ldr	r7, [r6], #4
	ldr	r3, [r6], #4
	ldr	r1, [r6], #4
	subs	r2, r2, #1
	@  Length == 0.  Just return
	blt	L321
	@  Point at actual data
	add	r7, r7, #8
	add	r1, r1, #8
L327:
L324:
	@  Each element of the array is 4 byte(s)
	add	r0, r7, r4, lsl #2
	add	r1, r1, r3, lsl #2
	add	r2, r2, #1
	mov	r2, r2, lsl #2
	mov	r7, lr
	bl	jvm_memcpy
	@  fill the top of stack cache and return
	mov	r2, #0
	mov	pc, r7
	@  No fallthrough
	swi	0x9f0001

	.global	native_jvm_unchecked_long_arraycopy_entry
native_jvm_unchecked_long_arraycopy_entry:
	@  Pop arguments from stack
	ldr	r2, [r6], #4
	ldr	r4, [r6], #4
	ldr	r7, [r6], #4
	ldr	r3, [r6], #4
	ldr	r1, [r6], #4
	subs	r2, r2, #1
	@  Length == 0.  Just return
	blt	L321
	@  Point at actual data
	add	r7, r7, #8
	add	r1, r1, #8
L325:
	@  Each element of the array is 8 byte(s)
	add	r0, r7, r4, lsl #3
	add	r1, r1, r3, lsl #3
	add	r2, r2, #1
	mov	r2, r2, lsl #3
	mov	r7, lr
	bl	jvm_memcpy
	@  fill the top of stack cache and return
	mov	r2, #0
	mov	pc, r7
	@  No fallthrough
	swi	0x9f0001

	.global	native_jvm_unchecked_obj_arraycopy_entry
native_jvm_unchecked_obj_arraycopy_entry:
	@  Pop arguments from stack
	ldr	r2, [r6], #4
	ldr	r4, [r6], #4
	ldr	r7, [r6], #4
	ldr	r3, [r6], #4
	ldr	r1, [r6], #4
	subs	r2, r2, #1
	@  Length == 0.  Just return
	blt	L321
	@  Point at actual data
	add	r7, r7, #8
	add	r1, r1, #8
L326:
	@  If dst is in new space, there's no need to set bitvector
	ldr	r12, [r5, #1256]         @ old_generation_end 
	cmp	r7, r12
	bge	L327
	@  if (src == dst) && (src_pos < dst_pos) goto copy_downwards;
	cmp	r1, r7
	bne	L330
	cmp	r3, r4
	blt	L329
L330:
	@  Copy the elements starting from low address upwards
	@  Point to the (inclusive) low-end of elements to copy
	add	r1, r1, r3, lsl #2
	add	r7, r7, r4, lsl #2
L331:
	@  r2 is one less than the number of elements still to copy
	@  Are we copying at least eight elements?
	cmp	r2, #7
	blt	L334
	@  Is the destination aligned?
	tst	r7, #28
	bne	L333
	@  We have at least 8 words to copy, and the dst is aligned
	ldr	r12, [r5, #1076]         @ bitvector_base 
	mvn	r0, #0
	@  The main copy loop copies 32 elements each iteration.
	@  If don't have (n*32 + {0..7}) elements, jump to the
	@  middle of the array
	add	r10, r2, #1
	and	r10, r10, #24
	cmp	r10, #8
	beq	L337
	cmp	r10, #16
	beq	L336
	cmp	r10, #24
	beq	L335
L332:
	strb	r0, [r12, +r7, lsr #5]
	ldmia	r1!, {r3, r4, r9, r10}
	stmia	r7!, {r3, r4, r9, r10}
	ldmia	r1!, {r3, r4, r9, r10}
	stmia	r7!, {r3, r4, r9, r10}
	sub	r2, r2, #8
L335:
	strb	r0, [r12, +r7, lsr #5]
	ldmia	r1!, {r3, r4, r9, r10}
	stmia	r7!, {r3, r4, r9, r10}
	ldmia	r1!, {r3, r4, r9, r10}
	stmia	r7!, {r3, r4, r9, r10}
	sub	r2, r2, #8
L336:
	strb	r0, [r12, +r7, lsr #5]
	ldmia	r1!, {r3, r4, r9, r10}
	stmia	r7!, {r3, r4, r9, r10}
	ldmia	r1!, {r3, r4, r9, r10}
	stmia	r7!, {r3, r4, r9, r10}
	sub	r2, r2, #8
L337:
	strb	r0, [r12, +r7, lsr #5]
	ldmia	r1!, {r3, r4, r9, r10}
	stmia	r7!, {r3, r4, r9, r10}
	ldmia	r1!, {r3, r4, r9, r10}
	stmia	r7!, {r3, r4, r9, r10}
	subs	r2, r2, #8
	blt	L321
	cmp	r2, #7
	bge	L332
L334:
	@  Copying less than 8 elements
	ldr	r10, [r1], #4
	mov	r9, r7
	str	r10, [r7], #4
	@  oop_write_barrier
	ldr	r10, [r5, #1076]         @ bitvector_base 
	ldr	r3, [r5, #1080]          @ bit_selector 
	ldrb	r4, [r10, +r9, lsr #5]!
	mvn	r9, r9, lsr #2
	orr	r4, r4, r3, ror r9
	strb	r4, [r10]
	subs	r2, r2, #1
	bge	L334
	mov	r2, #0
	mov	pc, lr
L333:
	@  length >= 8, but dst not aligned
	ldr	r10, [r1], #4
	mov	r9, r7
	str	r10, [r7], #4
	@  oop_write_barrier
	ldr	r10, [r5, #1076]         @ bitvector_base 
	ldr	r3, [r5, #1080]          @ bit_selector 
	ldrb	r4, [r10, +r9, lsr #5]!
	mvn	r9, r9, lsr #2
	orr	r4, r4, r3, ror r9
	strb	r4, [r10]
	sub	r2, r2, #1
	b	L331
	@  Copy the elements starting from high address downwards
L329:
	@  Point to the (inclusive) low-end of elements to copy
	add	r1, r1, r3, lsl #2
	add	r7, r7, r4, lsl #2
	@  Point to the (exclusive) high-end of elements
	@  r2 is one less than the number of elements still to copy
	add	r1, r1, r2, lsl #2
	add	r7, r7, r2, lsl #2
	add	r1, r1, #4
	add	r7, r7, #4
L338:
	@  r2 is one less than the number of elements still to copy
	@  Are we copying at least eight elements?
	cmp	r2, #7
	blt	L341
	@  Is the destination aligned?
	tst	r7, #28
	bne	L340
	@  We have at least 8 words to copy, and the dst is aligned
	ldr	r12, [r5, #1076]         @ bitvector_base 
	mvn	r0, #0
	@  The main copy loop copies 32 elements each iteration.
	@  If don't have (n*32 + {0..7}) elements, jump to the
	@  middle of the array
	add	r10, r2, #1
	and	r10, r10, #24
	cmp	r10, #8
	beq	L344
	cmp	r10, #16
	beq	L343
	cmp	r10, #24
	beq	L342
L339:
	ldmdb	r1!, {r3, r4, r9, r10}
	stmdb	r7!, {r3, r4, r9, r10}
	ldmdb	r1!, {r3, r4, r9, r10}
	stmdb	r7!, {r3, r4, r9, r10}
	strb	r0, [r12, +r7, lsr #5]
	sub	r2, r2, #8
L342:
	ldmdb	r1!, {r3, r4, r9, r10}
	stmdb	r7!, {r3, r4, r9, r10}
	ldmdb	r1!, {r3, r4, r9, r10}
	stmdb	r7!, {r3, r4, r9, r10}
	strb	r0, [r12, +r7, lsr #5]
	sub	r2, r2, #8
L343:
	ldmdb	r1!, {r3, r4, r9, r10}
	stmdb	r7!, {r3, r4, r9, r10}
	ldmdb	r1!, {r3, r4, r9, r10}
	stmdb	r7!, {r3, r4, r9, r10}
	strb	r0, [r12, +r7, lsr #5]
	sub	r2, r2, #8
L344:
	ldmdb	r1!, {r3, r4, r9, r10}
	stmdb	r7!, {r3, r4, r9, r10}
	ldmdb	r1!, {r3, r4, r9, r10}
	stmdb	r7!, {r3, r4, r9, r10}
	strb	r0, [r12, +r7, lsr #5]
	subs	r2, r2, #8
	blt	L321
	cmp	r2, #7
	bge	L339
L341:
	@  Copying less than 8 elements
	ldr	r10, [r1, #-4]!
	str	r10, [r7, #-4]!
	mov	r9, r7
	@  oop_write_barrier
	ldr	r10, [r5, #1076]         @ bitvector_base 
	ldr	r3, [r5, #1080]          @ bit_selector 
	ldrb	r4, [r10, +r9, lsr #5]!
	mvn	r9, r9, lsr #2
	orr	r4, r4, r3, ror r9
	strb	r4, [r10]
	subs	r2, r2, #1
	bge	L341
	mov	r2, #0
	mov	pc, lr
L340:
	@  length >= 8, but dst not aligned
	ldr	r10, [r1, #-4]!
	str	r10, [r7, #-4]!
	mov	r9, r7
	@  oop_write_barrier
	ldr	r10, [r5, #1076]         @ bitvector_base 
	ldr	r3, [r5, #1080]          @ bit_selector 
	ldrb	r4, [r10, +r9, lsr #5]!
	mvn	r9, r9, lsr #2
	orr	r4, r4, r3, ror r9
	strb	r4, [r10]
	sub	r2, r2, #1
	b	L338
L321:
	mov	r2, #0
	mov	pc, lr
L328:
	@  Shouldn't call this
	swi	0x9f0001
L320:
	@  Recover original parameters
	sub	r6, r6, #20
	sub	r1, r1, #8
	sub	r7, r7, #8
	add	r2, r2, #1
L319:
	@  Bail out to the general arraycopy implementation
	b	interpreter_method_entry
	.pool
	@  End segment: Native entry points for system functions


	@  ------------------------------------------------------
	@  Start segment: StringBuffer.append

	.global	native_stringbuffer_append_entry
native_stringbuffer_append_entry:
	@  Get the object.
	ldr	r2, [r6, #4]
	@  Get the near.
	ldr	r3, [r2]
	ldr	r9, [r2, #4]
	ldr	r3, [r3, #8]
	ldr	r4, [r2, #8]
	@  Check the lock bit.
	tst	r3, #1
	bne	interpreter_method_entry
	@  Check for insufficient capacity of the stringbuffer.
	ldr	r3, [r9, #4]
	add	r9, r9, r4, lsl #1
	cmp	r4, r3
	bge	interpreter_method_entry
	@  Increment the count, append the char.
	ldr	r3, [r6]
	add	r4, r4, #1
	str	r4, [r2, #8]
	strh	r3, [r9, #8]
	@  Return the object.
	mov	r0, r2
	mov	r2, #1
	add	r6, r6, #8
	mov	pc, lr
	.pool
	@  End segment: StringBuffer.append


	@  ------------------------------------------------------
	@  Start segment: Integer.toString

	.global	native_integer_toString_entry
native_integer_toString_entry:
	@  Find the size of inline allocation area to see if it is large
	@  enough to create new String object with associated char[] array
	ldr	r4, [r5, #1060]          @ inline_allocation_top 
	ldr	r12, [r5, #1064]         @ inline_allocation_end 
	add	r9, r4, #48
	cmp	r9, r12
	bhi	interpreter_method_entry
	@  Get JavaNear prototype for String and char[] from persistent handles
	.extern	persistent_handles
	ldr	r12, =persistent_handles
	str	r9, [r5, #1060]          @ inline_allocation_top 
	ldr	r2, [r12, #176]
	ldr	r3, [r12, #68]
	ldr	r2, [r2, #12]
	ldr	r3, [r3, #12]
	@  Fill in all the fields we know
	str	r2, [r4], #16            @ set String JavaNear
	str	r3, [r4]                 @ set char[] JavaNear
	str	r4, [r4, #-12]           @ set String.value to be our char[]
	mov	r12, #12                 @ set length field of char[] array
	str	r12, [r4, #4]
	@  Get the argument from Java stack
	ldr	r0, [r6], #4
	@  Remember the position of the last digit
	mov	r10, r9, lsr #1
	@  Load the magic constant for integer divsion by 10
	ldr	r12, L346
	@  Make the argument non-negative
	ands	r1, r0, #0x80000000
	rsbmi	r0, r0, #0
	@  Perform integer division by 10 in the loop
	@  and store digits in char[] array from right to left.
	@  Dividend is in r0, remainder is in r2
L345:
	umull	r2, r3, r0, r12
	add	r2, r0, #48
	movs	r0, r3, lsr #3
	sub	r2, r2, r0, lsl #3
	sub	r2, r2, r0, lsl #1
	strh	r2, [r9, #-2]!
	bne	L345
	@  Add '-' sign if the original number was negative
	orrs	r1, r1, #45
	strmih	r1, [r9, #-2]!
	@  Fill in remained fields
	sub	r0, r4, #16              @ pointer to result string
	sub	r9, r10, r9, lsr #1      @ set String.count
	str	r9, [r0, #12]
	rsb	r9, r9, #12              @ set String.offset
	str	r9, [r0, #8]
	@  Return to caller
	mov	r2, #1
	mov	pc, lr
L346:
	.long	0xcccccccd
	.pool
	@  End segment: Integer.toString


	@  ------------------------------------------------------
	@  Start segment: Native entry points for miscellaneous functions

	.global	native_vector_elementAt_entry
native_vector_elementAt_entry:
	ldr	r2, [r6, #4]
	ldr	r3, [r6]
	@  Get the near object.
	ldr	r4, [r2]
	ldr	r9, [r2, #8]
	ldr	r4, [r4, #8]
	@  Check for out of range index.
	cmp	r3, r9
	bcs	interpreter_method_entry
	@  Check the lock bit.
	tst	r4, #1
	ldreq	r9, [r2, #4]
	bne	interpreter_method_entry
	@  Get the object and return.
	add	r0, r9, r3, lsl #2
	ldr	r0, [r0, #8]
	mov	r2, #1
	@  continue in caller
	add	r6, r6, #8
	mov	pc, lr

	.global	native_vector_addElement_entry
native_vector_addElement_entry:
	.extern	memset
	@  Get this Vector object
	ldr	r2, [r6, #4]
	@  Load r3 = Near, r4 = elementData and r9 = elementCount
	ldmia	r2, {r3, r4, r9}
	@  Check if the object is locked
	ldr	r3, [r3, #8]
	ldr	r12, [r4, #4]
	tst	r3, #1
	bne	interpreter_method_entry
	@  Check if elementCount < elementData.length
	cmp	r9, r12
	bcs	L347
	@  Usual case: put new element in existing elementData array
	ldr	r10, [r6]
	add	r4, r4, r9, lsl #2
	str	r10, [r4, #8]!
	@  oop_write_barrier
	ldr	r10, [r5, #1076]         @ bitvector_base 
	ldr	r3, [r5, #1080]          @ bit_selector 
	ldrb	r12, [r10, +r4, lsr #5]!
	mvn	r4, r4, lsr #2
	orr	r12, r12, r3, ror r4
	strb	r12, [r10]
	add	r9, r9, #1
	str	r9, [r2, #8]
	@  Return to caller
	mov	r2, #0
	add	r6, r6, #8
	mov	pc, lr
L347:
	@  Need to increase Vector capacity
	ldr	r3, [r2, #12]
	ldr	r1, [r5, #1060]          @ inline_allocation_top 
	cmp	r3, #0
	addgt	r3, r3, r12
	addles	r3, r12, r12
	addeq	r3, r9, #1
	@  Check if we have enough space in inline allocation area
	ldr	r10, [r5, #1064]         @ inline_allocation_end 
	add	r12, r1, #8
	add	r12, r12, r3, lsl #2
	cmp	r10, r12
	bcc	interpreter_method_entry
	@  Create a new ObjArray with increased length
	str	r12, [r5, #1060]         @ inline_allocation_top 
	ldr	r10, [r4]
	str	r3, [r1, #4]
	str	r10, [r1]
	@  Update fields of this Vector object
	add	r10, r9, #1
	str	r10, [r2, #8]
	str	r1, [r2, #4]!
	@  oop_write_barrier
	ldr	r0, [r5, #1076]          @ bitvector_base 
	ldr	r10, [r5, #1080]         @ bit_selector 
	ldrb	r12, [r0, +r2, lsr #5]!
	mvn	r2, r2, lsr #2
	orr	r12, r12, r10, ror r2
	strb	r12, [r0]
	@  Copy old Vector data - prepare registers for jvm_memcpy
	add	r0, r1, #8
	add	r1, r4, #8
	mov	r2, r9, lsl #2
	adds	r4, lr, #0
	sbc	r9, r3, r9
	add	r10, r0, r2
	bl	jvm_memcpy
	@  Put new element into the Vector
	ldr	r1, [r6]
	add	r6, r6, #8
	str	r1, [r10]
	@  Return to caller
	mov	r2, #0
	mov	pc, r4
	.pool
	@  End segment: Native entry points for miscellaneous functions


.text
	@  ------------------------------------------------------
	@  Start segment: Floating point routines
	.extern	jvm_fadd
	.extern	jvm_fsub
	.extern	jvm_fmul
	.extern	jvm_fdiv
	.extern	jvm_frem
	.extern	jvm_dcmpl
	.extern	jvm_dcmpg
	.extern	jvm_fcmpl
	.extern	jvm_fcmpg
	.extern	jvm_dadd
	.extern	jvm_dsub
	.extern	jvm_dmul
	.extern	jvm_ddiv
	.extern	jvm_drem
	.extern	jvm_i2d
	.extern	jvm_i2f
	.extern	jvm_l2f
	.extern	jvm_l2d
	.extern	jvm_f2i
	.extern	jvm_f2l
	.extern	jvm_f2d
	.extern	jvm_d2i
	.extern	jvm_d2l
	.extern	jvm_d2f
	.pool
	@  End segment: Floating point routines


.data
	@  ------------------------------------------------------
	@  Start segment: Global pool

	.global	gp_base_label
gp_base_label:
	.long	bc_impl_nop                            /* 0x0 */ @ offset=0 (0x0)
	.long	bc_impl_aconst_null                    /* 0x1 */ @ offset=4 (0x4)
	.long	bc_impl_iconst_m1                      /* 0x2 */ @ offset=8 (0x8)
	.long	bc_impl_iconst_0                       /* 0x3 */ @ offset=12 (0xc)
	.long	bc_impl_iconst_1                       /* 0x4 */ @ offset=16 (0x10)
	.long	bc_impl_iconst_2                       /* 0x5 */ @ offset=20 (0x14)
	.long	bc_impl_iconst_3                       /* 0x6 */ @ offset=24 (0x18)
	.long	bc_impl_iconst_4                       /* 0x7 */ @ offset=28 (0x1c)
	.long	bc_impl_iconst_5                       /* 0x8 */ @ offset=32 (0x20)
	.long	bc_impl_lconst_0                       /* 0x9 */ @ offset=36 (0x24)
	.long	bc_impl_lconst_1                       /* 0xa */ @ offset=40 (0x28)
	.long	bc_impl_fconst_0                       /* 0xb */ @ offset=44 (0x2c)
	.long	bc_impl_fconst_1                       /* 0xc */ @ offset=48 (0x30)
	.long	bc_impl_fconst_2                       /* 0xd */ @ offset=52 (0x34)
	.long	bc_impl_dconst_0                       /* 0xe */ @ offset=56 (0x38)
	.long	bc_impl_dconst_1                       /* 0xf */ @ offset=60 (0x3c)
	.long	bc_impl_bipush                         /* 0x10 */ @ offset=64 (0x40)
	.long	bc_impl_sipush                         /* 0x11 */ @ offset=68 (0x44)
	.long	bc_impl_ldc                            /* 0x12 */ @ offset=72 (0x48)
	.long	bc_impl_ldc_w                          /* 0x13 */ @ offset=76 (0x4c)
	.long	bc_impl_ldc2_w                         /* 0x14 */ @ offset=80 (0x50)
	.long	bc_impl_iload                          /* 0x15 */ @ offset=84 (0x54)
	.long	bc_impl_lload                          /* 0x16 */ @ offset=88 (0x58)
	.long	bc_impl_fload                          /* 0x17 */ @ offset=92 (0x5c)
	.long	bc_impl_dload                          /* 0x18 */ @ offset=96 (0x60)
	.long	bc_impl_aload                          /* 0x19 */ @ offset=100 (0x64)
	.long	bc_impl_iload_0                        /* 0x1a */ @ offset=104 (0x68)
	.long	bc_impl_iload_1                        /* 0x1b */ @ offset=108 (0x6c)
	.long	bc_impl_iload_2                        /* 0x1c */ @ offset=112 (0x70)
	.long	bc_impl_iload_3                        /* 0x1d */ @ offset=116 (0x74)
	.long	bc_impl_lload_0                        /* 0x1e */ @ offset=120 (0x78)
	.long	bc_impl_lload_1                        /* 0x1f */ @ offset=124 (0x7c)
	.long	bc_impl_lload_2                        /* 0x20 */ @ offset=128 (0x80)
	.long	bc_impl_lload_3                        /* 0x21 */ @ offset=132 (0x84)
	.long	bc_impl_fload_0                        /* 0x22 */ @ offset=136 (0x88)
	.long	bc_impl_fload_1                        /* 0x23 */ @ offset=140 (0x8c)
	.long	bc_impl_fload_2                        /* 0x24 */ @ offset=144 (0x90)
	.long	bc_impl_fload_3                        /* 0x25 */ @ offset=148 (0x94)
	.long	bc_impl_dload_0                        /* 0x26 */ @ offset=152 (0x98)
	.long	bc_impl_dload_1                        /* 0x27 */ @ offset=156 (0x9c)
	.long	bc_impl_dload_2                        /* 0x28 */ @ offset=160 (0xa0)
	.long	bc_impl_dload_3                        /* 0x29 */ @ offset=164 (0xa4)
	.long	bc_impl_aload_0                        /* 0x2a */ @ offset=168 (0xa8)
	.long	bc_impl_aload_1                        /* 0x2b */ @ offset=172 (0xac)
	.long	bc_impl_aload_2                        /* 0x2c */ @ offset=176 (0xb0)
	.long	bc_impl_aload_3                        /* 0x2d */ @ offset=180 (0xb4)
	.long	bc_impl_iaload                         /* 0x2e */ @ offset=184 (0xb8)
	.long	bc_impl_laload                         /* 0x2f */ @ offset=188 (0xbc)
	.long	bc_impl_faload                         /* 0x30 */ @ offset=192 (0xc0)
	.long	bc_impl_daload                         /* 0x31 */ @ offset=196 (0xc4)
	.long	bc_impl_aaload                         /* 0x32 */ @ offset=200 (0xc8)
	.long	bc_impl_baload                         /* 0x33 */ @ offset=204 (0xcc)
	.long	bc_impl_caload                         /* 0x34 */ @ offset=208 (0xd0)
	.long	bc_impl_saload                         /* 0x35 */ @ offset=212 (0xd4)
	.long	bc_impl_istore                         /* 0x36 */ @ offset=216 (0xd8)
	.long	bc_impl_lstore                         /* 0x37 */ @ offset=220 (0xdc)
	.long	bc_impl_fstore                         /* 0x38 */ @ offset=224 (0xe0)
	.long	bc_impl_dstore                         /* 0x39 */ @ offset=228 (0xe4)
	.long	bc_impl_astore                         /* 0x3a */ @ offset=232 (0xe8)
	.long	bc_impl_istore_0                       /* 0x3b */ @ offset=236 (0xec)
	.long	bc_impl_istore_1                       /* 0x3c */ @ offset=240 (0xf0)
	.long	bc_impl_istore_2                       /* 0x3d */ @ offset=244 (0xf4)
	.long	bc_impl_istore_3                       /* 0x3e */ @ offset=248 (0xf8)
	.long	bc_impl_lstore_0                       /* 0x3f */ @ offset=252 (0xfc)
	.long	bc_impl_lstore_1                       /* 0x40 */ @ offset=256 (0x100)
	.long	bc_impl_lstore_2                       /* 0x41 */ @ offset=260 (0x104)
	.long	bc_impl_lstore_3                       /* 0x42 */ @ offset=264 (0x108)
	.long	bc_impl_fstore_0                       /* 0x43 */ @ offset=268 (0x10c)
	.long	bc_impl_fstore_1                       /* 0x44 */ @ offset=272 (0x110)
	.long	bc_impl_fstore_2                       /* 0x45 */ @ offset=276 (0x114)
	.long	bc_impl_fstore_3                       /* 0x46 */ @ offset=280 (0x118)
	.long	bc_impl_dstore_0                       /* 0x47 */ @ offset=284 (0x11c)
	.long	bc_impl_dstore_1                       /* 0x48 */ @ offset=288 (0x120)
	.long	bc_impl_dstore_2                       /* 0x49 */ @ offset=292 (0x124)
	.long	bc_impl_dstore_3                       /* 0x4a */ @ offset=296 (0x128)
	.long	bc_impl_astore_0                       /* 0x4b */ @ offset=300 (0x12c)
	.long	bc_impl_astore_1                       /* 0x4c */ @ offset=304 (0x130)
	.long	bc_impl_astore_2                       /* 0x4d */ @ offset=308 (0x134)
	.long	bc_impl_astore_3                       /* 0x4e */ @ offset=312 (0x138)
	.long	bc_impl_iastore                        /* 0x4f */ @ offset=316 (0x13c)
	.long	bc_impl_lastore                        /* 0x50 */ @ offset=320 (0x140)
	.long	bc_impl_fastore                        /* 0x51 */ @ offset=324 (0x144)
	.long	bc_impl_dastore                        /* 0x52 */ @ offset=328 (0x148)
	.long	bc_impl_aastore                        /* 0x53 */ @ offset=332 (0x14c)
	.long	bc_impl_bastore                        /* 0x54 */ @ offset=336 (0x150)
	.long	bc_impl_castore                        /* 0x55 */ @ offset=340 (0x154)
	.long	bc_impl_sastore                        /* 0x56 */ @ offset=344 (0x158)
	.long	bc_impl_pop                            /* 0x57 */ @ offset=348 (0x15c)
	.long	bc_impl_pop2                           /* 0x58 */ @ offset=352 (0x160)
	.long	bc_impl_dup                            /* 0x59 */ @ offset=356 (0x164)
	.long	bc_impl_dup_x1                         /* 0x5a */ @ offset=360 (0x168)
	.long	bc_impl_dup_x2                         /* 0x5b */ @ offset=364 (0x16c)
	.long	bc_impl_dup2                           /* 0x5c */ @ offset=368 (0x170)
	.long	bc_impl_dup2_x1                        /* 0x5d */ @ offset=372 (0x174)
	.long	bc_impl_dup2_x2                        /* 0x5e */ @ offset=376 (0x178)
	.long	bc_impl_swap                           /* 0x5f */ @ offset=380 (0x17c)
	.long	bc_impl_iadd                           /* 0x60 */ @ offset=384 (0x180)
	.long	bc_impl_ladd                           /* 0x61 */ @ offset=388 (0x184)
	.long	bc_impl_fadd                           /* 0x62 */ @ offset=392 (0x188)
	.long	bc_impl_dadd                           /* 0x63 */ @ offset=396 (0x18c)
	.long	bc_impl_isub                           /* 0x64 */ @ offset=400 (0x190)
	.long	bc_impl_lsub                           /* 0x65 */ @ offset=404 (0x194)
	.long	bc_impl_fsub                           /* 0x66 */ @ offset=408 (0x198)
	.long	bc_impl_dsub                           /* 0x67 */ @ offset=412 (0x19c)
	.long	bc_impl_imul                           /* 0x68 */ @ offset=416 (0x1a0)
	.long	bc_impl_lmul                           /* 0x69 */ @ offset=420 (0x1a4)
	.long	bc_impl_fmul                           /* 0x6a */ @ offset=424 (0x1a8)
	.long	bc_impl_dmul                           /* 0x6b */ @ offset=428 (0x1ac)
	.long	bc_impl_idiv                           /* 0x6c */ @ offset=432 (0x1b0)
	.long	bc_impl_ldiv                           /* 0x6d */ @ offset=436 (0x1b4)
	.long	bc_impl_fdiv                           /* 0x6e */ @ offset=440 (0x1b8)
	.long	bc_impl_ddiv                           /* 0x6f */ @ offset=444 (0x1bc)
	.long	bc_impl_irem                           /* 0x70 */ @ offset=448 (0x1c0)
	.long	bc_impl_lrem                           /* 0x71 */ @ offset=452 (0x1c4)
	.long	bc_impl_frem                           /* 0x72 */ @ offset=456 (0x1c8)
	.long	bc_impl_drem                           /* 0x73 */ @ offset=460 (0x1cc)
	.long	bc_impl_ineg                           /* 0x74 */ @ offset=464 (0x1d0)
	.long	bc_impl_lneg                           /* 0x75 */ @ offset=468 (0x1d4)
	.long	bc_impl_fneg                           /* 0x76 */ @ offset=472 (0x1d8)
	.long	bc_impl_dneg                           /* 0x77 */ @ offset=476 (0x1dc)
	.long	bc_impl_ishl                           /* 0x78 */ @ offset=480 (0x1e0)
	.long	bc_impl_lshl                           /* 0x79 */ @ offset=484 (0x1e4)
	.long	bc_impl_ishr                           /* 0x7a */ @ offset=488 (0x1e8)
	.long	bc_impl_lshr                           /* 0x7b */ @ offset=492 (0x1ec)
	.long	bc_impl_iushr                          /* 0x7c */ @ offset=496 (0x1f0)
	.long	bc_impl_lushr                          /* 0x7d */ @ offset=500 (0x1f4)
	.long	bc_impl_iand                           /* 0x7e */ @ offset=504 (0x1f8)
	.long	bc_impl_land                           /* 0x7f */ @ offset=508 (0x1fc)
	.long	bc_impl_ior                            /* 0x80 */ @ offset=512 (0x200)
	.long	bc_impl_lor                            /* 0x81 */ @ offset=516 (0x204)
	.long	bc_impl_ixor                           /* 0x82 */ @ offset=520 (0x208)
	.long	bc_impl_lxor                           /* 0x83 */ @ offset=524 (0x20c)
	.long	bc_impl_iinc                           /* 0x84 */ @ offset=528 (0x210)
	.long	bc_impl_i2l                            /* 0x85 */ @ offset=532 (0x214)
	.long	bc_impl_i2f                            /* 0x86 */ @ offset=536 (0x218)
	.long	bc_impl_i2d                            /* 0x87 */ @ offset=540 (0x21c)
	.long	bc_impl_l2i                            /* 0x88 */ @ offset=544 (0x220)
	.long	bc_impl_l2f                            /* 0x89 */ @ offset=548 (0x224)
	.long	bc_impl_l2d                            /* 0x8a */ @ offset=552 (0x228)
	.long	bc_impl_f2i                            /* 0x8b */ @ offset=556 (0x22c)
	.long	bc_impl_f2l                            /* 0x8c */ @ offset=560 (0x230)
	.long	bc_impl_f2d                            /* 0x8d */ @ offset=564 (0x234)
	.long	bc_impl_d2i                            /* 0x8e */ @ offset=568 (0x238)
	.long	bc_impl_d2l                            /* 0x8f */ @ offset=572 (0x23c)
	.long	bc_impl_d2f                            /* 0x90 */ @ offset=576 (0x240)
	.long	bc_impl_i2b                            /* 0x91 */ @ offset=580 (0x244)
	.long	bc_impl_i2c                            /* 0x92 */ @ offset=584 (0x248)
	.long	bc_impl_i2s                            /* 0x93 */ @ offset=588 (0x24c)
	.long	bc_impl_lcmp                           /* 0x94 */ @ offset=592 (0x250)
	.long	bc_impl_fcmpl                          /* 0x95 */ @ offset=596 (0x254)
	.long	bc_impl_fcmpg                          /* 0x96 */ @ offset=600 (0x258)
	.long	bc_impl_dcmpl                          /* 0x97 */ @ offset=604 (0x25c)
	.long	bc_impl_dcmpg                          /* 0x98 */ @ offset=608 (0x260)
	.long	bc_impl_ifeq                           /* 0x99 */ @ offset=612 (0x264)
	.long	bc_impl_ifne                           /* 0x9a */ @ offset=616 (0x268)
	.long	bc_impl_iflt                           /* 0x9b */ @ offset=620 (0x26c)
	.long	bc_impl_ifge                           /* 0x9c */ @ offset=624 (0x270)
	.long	bc_impl_ifgt                           /* 0x9d */ @ offset=628 (0x274)
	.long	bc_impl_ifle                           /* 0x9e */ @ offset=632 (0x278)
	.long	bc_impl_if_icmpeq                      /* 0x9f */ @ offset=636 (0x27c)
	.long	bc_impl_if_icmpne                      /* 0xa0 */ @ offset=640 (0x280)
	.long	bc_impl_if_icmplt                      /* 0xa1 */ @ offset=644 (0x284)
	.long	bc_impl_if_icmpge                      /* 0xa2 */ @ offset=648 (0x288)
	.long	bc_impl_if_icmpgt                      /* 0xa3 */ @ offset=652 (0x28c)
	.long	bc_impl_if_icmple                      /* 0xa4 */ @ offset=656 (0x290)
	.long	bc_impl_if_acmpeq                      /* 0xa5 */ @ offset=660 (0x294)
	.long	bc_impl_if_acmpne                      /* 0xa6 */ @ offset=664 (0x298)
	.long	bc_impl_goto                           /* 0xa7 */ @ offset=668 (0x29c)
	.long	0x0                      @ undefined bytecode 0xa8 offset=672 (0x2a0)
	.long	0x0                      @ undefined bytecode 0xa9 offset=676 (0x2a4)
	.long	bc_impl_tableswitch                    /* 0xaa */ @ offset=680 (0x2a8)
	.long	bc_impl_lookupswitch                   /* 0xab */ @ offset=684 (0x2ac)
	.long	bc_impl_ireturn                        /* 0xac */ @ offset=688 (0x2b0)
	.long	bc_impl_lreturn                        /* 0xad */ @ offset=692 (0x2b4)
	.long	bc_impl_freturn                        /* 0xae */ @ offset=696 (0x2b8)
	.long	bc_impl_dreturn                        /* 0xaf */ @ offset=700 (0x2bc)
	.long	bc_impl_areturn                        /* 0xb0 */ @ offset=704 (0x2c0)
	.long	bc_impl_return                         /* 0xb1 */ @ offset=708 (0x2c4)
	.long	bc_impl_getstatic                      /* 0xb2 */ @ offset=712 (0x2c8)
	.long	bc_impl_putstatic                      /* 0xb3 */ @ offset=716 (0x2cc)
	.long	bc_impl_getfield                       /* 0xb4 */ @ offset=720 (0x2d0)
	.long	bc_impl_putfield                       /* 0xb5 */ @ offset=724 (0x2d4)
	.long	bc_impl_invokevirtual                  /* 0xb6 */ @ offset=728 (0x2d8)
	.long	bc_impl_invokespecial                  /* 0xb7 */ @ offset=732 (0x2dc)
	.long	bc_impl_invokestatic                   /* 0xb8 */ @ offset=736 (0x2e0)
	.long	bc_impl_invokeinterface                /* 0xb9 */ @ offset=740 (0x2e4)
	.long	0x0                      @ undefined bytecode 0xba offset=744 (0x2e8)
	.long	bc_impl_new                            /* 0xbb */ @ offset=748 (0x2ec)
	.long	bc_impl_newarray                       /* 0xbc */ @ offset=752 (0x2f0)
	.long	bc_impl_anewarray                      /* 0xbd */ @ offset=756 (0x2f4)
	.long	bc_impl_arraylength                    /* 0xbe */ @ offset=760 (0x2f8)
	.long	bc_impl_athrow                         /* 0xbf */ @ offset=764 (0x2fc)
	.long	bc_impl_checkcast                      /* 0xc0 */ @ offset=768 (0x300)
	.long	bc_impl_instanceof                     /* 0xc1 */ @ offset=772 (0x304)
	.long	bc_impl_monitorenter                   /* 0xc2 */ @ offset=776 (0x308)
	.long	bc_impl_monitorexit                    /* 0xc3 */ @ offset=780 (0x30c)
	.long	bc_impl_wide                           /* 0xc4 */ @ offset=784 (0x310)
	.long	bc_impl_multianewarray                 /* 0xc5 */ @ offset=788 (0x314)
	.long	bc_impl_ifnull                         /* 0xc6 */ @ offset=792 (0x318)
	.long	bc_impl_ifnonnull                      /* 0xc7 */ @ offset=796 (0x31c)
	.long	bc_impl_goto_w                         /* 0xc8 */ @ offset=800 (0x320)
	.long	0x0                      @ undefined bytecode 0xc9 offset=804 (0x324)
	.long	bc_impl_breakpoint                     /* 0xca */ @ offset=808 (0x328)
	.long	bc_impl_fast_1_ldc                     /* 0xcb */ @ offset=812 (0x32c)
	.long	bc_impl_fast_1_ldc_w                   /* 0xcc */ @ offset=816 (0x330)
	.long	bc_impl_fast_2_ldc_w                   /* 0xcd */ @ offset=820 (0x334)
	.long	bc_impl_fast_1_putstatic               /* 0xce */ @ offset=824 (0x338)
	.long	bc_impl_fast_2_putstatic               /* 0xcf */ @ offset=828 (0x33c)
	.long	bc_impl_fast_a_putstatic               /* 0xd0 */ @ offset=832 (0x340)
	.long	bc_impl_fast_1_getstatic               /* 0xd1 */ @ offset=836 (0x344)
	.long	bc_impl_fast_2_getstatic               /* 0xd2 */ @ offset=840 (0x348)
	.long	bc_impl_fast_bputfield                 /* 0xd3 */ @ offset=844 (0x34c)
	.long	bc_impl_fast_sputfield                 /* 0xd4 */ @ offset=848 (0x350)
	.long	bc_impl_fast_iputfield                 /* 0xd5 */ @ offset=852 (0x354)
	.long	bc_impl_fast_lputfield                 /* 0xd6 */ @ offset=856 (0x358)
	.long	bc_impl_fast_fputfield                 /* 0xd7 */ @ offset=860 (0x35c)
	.long	bc_impl_fast_dputfield                 /* 0xd8 */ @ offset=864 (0x360)
	.long	bc_impl_fast_aputfield                 /* 0xd9 */ @ offset=868 (0x364)
	.long	bc_impl_fast_bgetfield                 /* 0xda */ @ offset=872 (0x368)
	.long	bc_impl_fast_sgetfield                 /* 0xdb */ @ offset=876 (0x36c)
	.long	bc_impl_fast_igetfield                 /* 0xdc */ @ offset=880 (0x370)
	.long	bc_impl_fast_lgetfield                 /* 0xdd */ @ offset=884 (0x374)
	.long	bc_impl_fast_fgetfield                 /* 0xde */ @ offset=888 (0x378)
	.long	bc_impl_fast_dgetfield                 /* 0xdf */ @ offset=892 (0x37c)
	.long	bc_impl_fast_agetfield                 /* 0xe0 */ @ offset=896 (0x380)
	.long	bc_impl_fast_cgetfield                 /* 0xe1 */ @ offset=900 (0x384)
	.long	bc_impl_fast_invokevirtual             /* 0xe2 */ @ offset=904 (0x388)
	.long	bc_impl_fast_invokestatic              /* 0xe3 */ @ offset=908 (0x38c)
	.long	bc_impl_fast_invokeinterface           /* 0xe4 */ @ offset=912 (0x390)
	.long	bc_impl_fast_invokenative              /* 0xe5 */ @ offset=916 (0x394)
	.long	bc_impl_fast_invokevirtual_final       /* 0xe6 */ @ offset=920 (0x398)
	.long	bc_impl_fast_invokespecial             /* 0xe7 */ @ offset=924 (0x39c)
	.long	bc_impl_fast_new                       /* 0xe8 */ @ offset=928 (0x3a0)
	.long	bc_impl_fast_anewarray                 /* 0xe9 */ @ offset=932 (0x3a4)
	.long	bc_impl_fast_checkcast                 /* 0xea */ @ offset=936 (0x3a8)
	.long	bc_impl_fast_instanceof                /* 0xeb */ @ offset=940 (0x3ac)
	.long	bc_impl_fast_igetfield_1               /* 0xec */ @ offset=944 (0x3b0)
	.long	bc_impl_fast_agetfield_1               /* 0xed */ @ offset=948 (0x3b4)
	.long	bc_impl_aload_0_fast_agetfield_1       /* 0xee */ @ offset=952 (0x3b8)
	.long	bc_impl_aload_0_fast_igetfield_1       /* 0xef */ @ offset=956 (0x3bc)
	.long	bc_impl_pop_and_npe_if_null            /* 0xf0 */ @ offset=960 (0x3c0)
	.long	bc_impl_init_static_array              /* 0xf1 */ @ offset=964 (0x3c4)
	.long	bc_impl_aload_0_fast_agetfield_4       /* 0xf2 */ @ offset=968 (0x3c8)
	.long	bc_impl_aload_0_fast_igetfield_4       /* 0xf3 */ @ offset=972 (0x3cc)
	.long	bc_impl_aload_0_fast_agetfield_8       /* 0xf4 */ @ offset=976 (0x3d0)
	.long	bc_impl_aload_0_fast_igetfield_8       /* 0xf5 */ @ offset=980 (0x3d4)
	.long	bc_impl_fast_init_1_putstatic          /* 0xf6 */ @ offset=984 (0x3d8)
	.long	bc_impl_fast_init_2_putstatic          /* 0xf7 */ @ offset=988 (0x3dc)
	.long	bc_impl_fast_init_a_putstatic          /* 0xf8 */ @ offset=992 (0x3e0)
	.long	bc_impl_fast_init_1_getstatic          /* 0xf9 */ @ offset=996 (0x3e4)
	.long	bc_impl_fast_init_2_getstatic          /* 0xfa */ @ offset=1000 (0x3e8)
	.long	bc_impl_fast_init_invokestatic         /* 0xfb */ @ offset=1004 (0x3ec)
	.long	bc_impl_fast_init_new                  /* 0xfc */ @ offset=1008 (0x3f0)
	.long	0x0                      @ undefined bytecode 0xfd offset=1012 (0x3f4)
	.long	0x0                      @ offset=1016 (0x3f8)
	.long	0x0                      @ offset=1020 (0x3fc)

	.global	gp_constants
gp_constants:

	.global	_current_stack_limit
_current_stack_limit:
	.space	4                        @ offset=1024 (0x400)

	.global	_compiler_stack_limit
_compiler_stack_limit:
	.space	4                        @ offset=1028 (0x404)

	.global	_rt_timer_ticks
_rt_timer_ticks:
	.space	4                        @ offset=1032 (0x408)

	.global	jvm_fast_globals
jvm_fast_globals:

	.global	_current_thread
_current_thread:
	.space	4                        @ offset=1036 (0x40c)

	.global	_current_pending_exception
_current_pending_exception:
	.space	4                        @ offset=1040 (0x410)

	.global	_last_handle
_last_handle:
	.space	4                        @ offset=1044 (0x414)

	.global	_collection_area_start
_collection_area_start:
	.space	4                        @ offset=1048 (0x418)

	.global	_heap_start
_heap_start:
	.space	4                        @ offset=1052 (0x41c)

	.global	_heap_top
_heap_top:
	.space	4                        @ offset=1056 (0x420)

	.global	_inline_allocation_top
_inline_allocation_top:
	.space	4                        @ offset=1060 (0x424)

	.global	_inline_allocation_end
_inline_allocation_end:
	.space	4                        @ offset=1064 (0x428)

	.global	_heap_limit
_heap_limit:
	.space	4                        @ offset=1068 (0x42c)

	.global	_heap_end
_heap_end:
	.space	4                        @ offset=1072 (0x430)

	.global	_bitvector_base
_bitvector_base:
	.space	4                        @ offset=1076 (0x434)

	.global	_bit_selector
_bit_selector:
	.long	0x80808080               @ offset=1080 (0x438)

	.global	_young_generation_start
_young_generation_start:
	.space	4                        @ offset=1084 (0x43c)

	.global	_collection_area_end
_collection_area_end:
	.space	4                        @ offset=1088 (0x440)

	.global	_young_generation_target_size
_young_generation_target_size:
	.space	4                        @ offset=1092 (0x444)

	.global	_end_fixed_objects
_end_fixed_objects:
	.space	4                        @ offset=1096 (0x448)

	.global	_marking_stack_start
_marking_stack_start:
	.space	4                        @ offset=1100 (0x44c)

	.global	_marking_stack_top
_marking_stack_top:
	.space	4                        @ offset=1104 (0x450)

	.global	_marking_stack_end
_marking_stack_end:
	.space	4                        @ offset=1108 (0x454)

	.global	_marking_stack_overflow
_marking_stack_overflow:
	.space	4                        @ offset=1112 (0x458)

	.global	_slices_start
_slices_start:
	.space	4                        @ offset=1116 (0x45c)

	.global	_slice_shift
_slice_shift:
	.space	4                        @ offset=1120 (0x460)

	.global	_slice_offset_bits
_slice_offset_bits:
	.space	4                        @ offset=1124 (0x464)

	.global	_slice_size
_slice_size:
	.space	4                        @ offset=1128 (0x468)

	.global	_near_mask
_near_mask:
	.space	4                        @ offset=1132 (0x46c)

	.global	_slice_offset_mask
_slice_offset_mask:
	.space	4                        @ offset=1136 (0x470)

	.global	_nof_slices
_nof_slices:
	.space	4                        @ offset=1140 (0x474)

	.global	_compaction_start
_compaction_start:
	.space	4                        @ offset=1144 (0x478)

	.global	_compaction_top
_compaction_top:
	.space	4                        @ offset=1148 (0x47c)

	.global	_heap_min
_heap_min:
	.space	4                        @ offset=1152 (0x480)

	.global	_heap_capacity
_heap_capacity:
	.space	4                        @ offset=1156 (0x484)

	.global	_heap_size
_heap_size:
	.space	4                        @ offset=1160 (0x488)

	.global	_compiler_area_start
_compiler_area_start:
	.space	4                        @ offset=1164 (0x48c)

	.global	_compiler_area_top
_compiler_area_top:
	.space	4                        @ offset=1168 (0x490)

	.global	_large_object_area_bottom
_large_object_area_bottom:
	.space	4                        @ offset=1172 (0x494)

	.global	_compiler_area_temp_top
_compiler_area_temp_top:
	.space	4                        @ offset=1176 (0x498)

	.global	_compiler_area_temp_bottom
_compiler_area_temp_bottom:
	.space	4                        @ offset=1180 (0x49c)

	.global	_compiler_method
_compiler_method:
	.space	4                        @ offset=1184 (0x4a0)

	.global	_compiler_code_generator
_compiler_code_generator:
	.space	4                        @ offset=1188 (0x4a4)

	.global	_compiler_bci
_compiler_bci:
	.space	4                        @ offset=1192 (0x4a8)

	.global	_compiler_closure
_compiler_closure:
	.space	4                        @ offset=1196 (0x4ac)

	.global	_num_stack_lock_words
_num_stack_lock_words:
	.space	4                        @ offset=1200 (0x4b0)

	.global	_class_list_base
_class_list_base:
	.space	4                        @ offset=1204 (0x4b4)

	.global	_mirror_list_base
_mirror_list_base:
	.space	4                        @ offset=1208 (0x4b8)

	.global	_interpretation_log
_interpretation_log:
	.space	36                       @ offset=1212 (0x4bc)

	.global	_interpretation_log_idx
_interpretation_log_idx:
	.space	4                        @ offset=1248 (0x4e0)

	.global	_primordial_sp
_primordial_sp:
	.space	4                        @ offset=1252 (0x4e4)

	.global	_old_generation_end
_old_generation_end:
	.space	4                        @ offset=1256 (0x4e8)

	.global	_bytecode_counter
_bytecode_counter:
	.space	4                        @ offset=1260 (0x4ec)

	.global	_kni_parameter_base
_kni_parameter_base:
	.space	4                        @ offset=1264 (0x4f0)

	.global	_rom_constant_pool_fast
_rom_constant_pool_fast:
	.space	4                        @ offset=1268 (0x4f4)

	.global	_gp_bytecode_counter
_gp_bytecode_counter:
	.space	4                        @ offset=1272 (0x4f8)

	.global	_jvm_in_quick_native_method
_jvm_in_quick_native_method:
	.space	4                        @ offset=1276 (0x4fc)

	.global	_jvm_quick_native_exception
_jvm_quick_native_exception:
	.space	4                        @ offset=1280 (0x500)

	.global	_method_execution_sensor
_method_execution_sensor:
	.space	512                      @ offset=1284 (0x504)

	.global	_interned_string_near_addr
_interned_string_near_addr:
	.space	4                        @ offset=1796 (0x704)

	.global	_persistent_handles_addr
_persistent_handles_addr:
	.space	4                        @ offset=1800 (0x708)

	.global	_current_task
_current_task:
	.space	4                        @ offset=1804 (0x70c)

	.global	_task_class_init_marker
_task_class_init_marker:
	.space	4                        @ offset=1808 (0x710)

	.global	gp_stack_overflow_ptr
gp_stack_overflow_ptr:
	.extern	stack_overflow
	.long	stack_overflow           @ offset=1812 (0x714)

	.global	gp_timer_tick_ptr
gp_timer_tick_ptr:
	.extern	timer_tick
	.long	timer_tick               @ offset=1816 (0x718)

	.global	gp_lock_stack_lock_ptr
gp_lock_stack_lock_ptr:
	.extern	lock_stack_lock
	.long	lock_stack_lock          @ offset=1820 (0x71c)

	.global	gp_signal_waiters_ptr
gp_signal_waiters_ptr:
	.extern	signal_waiters
	.long	signal_waiters           @ offset=1824 (0x720)

	.global	gp_array_index_out_of_bounds_exception_ptr
gp_array_index_out_of_bounds_exception_ptr:
	.extern	array_index_out_of_bounds_exception
	.long	array_index_out_of_bounds_exception @ offset=1828 (0x724)

	.global	gp_get_array_index_out_of_bounds_exception_ptr
gp_get_array_index_out_of_bounds_exception_ptr:
	.extern	get_array_index_out_of_bounds_exception
	.long	get_array_index_out_of_bounds_exception @ offset=1832 (0x728)

	.global	gp_null_pointer_exception_ptr
gp_null_pointer_exception_ptr:
	.extern	null_pointer_exception
	.long	null_pointer_exception   @ offset=1836 (0x72c)

	.global	gp_get_null_pointer_exception_ptr
gp_get_null_pointer_exception_ptr:
	.extern	get_null_pointer_exception
	.long	get_null_pointer_exception @ offset=1840 (0x730)

	.global	gp_illegal_monitor_state_exception_ptr
gp_illegal_monitor_state_exception_ptr:
	.extern	illegal_monitor_state_exception
	.long	illegal_monitor_state_exception @ offset=1844 (0x734)

	.global	gp_get_illegal_monitor_state_exception_ptr
gp_get_illegal_monitor_state_exception_ptr:
	.extern	get_illegal_monitor_state_exception
	.long	get_illegal_monitor_state_exception @ offset=1848 (0x738)

	.global	gp_division_by_zero_exception_ptr
gp_division_by_zero_exception_ptr:
	.extern	division_by_zero_exception
	.long	division_by_zero_exception @ offset=1852 (0x73c)

	.global	gp_get_division_by_zero_exception_ptr
gp_get_division_by_zero_exception_ptr:
	.extern	get_division_by_zero_exception
	.long	get_division_by_zero_exception @ offset=1856 (0x740)

	.global	gp_get_incompatible_class_change_error_ptr
gp_get_incompatible_class_change_error_ptr:
	.extern	get_incompatible_class_change_error
	.long	get_incompatible_class_change_error @ offset=1860 (0x744)

	.global	gp_multianewarray_ptr
gp_multianewarray_ptr:
	.extern	multianewarray
	.long	multianewarray           @ offset=1864 (0x748)

	.global	gp_instanceof_ptr
gp_instanceof_ptr:
	.extern	instanceof
	.long	instanceof               @ offset=1868 (0x74c)

	.global	gp__instanceof_ptr
gp__instanceof_ptr:
	.extern	_instanceof
	.long	_instanceof              @ offset=1872 (0x750)

	.global	gp_checkcast_ptr
gp_checkcast_ptr:
	.extern	checkcast
	.long	checkcast                @ offset=1876 (0x754)

	.global	gp_array_store_type_check_ptr
gp_array_store_type_check_ptr:
	.extern	array_store_type_check
	.long	array_store_type_check   @ offset=1880 (0x758)

	.global	gp_compiled_code_task_barrier_ptr
gp_compiled_code_task_barrier_ptr:
	.extern	compiled_code_task_barrier
	.long	compiled_code_task_barrier @ offset=1884 (0x75c)

	.global	gp_jvm_ldiv_ptr
gp_jvm_ldiv_ptr:
	.extern	jvm_ldiv
	.long	jvm_ldiv                 @ offset=1888 (0x760)

	.global	gp_jvm_lrem_ptr
gp_jvm_lrem_ptr:
	.extern	jvm_lrem
	.long	jvm_lrem                 @ offset=1892 (0x764)

	.global	gp_native_system_arraycopy_entry_ptr
gp_native_system_arraycopy_entry_ptr:
	.long	native_system_arraycopy_entry @ offset=1896 (0x768)

	.global	gp_native_jvm_unchecked_byte_arraycopy_entry_ptr
gp_native_jvm_unchecked_byte_arraycopy_entry_ptr:
	.long	native_jvm_unchecked_byte_arraycopy_entry @ offset=1900 (0x76c)

	.global	gp_native_jvm_unchecked_char_arraycopy_entry_ptr
gp_native_jvm_unchecked_char_arraycopy_entry_ptr:
	.long	native_jvm_unchecked_char_arraycopy_entry @ offset=1904 (0x770)

	.global	gp_native_jvm_unchecked_int_arraycopy_entry_ptr
gp_native_jvm_unchecked_int_arraycopy_entry_ptr:
	.long	native_jvm_unchecked_int_arraycopy_entry @ offset=1908 (0x774)

	.global	gp_native_jvm_unchecked_obj_arraycopy_entry_ptr
gp_native_jvm_unchecked_obj_arraycopy_entry_ptr:
	.long	native_jvm_unchecked_obj_arraycopy_entry @ offset=1912 (0x778)

	.global	gp_native_string_indexof_entry_ptr
gp_native_string_indexof_entry_ptr:
	.long	native_string_indexof_entry @ offset=1916 (0x77c)

	.global	gp_native_string_indexof0_entry_ptr
gp_native_string_indexof0_entry_ptr:
	.long	native_string_indexof0_entry @ offset=1920 (0x780)

	.global	gp_fixed_interpreter_fast_method_entry_0_ptr
gp_fixed_interpreter_fast_method_entry_0_ptr:
	.long	fixed_interpreter_fast_method_entry_0 @ offset=1924 (0x784)

	.global	gp_fixed_interpreter_fast_method_entry_1_ptr
gp_fixed_interpreter_fast_method_entry_1_ptr:
	.long	fixed_interpreter_fast_method_entry_1 @ offset=1928 (0x788)

	.global	gp_fixed_interpreter_fast_method_entry_2_ptr
gp_fixed_interpreter_fast_method_entry_2_ptr:
	.long	fixed_interpreter_fast_method_entry_2 @ offset=1932 (0x78c)

	.global	gp_fixed_interpreter_fast_method_entry_3_ptr
gp_fixed_interpreter_fast_method_entry_3_ptr:
	.long	fixed_interpreter_fast_method_entry_3 @ offset=1936 (0x790)

	.global	gp_fixed_interpreter_fast_method_entry_4_ptr
gp_fixed_interpreter_fast_method_entry_4_ptr:
	.long	fixed_interpreter_fast_method_entry_4 @ offset=1940 (0x794)

	.global	gp__newobject_ptr
gp__newobject_ptr:
	.long	_newobject               @ offset=1944 (0x798)

	.global	gp_interpreter_throw_NullPointerException_ptr
gp_interpreter_throw_NullPointerException_ptr:
	.long	interpreter_throw_NullPointerException @ offset=1948 (0x79c)

	.global	gp_interpreter_throw_NullPointerException_tos_cached_ptr
gp_interpreter_throw_NullPointerException_tos_cached_ptr:
	.long	interpreter_throw_NullPointerException_tos_cached @ offset=1952 (0x7a0)

	.global	gp_interpreter_throw_IncompatibleClassChangeError_ptr
gp_interpreter_throw_IncompatibleClassChangeError_ptr:
	.long	interpreter_throw_IncompatibleClassChangeError @ offset=1956 (0x7a4)

	.global	gp_interpreter_grow_stack_ptr
gp_interpreter_grow_stack_ptr:
	.long	interpreter_grow_stack   @ offset=1960 (0x7a8)

	.global	gp_interpreter_throw_ArrayIndexOutOfBoundsException_ptr
gp_interpreter_throw_ArrayIndexOutOfBoundsException_ptr:
	.long	interpreter_throw_ArrayIndexOutOfBoundsException @ offset=1964 (0x7ac)
	.extern	_rom_linkcheck_hle_true
	.long	_rom_linkcheck_hle_true  @ offset=1968 (0x7b0)
	.extern	_rom_linkcheck_mffd_false
	.long	_rom_linkcheck_mffd_false @ offset=1972 (0x7b4)
	.extern	_rom_linkcheck_mffl_false
	.long	_rom_linkcheck_mffl_false @ offset=1976 (0x7b8)

	.global	gp_constants_end
gp_constants_end:
	@  End segment: Global pool


	.end
