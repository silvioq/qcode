# Este script arranca de 20 y va bajando hasta el 5
    sto  r1, 20
    sto  r2, 5
while1:
    dec  r1
    equ  r1, r2
    jnz  while1
    sto  r0, 0

