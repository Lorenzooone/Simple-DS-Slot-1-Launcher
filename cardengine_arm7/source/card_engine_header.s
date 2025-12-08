@---------------------------------------------------------------------------------
	.section ".init"
@---------------------------------------------------------------------------------
	.global _start
	.align	4
	.arm

.global card_engine_start
.global card_engine_start_sync
.global card_engine_end
.global cardStruct
.global copy_offset
.global reset_data
.global boot_path
.global sdk_version
.global fileCluster
.global saveCluster
.global saveSize
.global language
.global gameSoftReset
.global cheat_data_offset

#define ICACHE_SIZE	0x2000
#define DCACHE_SIZE	0x1000
#define CACHE_LINE_SIZE	32

copy_offset:
	.word copy_offset
copy_end:
	.word end
patches_offset:
	.word	patches
intr_vblank_orig_return:
	.word	0x00000000
language:
	.word	0x00000000
gameSoftReset:
	.word	0x00000000
cheat_data_offset:    
	.word	0x00000000
boot_type:
	.word	0x00000000
read_power_button:
	.word	0x00000000
reset_data_offset:
	.word	reset_data - copy_offset
reset_data_len:
	.word	reset_data_end - reset_data
boot_path_offset:
	.word	boot_path - copy_offset
boot_path_max_len:
	.word	boot_path_end - boot_path
reset_data:
	.space 32
reset_data_end:
boot_path:
	.space 256
boot_path_end:

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

card_engine_start:

vblankHandler:
@ Hook the return address, then go back to the original function
	stmdb	sp!, {lr}
	adr 	lr, code_handler_start_vblank
	ldr 	r0,	intr_vblank_orig_return
	bx  	r0

code_handler_start_vblank:
	push	{r0-r12} 
	ldr	r3, =myIrqHandlerVBlank
	bl	_blx_r3_stub		@ jump to myIrqHandler

	@ exit after return
	b	exit

@---------------------------------------------------------------------------------
_blx_r3_stub:
@---------------------------------------------------------------------------------
	bx	r3

@---------------------------------------------------------------------------------

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

exit:
	pop   	{r0-r12} 
	pop  	{lr}
	bx  lr

.pool

card_engine_end:

patches:
.word	vblankHandler
.pool
