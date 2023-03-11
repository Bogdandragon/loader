Nume: Angheloiu George-Bogdan
Grupă: 324CB

# Tema 1

Se rulează un fişier de tip ELF care nu are memoria mapată, lucru de care se ocupă programul. Am realizat un handler pentru
semnalele primite, dar care se ocupă doar de semnalul SEGFAULT, celelalte mergând pe un handler default. Paşii realizaţi sunt următorii:

1. Calculăm segmentul în care ne aflăm şi pagina corespunzătoare din el. Dacă nu ne aflăm într-unul dintre segmentele valide,
apelăm handlerul default.
2. Verificăm dacă pagina la care ne aflăm a mai fost mapată. Dacă a mai fost, apelăm handlerul default. Dacă nu, continuăm spre mapare.
Verificarea se face stocând în câmpul data al segmentului un vector de aparitii pentru paginile care îl reprezintă.
3. Mapăm pagina şi calculăm cât trebuie citit din fisier în pagină. Dacă nu se citeşte nimic, se termină funcţia.
4. Dacă se citeşte ceva din fişier, se citeşte cel mult dimensiunea unei pagini. Citim într-un buffer de dimensiunea paginii
cât trebuie din fişier. Apoi, copiem din buffer în pagină şi setăm permisiunile corespunzătoare.

Paşii de mai sus tratează toate posibilele semnale şi toate posibilităţii apariţiei semnalului SIGSEGV. Tema nu a fost lungă din punctul
de vedere al scrierii, dar a fost extrem de mult research de făcut chiar şi înafara laboratoarelor disponibile. De asemenea, nu mi se pare
suficientă libertate creativă: există un singur mod de a implementa tema, iar paşii (şi funcţiile folosite) sunt în mare la fel pentru
orice mod aş vedea în care să poată fi implementată tema diferit.