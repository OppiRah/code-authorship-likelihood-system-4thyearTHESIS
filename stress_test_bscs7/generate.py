#!/usr/bin/env python3
"""
CALSS demo dataset generator — BSCS-7, Programming 1 + Programming 2.

Every student has a fixed personal coding style (brace placement, operator
spacing, indentation, naming, comment habits, ...). Every file a student
writes is rendered in that style, and every exam is built from randomly
chosen *structural* options, so honest classmates solving the same exam
don't come out token-identical.

A few cases are planted per subject (see PLANTED below and manifest.csv).
Deterministic: same SEED -> byte-identical dataset.

    python generate.py
"""
import csv, hashlib, os, random, re, shutil

SEED = 7
ROOT = os.path.dirname(os.path.abspath(__file__))
SECTION = "BSCS-7"
PERIODS = ["prelim", "midterm", "semifinal", "final"]

# ─────────────────────────────────────────────────────────────
# Program builder: code is written ONCE in a neutral canonical form
# (4-space levels, same-line braces, spaced binary operators,
# @snake_case@ identifier placeholders) and a student's style is
# applied at render time.
# ─────────────────────────────────────────────────────────────
class P:
    def __init__(self, name="main"):
        self.name, self.e, self.l = name, [], 0
    def pp(self, t):            self.e.append(("pp", 0, t, None))
    def c(self, t, tr=None):    self.e.append(("code", self.l, t, tr))
    def o(self, t, tr=None):    self.e.append(("open", self.l, t, tr)); self.l += 1
    def f(self, t, tr=None):    self.e.append(("fopen", self.l, t, tr)); self.l += 1
    def x(self, t="}"):         self.l -= 1; self.e.append(("close", self.l, t, None))
    def cm(self, t):            self.e.append(("cmt", self.l, t, None))
    def b(self):                self.e.append(("blank", self.l, "", None))

SHORT = {
    "length": "l", "width": "w", "area": "a", "perimeter": "p", "celsius": "c",
    "fahrenheit": "f", "choice": "ch", "grade1": "g1", "grade2": "g2",
    "grade3": "g3", "grade4": "g4", "average": "avg", "total": "t", "num": "x",
    "num1": "a", "num2": "b", "num3": "c", "largest": "mx", "year": "y",
    "limit": "n", "sum": "s", "count": "c", "factorial": "f", "size": "n",
    "numbers": "arr", "maximum": "mx", "minimum": "mn", "position": "p",
    "even_count": "ev", "odd_count": "od", "letter": "g", "result": "r",
    "operation": "op", "digits": "d", "reversed": "rev", "original": "orig",
    "temp": "t", "remainder": "r", "digit_sum": "ds", "above": "ab",
    "scores": "sc", "is_prime_num": "isp", "value": "v", "base": "b",
    "exponent": "e", "power_of": "pw", "first": "a", "second": "b",
    "text": "s", "string_length": "len", "vowels": "v", "consonants": "k",
    "ch": "c", "left": "l", "right": "r", "matrix": "m", "rows": "r",
    "cols": "c", "row_sum": "rs", "student": "st", "students": "st",
    "ptr": "p", "end_ptr": "e", "key": "k", "index": "idx", "low": "lo",
    "high": "hi", "mid": "md", "swapped": "sw", "median": "med",
    "passed": "ps", "failed": "fl", "highest": "hi", "best": "b",
    "start": "a", "stop": "b", "prime_count": "pc", "prime_sum": "psum",
    "divisor": "d", "is_palindrome": "pal", "upper": "u", "copy": "cp",
    "min_index": "mi", "found": "fd", "counter": "k", "repeat": "again",
    "print_array": "pr", "sort_array": "srt", "binary_search": "bs",
    "find_max": "fmax", "find_min": "fmin", "compute_average": "cavg",
    "count_above": "cab", "count_vowels": "cv", "reverse_string": "rv",
    "check_palindrome": "cpal", "to_upper": "up", "get_length": "gl",
    "is_prime": "prime", "list_primes": "lp", "print_student": "ps",
    "class_average": "ca", "top_student": "top", "swap": "sw",
    "raise_power": "pw", "linear_search": "ls", "remarks": "rm",
    "total_grade": "tg", "name": "nm", "final_grade": "fg", "record": "rec",
    "grades": "g", "unit_price": "up", "quantity": "q", "subtotal": "st",
}
RESERVED = {"i", "j", "do", "if", "in", "or"}  # loop variables + keywords

class Style:
    """One person's formatting habits."""
    def __init__(self, **kw):
        self.brace = "same"      # same | next | func_next | mixed
        self.spaced = 0.95       # P(binary operator written with spaces)
        self.comma_space = True
        self.kw_space = True     # `if (` vs `if(`
        self.indent = "    "
        self.naming = "camel"    # camel | snake | short
        self.comments = "own"    # none | own | inline | mixed
        self.cdens = 0.6         # P(any given comment is actually written)
        self.header = None       # None | line | block
        self.blank = 0.6         # P(blank line kept)
        self.cuddle = True       # `} else {`
        self.main_void = False
        # Token-level habits (not formatting — they change what the
        # tokenizer sees, the way real classmates differ).
        self.compound = None     # True: x += y | False: x = x + y | None: as written
        self.incr = "post"       # post: i++ | pre: ++i | plus1: i += 1
        self.prompts = True      # prints "Enter ..." before scanf
        self.puts = False        # puts("...") for plain messages
        self.ret0 = True         # ends main with return 0;
        self.cast = "pre"        # pre: (float)s / n | post: s / (float)n | mul: s * 1.0 / n
        self.cond_short = False  # while (t) instead of while (t != 0)
        self.__dict__.update(kw)

def random_habits(rng):
    return dict(compound=rng.choice([True, False, None, None]),
                incr=rng.choice(["post", "post", "post", "pre", "plus1"]),
                prompts=rng.random() < 0.7, puts=rng.random() < 0.2,
                ret0=rng.random() < 0.85, cast=rng.choice(["pre", "pre", "post", "mul"]),
                cond_short=rng.random() < 0.25)

ARCHETYPES = {
    "neat":      dict(brace="same", spaced=0.96, indent="    ", naming="camel",
                      comments="mixed", cdens=0.55, header="line", blank=0.7),
    "allman":    dict(brace="next", spaced=0.93, indent="    ", naming="snake",
                      comments="own", cdens=0.5, header="block", blank=0.8,
                      cuddle=False, main_void=True),
    "compact":   dict(brace="same", spaced=0.06, comma_space=False, kw_space=False,
                      indent="  ", naming="short", comments="none", cdens=0.0,
                      blank=0.15),
    "commenter": dict(brace="same", spaced=0.9, indent="    ", naming="snake",
                      comments="inline", cdens=0.85, header="block", blank=0.5),
    "linux":     dict(brace="func_next", spaced=0.92, indent="\t", naming="snake",
                      comments="own", cdens=0.35, header=None, blank=0.6,
                      main_void=True),
    "messy":     dict(brace="mixed", spaced=0.5, indent="  ", naming="short",
                      comments="own", cdens=0.2, kw_space=False, blank=0.35),
    "tabcamel":  dict(brace="same", spaced=0.85, indent="\t", naming="camel",
                      comments="own", cdens=0.4, header="line", blank=0.5,
                      cuddle=False),
}

STR_RE = re.compile(r'("(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\')')
OP_RE = re.compile(r" (==|!=|<=|>=|\+=|-=|\*=|/=|%=|&&|\|\||=|<|>|\+|-|\*|/|%) ")
KW_RE = re.compile(r"\b(if|for|while|switch) \(")

class Renderer:
    def __init__(self, style, rng):
        self.s, self.r, self.names = style, rng, {}

    def name(self, key):
        if key in self.names: return self.names[key]
        parts = key.split("_")
        if self.s.naming == "snake": v = key
        elif self.s.naming == "camel": v = parts[0] + "".join(p.capitalize() for p in parts[1:])
        else:
            v = SHORT.get(key, "".join(p[0] for p in parts))
            base, k = v, 2
            while v in RESERVED or v in self.names.values():
                v = f"{base}{k}"; k += 1
        self.names[key] = v
        return v

    def habits(self, t):
        s = self.s
        if s.compound is True:
            t = re.sub(r"^(@\w+@) = \1 ([+\-*/]) (.+);$", r"\1 \2= \3;", t)
        elif s.compound is False:
            t = re.sub(r"^(@\w+@) ([+\-*/])= (.+);$", r"\1 = \1 \2 \3;", t)
        if s.incr == "pre":
            t = re.sub(r"(?<![\w@(*])(i|j|@\w+@)\+\+", r"++\1", t)
        elif s.incr == "plus1":
            t = re.sub(r"(?<![\w@(*])(i|j|@\w+@)\+\+(?=[;)])", r"\1 += 1", t)
        if s.cast == "post":
            t = re.sub(r"\(float\)(@\w+@) / (@\w+@)", r"\1 / (float)\2", t)
        elif s.cast == "mul":
            t = re.sub(r"\(float\)(@\w+@) / (@\w+@)", r"\1 * 1.0 / \2", t)
        if s.cond_short:
            t = re.sub(r"\((@\w+@) != 0\)", r"(\1)", t)
            t = re.sub(r"; (@\w+@) != 0;", r"; \1;", t)
        if s.puts:
            t = re.sub(r'^printf\("((?:[^"%\\]|\\[^n])*)\\n"\);$', r'puts("\1");', t)
        return t

    PROMPT_RE = re.compile(r'^printf\("(Enter|How many|Search for|Name of|Grade|Base|'
                           r'Number of|Choice|Again)')

    def fmt(self, t):
        t = self.habits(t)
        t = re.sub(r"@(\w+)@", lambda m: self.name(m.group(1)), t)
        t = t.replace("MAINARGS", "void" if self.s.main_void else "")
        segs = STR_RE.split(t)
        for i in range(0, len(segs), 2):
            g = segs[i]
            g = OP_RE.sub(lambda m: m.group(0) if self.r.random() < self.s.spaced
                          else m.group(1), g)
            if not self.s.comma_space: g = g.replace(", ", ",")
            if not self.s.kw_space: g = KW_RE.sub(lambda m: m.group(1) + "(", g)
            segs[i] = g
        return "".join(segs)

    def brace_same(self, kind):
        b = self.s.brace
        if b == "mixed": return self.r.random() < 0.55
        if b == "func_next": return kind != "fopen"
        return b == "same"

    def keep_comment(self):
        return self.s.comments != "none" and self.r.random() < self.s.cdens

    def render(self, prog):
        out, pending = [], None
        for kind, lvl, t, tr in prog.e:
            ind = self.s.indent * lvl
            if kind == "blank":
                if out and out[-1] != "" and self.r.random() < self.s.blank: out.append("")
            elif kind == "pp":
                out.append(t)
            elif kind == "cmt":
                if not self.keep_comment(): continue
                if self.s.comments == "inline": pending = t
                else: out.append(f"{ind}// {t}")
            else:
                if kind == "code" and not self.s.prompts and self.PROMPT_RE.match(t): continue
                if kind == "code" and not self.s.ret0 and t == "return 0;" and lvl == 1: continue
                text = self.fmt(t) if kind != "close" or t != "}" else t
                trail = tr if (tr and self.keep_comment()) else None
                if pending and kind != "close":
                    trail, pending = trail or pending, None
                if trail and self.s.comments == "own":
                    out.append(f"{ind}// {trail}"); trail = None
                suf = f"  // {trail}" if trail else ""
                if kind == "code":
                    out.append(ind + text + suf)
                elif kind == "close":
                    out.append(ind + text)  # already fmt'ed (do-while tails have names)
                else:
                    same = self.brace_same(kind)
                    if (t.startswith("else") and out and out[-1].strip() == "}"
                            and same and self.s.cuddle):
                        out[-1] = f"{out[-1]} {text} {{{suf}"
                    elif same:
                        out.append(f"{ind}{text} {{{suf}")
                    else:
                        out.append(ind + text + suf); out.append(ind + "{")
        while out and out[-1] == "": out.pop()
        return out

