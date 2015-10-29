/*
** This file has been pre-processed with DynASM.
** http://luajit.org/dynasm.html
** DynASM version 1.3.0, DynASM x86 version 1.3.0
** DO NOT EDIT! The original file is in "(stdin)".
*/

#line 1 "(stdin)"
#include "dynasm/dasm_proto.h"
#include "dynasm/dasm_x86.h"
#if _WIN32
#include <Windows.h>
#else
#include <sys/mman.h>
#if !defined(MAP_ANONYMOUS) && defined(MAP_ANON)
#define MAP_ANONYMOUS MAP_ANON
#endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include "parser.h"
#include "expr.h"

//|.arch x86
#if DASM_VERSION != 10300
#error "Version mismatch between DynASM and included encoding engine"
#endif
#line 22 "(stdin)"
//|.globals L_
enum {
  L_START,
  L__MAX
};
#line 23 "(stdin)"
//|.actionlist rubiactions
static const unsigned char rubiactions[269] = {
  252,233,245,255,133,192,15,133,244,247,252,233,245,248,1,255,249,255,252,
  233,245,249,255,249,85,137,229,129,252,236,0,0,0,128,249,255,139,133,233,
  137,133,233,255,201,195,255,249,85,137,229,129,252,236,0,0,0,128,249,139,
  181,233,255,184,237,255,80,255,252,255,150,233,255,252,255,22,255,131,196,
  4,255,184,237,137,4,36,255,137,132,253,36,233,255,248,10,252,233,245,255,
  139,141,233,90,255,137,4,145,255,137,4,17,255,139,133,233,80,255,64,255,72,
  255,88,255,139,13,237,90,255,137,4,18,255,163,237,255,161,237,80,255,137,
  193,255,139,149,233,255,139,21,237,255,139,4,138,255,15,182,4,10,255,232,
  245,129,196,239,255,139,133,233,255,161,237,255,139,4,129,255,137,195,88,
  255,252,247,252,235,255,49,210,252,247,252,251,255,49,210,252,247,252,251,
  137,208,255,1,216,255,41,216,255,137,195,88,57,216,255,15,156,208,255,15,
  159,208,255,15,149,208,255,15,148,208,255,15,158,208,255,15,157,208,255,15,
  182,192,255,33,216,255,9,216,255,49,216,255,193,224,2,137,4,36,252,255,150,
  233,80,137,4,36,252,255,150,233,88,255
};

#line 24 "(stdin)"

dasm_State* d;
static dasm_State** Dst = &d;
void* rubilabels[L__MAX];
void* jit_buf;
size_t jit_sz;

int npc;
static int main_address, mainFunc;

struct {
    Variable var[0xFF];
    int count;
    int data[0xFF];
} gblVar;

static Variable locVar[0xFF][0xFF];
static int varSize[0xFF], varCounter;

static int nowFunc; // number of function                                              
struct {
    char *text[0xff];
    int *addr;
    int count;
} strings;

struct {
    func_t func[0xff];
    int count, inside;
} functions;

#define NON 0

void jit_init() {
    dasm_init(&d, 1);
    dasm_setupglobal(&d, rubilabels, L__MAX);
    dasm_setup(&d, rubiactions);
}

