# InstantCompiler

Kompilator języka **Instant** do dwóch platform:

-   **JVM** (Java Virtual Machine)
-   **LLVM** (Low Level Virtual Machine)

Projekt napisany w **C++17**, wykorzystujący:

-   **BNFC** – generacja parsera i AST z pliku gramatyki `Instant.cf`
-   **Jasmin** – assembler Javy do tworzenia plików `.class`
-   **LLVM tools** – generacja kodu pośredniego `.ll` i bitkodu `.bc`
-   **Java Runtime** – klasa `Runtime` z metodą `printInt(I)V`

---

# Budowanie projektu

## 1. Generowanie parsera i AST (tylko przy pierwszym uruchomieniu)

Jeśli w katalogu `src/frontend` nie ma plików `Absyn.C`, `Parser.C`, itp.,  
należy najpierw wygenerować je przy pomocy **BNFC**:

bnfc --cpp -m src/Instant.cf -o src/frontend  
make -C src/frontend

## 2. Kompilacja kompilatorów JVM i LLVM

W katalogu głównym:  
make  
pojawią się pliki wykonywalne:  
./insc_jvm
./insc_llvm

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

### Przykład:

./insc_jvm examples/test.ins

### Wynik:

Zapisano: examples/test.j  
Utworzono: examples/test.class

### Aby uruchomić wygenerowaną klasę:

java -cp ".;lib;examples" test

### Przykład:

./insc_llvm examples/test07.ins

### Wynik:

$ ./insc_llvm examples/test07.ins
Plik otwarty poprawnie: examples/test07.ins
Parsowanie powiodlo sie!
Wyjscie: examples/test07.ll oraz examples/test07.bc
Zapisano: examples/test07.ll
Utworzono: examples/test07.bc

### Aby uruchomić wygenerowaną klasę:

lli examples/test07.bc

# Użyte materiały i inspiracje

-   [Sethi–Ullman Algorithm](https://en.wikipedia.org/wiki/Sethi%E2%80%93Ullman_algorithm)
-   [lolzdev – I made a C compiler in C](https://www.youtube.com/watch?v=UW8LgC-S_co)
-   [Pixeled – Creating a Compiler playlist](https://www.youtube.com/watch?v=vcSijrRsrY0&list=PLUDlas_Zy_qC7c5tCgTMYq2idyyT241qs)
-   Wsparcie narzędziowe, struktura projektu oraz część dokumentacji przygotowane z pomocą ChatGPT (organizacja kodu, Makefile, README).

---

## Wymagania środowiskowe

-   C++17
-   BNFC
-   Jasmin (`lib/jasmin.jar`)
-   Java 11+ (JRE/JDK)
-   llvm, lli (na mysys2: pacman -S mingw-w64-ucrt-x86_64-llvm)

---

## Github:

https://github.com/JakubPaczek/InstantCompiler