def render_parts(parts, rd):
    """[(part name, rendered lines)] — kept per part so a planted partial
    copy can paste another student's exact rendered function."""
    return [(part.name, rd.render(part)) for part in parts]

def assemble(rendered, style, who, title):
    lines = []
    if style.header == "line":
        lines += [f"// {who} - {title}", ""]
    elif style.header == "block":
        lines += ["/*", f" * Name: {who}", f" * Program: {title}", " */", ""]
    for _, body in rendered:
        if not body: continue
        if lines and lines[-1] != "": lines.append("")
        lines += body
    return "\n".join(lines) + "\n"

def render_file(parts, style, rng, who, title):
    return assemble(render_parts(parts, Renderer(style, rng)), style, who, title)

def includes(*h):
    p = P("includes")
    for x in h: p.pp(f"#include <{x}>")
    return p

def main_open(p):
    p.f("int main(MAINARGS)")

def pick(r, *opts): return r.choice(opts)

# ─────────────────────────────────────────────────────────────
# PROGRAMMING 1 — activities (3 per period) and exams
# ─────────────────────────────────────────────────────────────
def p1_rect(r):
    p = P(); main_open(p)
    t = pick(r, "float", "int"); fm = "%f" if t == "float" else "%d"
    p.c(f"{t} @length@, @width@, @area@, @perimeter@;")
    p.b()
    p.cm("get the dimensions")
    p.c('printf("Enter length: ");'); p.c(f'scanf("{fm}", &@length@);')
    p.c('printf("Enter width: ");');  p.c(f'scanf("{fm}", &@width@);')
    p.b()
    p.c("@area@ = @length@ * @width@;", "area formula")
    p.c("@perimeter@ = 2 * (@length@ + @width@);", "perimeter formula")
    p.b()
    p.c(f'printf("Area: {fm}\\n", @area@);'); p.c(f'printf("Perimeter: {fm}\\n", @perimeter@);')
    p.c("return 0;"); p.x()
    return [includes("stdio.h"), p]

def p1_temp(r):
    p = P(); main_open(p)
    p.c("float @celsius@, @fahrenheit@;"); p.c("int @choice@;")
    p.b()
    p.c('printf("1 - Celsius to Fahrenheit\\n2 - Fahrenheit to Celsius\\n");')
    p.c('printf("Choice: ");'); p.c('scanf("%d", &@choice@);')
    p.b()
    p.o("if (@choice@ == 1)")
    p.c('printf("Enter Celsius: ");'); p.c('scanf("%f", &@celsius@);')
    p.c("@fahrenheit@ = @celsius@ * 9 / 5 + 32;", "C to F")
    p.c('printf("%.2f F\\n", @fahrenheit@);')
    p.x(); p.o("else if (@choice@ == 2)")
    p.c('printf("Enter Fahrenheit: ");'); p.c('scanf("%f", &@fahrenheit@);')
    p.c("@celsius@ = (@fahrenheit@ - 32) * 5 / 9;", "F to C")
    p.c('printf("%.2f C\\n", @celsius@);')
    p.x(); p.o("else")
    p.c('printf("Invalid choice\\n");')
    p.x()
    p.c("return 0;"); p.x()
    return [includes("stdio.h"), p]

def p1_avg3(r):
    p = P(); main_open(p)
    p.c("float @grade1@, @grade2@, @grade3@, @average@;")
    p.b(); p.cm("read the three grades")
    p.c('printf("Enter three grades: ");')
    p.c('scanf("%f %f %f", &@grade1@, &@grade2@, &@grade3@);')
    p.c("@average@ = (@grade1@ + @grade2@ + @grade3@) / 3;", "average of three")
    p.c('printf("Average: %.2f\\n", @average@);')
    if r.random() < 0.6:
        p.o("if (@average@ >= 75)"); p.c('printf("Passed\\n");')
        p.x(); p.o("else"); p.c('printf("Failed\\n");'); p.x()
    p.c("return 0;"); p.x()
    return [includes("stdio.h"), p]

def p1_evenodd(r):
    p = P(); main_open(p)
    p.c("int @num@;"); p.c('printf("Enter a number: ");'); p.c('scanf("%d", &@num@);')
    p.b(); p.cm("check even or odd")
    p.o("if (@num@ % 2 == 0)"); p.c('printf("%d is even\\n", @num@);')
    p.x(); p.o("else"); p.c('printf("%d is odd\\n", @num@);'); p.x()
    p.b(); p.cm("check the sign")
    p.o("if (@num@ > 0)"); p.c('printf("positive\\n");')
    p.x(); p.o("else if (@num@ < 0)"); p.c('printf("negative\\n");')
    p.x(); p.o("else"); p.c('printf("zero\\n");'); p.x()
    p.c("return 0;"); p.x()
    return [includes("stdio.h"), p]

def p1_largest(r):
    p = P(); main_open(p)
    p.c("int @num1@, @num2@, @num3@, @largest@;")
    p.c('printf("Enter three numbers: ");')
    p.c('scanf("%d %d %d", &@num1@, &@num2@, &@num3@);')
    p.b()
    if r.random() < 0.5:
        p.c("@largest@ = @num1@;", "assume the first is largest")
        p.o("if (@num2@ > @largest@)"); p.c("@largest@ = @num2@;"); p.x()
        p.o("if (@num3@ > @largest@)"); p.c("@largest@ = @num3@;"); p.x()
    else:
        p.o("if (@num1@ >= @num2@ && @num1@ >= @num3@)"); p.c("@largest@ = @num1@;")
        p.x(); p.o("else if (@num2@ >= @num1@ && @num2@ >= @num3@)"); p.c("@largest@ = @num2@;")
        p.x(); p.o("else"); p.c("@largest@ = @num3@;"); p.x()
    p.c('printf("Largest: %d\\n", @largest@);')
    p.c("return 0;"); p.x()
    return [includes("stdio.h"), p]

def p1_leap(r):
    p = P(); main_open(p)
    p.c("int @year@;"); p.c('printf("Enter year: ");'); p.c('scanf("%d", &@year@);')
    p.b(); p.cm("leap year rule")
    if r.random() < 0.6:
        p.o("if ((@year@ % 4 == 0 && @year@ % 100 != 0) || @year@ % 400 == 0)")
        p.c('printf("%d is a leap year\\n", @year@);')
        p.x(); p.o("else"); p.c('printf("%d is not a leap year\\n", @year@);'); p.x()
    else:
        p.o("if (@year@ % 400 == 0)"); p.c('printf("Leap year\\n");')
        p.x(); p.o("else if (@year@ % 100 == 0)"); p.c('printf("Not a leap year\\n");')
        p.x(); p.o("else if (@year@ % 4 == 0)"); p.c('printf("Leap year\\n");')
        p.x(); p.o("else"); p.c('printf("Not a leap year\\n");'); p.x()
    p.c("return 0;"); p.x()
    return [includes("stdio.h"), p]

def p1_sumn(r):
    p = P(); main_open(p)
    p.c("int @limit@, @sum@ = 0;")
    p.c('printf("Enter n: ");'); p.c('scanf("%d", &@limit@);')
    p.b(); p.cm("add 1 to n")
    if r.random() < 0.55:
        p.o("for (int i = 1; i <= @limit@; i++)"); p.c("@sum@ = @sum@ + i;"); p.x()
    else:
        p.c("int i = 1;"); p.o("while (i <= @limit@)")
        p.c("@sum@ += i;"); p.c("i++;"); p.x()
    p.c('printf("Sum: %d\\n", @sum@);')
    p.c('printf("Average: %.2f\\n", (float)@sum@ / @limit@);')
    p.c("return 0;"); p.x()
    return [includes("stdio.h"), p]

def p1_table(r):
    p = P(); main_open(p)
    p.c("int @num@;"); p.c('printf("Enter a number: ");'); p.c('scanf("%d", &@num@);')
    p.b(); p.cm("print the table up to 10")
    p.o("for (int i = 1; i <= 10; i++)")
    p.c('printf("%d x %d = %d\\n", @num@, i, @num@ * i);'); p.x()
    p.c("return 0;"); p.x()
    return [includes("stdio.h"), p]

