root=`pwd`
mkdir -p out >/dev/null
cd out
src=$root/src
libjit=$root/../libjit

build() {
  gcc -std=c99 -Wall -fPIC -g -c $src/$1 -I $libjit/include
}
build toy_jit.c
build test.c
build jni_main.c
gcc -o test test.o toy_jit.o $libjit/jit/.libs/libjit.a -lm -lpthread
gcc -shared -o libtoyjit.so toy_jit.o jni_main.o -L $libjit/jit/.libs -ljit -lc

