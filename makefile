lomo: lomo.c liblomo.a
	gcc lomo.c liblomo.a -o lomo -m32

clean: 
	rm -r *.o
