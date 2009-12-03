
puntero:  1

   sto r1, 1
   dec r1
   jnz error

   sto r1, &puntero
   dec r1
   jnz error

ok:
   sto r0, 0
   ret

error:
   ret