def p1_fact(r):
    p = P(); main_open(p)
    p.c("int @num@;"); p.c("long long @factorial@ = 1;")
    p.c('printf("Enter a number: ");'); p.c('scanf("%d", &@num@);')
    p.b()
    p.o("if (@num@ < 0)"); p.c('printf("No factorial for negatives\\n");')
    p.x(); p.o("else")
    if r.random() < 0.6:
        p.o("for (int i = 2; i <= @num@; i++)"); p.c("@factorial@ = @factorial@ * i;", "multiply up"); p.x()
    else:
        p.c("int i = @num@;"); p.o("while (i > 1)")
        p.c("@factorial@ *= i;", "multiply down"); p.c("i--;"); p.x()
    p.c('printf("%d! = %lld\\n", @num@, @factorial@);'); p.x()
    p.c("return 0;"); p.x()
    return [includes("stdio.h"), p]

def _array_read(p, n=True):
    p.c("int @numbers@[50], @size@;")
    p.c('printf("How many numbers? ");'); p.c('scanf("%d", &@size@);')
    p.o("for (int i = 0; i < @size@; i++)")
    p.c('printf("Enter number %d: ", i + 1);'); p.c('scanf("%d", &@numbers@[i]);'); p.x()

def p1_arrsum(r):
    p = P(); main_open(p)
    _array_read(p); p.c("int @sum@ = 0;"); p.b()
    p.cm("add all elements")
    p.o("for (int i = 0; i < @size@; i++)"); p.c("@sum@ += @numbers@[i];"); p.x()
    p.c('printf("Sum = %d\\n", @sum@);')
    p.c('printf("Average = %.2f\\n", (float)@sum@ / @size@);')
    p.c("return 0;"); p.x()
    return [includes("stdio.h"), p]

def p1_arrmax(r):
    p = P(); main_open(p)
    _array_read(p); p.c("int @maximum@ = @numbers@[0], @position@ = 0;"); p.b()
    p.cm("look for the largest element")
    p.o("for (int i = 1; i < @size@; i++)")
    p.o("if (@numbers@[i] > @maximum@)")
    p.c("@maximum@ = @numbers@[i];"); p.c("@position@ = i;", "remember where"); p.x(); p.x()
    p.c('printf("Max = %d at index %d\\n", @maximum@, @position@);')
    p.c("return 0;"); p.x()
    return [includes("stdio.h"), p]

def p1_arrevens(r):
    p = P(); main_open(p)
    _array_read(p); p.c("int @even_count@ = 0, @odd_count@ = 0;"); p.b()
    p.o("for (int i = 0; i < @size@; i++)")
    p.o("if (@numbers@[i] % 2 == 0)"); p.c("@even_count@++;")
    p.x(); p.o("else"); p.c("@odd_count@++;"); p.x(); p.x()
    p.c('printf("Even: %d, Odd: %d\\n", @even_count@, @odd_count@);')
    p.c("return 0;"); p.x()
    return [includes("stdio.h"), p]

# ── Programming 1 exams: many independent structural choices each ──
def p1x_grades(r):
    """Prelim exam: grade computation with letter grade and remarks."""
    inp = pick(r, "each", "array", "loop_sum")
    n = pick(r, 3, 4)
    letter = pick(r, "chain_desc", "chain_range", "switch")
    remark = pick(r, "if", "ternary", "in_chain")
    validate = r.random() < 0.4
    p = P(); main_open(p)
    gs = [f"@grade{k}@" for k in range(1, n + 1)]
    if inp == "each":
        p.c("float " + ", ".join(gs) + ", @average@;")
    elif inp == "array":
        p.c(f"float @grades@[{n}], @total@ = 0, @average@;")
    else:
        p.c("float @value@, @total@ = 0, @average@;")
    p.c("char @letter@;")
    p.b(); p.cm("input")
    if inp == "each":
        for g in gs:
            p.c(f'printf("Enter grade: ");'); p.c(f'scanf("%f", &{g});')
        p.c("@average@ = (" + " + ".join(gs) + f") / {n};", "compute the average")
    elif inp == "array":
        p.o(f"for (int i = 0; i < {n}; i++)")
        p.c('printf("Grade %d: ", i + 1);'); p.c('scanf("%f", &@grades@[i]);')
        p.c("@total@ += @grades@[i];"); p.x()
        p.c(f"@average@ = @total@ / {n};", "compute the average")
    else:
        p.c("int i = 0;"); p.o(f"while (i < {n})")
        p.c('printf("Enter grade %d: ", i + 1);'); p.c('scanf("%f", &@value@);')
        p.c("@total@ = @total@ + @value@;"); p.c("i++;"); p.x()
        p.c(f"@average@ = @total@ / {n};")
    if validate:
        p.o("if (@average@ < 0 || @average@ > 100)")
        p.c('printf("Invalid grades\\n");'); p.c("return 1;"); p.x()
    p.b(); p.cm("letter grade")
    if letter == "chain_desc":
        for cut, g in [(90, "A"), (80, "B"), (75, "C")]:
            p.o(("if" if g == "A" else "else if") + f" (@average@ >= {cut})")
            p.c(f"@letter@ = '{g}';")
            if remark == "in_chain": p.c('printf("Remarks: Passed\\n");')
            p.x()
        p.o("else"); p.c("@letter@ = 'F';")
        if remark == "in_chain": p.c('printf("Remarks: Failed\\n");')
        p.x()
    elif letter == "chain_range":
        p.o("if (@average@ < 75)"); p.c("@letter@ = 'F';")
        p.x(); p.o("else if (@average@ < 80)"); p.c("@letter@ = 'C';")
        p.x(); p.o("else if (@average@ < 90)"); p.c("@letter@ = 'B';")
        p.x(); p.o("else"); p.c("@letter@ = 'A';"); p.x()
    else:
        p.o("switch ((int)@average@ / 10)")
        p.c("case 10:"); p.c("case 9: @letter@ = 'A'; break;")
        p.c("case 8: @letter@ = 'B'; break;")
        p.c("case 7: @letter@ = (@average@ >= 75) ? 'C' : 'F'; break;")
        p.c("default: @letter@ = 'F';"); p.x()
    p.b()
    p.c('printf("Average: %.2f\\n", @average@);')
    p.c('printf("Grade: %c\\n", @letter@);')
    if remark == "if" or (remark == "in_chain" and letter != "chain_desc"):
        p.o("if (@average@ >= 75)"); p.c('printf("Remarks: Passed\\n");')
        p.x(); p.o("else"); p.c('printf("Remarks: Failed\\n");'); p.x()
    elif remark == "ternary":
        p.c('printf("Remarks: %s\\n", @average@ >= 75 ? "Passed" : "Failed");')
    p.c("return 0;"); p.x()
    return [includes("stdio.h"), p]

def p1x_calc(r):
    """Midterm exam: simple calculator."""
    dispatch = pick(r, "switch", "ifchain")
    loop = pick(r, "none", "dowhile", "while")
    typ = pick(r, "float", "double")
    fm = "%f" if typ == "float" else "%lf"
    zero_first = r.random() < 0.5
    store = r.random() < 0.5
    p = P(); main_open(p)
    p.c(f"{typ} @num1@, @num2@" + (", @result@;" if store else ";"))
    p.c("char @operation@;")
    if loop != "none": p.c("char @repeat@ = 'y';")
    p.b()
    if loop == "dowhile": p.o("do")
    elif loop == "while": p.o("while (@repeat@ == 'y' || @repeat@ == 'Y')")
    p.cm("read the expression")
    p.c('printf("Enter first number: ");'); p.c(f'scanf("{fm}", &@num1@);')
    p.c('printf("Enter operator (+ - * /): ");'); p.c('scanf(" %c", &@operation@);')
    p.c('printf("Enter second number: ");'); p.c(f'scanf("{fm}", &@num2@);')
    if zero_first:
        p.o("if (@operation@ == '/' && @num2@ == 0)")
        p.c('printf("Error: division by zero\\n");')
        p.x(); p.o("else")
    ops = [("+", "@num1@ + @num2@"), ("-", "@num1@ - @num2@"),
           ("*", "@num1@ * @num2@"), ("/", "@num1@ / @num2@")]
    def emit(expr):
        if store: p.c(f"@result@ = {expr};"); p.c('printf("Result: %.2f\\n", @result@);')
        else: p.c(f'printf("Result: %.2f\\n", {expr});')
    if dispatch == "switch":
        p.o("switch (@operation@)")
        for op, expr in ops:
            p.c(f"case '{op}':")
            if op == "/" and not zero_first:
                p.o("if (@num2@ == 0)"); p.c('printf("Cannot divide by zero\\n");')
                p.x(); p.o("else"); emit(expr); p.x()
            else:
                emit(expr)
            p.c("break;")
        p.c("default:"); p.c('printf("Unknown operator\\n");'); p.x()
    else:
        first = True
        for op, expr in ops:
            p.o(("if" if first else "else if") + f" (@operation@ == '{op}')"); first = False
            if op == "/" and not zero_first:
                p.o("if (@num2@ != 0)"); emit(expr)
                p.x(); p.o("else"); p.c('printf("Cannot divide by zero\\n");'); p.x()
            else:
                emit(expr)
            p.x()
        p.o("else"); p.c('printf("Unknown operator\\n");'); p.x()
    if zero_first: p.x()
    if loop != "none":
        p.c('printf("Again? (y/n): ");'); p.c('scanf(" %c", &@repeat@);')
        if loop == "dowhile": p.x("} while (@repeat@ == 'y' || @repeat@ == 'Y');")
        else: p.x()
    p.c("return 0;"); p.x()
    return [includes("stdio.h"), p]

