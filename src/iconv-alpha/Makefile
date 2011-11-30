FOLDER="/home/jan/tmp/eglibc-2.13/"

all: ALPHA.so

ALPHA.so: alpha.h alpha.c
	gcc -O2 -g -D_GNU_SOURCE alpha.c -I. -I${FOLDER}/sysdeps/x86_64/ -I${FOLDER}/ -I${FOLDER}/iconvdata/ -shared -fPIC -o ALPHA.so

alpha.h: ALPHA
	sh -c 'PWD="`pwd`"; cd ${FOLDER}/iconvdata; sh ./gen-8bit-gap.sh "${PWD}/ALPHA" > alpha.h; mv alpha.h "${PWD}";'

clean:
	rm alpha.h ALPHA.so