void* jit_finalize() {
    dasm_link(&d, &jit_sz);
#ifdef _WIN32
    jit_buf = VirtualAlloc(0, jit_sz, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#else
    jit_buf = mmap(0, jit_sz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#endif
    dasm_encode(&d, jit_buf);
#ifdef _WIN32
    {DWORD dwOld; VirtualProtect(jit_buf, jit_sz, PAGE_EXECUTE_READWRITE, &dwOld); }
#else
    mprotect(jit_buf, jit_sz, PROT_READ | PROT_WRITE | PROT_EXEC);
#endif
    return jit_buf;
}

char* getString()
{
    strings.text[ strings.count ] =
        calloc(sizeof(char), strlen(tok.tok[tok.pos].val) + 1);
    strcpy(strings.text[strings.count], tok.tok[tok.pos++].val);
    return strings.text[strings.count++];
}

Variable *getVar(char *name)
{
    /* local variable */
    for (int i = 0; i < varCounter; i++) {
        if (!strcmp(name, locVar[nowFunc][i].name))
            return &locVar[nowFunc][i];
    }
    /* global variable */
    for (int i = 0; i < gblVar.count; i++) {
        if (!strcmp(name, gblVar.var[i].name))
            return &gblVar.var[i];
    }
    return NULL;
}

static Variable *appendVar(char *name, int type)
{
    if (functions.inside == IN_FUNC) {
        int32_t sz = 1 + ++varSize[nowFunc];
        strcpy(locVar[nowFunc][varCounter].name, name);
        locVar[nowFunc][varCounter].type = type;
        locVar[nowFunc][varCounter].id = sz;
        locVar[nowFunc][varCounter].loctype = V_LOCAL;

        return &locVar[nowFunc][varCounter++];
    } else if (functions.inside == IN_GLOBAL) {
        /* global varibale */
        strcpy(gblVar.var[gblVar.count].name, name);
        gblVar.var[gblVar.count].type = type;
        gblVar.var[gblVar.count].loctype = V_GLOBAL;
        gblVar.var[gblVar.count].id = (int)&gblVar.data[gblVar.count];
        return &gblVar.var[gblVar.count++];
    }
    return NULL;
}

func_t *getFunc(char *name)
{
    for (int i = 0; i < functions.count; i++) {
        if (!strcmp(functions.func[i].name, name))
            return &functions.func[i];
    }
    return NULL;
}

static func_t *appendFunc(char *name, int address, int espBgn, int args)
{
    functions.func[functions.count].address = address;
    functions.func[functions.count].espBgn = espBgn;
    functions.func[functions.count].args = args;
    strcpy(functions.func[functions.count].name, name);
    return &functions.func[functions.count++];
}

static int32_t appendBreak()
{
    uint32_t lbl = npc++;
    dasm_growpc(&d, npc);
    //| jmp =>lbl
    dasm_put(Dst, 0, lbl);
#line 146 "(stdin)"
    brks.addr = realloc(brks.addr, 4 * (brks.count + 1));
    brks.addr[brks.count] = lbl;
    return brks.count++;
}

static int32_t appendReturn()
{
    relExpr(); /* get argument */

    int lbl = npc++;
    dasm_growpc(&d, npc);

    //| jmp =>lbl
    dasm_put(Dst, 0, lbl);
#line 159 "(stdin)"

    rets.addr = realloc(rets.addr, 4 * (rets.count + 1));
    if (rets.addr == NULL) error("no enough memory");
    rets.addr[rets.count] = lbl;
    return rets.count++;
}

int32_t skip(char *s)
{
    if (!strcmp(s, tok.tok[tok.pos].val)) {
        tok.pos++;
        return 1;
    }
    return 0;
}

int32_t error(char *errs, ...)
{
    va_list args;
    va_start(args, errs);
    printf("error: ");
    vprintf(errs, args);
    puts("");
    va_end(args);
    exit(0);
    return 0;
}

static int eval(int pos, int status)
{
    while (tok.pos < tok.size)
        if (expression(pos, status)) return 1;
    return 0;
}

static Variable *declareVariable()
{
    int32_t npos = tok.pos;
    if (isalpha(tok.tok[tok.pos].val[0])) {
        tok.pos++;
        if (skip(":")) {
            if (skip("int")) {
                --tok.pos;
                return appendVar(tok.tok[npos].val, T_INT);
            }
            if (skip("string")) {
                --tok.pos;
                return appendVar(tok.tok[npos].val, T_STRING);
            }
            if (skip("double")) {
                --tok.pos;
                return appendVar(tok.tok[npos].val, T_DOUBLE);
            }
        } else {
            --tok.pos;
            return appendVar(tok.tok[npos].val, T_INT);
        }
    } else error("%d: can't declare variable", tok.tok[tok.pos].nline);
    return NULL;
}

static int ifStmt()
{
    relExpr(); /* if condition */
    uint32_t end = npc++;
    dasm_growpc(&d, npc);
    // didn't simply 'jz =>end' to prevent address diff too large
    //| test eax, eax
    //| jnz >1
    //| jmp =>end
    //|1:
    dasm_put(Dst, 4, end);
#line 230 "(stdin)"
    return eval(end, NON);
}

static int whileStmt()
{
    uint32_t loopBgn = npc++;
    dasm_growpc(&d, npc);
    //|=>loopBgn:
    dasm_put(Dst, 16, loopBgn);
#line 238 "(stdin)"
    relExpr(); /* condition */
    uint32_t stepBgn[2], stepOn = 0;
    if (skip(",")) {
        stepOn = 1;
        stepBgn[0] = tok.pos;
        for (; tok.tok[tok.pos].val[0] != ';'; tok.pos++)
            /* next */;
    }
    uint32_t end = npc++;
    dasm_growpc(&d, npc);
    //| test eax, eax
    //| jnz >1
    //| jmp =>end
    //|1:
    dasm_put(Dst, 4, end);
#line 252 "(stdin)"

    if (skip(":")) expression(0, BLOCK_LOOP);
    else eval(0, BLOCK_LOOP);

    if (stepOn) {
        stepBgn[1] = tok.pos;
        tok.pos = stepBgn[0];
        if (isassign()) assignment();
        tok.pos = stepBgn[1];
    }
    //| jmp =>loopBgn
    //|=>end:
    dasm_put(Dst, 18, loopBgn, end);
#line 264 "(stdin)"

    for (--brks.count; brks.count >= 0; brks.count--)
        //|=>brks.addr[brks.count]:
        dasm_put(Dst, 16, brks.addr[brks.count]);
#line 267 "(stdin)"
    brks.count = 0;

    return 0;
}

static int32_t functionStmt()
{
    int32_t argsc = 0;
    char *funcName = tok.tok[tok.pos++].val;
    nowFunc++; functions.inside = IN_FUNC;
    if (skip("(")) {
        do {
            declareVariable();
            tok.pos++;
            argsc++;
        } while(skip(","));
        skip(")");
    }
    int func_addr = npc++;
    dasm_growpc(&d, npc);

    int func_esp = npc++;
    dasm_growpc(&d, npc);

    appendFunc(funcName, func_addr, func_esp, argsc); // append function

    //|=>func_addr:
    //| push ebp
    //| mov ebp, esp
    //| sub esp, 0x80000000
    //|=>func_esp:
    dasm_put(Dst, 23, func_addr, func_esp);
#line 298 "(stdin)"

    for(int i = 0; i < argsc; i++) {
        //| mov eax, [ebp + ((argsc - i - 1) * sizeof(int32_t) + 8)]
        //| mov [ebp - (i + 2)*4], eax
        dasm_put(Dst, 36, ((argsc - i - 1) * sizeof(int32_t) + 8), - (i + 2)*4);
#line 302 "(stdin)"
    }
    eval(0, BLOCK_FUNC);

    for (--rets.count; rets.count >= 0; --rets.count) {
        //|=>rets.addr[rets.count]:
        dasm_put(Dst, 16, rets.addr[rets.count]);
#line 307 "(stdin)"
    }
    rets.count = 0;

    //| leave
    //| ret
    dasm_put(Dst, 43);
#line 312 "(stdin)"

    return 0;
}

int expression(int pos, int status)
{
    int isputs = 0;

    if (skip("$")) { // global varibale?
        if (isassign()) assignment();
    } else if (skip("def")) {
        functionStmt();
    } else if (functions.inside == IN_GLOBAL &&
               strcmp("def", tok.tok[tok.pos+1].val) &&
               strcmp("$", tok.tok[tok.pos+1].val) &&
               (tok.pos+1 == tok.size || strcmp(";", tok.tok[tok.pos+1].val))) { // main function entry
        functions.inside = IN_FUNC;
        mainFunc = ++nowFunc;

        int main_esp = npc++;
        dasm_growpc(&d, npc);

        appendFunc("main", main_address, main_esp, 0); // append function

        //|=>main_address:
        //| push ebp
        //| mov ebp, esp
        //| sub esp, 0x80000000
        //|=>main_esp:
        //| mov esi, [ebp + 12]
        dasm_put(Dst, 46, main_address, main_esp, 12);
#line 342 "(stdin)"
        eval(0, NON);
        //| leave
        //| ret
        dasm_put(Dst, 43);
#line 345 "(stdin)"

        functions.inside = IN_GLOBAL;
    } else if (isassign()) {
        assignment();
    } else if ((isputs = skip("puts")) || skip("output")) {
        do {
            int isstring = 0;
            if (skip("\"")) {
                // mov eax string_address
                //| mov eax, getString()
                dasm_put(Dst, 62, getString());
#line 355 "(stdin)"
                isstring = 1;
            } else {
                relExpr();
            }
            //| push eax
            dasm_put(Dst, 65);
#line 360 "(stdin)"
            if (isstring) {
                //| call dword [esi + 4]
                dasm_put(Dst, 67, 4);
#line 362 "(stdin)"
            } else {
                //| call dword [esi]
                dasm_put(Dst, 72);
#line 364 "(stdin)"
            }
            //| add esp, 4
            dasm_put(Dst, 76);
#line 366 "(stdin)"
        } while (skip(","));
        /* new line ? */
        if (isputs) {
            //| call dword [esi + 0x8]
            dasm_put(Dst, 67, 0x8);
#line 370 "(stdin)"
        }
    } else if(skip("printf")) {
        // support maximum 5 arguments for now
        if (skip("\"")) {
            // mov eax string_address
            //| mov eax, getString()
            //| mov [esp], eax
            dasm_put(Dst, 80, getString());
#line 377 "(stdin)"
        }
        if (skip(",")) {
            uint32_t a = 4;
            do {
                relExpr();
                //| mov [esp + a], eax
                dasm_put(Dst, 86, a);
#line 383 "(stdin)"
                a += 4;
            } while(skip(","));
        }
        //| call dword [esi + 0x14]
        dasm_put(Dst, 67, 0x14);
#line 387 "(stdin)"
    } else if (skip("for")) {
        assignment();
        skip(",");
        whileStmt();
    } else if (skip("while")) {
        whileStmt();
    } else if(skip("return")) {
        appendReturn();
    } else if(skip("if")) {
        ifStmt();
    } else if(skip("else")) {
        int32_t end = npc++;
        dasm_growpc(&d, npc);
        // jmp if/elsif/while end
        //| jmp =>end
        //|=>pos:
        dasm_put(Dst, 18, end, pos);
#line 403 "(stdin)"
        eval(end, NON);
        return 1;
    } else if (skip("elsif")) {
        int32_t endif = npc++;
        dasm_growpc(&d, npc);
        // jmp if/elsif/while end
        //| jmp =>endif
        //|=>pos:
        dasm_put(Dst, 18, endif, pos);
#line 411 "(stdin)"
        relExpr(); /* if condition */
        uint32_t end = npc++;
        dasm_growpc(&d, npc);
        //| test eax, eax
        //| jnz >1
        //| jmp =>end
        //|1:
        dasm_put(Dst, 4, end);
#line 418 "(stdin)"
        eval(end, NON);
        //|=>endif:
        dasm_put(Dst, 16, endif);
#line 420 "(stdin)"
        return 1;
    } else if (skip("break")) {
        appendBreak();
    } else if (skip("end")) {
        if (status == NON) {
            //|=>pos:
            dasm_put(Dst, 16, pos);
#line 426 "(stdin)"
        } else if (status == BLOCK_FUNC) functions.inside = IN_GLOBAL;
        return 1;
    } else if (!skip(";")) { relExpr(); }

    return 0;
}

static char *replaceEscape(char *str)
{
    char escape[12][3] = {
        "\\a", "\a", "\\r", "\r", "\\f", "\f",
        "\\n", "\n", "\\t", "\t", "\\b", "\b"
    };
    for (int32_t i = 0; i < 12; i += 2) {
        char *pos;
        while ((pos = strstr(str, escape[i])) != NULL) {
            *pos = escape[i + 1][0];
            memmove(pos + 1, pos + 2, strlen(pos + 2) + 1);
        }
    }
    return str;
}

int (*parser())(int *, void **)
{
    jit_init();

    tok.pos = 0;
    strings.addr = calloc(0xFF, sizeof(int32_t));
    main_address = npc++;
    dasm_growpc(&d, npc);

    //|->START:
    //| jmp =>main_address
    dasm_put(Dst, 92, main_address);
#line 460 "(stdin)"
    eval(0, 0);

    for (int i = 0; i < strings.count; ++i)
        replaceEscape(strings.text[i]);

    uint8_t* buf = (uint8_t*)jit_finalize();

    for (int i = 0; i < functions.count; i++)
        *(int*)(buf + dasm_getpclabel(&d, functions.func[i].espBgn) - 4) = (varSize[i+1] + 6)*4;

    dasm_free(&d);
    return ((int (*)(int *, void **))rubilabels[L_START]);
}

int32_t isassign()
{
    char *val = tok.tok[tok.pos + 1].val;
    if (!strcmp(val, "=") || !strcmp(val, "++") || !strcmp(val, "--")) return 1;
    if (!strcmp(val, "[")) {
        int32_t i = tok.pos + 2, t = 1;
        while (t) {
            val = tok.tok[i].val;
            if (!strcmp(val, "[")) t++; if (!strcmp(val, "]")) t--;
            if (!strcmp(val, ";"))
                error("%d: invalid expression", tok.tok[tok.pos].nline);
            i++;
        }
        if (!strcmp(tok.tok[i].val, "=")) return 1;
    } else if (!strcmp(val, ":") && !strcmp(tok.tok[tok.pos + 3].val, "=")) {
        return 1;
    }
    return 0;
}

int32_t assignment()
{
    Variable *v = getVar(tok.tok[tok.pos].val);
    int32_t inc = 0, dec = 0, declare = 0;
    if (v == NULL) {
        declare++;
        v = declareVariable();
    }
    tok.pos++;

    int siz = (v->type == T_INT ? sizeof(int32_t) :
               v->type == T_STRING ? sizeof(int32_t *) :
               v->type == T_DOUBLE ? sizeof(double) : 4);

    if (v->loctype == V_LOCAL) {
        if (skip("[")) { // Array?
            relExpr();
            //| push eax
            dasm_put(Dst, 65);
#line 512 "(stdin)"
            if (skip("]") && skip("=")) {
                relExpr();
                // mov ecx, array
                //| mov ecx, [ebp - siz*v->id]
                //| pop edx
                dasm_put(Dst, 98, - siz*v->id);
#line 517 "(stdin)"
                if (v->type == T_INT) {
                    //| mov [ecx+edx*4], eax
                    dasm_put(Dst, 103);
#line 519 "(stdin)"
                } else {
                    //| mov [ecx+edx], eax
                    dasm_put(Dst, 107);
#line 521 "(stdin)"
                }
            } else if ((inc = skip("++")) || (dec = skip("--"))) {
                // TODO
            } else 
                error("%d: invalid assignment", tok.tok[tok.pos].nline);
        } else { // Scalar?
            if(skip("=")) {
                relExpr();
            } else if((inc = skip("++")) || (dec = skip("--"))) {
                // mov eax, varaible
                //| mov eax, [ebp - siz*v->id]
                //| push eax
                dasm_put(Dst, 111, - siz*v->id);
#line 533 "(stdin)"
                if (inc)
                    //| inc eax
                    dasm_put(Dst, 116);
#line 535 "(stdin)"
                else if(dec)
                    //| dec eax
                    dasm_put(Dst, 118);
#line 537 "(stdin)"
            }
            // mov variable, eax
            //| mov [ebp - siz*v->id], eax
            dasm_put(Dst, 39, - siz*v->id);
#line 540 "(stdin)"
            if (inc || dec)
                //| pop eax
                dasm_put(Dst, 120);
#line 542 "(stdin)"
        }
    } else if (v->loctype == V_GLOBAL) {
        if (declare) { // first declare for global variable?
            // assignment only int32_terger
            if (skip("=")) {
                unsigned *m = (unsigned *) v->id;
                *m = atoi(tok.tok[tok.pos++].val);
            }
        } else {
            if (skip("[")) { // Array?
                relExpr();
                //| push eax
                dasm_put(Dst, 65);
#line 554 "(stdin)"
                if(skip("]") && skip("=")) {
                    relExpr();
                    // mov ecx GLOBAL_ADDR
                    //| mov ecx, [v->id]
                    //| pop edx
                    dasm_put(Dst, 122, v->id);
#line 559 "(stdin)"
                    if (v->type == T_INT) {
                        //| mov [ecx + edx*4], eax
                        dasm_put(Dst, 103);
#line 561 "(stdin)"
                    } else {
                        //| mov [edx+edx], eax
                        dasm_put(Dst, 127);
#line 563 "(stdin)"
                    }
                } else error("%d: invalid assignment",
                             tok.tok[tok.pos].nline);
            } else if (skip("=")) {
                relExpr();
                //| mov [v->id], eax
                dasm_put(Dst, 131, v->id);
#line 569 "(stdin)"
            } else if ((inc = skip("++")) || (dec = skip("--"))) {
                // mov eax GLOBAL_ADDR
                //| mov eax, [v->id]
                //| push eax
                dasm_put(Dst, 134, v->id);
#line 573 "(stdin)"
                if (inc)
                    //| inc eax
                    dasm_put(Dst, 116);
#line 575 "(stdin)"
                else if (dec)
                    //| dec eax
                    dasm_put(Dst, 118);
#line 577 "(stdin)"
                // mov GLOBAL_ADDR eax
                //| mov [v->id], eax
                dasm_put(Dst, 131, v->id);
#line 579 "(stdin)"
            }
            if (inc || dec)
                //| pop eax
                dasm_put(Dst, 120);
#line 582 "(stdin)"
        }
    }
    return 0;
}
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "expr.h"
#include "rubi.h"
#include "parser.h"

extern int make_stdfunc(char *);

static inline int32_t isIndex() { return !strcmp(tok.tok[tok.pos].val, "["); }

static void primExpr()
{
    if (isdigit(tok.tok[tok.pos].val[0])) { // number?
        //| mov eax, atoi(tok.tok[tok.pos++].val)
        dasm_put(Dst, 62, atoi(tok.tok[tok.pos++].val));
#line 602 "(stdin)"
    } else if (skip("'")) { // char?
        //| mov eax, tok.tok[tok.pos++].val[0]
        dasm_put(Dst, 62, tok.tok[tok.pos++].val[0]);
#line 604 "(stdin)"
        skip("'");
    } else if (skip("\"")) { // string?
        // mov eax string_address
        //| mov eax, getString()
        dasm_put(Dst, 62, getString());
#line 608 "(stdin)"
    } else if (isalpha(tok.tok[tok.pos].val[0])) { // variable or inc or dec
        char *name = tok.tok[tok.pos].val;
        Variable *v;

        if (isassign()) assignment();
        else {
            tok.pos++;
            if (skip("[")) { // Array?
                if ((v = getVar(name)) == NULL)
                    error("%d: '%s' was not declared",
                          tok.tok[tok.pos].nline, name);
                relExpr();
                //| mov ecx, eax
                dasm_put(Dst, 138);
#line 621 "(stdin)"

                if (v->loctype == V_LOCAL) {
                    //| mov edx, [ebp - v->id*4]
                    dasm_put(Dst, 141, - v->id*4);
#line 624 "(stdin)"
                } else if (v->loctype == V_GLOBAL) {
                    // mov edx, GLOBAL_ADDR
                    //| mov edx, [v->id]
                    dasm_put(Dst, 145, v->id);
#line 627 "(stdin)"
                }

                if (v->type == T_INT) {
                    //| mov eax, [edx + ecx * 4]
                    dasm_put(Dst, 149);
#line 631 "(stdin)"
                } else {
                    //| movzx eax, byte [edx + ecx]
                    dasm_put(Dst, 153);
#line 633 "(stdin)"
                }

                if (!skip("]"))
                    error("%d: expected expression ']'",
                          tok.tok[tok.pos].nline);
            } else if (skip("(")) { // Function?
                if (!make_stdfunc(name)) {	// standard function
                    func_t *function = getFunc(name);
                    char *val = tok.tok[tok.pos].val;
                    if (isalpha(val[0]) || isdigit(val[0]) ||
                        !strcmp(val, "\"") || !strcmp(val, "(")) { // has arg?
                        for (int i = 0; i < function->args; i++) {
                            relExpr();
                            //| push eax
                            dasm_put(Dst, 65);
#line 647 "(stdin)"
                            skip(",");
                        }
                    }
                    // call func
                    //| call =>function->address
                    //| add esp, function->args * sizeof(int32_t)
                    dasm_put(Dst, 158, function->address, function->args * sizeof(int32_t));
#line 653 "(stdin)"
                }
                if (!skip(")"))
                    error("func: %d: expected expression ')'",
                          tok.tok[tok.pos].nline);
            } else {
                if ((v = getVar(name)) == NULL)
                    error("var: %d: '%s' was not declared",
                          tok.tok[tok.pos].nline, name);
                if (v->loctype == V_LOCAL) {
                    // mov eax variable
                    //| mov eax, [ebp - v->id*4]
                    dasm_put(Dst, 164, - v->id*4);
#line 664 "(stdin)"
                } else if (v->loctype == V_GLOBAL) {
                    // mov eax GLOBAL_ADDR
                    //| mov eax, [v->id]
                    dasm_put(Dst, 168, v->id);
#line 667 "(stdin)"
                }
            }
        }
    } else if (skip("(")) {
        if (isassign()) assignment(); else relExpr();
        if (!skip(")")) error("%d: expected expression ')'",
                              tok.tok[tok.pos].nline);
    }

    while (isIndex()) {
        //| mov ecx, eax
        dasm_put(Dst, 138);
#line 678 "(stdin)"
        skip("[");
        relExpr();
        skip("]");
        //| mov eax, [ecx + eax*4]
        dasm_put(Dst, 171);
#line 682 "(stdin)"
    }
}

static void mulDivExpr()
{
    int32_t mul = 0, div = 0;
    primExpr();
    while ((mul = skip("*")) || (div = skip("/")) || skip("%")) {
        //| push eax
        dasm_put(Dst, 65);
#line 691 "(stdin)"
        primExpr();
        //| mov ebx, eax
        //| pop eax
        dasm_put(Dst, 175);
#line 694 "(stdin)"
        if (mul) {
            //| imul ebx
            dasm_put(Dst, 179);
#line 696 "(stdin)"
        } else if (div) {
            //| xor edx, edx
            //| idiv ebx
            dasm_put(Dst, 184);
#line 699 "(stdin)"
        } else { /* mod */
            //| xor edx, edx
            //| idiv ebx
            //| mov eax, edx
            dasm_put(Dst, 191);
#line 703 "(stdin)"
        }
    }
}

static void addSubExpr()
{
    int32_t add;
    mulDivExpr();
    while ((add = skip("+")) || skip("-")) {
        //| push eax
        dasm_put(Dst, 65);
#line 713 "(stdin)"
        mulDivExpr();
        //| mov ebx, eax
        //| pop eax
        dasm_put(Dst, 175);
#line 716 "(stdin)"
        if (add) {
            //| add eax, ebx
            dasm_put(Dst, 200);
#line 718 "(stdin)"
        } else {
            //| sub eax, ebx
            dasm_put(Dst, 203);
#line 720 "(stdin)"
        }
    }
}

static void condExpr()
{
    int32_t lt = 0, gt = 0, ne = 0, eql = 0, fle = 0;
    addSubExpr();
    if ((lt = skip("<")) || (gt = skip(">")) || (ne = skip("!=")) ||
        (eql = skip("==")) || (fle = skip("<=")) || skip(">=")) {
        //| push eax
        dasm_put(Dst, 65);
#line 731 "(stdin)"
        addSubExpr();
        //| mov ebx, eax
        //| pop eax
        //| cmp eax, ebx
        dasm_put(Dst, 206);
#line 735 "(stdin)"
        if (lt)
            //| setl al
            dasm_put(Dst, 212);
#line 737 "(stdin)"
        else if (gt)
            //| setg al
            dasm_put(Dst, 216);
#line 739 "(stdin)"
        else if (ne)
            //| setne al
            dasm_put(Dst, 220);
#line 741 "(stdin)"
        else if (eql)
            //| sete al
            dasm_put(Dst, 224);
#line 743 "(stdin)"
        else if (fle)
            //| setle al
            dasm_put(Dst, 228);
#line 745 "(stdin)"
        else
            //| setge al
            dasm_put(Dst, 232);
#line 747 "(stdin)"

        //| movzx eax, al
        dasm_put(Dst, 236);
#line 749 "(stdin)"
    }
}

void relExpr()
{
    int and = 0, or = 0;
    condExpr();
    while ((and = skip("and") || skip("&")) ||
           (or = skip("or") || skip("|")) || (skip("xor") || skip("^"))) {
        //| push eax
        dasm_put(Dst, 65);
#line 759 "(stdin)"
        condExpr();
        //| mov ebx, eax
        //| pop eax
        dasm_put(Dst, 175);
#line 762 "(stdin)"
        if (and)
            //| and eax, ebx
            dasm_put(Dst, 240);
#line 764 "(stdin)"
        else if (or)
            //| or eax, ebx
            dasm_put(Dst, 243);
#line 766 "(stdin)"
        else
            //| xor eax, ebx
            dasm_put(Dst, 246);
#line 768 "(stdin)"
    }
}
#include <string.h>
#include "expr.h"

typedef struct {
    char *name;
    int args, addr;
} std_function;

static std_function stdfunc[] = {
    {"Array", 1, 12},
    {"rand", 0, 16}, {"printf", -1, 20}, {"sleep", 1, 28},
    {"fopen", 2, 32}, {"fprintf", -1, 36}, {"fclose", 1, 40}, {"fgets", 3, 44},
    {"free", 1, 48}, {"freeLocal", 0, 52}
};

int make_stdfunc(char *name)
{
    for (size_t i = 0; i < sizeof(stdfunc) / sizeof(stdfunc[0]); i++) {
        if (!strcmp(stdfunc[i].name, name)) {
            if(!strcmp(name, "Array")) {
                relExpr(); // get array size
                //| shl eax, 2
                //| mov [esp], eax
                //| call dword [esi + 12]
                //| push eax
                //| mov [esp], eax
                //| call dword [esi + 24]
                //| pop eax
                dasm_put(Dst, 249, 12, 24);
#line 798 "(stdin)"
            } else {
                if (stdfunc[i].args == -1) { // vector
                    uint32_t a = 0;
                    do {
                        relExpr();
                        //| mov [esp + a], eax
                        dasm_put(Dst, 86, a);
#line 804 "(stdin)"
                        a += 4;
                    } while(skip(","));
                } else {
                    for(int arg = 0; arg < stdfunc[i].args; arg++) {
                        relExpr();
                        //| mov [esp + arg*4], eax
                        dasm_put(Dst, 86, arg*4);
#line 810 "(stdin)"
                        skip(",");
                    }
                }
                // call $function
                //| call dword [esi + stdfunc[i].addr]
                dasm_put(Dst, 67, stdfunc[i].addr);
#line 815 "(stdin)"
            }
            return 1;
        }
    }
    return 0;
}
