
teflon: teflon.c
	gcc -shared -fPIC -pie -rdynamic $< -o $@ -ldl