def p1x_digits(r):
    """Semifinal exam: number analyzer."""
    single = r.random() < 0.5
    handle_neg = r.random() < 0.5
    show_rev = r.random() < 0.6
    loopk = pick(r, "while", "for")
    p = P(); main_open(p)
    p.c("int @num@, @temp@, @digits@ = 0, @digit_sum@ = 0, @reversed@ = 0;")
    p.c('printf("Enter a number: ");'); p.c('scanf("%d", &@num@);')
    if handle_neg:
        p.o("if (@num@ < 0)"); p.c("@num@ = -@num@;", "ignore the sign"); p.x()
    p.b()
    def loop_open(cond, upd):
        if loopk == "while": p.o(f"while ({cond})")
        else: p.o(f"for (; {cond}; {upd})")
    if single:
        p.c("@temp@ = @num@;")
        loop_open("@temp@ > 0", "@temp@ /= 10")
        p.c("int @remainder@ = @temp@ % 10;", "last digit")
        p.c("@digits@++;"); p.c("@digit_sum@ += @remainder@;")
        p.c("@reversed@ = @reversed@ * 10 + @remainder@;")
        if loopk == "while": p.c("@temp@ /= 10;")
        p.x()
    else:
        p.cm("count the digits"); p.c("@temp@ = @num@;")
        loop_open("@temp@ != 0", "@temp@ = @temp@ / 10")
        p.c("@digits@++;")
        if loopk == "while": p.c("@temp@ = @temp@ / 10;")
        p.x(); p.cm("sum and reverse"); p.c("@temp@ = @num@;")
        loop_open("@temp@ != 0", "@temp@ = @temp@ / 10")
        p.c("@digit_sum@ = @digit_sum@ + @temp@ % 10;")
        p.c("@reversed@ = @reversed@ * 10 + @temp@ % 10;")
        if loopk == "while": p.c("@temp@ = @temp@ / 10;")
        p.x()
    if r.random() < 0.3:
        p.o("if (@num@ == 0)"); p.c("@digits@ = 1;"); p.x()
    p.b()
    p.c('printf("Digits: %d\\n", @digits@);'); p.c('printf("Sum of digits: %d\\n", @digit_sum@);')
    if show_rev: p.c('printf("Reversed: %d\\n", @reversed@);')
    if r.random() < 0.5:
        p.c('printf("%s\\n", @reversed@ == @num@ ? "Palindrome" : "Not a palindrome");')
    else:
        p.o("if (@reversed@ == @num@)"); p.c('printf("It is a palindrome\\n");')
        p.x(); p.o("else"); p.c('printf("It is not a palindrome\\n");'); p.x()
    p.c("return 0;"); p.x()
    return [includes("stdio.h"), p]

def p1x_arrstats(r):
    """Final exam: array statistics."""
    helpers = r.random() < 0.35
    single = (not helpers) and r.random() < 0.5
    loopk = pick(r, "for", "while")
    count_above = r.random() < 0.7
    parts = [includes("stdio.h")]
    if helpers:
        for fn, cmp in [("find_max", ">"), ("find_min", "<")]:
            h = P(fn); h.f(f"int @{fn}@(int @numbers@[], int @size@)")
            h.c("int @best@ = @numbers@[0];")
            h.o("for (int i = 1; i < @size@; i++)")
            h.o(f"if (@numbers@[i] {cmp} @best@)"); h.c("@best@ = @numbers@[i];"); h.x(); h.x()
            h.c("return @best@;"); h.x(); parts.append(h)
        h = P("compute_average"); h.f("float @compute_average@(int @numbers@[], int @size@)")
        h.c("int @sum@ = 0;")
        h.o("for (int i = 0; i < @size@; i++)"); h.c("@sum@ += @numbers@[i];"); h.x()
        h.c("return (float)@sum@ / @size@;"); h.x(); parts.append(h)
    p = P(); main_open(p)
    _array_read(p)
    p.b()
    if helpers:
        p.c("int @maximum@ = @find_max@(@numbers@, @size@);")
        p.c("int @minimum@ = @find_min@(@numbers@, @size@);")
        p.c("float @average@ = @compute_average@(@numbers@, @size@);")
    else:
        p.c("int @maximum@ = @numbers@[0], @minimum@ = @numbers@[0], @sum@ = 0;")
        if loopk == "while": p.c("int i;")
        def lopen():
            if loopk == "for": p.o("for (int i = 0; i < @size@; i++)")
            else: p.c("i = 0;"); p.o("while (i < @size@)")
        if single:
            p.cm("one pass for everything"); lopen()
            p.c("@sum@ += @numbers@[i];")
            p.o("if (@numbers@[i] > @maximum@)"); p.c("@maximum@ = @numbers@[i];"); p.x()
            p.o("if (@numbers@[i] < @minimum@)"); p.c("@minimum@ = @numbers@[i];"); p.x()
            if loopk == "while": p.c("i++;")
            p.x()
        else:
            p.cm("maximum and minimum"); lopen()
            p.o("if (@numbers@[i] > @maximum@)"); p.c("@maximum@ = @numbers@[i];")
            p.x(); p.o("else if (@numbers@[i] < @minimum@)"); p.c("@minimum@ = @numbers@[i];"); p.x()
            if loopk == "while": p.c("i++;")
            p.x(); p.cm("total"); lopen(); p.c("@sum@ = @sum@ + @numbers@[i];")
            if loopk == "while": p.c("i++;")
            p.x()
        p.c("float @average@ = (float)@sum@ / @size@;")
    if count_above:
        p.c("int @above@ = 0;")
        p.o("for (int i = 0; i < @size@; i++)")
        p.o("if (@numbers@[i] > @average@)"); p.c("@above@++;"); p.x(); p.x()
    p.b()
    p.c('printf("Max: %d\\n", @maximum@);'); p.c('printf("Min: %d\\n", @minimum@);')
    p.c('printf("Average: %.2f\\n", @average@);')
    if count_above: p.c('printf("Above average: %d\\n", @above@);')
    p.c("return 0;"); p.x()
    parts.append(p)
    return parts

# ─────────────────────────────────────────────────────────────
# PROGRAMMING 2 — functions, pointers, strings, sorting, structs
# ─────────────────────────────────────────────────────────────
def _isprime_fn(r, name="is_prime"):
    h = P(name); h.f(f"int @{name}@(int @num@)")
    h.o("if (@num@ < 2)"); h.c("return 0;"); h.x()
    bound = pick(r, "i * i <= @num@", "i <= @num@ / 2", "i < @num@")
    h.o(f"for (int i = 2; {bound}; i++)")
    h.o("if (@num@ % i == 0)"); h.c("return 0;", "found a divisor"); h.x(); h.x()
    h.c("return 1;"); h.x()
    return h

def p2_prime(r):
    p = P(); main_open(p)
    p.c("int @num@;"); p.c('printf("Enter a number: ");'); p.c('scanf("%d", &@num@);')
    p.o("if (@is_prime@(@num@))"); p.c('printf("%d is prime\\n", @num@);')
    p.x(); p.o("else"); p.c('printf("%d is not prime\\n", @num@);'); p.x()
    p.c("return 0;"); p.x()
    return [includes("stdio.h"), _isprime_fn(r), p]

def p2_power(r):
    h = P("raise_power"); h.f("long @raise_power@(int @base@, int @exponent@)")
    h.c("long @result@ = 1;")
    if r.random() < 0.5:
        h.o("for (int i = 0; i < @exponent@; i++)"); h.c("@result@ *= @base@;"); h.x()
    else:
        h.o("while (@exponent@ > 0)"); h.c("@result@ = @result@ * @base@;"); h.c("@exponent@--;"); h.x()
    h.c("return @result@;"); h.x()
    p = P(); main_open(p)
    p.c("int @base@, @exponent@;")
    p.c('printf("Base and exponent: ");'); p.c('scanf("%d %d", &@base@, &@exponent@);')
    p.c('printf("%d^%d = %ld\\n", @base@, @exponent@, @raise_power@(@base@, @exponent@));')
    p.c("return 0;"); p.x()
    return [includes("stdio.h"), h, p]

def p2_swap(r):
    h = P("swap"); h.f("void @swap@(int *@first@, int *@second@)")
    h.c("int @temp@ = *@first@;", "keep the first value"); h.c("*@first@ = *@second@;")
    h.c("*@second@ = @temp@;"); h.x()
    p = P(); main_open(p)
    p.c("int @num1@, @num2@;"); p.c('printf("Enter two numbers: ");')
    p.c('scanf("%d %d", &@num1@, &@num2@);')
    p.c('printf("Before: %d %d\\n", @num1@, @num2@);'); p.c("@swap@(&@num1@, &@num2@);")
    p.c('printf("After: %d %d\\n", @num1@, @num2@);'); p.c("return 0;"); p.x()
    return [includes("stdio.h"), h, p]

def _strlen_fn(r):
    h = P("get_length"); h.f("int @get_length@(char @text@[])")
    h.c("int @count@ = 0;")
    h.o("while (@text@[@count@] != '\\0')"); h.c("@count@++;"); h.x()
    h.c("return @count@;"); h.x()
    return h

def _read_line(p):
    p.c("char @text@[100];"); p.c('printf("Enter a string: ");')
    p.c('fgets(@text@, sizeof(@text@), stdin);')
    p.c("@text@[strcspn(@text@, \"\\n\")] = '\\0';", "drop the newline")

def p2_strlen(r):
    p = P(); main_open(p); _read_line(p)
    p.c('printf("Length: %d\\n", @get_length@(@text@));'); p.c("return 0;"); p.x()
    return [includes("stdio.h", "string.h"), _strlen_fn(r), p]

def p2_vowels(r):
    h = P("count_vowels"); h.f("int @count_vowels@(char @text@[])")
    h.c("int @count@ = 0;")
    h.o("for (int i = 0; @text@[i] != '\\0'; i++)")
    h.c("char @ch@ = tolower(@text@[i]);")
    h.o("if (@ch@ == 'a' || @ch@ == 'e' || @ch@ == 'i' || @ch@ == 'o' || @ch@ == 'u')")
    h.c("@count@++;"); h.x(); h.x(); h.c("return @count@;"); h.x()
    p = P(); main_open(p); _read_line(p)
    p.c('printf("Vowels: %d\\n", @count_vowels@(@text@));'); p.c("return 0;"); p.x()
    return [includes("stdio.h", "string.h", "ctype.h"), h, p]

