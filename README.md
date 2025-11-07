# InstantCompiler

Kompilator języka **Instant** do **JVM** (w przyszłości także LLVM).  
Projekt realizowany w języku C++ z wykorzystaniem **BNFC**, **Jasmin** oraz **Java Runtime** (dla metody `printInt`).

---

# Budowanie projektu

## 1. Wygenerowanie parsera i AST z BNFC

bnfc --cpp -m src/Instant.cf -o src/frontend  
make -C src/frontend

## 2. Kompilacja kompilatora JVM

make  
W katalogu głównym pojawi się plik wykonywalny:  
./insc_jvm

---

# Działanie programu

## Format języka Instant

Program to lista instrukcji rozdzielonych średnikami:

-   `x = e;` – przypisanie
-   `e;` – wyrażenie, które wypisuje wynik

Przykład:  
1+2;  
5+7+3;  
a=2;  
a+2;

## Etapy działania kompilatora JVM

1. Parsowanie programu przy użyciu BNFC (`pProgram`).
2. Obliczenie:
    - `.limit stack` – na podstawie **Sethi–Ullman algorithm**  
      (https://en.wikipedia.org/wiki/Sethi%E2%80%93Ullman_algorithm)
    - `.limit locals` – liczba zmiennych + 1 (dla `args`).
3. Generacja kodu `.j` (Jasmin assembler).
4. Złożenie `.class` przez Jasmin (`lib/jasmin.jar`).
5. Uruchomienie programu w JVM.

---

## Kompilacja runtime

Runtime (jedna metoda `printInt(I)V`) jest umieszczony w katalogu `lib`:  
javac -d lib lib/Runtime.java

---

## Uruchamianie programu Instant

Przykład:  
./insc_jvm examples/test.ins

Wynik:  
Zapisano: examples/test.j  
Utworzono: examples/test.class

Aby uruchomić wygenerowaną klasę:  
java -cp ".;lib;examples" test

Przykład:  
./insc_llvm examples/test07.ins

Wynik:  
$ ./insc_llvm examples/test07.ins
Plik otwarty poprawnie: examples/test07.ins
Parsowanie powiodlo sie!
Wyjscie: examples/test07.ll oraz examples/test07.bc
Zapisano: examples/test07.ll
Utworzono: examples/test07.bc

Aby uruchomić wygenerowaną klasę:  
lli examples/test07.bc

# Użyte materiały i inspiracje

-   [Sethi–Ullman Algorithm](https://en.wikipedia.org/wiki/Sethi%E2%80%93Ullman_algorithm)
-   [lolzdev – I made a C compiler in C](https://www.youtube.com/watch?v=UW8LgC-S_co)
-   [Pixeled – Creating a Compiler playlist](https://www.youtube.com/watch?v=vcSijrRsrY0&list=PLUDlas_Zy_qC7c5tCgTMYq2idyyT241qs)
-   Wsparcie narzędziowe: Makefile, ścieżki, struktura projektu i opracowanie README przygotowane z pomocą ChatGPT

---

## Wymagania środowiskowe

-   C++17
-   BNFC
-   Jasmin (`lib/jasmin.jar`)
-   Java 11+ (JRE/JDK)
-   llvm (w mysys2: pacman -S mingw-w64-ucrt-x86_64-llvm)

---

## Pakowanie do tar:

tar.exe -czf InstantCompiler.tgz `  --exclude=.git`
--exclude=.vscode `  --exclude='*.o'`
--exclude='_.obj' `
--exclude='_.exe' `  --exclude='*.class'`
--exclude='_.j' `
--exclude='_.ll' `  --exclude='*.bc'`
--exclude='\*.tgz' `
InstantCompiler

## Github:

https://github.com/JakubPaczek/InstantCompiler
