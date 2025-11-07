#include <cstdio>
#include <cstring>
#include <string>
#include <sstream>
#include <iostream>
#include <unordered_map>
#include <algorithm> // std::max

#include "Parser.H"
#include "Absyn.H"
#include "ParserError.H"
#include "Printer.H"

// -----------------------------------------------------------------------------------
// .limit stack
static std::unordered_map<Exp*, int> SU_CACHE;

// Sethi–Ullman
static int su(Exp* e)
{
    if (dynamic_cast<ExpLit*>(e)) return 1; // literal (value)
    if (dynamic_cast<ExpVar*>(e)) return 1; // variable

    // + and * is commutative | chose optimal order
    if (auto a = dynamic_cast<ExpAdd*>(e))
    {
        int L = su(a->exp_1), R = su(a->exp_2);
        return (L == R) ? L + 1 : (L > R ? L : R);
    }
    if (auto m = dynamic_cast<ExpMul*>(e))
    {
        int L = su(m->exp_1), R = su(m->exp_2);
        return (L == R) ? L + 1 : (L > R ? L : R);
    }

    // / and - is not commutative | can't change order
    if (auto s = dynamic_cast<ExpSub*>(e))
    {
        int L = su(s->exp_1), R = su(s->exp_2);
        return std::max(L, R + 1);
    }
    if (auto d = dynamic_cast<ExpDiv*>(e))
    {
        int L = su(d->exp_1), R = su(d->exp_2);
        return std::max(L, R + 1);
    }
    return 1; // default
}

// for O(nlog(n)) time complexity
static int su_memo(Exp* e)
{
    auto it = SU_CACHE.find(e); // find expression in unordered_map
    if (it != SU_CACHE.end()) return it->second;
    int val = su(e);
    SU_CACHE[e] = val; // add expresion to unordered_map
    return val;
}

// count max stack
static int max_stack_prog(Program* p)
{
    int M = 1; // default
    if (auto P = dynamic_cast<Prog*>(p))
    {
        for (Stmt* s : *P->liststmt_) // iterate through instructions
        {
            if (auto se = dynamic_cast<SExp*>(s))   M = std::max(M, su_memo(se->exp_)); // expression
            else if (auto sa = dynamic_cast<SAss*>(s)) M = std::max(M, su_memo(sa->exp_)); // assesment
        }
    }
    return M; // maximum
}
// -----------------------------------------------------------------------------------

// -----------------------------------------------------------------------------------
// .limit locals
struct VarSlots {
    std::unordered_map<std::string, int> slot;
    int next_slot = 1;

    int assign_if_new(const std::string& name)
    {
        auto it = slot.find(name);
        if (it != slot.end()) return it->second;
        int s = next_slot++; // reserve slot
        slot.emplace(name, s);
        return s;
    }

    // find slot (or return -1)
    int find(const std::string& name) const
    {
        auto it = slot.find(name);
        return (it == slot.end()) ? -1 : it->second;
    }
};

static void assign_slots_in_order(Program* p, VarSlots& vs)
{
    if (auto P = dynamic_cast<Prog*>(p))
    {
        for (Stmt* s : *P->liststmt_) // iterate through instructions
        {
            if (auto sa = dynamic_cast<SAss*>(s))
            {
                vs.assign_if_new(sa->ident_);
            }
        }
    }
}
// -----------------------------------------------------------------------------------

// -----------------------------------------------------------------------------------
// Jasmin Emitter
struct Emitter {
    std::ostringstream buf;

    void emit(const std::string& s) { buf << s << "\n"; }

    // int constants
    void iconst(int v)
    {
        if (v == -1) { emit("  iconst_m1"); return; }
        if (0 <= v && v <= 5) { emit("  iconst_" + std::to_string(v)); return; }
        if (-128 <= v && v <= 127) { emit("  bipush " + std::to_string(v)); return; }
        if (-32768 <= v && v <= 32767) { emit("  sipush " + std::to_string(v)); return; }
        emit("  ldc " + std::to_string(v));
    }

    // load/store (_0-_3)
    static std::string idx4(int i, const char* base)
    {
        if (0 <= i && i <= 3) return std::string(base) + "_" + std::to_string(i);
        return std::string(base) + " " + std::to_string(i);
    }
    void iload(int i) { emit("  " + idx4(i, "iload")); }
    void istore(int i) { emit("  " + idx4(i, "istore")); }

    // binop
    void iadd() { emit("  iadd"); }
    void isub() { emit("  isub"); }
    void imul() { emit("  imul"); }
    void idiv() { emit("  idiv"); }
};
// -----------------------------------------------------------------------------------