def p2_revstr(r):
    h = P("reverse_string"); h.f("void @reverse_string@(char @text@[])")
    h.c("int @left@ = 0, @right@ = strlen(@text@) - 1;")
    h.o("while (@left@ < @right@)")
    h.c("char @temp@ = @text@[@left@];"); h.c("@text@[@left@] = @text@[@right@];")
    h.c("@text@[@right@] = @temp@;"); h.c("@left@++;"); h.c("@right@--;"); h.x(); h.x()
    p = P(); main_open(p); _read_line(p)
    p.c("@reverse_string@(@text@);"); p.c('printf("Reversed: %s\\n", @text@);')
    p.c("return 0;"); p.x()
    return [includes("stdio.h", "string.h"), h, p]

def _print_arr_fn():
    h = P("print_array"); h.f("void @print_array@(int @numbers@[], int @size@)")
    h.o("for (int i = 0; i < @size@; i++)"); h.c('printf("%d ", @numbers@[i]);'); h.x()
    h.c('printf("\\n");'); h.x()
    return h

def _bubble_fn(r, early=None):
    h = P("sort_array"); h.f("void @sort_array@(int @numbers@[], int @size@)")
    early = (r.random() < 0.5) if early is None else early
    if early: h.c("int @swapped@;")
    h.o("for (int i = 0; i < @size@ - 1; i++)")
    if early: h.c("@swapped@ = 0;")
    h.o("for (int j = 0; j < @size@ - i - 1; j++)")
    h.o("if (@numbers@[j] > @numbers@[j + 1])")
    h.c("int @temp@ = @numbers@[j];"); h.c("@numbers@[j] = @numbers@[j + 1];")
    h.c("@numbers@[j + 1] = @temp@;")
    if early: h.c("@swapped@ = 1;")
    h.x(); h.x()
    if early: h.o("if (!@swapped@)"); h.c("break;", "already sorted"); h.x()
    h.x(); h.x()
    return h

def _selection_fn():
    h = P("sort_array"); h.f("void @sort_array@(int @numbers@[], int @size@)")
    h.o("for (int i = 0; i < @size@ - 1; i++)")
    h.c("int @min_index@ = i;")
    h.o("for (int j = i + 1; j < @size@; j++)")
    h.o("if (@numbers@[j] < @numbers@[@min_index@])"); h.c("@min_index@ = j;"); h.x(); h.x()
    h.c("int @temp@ = @numbers@[i];"); h.c("@numbers@[i] = @numbers@[@min_index@];")
    h.c("@numbers@[@min_index@] = @temp@;"); h.x(); h.x()
    return h

def p2_bubble(r):
    p = P(); main_open(p)
    p.c("int @numbers@[] = {64, 25, 12, 22, 11, 90, 38};"); p.c("int @size@ = 7;")
    p.c('printf("Before: ");'); p.c("@print_array@(@numbers@, @size@);")
    p.c("@sort_array@(@numbers@, @size@);"); p.c('printf("After: ");')
    p.c("@print_array@(@numbers@, @size@);"); p.c("return 0;"); p.x()
    return [includes("stdio.h"), _print_arr_fn(), _bubble_fn(r), p]

def p2_linsearch(r):
    h = P("linear_search"); h.f("int @linear_search@(int @numbers@[], int @size@, int @key@)")
    h.o("for (int i = 0; i < @size@; i++)")
    h.o("if (@numbers@[i] == @key@)"); h.c("return i;"); h.x(); h.x()
    h.c("return -1;", "not found"); h.x()
    p = P(); main_open(p)
    p.c("int @numbers@[] = {5, 17, 3, 42, 8, 29};"); p.c("int @key@;")
    p.c('printf("Search for: ");'); p.c('scanf("%d", &@key@);')
    p.c("int @index@ = @linear_search@(@numbers@, 6, @key@);")
    p.o("if (@index@ >= 0)"); p.c('printf("Found at %d\\n", @index@);')
    p.x(); p.o("else"); p.c('printf("Not found\\n");'); p.x()
    p.c("return 0;"); p.x()
    return [includes("stdio.h"), h, p]

def p2_matrix(r):
    p = P(); main_open(p)
    p.c("int @matrix@[3][3], @total@ = 0;")
    p.o("for (int i = 0; i < 3; i++)")
    p.o("for (int j = 0; j < 3; j++)")
    p.c('printf("m[%d][%d]: ", i, j);'); p.c('scanf("%d", &@matrix@[i][j]);'); p.x(); p.x()
    p.cm("row sums and grand total")
    p.o("for (int i = 0; i < 3; i++)")
    p.c("int @row_sum@ = 0;")
    p.o("for (int j = 0; j < 3; j++)"); p.c("@row_sum@ += @matrix@[i][j];"); p.x()
    p.c('printf("Row %d sum: %d\\n", i, @row_sum@);'); p.c("@total@ += @row_sum@;"); p.x()
    p.c('printf("Total: %d\\n", @total@);'); p.c("return 0;"); p.x()
    return [includes("stdio.h"), p]

def _student_struct():
    s = P("struct"); s.pp("struct Student {"); s.pp("    char name[50];"); s.pp("    float grade;"); s.pp("};")
    return s

def p2_struct1(r):
    h = P("print_student"); h.f("void @print_student@(struct Student @student@)")
    h.c('printf("Name: %s\\n", @student@.name);'); h.c('printf("Grade: %.2f\\n", @student@.grade);'); h.x()
    p = P(); main_open(p)
    p.c("struct Student @student@;")
    p.c('printf("Name: ");'); p.c('scanf("%49s", @student@.name);')
    p.c('printf("Grade: ");'); p.c('scanf("%f", &@student@.grade);')
    p.c("@print_student@(@student@);"); p.c("return 0;"); p.x()
    return [includes("stdio.h"), _student_struct(), h, p]

def p2_struct2(r):
    h = P("class_average"); h.f("float @class_average@(struct Student @students@[], int @size@)")
    h.c("float @total@ = 0;")
    h.o("for (int i = 0; i < @size@; i++)"); h.c("@total@ += @students@[i].grade;"); h.x()
    h.c("return @total@ / @size@;"); h.x()
    p = P(); main_open(p)
    p.c("struct Student @students@[3] = {{\"Ana\", 88}, {\"Ben\", 74}, {\"Cara\", 91}};")
    p.c('printf("Class average: %.2f\\n", @class_average@(@students@, 3));')
    p.c("return 0;"); p.x()
    return [includes("stdio.h"), _student_struct(), h, p]

def p2_ptr(r):
    p = P(); main_open(p)
    p.c("int @numbers@[] = {12, 7, 31, 4, 19, 26};"); p.c("int *@ptr@ = @numbers@;")
    p.c("int *@end_ptr@ = @numbers@ + 6;"); p.c("int @sum@ = 0, @maximum@ = *@ptr@;")
    p.cm("walk the array with a pointer")
    p.o("while (@ptr@ < @end_ptr@)")
    p.c("@sum@ += *@ptr@;")
    p.o("if (*@ptr@ > @maximum@)"); p.c("@maximum@ = *@ptr@;"); p.x()
    p.c("@ptr@++;"); p.x()
    p.c('printf("Sum: %d, Max: %d\\n", @sum@, @maximum@);'); p.c("return 0;"); p.x()
    return [includes("stdio.h"), p]

# ── Optional extras: each student adds a random subset to their exam,
# the way real submissions go beyond (or fall short of) the spec. ──
def _extras(p, r, options):
    chosen = [fn for fn in options if r.random() < 0.45]
    r.shuffle(chosen)
    for fn in chosen:
        p.b(); fn(p)

def _x_largest_prime(p):
    p.c("int @largest@ = -1;")
    p.o("for (int i = @stop@; i >= @start@; i--)")
    p.o("if (@is_prime@(i))"); p.c("@largest@ = i;"); p.c("break;"); p.x(); p.x()
    p.c('printf("Largest prime: %d\\n", @largest@);')
def _x_twin_primes(p):
    p.c("int @twins@ = 0;")
    p.o("for (int i = @start@; i + 2 <= @stop@; i++)")
    p.o("if (@is_prime@(i) && @is_prime@(i + 2))"); p.c("@twins@++;"); p.x(); p.x()
    p.c('printf("Twin prime pairs: %d\\n", @twins@);')
def _x_range_size(p):
    p.c('printf("Numbers checked: %d\\n", @stop@ - @start@ + 1);')
def _x_composites(p):
    p.c("int @composites@ = 0;")
    p.o("for (int i = @start@; i <= @stop@; i++)")
    p.o("if (i > 1 && !@is_prime@(i))"); p.c("@composites@ = @composites@ + 1;"); p.x(); p.x()
    p.c('printf("Composite numbers: %d\\n", @composites@);')
EXTRAS_PRIMES = [_x_largest_prime, _x_twin_primes, _x_range_size, _x_composites]

def _x_words(p):
    p.c("int @words@ = 0, @in_word@ = 0;")
    p.o("for (int i = 0; @text@[i] != '\\0'; i++)")
    p.o("if (@text@[i] == ' ')"); p.c("@in_word@ = 0;")
    p.x(); p.o("else if (!@in_word@)"); p.c("@in_word@ = 1;"); p.c("@words@++;"); p.x(); p.x()
    p.c('printf("Words: %d\\n", @words@);')
def _x_digits(p):
    p.c("int @digit_count@ = 0;")
    p.o("for (int i = 0; @text@[i] != '\\0'; i++)")
    p.o("if (isdigit(@text@[i]))"); p.c("@digit_count@++;"); p.x(); p.x()
    p.c('printf("Digits: %d\\n", @digit_count@);')
def _x_spaces(p):
    p.c("int @spaces@ = 0, @index@ = 0;")
    p.o("while (@text@[@index@] != '\\0')")
    p.o("if (@text@[@index@] == ' ')"); p.c("@spaces@ += 1;"); p.x(); p.c("@index@++;"); p.x()
    p.c('printf("Spaces: %d\\n", @spaces@);')
def _x_first_last(p):
    p.o("if (strlen(@text@) > 0)")
    p.c('printf("First: %c, Last: %c\\n", @text@[0], @text@[strlen(@text@) - 1]);'); p.x()
EXTRAS_STRINGS = [_x_words, _x_digits, _x_spaces, _x_first_last]

