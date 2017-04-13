
teflon.so: teflon.c
	gcc -shared -fPIC $< -o $@ -ldl

