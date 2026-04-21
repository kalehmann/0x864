demo_01_call:
	call bar
	retn

foo:
	nop
	retn

bar:
	call foo
	retn