def _x_minmax_sorted(p):
    p.c('printf("Smallest: %d, Largest: %d\\n", @numbers@[0], @numbers@[@size@ - 1]);')
def _x_descending(p):
    p.c('printf("Descending: ");')
    p.o("for (int i = @size@ - 1; i >= 0; i--)"); p.c('printf("%d ", @numbers@[i]);'); p.x()
    p.c('printf("\\n");')
def _x_sum_avg(p):
    p.c("int @total@ = 0;")
    p.o("for (int i = 0; i < @size@; i++)"); p.c("@total@ += @numbers@[i];"); p.x()
    p.c('printf("Sum: %d, Average: %.2f\\n", @total@, (float)@total@ / @size@);')
def _x_duplicates(p):
    p.c("int @duplicates@ = 0;")
    p.o("for (int i = 1; i < @size@; i++)")
    p.o("if (@numbers@[i] == @numbers@[i - 1])"); p.c("@duplicates@++;"); p.x(); p.x()
    p.c('printf("Duplicates: %d\\n", @duplicates@);')
EXTRAS_SORT = [_x_minmax_sorted, _x_descending, _x_sum_avg, _x_duplicates]

def _x_table(p):
    p.c('printf("\\n%-20s %s\\n", "NAME", "GRADE");')
    p.o("for (int i = 0; i < @size@; i++)")
    p.c('printf("%-20s %6.2f\\n", @students@[i].name, @students@[i].grade);'); p.x()
def _x_lowest(p):
    p.c("int @worst@ = 0;")
    p.o("for (int i = 1; i < @size@; i++)")
    p.o("if (@students@[i].grade < @students@[@worst@].grade)"); p.c("@worst@ = i;"); p.x(); p.x()
    p.c('printf("Lowest: %s (%.2f)\\n", @students@[@worst@].name, @students@[@worst@].grade);')
def _x_remarks(p):
    p.o("for (int i = 0; i < @size@; i++)")
    p.c('printf("%s: %s\\n", @students@[i].name, @students@[i].grade >= 75 ? "Passed" : "Failed");'); p.x()
def _x_honors(p):
    p.c("int @honors@ = 0;")
    p.o("for (int i = 0; i < @size@; i++)")
    p.o("if (@students@[i].grade >= 90)"); p.c("@honors@++;"); p.x(); p.x()
    p.c('printf("With honors: %d\\n", @honors@);')
EXTRAS_RECORDS = [_x_table, _x_lowest, _x_remarks, _x_honors]

# ── Programming 2 exams ──
def p2x_primes(r):
    """Prelim exam: primes in a range."""
    helper_list = r.random() < 0.4
    store = r.random() < 0.5
    parts = [includes("stdio.h"), _isprime_fn(r)]
    if helper_list:
        h = P("list_primes"); h.f("int @list_primes@(int @start@, int @stop@)")
        h.c("int @prime_count@ = 0;")
        h.o("for (int i = @start@; i <= @stop@; i++)")
        h.o("if (@is_prime@(i))"); h.c('printf("%d ", i);'); h.c("@prime_count@++;"); h.x(); h.x()
        h.c('printf("\\n");'); h.c("return @prime_count@;"); h.x(); parts.append(h)
    p = P(); main_open(p)
    p.c("int @start@, @stop@;")
    p.c('printf("Enter range (start end): ");'); p.c('scanf("%d %d", &@start@, &@stop@);')
    if r.random() < 0.5:
        p.o("if (@start@ > @stop@)")
        p.c("int @temp@ = @start@;"); p.c("@start@ = @stop@;"); p.c("@stop@ = @temp@;"); p.x()
    if helper_list:
        p.c("int @prime_count@ = @list_primes@(@start@, @stop@);")
        p.c('printf("%d primes found\\n", @prime_count@);')
    else:
        p.c("int @prime_count@ = 0, @prime_sum@ = 0;")
        if store: p.c("int @numbers@[500];")
        p.o("for (int i = @start@; i <= @stop@; i++)")
        p.o("if (@is_prime@(i))")
        if store: p.c("@numbers@[@prime_count@] = i;")
        else: p.c('printf("%d ", i);')
        p.c("@prime_count@++;"); p.c("@prime_sum@ += i;"); p.x(); p.x()
        if store:
            p.o("for (int i = 0; i < @prime_count@; i++)"); p.c('printf("%d ", @numbers@[i]);'); p.x()
        p.c('printf("\\nCount: %d, Sum: %d\\n", @prime_count@, @prime_sum@);')
    _extras(p, r, EXTRAS_PRIMES)
    p.c("return 0;"); p.x()
    parts.append(p)
    return parts

def p2x_strings(r):
    """Midterm exam: string analyzer."""
    own_len = r.random() < 0.5
    vowel_mode = pick(r, "switch", "strchr", "ifchain")
    pal_mode = pick(r, "twoidx", "revcopy")
    upper_mode = pick(r, "manual", "toupper")
    hdrs = ["stdio.h", "string.h", "ctype.h"]
    parts = [includes(*hdrs)]
    if own_len: parts.append(_strlen_fn(r))
    ln = "@get_length@(@text@)" if own_len else "strlen(@text@)"
    h = P("count_vowels"); h.f("void @count_vowels@(char @text@[], int *@vowels@, int *@consonants@)")
    h.c("*@vowels@ = 0;"); h.c("*@consonants@ = 0;")
    h.o(f"for (int i = 0; @text@[i] != '\\0'; i++)")
    h.c("char @ch@ = tolower(@text@[i]);")
    h.o("if (!isalpha(@ch@))"); h.c("continue;"); h.x()
    if vowel_mode == "switch":
        h.o("switch (@ch@)")
        h.c("case 'a': case 'e': case 'i': case 'o': case 'u':"); h.c("(*@vowels@)++;"); h.c("break;")
        h.c("default:"); h.c("(*@consonants@)++;"); h.x()
    elif vowel_mode == "strchr":
        h.o('if (strchr("aeiou", @ch@))'); h.c("(*@vowels@)++;")
        h.x(); h.o("else"); h.c("(*@consonants@)++;"); h.x()
    else:
        h.o("if (@ch@ == 'a' || @ch@ == 'e' || @ch@ == 'i' || @ch@ == 'o' || @ch@ == 'u')")
        h.c("*@vowels@ = *@vowels@ + 1;"); h.x(); h.o("else"); h.c("*@consonants@ = *@consonants@ + 1;"); h.x()
    h.x(); h.x(); parts.append(h)
    h = P("check_palindrome"); h.f("int @check_palindrome@(char @text@[])")
    if pal_mode == "twoidx":
        h.c(f"int @left@ = 0, @right@ = {ln} - 1;")
        h.o("while (@left@ < @right@)")
        h.o("if (tolower(@text@[@left@]) != tolower(@text@[@right@]))"); h.c("return 0;"); h.x()
        h.c("@left@++;"); h.c("@right@--;"); h.x(); h.c("return 1;")
    else:
        h.c("char @copy@[100];"); h.c(f"int @size@ = {ln};")
        h.o("for (int i = 0; i < @size@; i++)"); h.c("@copy@[i] = tolower(@text@[@size@ - 1 - i]);"); h.x()
        h.c("@copy@[@size@] = '\\0';")
        h.o("for (int i = 0; i < @size@; i++)")
        h.o("if (tolower(@text@[i]) != @copy@[i])"); h.c("return 0;"); h.x(); h.x()
        h.c("return 1;")
    h.x(); parts.append(h)
    h = P("to_upper"); h.f("void @to_upper@(char @text@[])")
    h.o("for (int i = 0; @text@[i] != '\\0'; i++)")
    if upper_mode == "manual":
        h.o("if (@text@[i] >= 'a' && @text@[i] <= 'z')"); h.c("@text@[i] = @text@[i] - 32;"); h.x()
    else:
        h.c("@text@[i] = toupper(@text@[i]);")
    h.x(); h.x(); parts.append(h)
    p = P(); main_open(p)
    _read_line(p); p.c("int @vowels@, @consonants@;")
    p.c("@count_vowels@(@text@, &@vowels@, &@consonants@);")
    p.c(f'printf("Length: %d\\n", {ln.replace("@text@", "@text@")});')
    p.c('printf("Vowels: %d, Consonants: %d\\n", @vowels@, @consonants@);')
    if r.random() < 0.5:
        p.c('printf("Palindrome: %s\\n", @check_palindrome@(@text@) ? "yes" : "no");')
    else:
        p.o("if (@check_palindrome@(@text@))"); p.c('printf("It is a palindrome\\n");')
        p.x(); p.o("else"); p.c('printf("Not a palindrome\\n");'); p.x()
    p.c("@to_upper@(@text@);"); p.c('printf("Uppercase: %s\\n", @text@);')
    _extras(p, r, EXTRAS_STRINGS)
    p.c("return 0;"); p.x()
    parts.append(p)
    return parts

