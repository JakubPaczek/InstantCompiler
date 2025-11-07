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
// alloca i32
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
// LLVM emitter
struct LLEmitter {
    std::ostringstream buf;
    int temp = 0;

    void emit(const std::string& s) { buf << s << "\n"; }
    std::string tmp() { return "%t" + std::to_string(++temp); }

    // GEP do &"%d\n"
    std::string fmt_gep()
    {
        std::string t = tmp();
        emit("  " + t + " = getelementptr inbounds [4 x i8], [4 x i8]* @.fmt, i64 0, i64 0");
        return t;
    }
};
// -----------------------------------------------------------------------------------

// -----------------------------------------------------------------------------------
// generator
static std::string gen_exp(Exp* e, LLEmitter& E, VarSlots& vs)
{
    if (auto lit = dynamic_cast<ExpLit*>(e))
    {
        return std::to_string(lit->integer_);
    }

    if (auto v = dynamic_cast<ExpVar*>(e))
    {
        std::string name = v->ident_;
        int s = vs.find(name);
        if (s < 0)
        {
            std::cerr << "Blad: uzycie zmiennej '" << name << "' przed przypisaniem.\n";
            std::exit(2);
        }
        // alloca name is %<ident>
        std::string t = E.tmp();
        E.emit("  " + t + " = load i32, i32* %" + name);
        return t;
    }

    if (auto a = dynamic_cast<ExpAdd*>(e))
    {
        std::string L = gen_exp(a->exp_1, E, vs);
        std::string R = gen_exp(a->exp_2, E, vs);
        std::string t = E.tmp();
        E.emit("  " + t + " = add i32 " + L + ", " + R);
        return t;
    }
    if (auto m = dynamic_cast<ExpMul*>(e))
    {
        std::string L = gen_exp(m->exp_1, E, vs);
        std::string R = gen_exp(m->exp_2, E, vs);
        std::string t = E.tmp();
        E.emit("  " + t + " = mul i32 " + L + ", " + R);
        return t;
    }
    if (auto s = dynamic_cast<ExpSub*>(e))
    {
        std::string L = gen_exp(s->exp_1, E, vs);
        std::string R = gen_exp(s->exp_2, E, vs);
        std::string t = E.tmp();
        E.emit("  " + t + " = sub i32 " + L + ", " + R);
        return t;
    }
    if (auto d = dynamic_cast<ExpDiv*>(e))
    {
        std::string L = gen_exp(d->exp_1, E, vs);
        std::string R = gen_exp(d->exp_2, E, vs);
        std::string t = E.tmp();
        E.emit("  " + t + " = sdiv i32 " + L + ", " + R);
        return t;
    }

    return "0";
}

static void gen_stmt(Stmt* s, LLEmitter& E, VarSlots& vs)
{
    if (auto se = dynamic_cast<SExp*>(s))
    {
        std::string v = gen_exp(se->exp_, E, vs);
        std::string g = E.fmt_gep();
        E.emit("  call i32 (i8*, ...) @printf(i8* " + g + ", i32 " + v + ")");
        return;
    }
    if (auto sa = dynamic_cast<SAss*>(s))
    {
        std::string v = gen_exp(sa->exp_, E, vs);
        E.emit("  store i32 " + v + ", i32* %" + sa->ident_);
        return;
    }
}

static void gen_body(Program* p, LLEmitter& E, VarSlots& vs)
{
    if (auto P = dynamic_cast<Prog*>(p))
        for (Stmt* s : *P->liststmt_) gen_stmt(s, E, vs);
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

    // ---------------------------------------------------------
    // alloca i32 elements
    VarSlots vs;
    assign_slots_in_order(tree, vs);

    // output paths
    std::string full = filename;
    std::string base = full;
    size_t pos = full.find_last_of("/\\");
    if (pos != std::string::npos) base = full.substr(pos + 1);
    size_t dot = base.find_last_of('.');
    if (dot != std::string::npos) base = base.substr(0, dot);

    size_t pdir = full.find_last_of("/\\");
    std::string outdir = (pdir == std::string::npos) ? "" : full.substr(0, pdir);
    std::string llpath = (outdir.empty() ? base + ".ll" : outdir + "/" + base + ".ll");
    std::string bcpath = (outdir.empty() ? base + ".bc" : outdir + "/" + base + ".bc");

    std::cout << "Wyjscie: " << llpath << " oraz " << bcpath << "\n";

    // front
    LLEmitter E;
    E.emit("; module: " + base);
    E.emit("declare i32 @printf(i8*, ...)");
    E.emit("@.fmt = private unnamed_addr constant [4 x i8] c\"%d\\0A\\00\", align 1");
    E.emit("");
    E.emit("define i32 @main() {");
    E.emit("entry:");

    // alloca i32
    for (auto& kv : vs.slot)
    {
        const std::string& name = kv.first;
        E.emit("  %" + name + " = alloca i32");
    }

    gen_body(tree, E, vs);

    // end
    E.emit("  ret i32 0");
    E.emit("}");

    // save .ll
    {
        std::string s = E.buf.str();
        FILE* f = std::fopen(llpath.c_str(), "wb");
        if (!f) { std::cerr << "Nie moge zapisac pliku: " << llpath << "\n"; delete tree; return 1; }
        std::fwrite(s.data(), 1, s.size(), f);
        std::fclose(f);
    }
    std::cout << "Zapisano: " << llpath << "\n";

    // llvm-as for .bc
    const std::string out = outdir.empty() ? "." : outdir;
    std::string cmd = "llvm-as -o \"" + bcpath + "\" \"" + llpath + "\"";
    int code = std::system(cmd.c_str());
    if (code != 0)
    {
        std::cerr << "llvm-as zwrocil kod " << code << ". Sprawdz plik: " << llpath << "\n";
        delete tree;
        return 1;
    }
    std::cout << "Utworzono: " << bcpath << "\n";

    delete tree;

    return 0;
}