// -----------------------------------------------------------------------------------
// generator
static void gen_exp(Exp* e, Emitter& E, VarSlots& vs)
{
    if (auto k = dynamic_cast<ExpLit*>(e)) { E.iconst(k->integer_); return; }

    if (auto v = dynamic_cast<ExpVar*>(e))
    {
        std::string name = v->ident_;
        int s = vs.find(name);
        if (s < 0)
        {
            std::cerr << "Blad: uzycie zmiennej '" << name << "' przed przypisaniem.\n";
            std::exit(2);
        }
        E.iload(s);
        return;
    }

    if (auto a = dynamic_cast<ExpAdd*>(e))
    {
        int L = su_memo(a->exp_1), R = su_memo(a->exp_2);
        // komutatywne: najpierw cięższe – mniejszy szczyt stosu
        if (R > L) { gen_exp(a->exp_2, E, vs); gen_exp(a->exp_1, E, vs); }
        else { gen_exp(a->exp_1, E, vs); gen_exp(a->exp_2, E, vs); }
        E.iadd(); return;
    }
    if (auto m = dynamic_cast<ExpMul*>(e))
    {
        int L = su_memo(m->exp_1), R = su_memo(m->exp_2);
        if (R > L) { gen_exp(m->exp_2, E, vs); gen_exp(m->exp_1, E, vs); }
        else { gen_exp(m->exp_1, E, vs); gen_exp(m->exp_2, E, vs); }
        E.imul(); return;
    }
    if (auto s = dynamic_cast<ExpSub*>(e))
    {
        gen_exp(s->exp_1, E, vs);
        gen_exp(s->exp_2, E, vs);
        E.isub(); return;
    }
    if (auto d = dynamic_cast<ExpDiv*>(e))
    {
        gen_exp(d->exp_1, E, vs);
        gen_exp(d->exp_2, E, vs);
        E.idiv(); return;
    }
}

// single instruction
static void gen_stmt(Stmt* s, Emitter& E, VarSlots& vs)
{
    if (auto se = dynamic_cast<SExp*>(s))
    {
        gen_exp(se->exp_, E, vs);
        E.emit("  invokestatic Runtime/printInt(I)V");
    }
    else if (auto sa = dynamic_cast<SAss*>(s))
    {
        gen_exp(sa->exp_, E, vs);
        int slot = vs.assign_if_new(sa->ident_);
        E.istore(slot);
    }
}

static void gen_body(Program* p, Emitter& E, VarSlots& vs)
{
    if (auto P = dynamic_cast<Prog*>(p))
    {
        for (Stmt* s : *P->liststmt_) gen_stmt(s, E, vs);
    }
}
// -----------------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    // ------------------------------------------------------------------
    // check for arg
    if (argc < 2)
    {
        std::cerr << "Uzycie: " << argv[0] << " <plik.ins>\n";
        return 1;
    }

    // open file
    const char* filename = argv[1];
    FILE* input = std::fopen(filename, "r");
    if (!input)
    {
        std::cerr << "Nie mozna otworzyc pliku: " << filename << "\n";
        return 1;
    }
    std::cout << "Plik otwarty poprawnie: " << filename << "\n";

    // ------------------------------------------------------------------
    // frontend
    Program* tree = nullptr;
    try
    {
        tree = pProgram(input);
    }
    catch (parse_error& e)
    {
        std::cerr << "Blad parsera w linii " << e.getLine() << "\n";
    }
    std::fclose(input);

    if (!tree)
    {
        std::cerr << "Parsowanie nie powiodlo sie.\n";
        return 1;
    }
    std::cout << "Parsowanie powiodlo sie!\n";

    // ------------------------------------------------------------------
    // limit stack
    int max_stack = max_stack_prog(tree);
    std::cout << "limit stack = " << max_stack << "\n";


    // ---------------------------------------------------------
    // limit locals
    VarSlots vs;
    assign_slots_in_order(tree, vs);
    int limit_locals = 1 + (int)vs.slot.size(); // slot 0 = args
    std::cout << "limit locals = " << limit_locals << "\n";

    // generate .j code
    Emitter E;
    gen_body(tree, E, vs);

    // class name = basename
    std::string full = filename;
    std::string base = full;
    {
        // simple basename
        size_t pos = full.find_last_of("/\\");
        if (pos != std::string::npos) base = full.substr(pos + 1);
        size_t dot = base.find_last_of('.');
        if (dot != std::string::npos) base = base.substr(0, dot);
    }

    // whole .j file
    std::ostringstream J;
    J << ".class public " << base << "\n";
    J << ".super java/lang/Object\n\n";
    J << ".method public static main([Ljava/lang/String;)V\n";
    J << ".limit stack " << max_stack << "\n";
    J << ".limit locals " << limit_locals << "\n";
    J << E.buf.str();
    J << "  return\n";
    J << ".end method\n";

    // save to same directory
    size_t p = full.find_last_of("/\\");
    std::string outdir = (p == std::string::npos) ? "" : full.substr(0, p);
    std::string jpath = (outdir.empty() ? base + ".j" : outdir + "/" + base + ".j");

    // cstdio save
    {
        std::string js = J.str();
        FILE* jf = std::fopen(jpath.c_str(), "wb");
        if (!jf)
        {
            std::cerr << "Nie moge zapisac pliku: " << jpath << "\n";
            delete tree;
            return 1;
        }
        std::fwrite(js.data(), 1, js.size(), jf);
        std::fclose(jf);
    }

    std::cout << "Zapisano: " << jpath << "\n";

    // assemble .class jasmin.jar
    const std::string jasmin = "lib/jasmin.jar";
    const std::string out = outdir.empty() ? "." : outdir;

    // paths (MSYS2)
    std::string cmd = "java -jar \"" + jasmin + "\" -d \"" + out + "\" \"" + jpath + "\"";
    int code = std::system(cmd.c_str());
    if (code != 0)
    {
        std::cerr << "Jasmin zwrocil kod " << code << ". Sprawdz plik: " << jpath << "\n";
        delete tree;
        return 1;
    }

    std::cout << "Utworzono: "
        << (outdir.empty() ? (base + ".class") : (outdir + "/" + base + ".class"))
        << "\n";

    delete tree;

    return 0;
}