def p2x_sort(r, force=None):
    """Semifinal exam: sort, binary search and median."""
    force = force or {}
    algo = force.get("algo", pick(r, "bubble", "selection"))
    search = force.get("search", pick(r, "iterative", "linear"))
    parts = [includes("stdio.h"), _print_arr_fn(),
             _bubble_fn(r, force.get("early")) if algo == "bubble" else _selection_fn()]
    h = P("binary_search")
    if search == "iterative":
        h.f("int @binary_search@(int @numbers@[], int @size@, int @key@)")
        h.c("int @low@ = 0, @high@ = @size@ - 1;")
        h.o("while (@low@ <= @high@)")
        h.c("int @mid@ = (@low@ + @high@) / 2;", "middle index")
        h.o("if (@numbers@[@mid@] == @key@)"); h.c("return @mid@;")
        h.x(); h.o("else if (@numbers@[@mid@] < @key@)"); h.c("@low@ = @mid@ + 1;")
        h.x(); h.o("else"); h.c("@high@ = @mid@ - 1;"); h.x(); h.x()
        h.c("return -1;"); h.x()
    else:
        h.f("int @binary_search@(int @numbers@[], int @size@, int @key@)")
        h.cm("array is sorted so we can stop early")
        h.o("for (int i = 0; i < @size@ && @numbers@[i] <= @key@; i++)")
        h.o("if (@numbers@[i] == @key@)"); h.c("return i;"); h.x(); h.x()
        h.c("return -1;"); h.x()
    parts.append(h)
    p = P(); main_open(p)
    _array_read(p)
    p.c("@sort_array@(@numbers@, @size@);"); p.c('printf("Sorted: ");')
    p.c("@print_array@(@numbers@, @size@);")
    p.cm("median of the sorted list")
    if r.random() < 0.5:
        p.c("float @median@;")
        p.o("if (@size@ % 2 == 0)"); p.c("@median@ = (@numbers@[@size@ / 2 - 1] + @numbers@[@size@ / 2]) / 2.0;")
        p.x(); p.o("else"); p.c("@median@ = @numbers@[@size@ / 2];"); p.x()
    else:
        p.c("float @median@ = (@size@ % 2) ? @numbers@[@size@ / 2] : (@numbers@[@size@ / 2 - 1] + @numbers@[@size@ / 2]) / 2.0;")
    p.c('printf("Median: %.1f\\n", @median@);')
    p.c("int @key@;"); p.c('printf("Search for: ");'); p.c('scanf("%d", &@key@);')
    p.c("int @index@ = @binary_search@(@numbers@, @size@, @key@);")
    p.c('printf(@index@ >= 0 ? "Found at %d\\n" : "Not found\\n", @index@);')
    _extras(p, r, EXTRAS_SORT)
    p.c("return 0;"); p.x()
    parts.append(p)
    return parts

def p2x_records(r):
    """Final exam: student records with structs."""
    use_ptr = r.random() < 0.4
    top_fn = r.random() < 0.6
    parts = [includes("stdio.h"), _student_struct()]
    h = P("class_average"); h.f("float @class_average@(struct Student @students@[], int @size@)")
    h.c("float @total@ = 0;")
    if use_ptr:
        h.o("for (struct Student *@ptr@ = @students@; @ptr@ < @students@ + @size@; @ptr@++)")
        h.c("@total@ += @ptr@->grade;"); h.x()
    else:
        h.o("for (int i = 0; i < @size@; i++)"); h.c("@total@ = @total@ + @students@[i].grade;"); h.x()
    h.c("return @total@ / @size@;"); h.x(); parts.append(h)
    if top_fn:
        h = P("top_student"); h.f("int @top_student@(struct Student @students@[], int @size@)")
        h.c("int @best@ = 0;")
        h.o("for (int i = 1; i < @size@; i++)")
        h.o("if (@students@[i].grade > @students@[@best@].grade)"); h.c("@best@ = i;"); h.x(); h.x()
        h.c("return @best@;"); h.x(); parts.append(h)
    p = P(); main_open(p)
    p.c("struct Student @students@[30];"); p.c("int @size@, @passed@ = 0;")
    p.c('printf("Number of students: ");'); p.c('scanf("%d", &@size@);')
    p.o("for (int i = 0; i < @size@; i++)")
    p.c('printf("Name of student %d: ", i + 1);'); p.c('scanf("%49s", @students@[i].name);')
    p.c('printf("Grade: ");'); p.c('scanf("%f", &@students@[i].grade);')
    if r.random() < 0.5:
        p.o("if (@students@[i].grade >= 75)"); p.c("@passed@++;"); p.x()
    p.x()
    if "@passed@++" not in str(p.e):
        p.o("for (int i = 0; i < @size@; i++)")
        p.o("if (@students@[i].grade >= 75)"); p.c("@passed@++;"); p.x(); p.x()
    p.c('printf("Class average: %.2f\\n", @class_average@(@students@, @size@));')
    if top_fn:
        p.c("int @best@ = @top_student@(@students@, @size@);")
    else:
        p.c("int @best@ = 0;")
        p.o("for (int i = 1; i < @size@; i++)")
        p.o("if (@students@[i].grade > @students@[@best@].grade)"); p.c("@best@ = i;"); p.x(); p.x()
    p.c('printf("Top student: %s (%.2f)\\n", @students@[@best@].name, @students@[@best@].grade);')
    p.c('printf("Passed: %d, Failed: %d\\n", @passed@, @size@ - @passed@);')
    _extras(p, r, EXTRAS_RECORDS)
    p.c("return 0;"); p.x()
    parts.append(p)
    return parts

def vary_exam(parts, r):
    """Per-student extras on top of an exam's structural choices: input
    validation, echoing the input, a results banner, merged final prints."""
    p = parts[-1]
    e = p.e
    # validation right after the size is read
    k = next((i for i, x in enumerate(e) if x[2] == 'scanf("%d", &@size@);'), None)
    if k is not None and r.random() < 0.4:
        lvl = e[k][1]
        e[k + 1:k + 1] = [("open", lvl, "if (@size@ <= 0 || @size@ > 50)", None),
                          ("code", lvl + 1, 'printf("Invalid size\\n");', None),
                          ("code", lvl + 1, "return 1;", None), ("close", lvl, "}", None)]
    # echo the array back after it's read
    k = next((i for i, x in enumerate(e) if x[2] == 'scanf("%d", &@numbers@[i]);'), None)
    if k is not None and r.random() < 0.3:
        lvl = e[k][1] - 1
        e[k + 2:k + 2] = [("code", lvl, 'printf("You entered: ");', None),
                          ("open", lvl, "for (int i = 0; i < @size@; i++)", None),
                          ("code", lvl + 1, 'printf("%d ", @numbers@[i]);', None),
                          ("close", lvl, "}", None), ("code", lvl, 'printf("\\n");', None)]
    # final run of top-level printf lines before return 0
    ret = next((i for i in range(len(e) - 1, -1, -1) if e[i][2] == "return 0;"), None)
    if ret is not None:
        j = ret
        while j > 0 and e[j - 1][0] == "code" and e[j - 1][1] == 1 and \
                re.match(r'^printf\("[^"]*"(, .*)?\);$', e[j - 1][2]):
            j -= 1
        run = e[j:ret]
        if len(run) >= 2 and r.random() < 0.4:
            fmts, args = "", []
            for x in run:
                m = re.match(r'^printf\("([^"]*)"(?:, (.*))?\);$', x[2])
                fmts += m.group(1)
                if m.group(2): args.append(m.group(2))
            merged = 'printf("' + fmts + '"' + ("".join(", " + a for a in args)) + ");"
            e[j:ret] = [("code", 1, merged, None)]
            ret = j + 1
        if r.random() < 0.3:
            e.insert(j, ("code", 1, 'printf("\\n--- Results ---\\n");', None))
    return reorganize(parts, r)

# Helpers that other helpers call: must stay defined first when no
# prototypes are used.
HELPER_FIRST = ("is_prime", "get_length")

def reorganize(parts, r):
    """Helper-function layout: shuffled order, and for some students
    prototypes up top with the definitions after main."""
    head = [x for x in parts if x.name in ("includes", "struct")]
    helpers = [x for x in parts if x.name not in ("includes", "struct", "main")]
    main_p = [x for x in parts if x.name == "main"]
    if not helpers:
        return parts
    r.shuffle(helpers)
    if r.random() < 0.35:
        protos = P("protos")
        for h in helpers:
            protos.c(h.e[0][2] + ";")  # the fopen header line
        return head + [protos] + main_p + helpers
    helpers.sort(key=lambda h: h.name not in HELPER_FIRST)
    return head + helpers + main_p

SUBJECTS = {
    "Programming1": {
        "title": "Programming 1",
        "activities": {
            "prelim":    [("Rectangle Area and Perimeter", p1_rect), ("Temperature Converter", p1_temp), ("Average of Three Grades", p1_avg3)],
            "midterm":   [("Even/Odd and Sign", p1_evenodd), ("Largest of Three", p1_largest), ("Leap Year", p1_leap)],
            "semifinal": [("Sum 1 to N", p1_sumn), ("Multiplication Table", p1_table), ("Factorial", p1_fact)],
            "final":     [("Array Sum and Average", p1_arrsum), ("Array Maximum", p1_arrmax), ("Count Even and Odd", p1_arrevens)],
        },
        "exams": {"prelim": ("Grade Computation", p1x_grades), "midterm": ("Simple Calculator", p1x_calc),
                  "semifinal": ("Number Analyzer", p1x_digits), "final": ("Array Statistics", p1x_arrstats)},
    },
    "Programming2": {
        "title": "Programming 2",
        "activities": {
            "prelim":    [("Prime Check Function", p2_prime), ("Power Function", p2_power), ("Swap with Pointers", p2_swap)],
            "midterm":   [("String Length", p2_strlen), ("Count Vowels", p2_vowels), ("Reverse a String", p2_revstr)],
            "semifinal": [("Bubble Sort", p2_bubble), ("Linear Search", p2_linsearch), ("Matrix Row Sums", p2_matrix)],
            "final":     [("Student Struct", p2_struct1), ("Class Average (Structs)", p2_struct2), ("Pointer Traversal", p2_ptr)],
        },
        "exams": {"prelim": ("Primes in a Range", p2x_primes), "midterm": ("String Analyzer", p2x_strings),
                  "semifinal": ("Sort, Search and Median", p2x_sort), "final": ("Student Records", p2x_records)},
    },
}

# ─────────────────────────────────────────────────────────────
# Class list (30) and styles
# ─────────────────────────────────────────────────────────────
STUDENTS = [
    ("Althea", "Bautista"), ("Bryan", "Castro"), ("Czarina", "Dizon"), ("Daniel", "Estrada"),
    ("Erika", "Fajardo"), ("Francis", "Galang"), ("Gwen", "Hernandez"), ("Harvey", "Ilagan"),
    ("Isabel", "Jimenez"), ("Joshua", "Katigbak"), ("Kyla", "Lacson"), ("Lorenzo", "Macaraeg"),
    ("Mikaela", "Navarro"), ("Nathan", "Ocampo"), ("Patricia", "Pangilinan"), ("Quincy", "Quiambao"),
    ("Rhea", "Ramirez"), ("Sean", "Salazar"), ("Trisha", "Tolentino"), ("Ulysses", "Umali"),
    ("Vanessa", "Valdez"), ("Warren", "Villanueva"), ("Xyza", "Yambao"), ("Yuri", "Zamora"),
    ("Aaron", "Aquino"), ("Bianca", "Belmonte"), ("Carlo", "Cruz"), ("Denise", "De Guzman"),
    ("Elijah", "Enriquez"), ("Faith", "Flores"),
]
def sid(first, last): return last.replace(" ", "") + first
def display(first, last): return f"{first} {last}"

# Style archetype per student. The planted students' archetypes are chosen
# on purpose (see PLANTED); everyone else is spread over the archetypes.
FIXED_ARCH = {
    # Programming 1 plants
    "CastroBryan": "allman",            "DizonCzarina": "compact",      # P1 verbatim copy
    "GalangFrancis": "allman",        "HernandezGwen": "linux",       # P1 disguised copy
    "IlaganHarvey": "compact",        "JimenezIsabel": "tabcamel",    # P1 ghostwritten
    "LacsonKyla": "compact",                                          # P1 solo style swap
    # Programming 2 plants
    "NavarroMikaela": "commenter",    "OcampoNathan": "compact",      # P2 verbatim copy
    "PangilinanPatricia": "neat",     "RamirezRhea": "allman",        # P2 disguised copy
    "SalazarSean": "commenter",       "TolentinoTrisha": "compact",   # P2 partial copy
    "UmaliUlysses": "compact",        "ValdezVanessa": "allman",      # P2 ghostwritten
}
GHOST_STYLE = Style(brace="same", spaced=0.98, indent="    ", naming="camel",
                    comments="inline", cdens=0.9, header="block", blank=0.8,
                    main_void=True)

def make_styles(rng):
    others = ["neat", "allman", "compact", "commenter", "linux", "messy", "tabcamel",
              "neat", "commenter", "messy"]
    styles, arch = {}, {}
    for first, last in STUDENTS:
        s = sid(first, last)
        a = FIXED_ARCH.get(s) or rng.choice(others)
        kw = dict(ARCHETYPES[a])
        # personal noise on top of the archetype
        kw.update(random_habits(rng))
        kw["spaced"] = min(0.99, max(0.02, kw["spaced"] + rng.uniform(-0.04, 0.04)))
        if kw["cdens"] > 0: kw["cdens"] = min(0.95, max(0.1, kw["cdens"] + rng.uniform(-0.1, 0.1)))
        styles[s], arch[s] = Style(**kw), a
    return styles, arch

# ─────────────────────────────────────────────────────────────
# Planted cases. Each: subject, period, kind, students, note.
#   verbatim   — B submits A's file, only the name in the header changed
#   disguised  — B rebuilt A's exact solution in B's own style/names
#   ghost      — both got the same outside-written solution (ghost style)
#   partial    — B copied one of A's functions verbatim, rest is B's own
#   solo_swap  — exam written by someone outside the class (no partner)
# ─────────────────────────────────────────────────────────────
PLANTED = [
    ("Programming1", "midterm",   "verbatim",  ("CastroBryan", "DizonCzarina")),
    ("Programming1", "semifinal", "disguised", ("GalangFrancis", "HernandezGwen")),
    ("Programming1", "final",     "ghost",     ("IlaganHarvey", "JimenezIsabel")),
    ("Programming1", "prelim",    "solo_swap", ("LacsonKyla",)),
    ("Programming2", "prelim",    "verbatim",  ("NavarroMikaela", "OcampoNathan")),
    ("Programming2", "midterm",   "disguised", ("PangilinanPatricia", "RamirezRhea")),
    ("Programming2", "semifinal", "partial",   ("SalazarSean", "TolentinoTrisha")),
    ("Programming2", "final",     "ghost",     ("UmaliUlysses", "ValdezVanessa")),
]

EXPECT = {
    "verbatim":  ("gold on both files (near-total); red on {B}'s file — {A}'s habits "
                  "(braces/spacing/line length) don't match {B}'s own"),
    "disguised": ("gold on both files (renamed variables still match); NO red — {B} "
                  "retyped it in {B}'s own style, so style can't give it away"),
    "ghost":     ("gold on both files; red on BOTH — neither file looks like its "
                  "owner's activities (same outside writer)"),
    "partial":   ("gold only on the copied function(s); red only on those functions in "
                  "{B}'s file — the rest of {B}'s file is {B}'s own"),
    "solo_swap": ("no flagged pair (nothing copied from a classmate) — shows up only as "
                  "a low authorship score, not in View source"),
}

# Plants where red does NOT do what EXPECT describes, as measured against
# the app's red rule (2+ features changed in the same function; line
# length needs >12 chars). Kept honest rather than tuned — see README.
EXPECT_OVERRIDE = {
    ("Programming1", "final"):
        ("gold on both files; red on {A}'s file only — KNOWN MISS on {B}'s file: "
         "the outside writer's braces and spacing already match {B}'s habits and "
         "line length is only ~10 chars longer (cutoff 12), so fewer than 2 features change"),
    ("Programming2", "prelim"):
        ("gold on both files (near-total); KNOWN MISS: no red on {B}'s file — {A}'s code "
         "is split into short functions and none changes 2 features at once (spacing "
         "changes in one, line length ~11 chars (cutoff 12) in another)"),
    ("Programming2", "semifinal"):
        ("gold only on the copied function(s); KNOWN MISS: no red in {B}'s file — the "
         "copied functions clearly change {B}'s operator spacing, but line length only "
         "~10-12 chars (cutoff 12), so just 1 feature changes"),
}

def fingerprint(*k):
    return hashlib.md5("|".join(k).encode()).hexdigest()[:6]

def main():
    rng = random.Random(SEED)
    styles, arch = make_styles(rng)
    dname = {sid(f, l): display(f, l) for f, l in STUDENTS}
    rows = []
    for subj, spec in SUBJECTS.items():
        out_dir = os.path.join(ROOT, subj, SECTION)
        if os.path.isdir(out_dir): shutil.rmtree(out_dir)
        os.makedirs(out_dir)
        plants = {(pl[1], s): pl for pl in PLANTED if pl[0] == subj for s in pl[3]}
        written = {}

        def fname(period, tag, s):
            return f"{period}{tag}__{s}_ASGN{fingerprint(subj, period, tag, s)}.c"

        def put(period, tag, s, text, category, problem, scenario, note):
            fn = fname(period, tag, s)
            with open(os.path.join(out_dir, fn), "w", newline="\n") as fh: fh.write(text)
            written[(period, tag, s)] = text
            rows.append(dict(subject=spec["title"], section=SECTION, student=dname[s],
                             student_id=s, style=arch[s], period=period, category=category,
                             problem=problem, filename=fn, scenario=scenario, note=note))

        for period in PERIODS:
            for k, (title, gen) in enumerate(spec["activities"][period], 1):
                for first, last in STUDENTS:
                    s = sid(first, last)
                    r = random.Random(f"{subj}|{period}|act{k}|{s}")
                    text = render_file(gen(r), styles[s], r, dname[s], title)
                    put(period, f"activity{k}", s, text, "activity", title, "normal", "")
            title, gen = spec["exams"][period]
            # honest exams first (planted ones are overwritten below)
            for first, last in STUDENTS:
                s = sid(first, last)
                r = random.Random(f"{subj}|{period}|exam|{s}")
                text = render_file(vary_exam(gen(r), r), styles[s], r, dname[s], title)
                if (period, s) not in plants:
                    put(period, "exam", s, text, "exam", title, "normal", "")
            for pl in [pl for pl in PLANTED if pl[0] == subj and pl[1] == period]:
                _, _, kind, who = pl
                A = who[0]; B = who[1] if len(who) > 1 else None
                note = EXPECT_OVERRIDE.get((subj, period), EXPECT[kind]).format(
                    A=dname[A], B=dname.get(B, ""))
                sr = lambda s: random.Random(f"{subj}|{period}|exam|{s}")
                if kind == "solo_swap":
                    outsider = Style(**ARCHETYPES["allman"]); outsider.naming = "camel"
                    outsider.comments, outsider.cdens = "inline", 0.9
                    text = render_file(vary_exam(gen(sr(A)), sr(A)), outsider, sr(A), dname[A], title)
                    put(period, "exam", A, text, "exam", title, "solo_swap", note)
                    continue
                force = {"algo": "bubble", "search": "iterative", "early": True} if kind == "partial" else None
                genA = lambda r: vary_exam(gen(r, force) if force else gen(r), r)
                styleA = GHOST_STYLE if kind == "ghost" else styles[A]
                rdA = Renderer(styleA, sr(A))
                partsA = render_parts(genA(sr(A)), rdA)
                textA = assemble(partsA, styleA, dname[A], title)
                if kind == "verbatim":
                    textB = textA.replace(dname[A], dname[B])
                elif kind == "disguised":
                    textB = render_file(genA(sr(A)), styles[B], sr(B), dname[B], title)
                elif kind == "ghost":
                    g2 = Style(**GHOST_STYLE.__dict__); g2.naming = "snake"
                    textB = render_file(genA(sr(A)), g2, sr(B), dname[B], title)
                elif kind == "partial":
                    # B's own solution (same algorithm choices), with A's sort
                    # and search functions pasted in verbatim. B's calls use
                    # A's function names, as they would after pasting.
                    copied = ("sort_array", "binary_search")
                    rdB = Renderer(styles[B], sr(B))
                    for key in copied: rdB.names[key] = rdA.names[key]
                    ownB = render_parts(genA(sr(B)), rdB)
                    fromA = dict(partsA)
                    textB = assemble([(n, fromA[n] if n in copied else body) for n, body in ownB],
                                     styles[B], dname[B], title)
                put(period, "exam", A, textA, "exam", title, kind, note + " (source file)")
                put(period, "exam", B, textB, "exam", title, kind, note)

    with open(os.path.join(ROOT, "manifest.csv"), "w", newline="") as fh:
        w = csv.DictWriter(fh, fieldnames=list(rows[0].keys())); w.writeheader(); w.writerows(rows)
    print(f"wrote {len(rows)} files")

if __name__ == "__main__":
    main()
