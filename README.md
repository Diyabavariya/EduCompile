# EduCompile

**Interactive Compiler Phase Visualizer**  
*Compiler Design Lab– Complex Engineering Problem*

EduCompile is an educational full-stack application that helps students understand the internal phases of a compiler through interactive visualization. Users write simple arithmetic expressions, and the system demonstrates Lexical Analysis, Syntax Analysis, Semantic Analysis, Intermediate Code Generation, and Constant Folding optimization in real time.

---

## Real-World Problem

Compiler Design concepts (tokenization, parsing, symbol tables, intermediate representation, optimization) are often taught theoretically. Beginners struggle to visualize:

- How source code is broken into tokens
- How a parse tree is constructed
- Where and why errors occur (with exact location)
- How optimizations such as Constant Folding reduce instructions

**EduCompile** addresses this gap by providing a live, visual demonstration of every compilation phase, making it suitable both for self-learning and classroom teaching.

---

## Features

- **Lexical Analysis** – Token stream with type, value, line, and column; invalid token detection
- **Syntax Analysis** – Recursive Descent Parser based on a Context-Free Grammar; Abstract Syntax Tree visualization
- **Semantic Analysis** – Symbol table construction and undeclared variable detection
- **Intermediate Code Generation** – Three Address Code (TAC)
- **Code Optimization** – Constant Folding with clear **Before vs After** comparison
- **Error Reporting** – Lexical, syntax, and semantic errors with precise line and column numbers
- **Modern Web Dashboard** – Clean interface designed for learning and live demonstration

---

## Technology Stack

| Layer              | Technology              |
|--------------------|-------------------------|
| Compiler Core      | C (hand-written)        |
| Backend API        | Python + Flask          |
| Frontend Dashboard | React (Vite)            |

---

## Implemented Compiler Concepts

1. **Lexical Analysis**  
   Tokenization, recognition of identifiers, numbers and operators, invalid token detection, line/column tracking.

2. **Syntax Analysis**  
   Context-Free Grammar, Recursive Descent Parser, Abstract Syntax Tree (Parse Tree) generation, panic-mode error recovery.

3. **Semantic Analysis + Intermediate Code + Optimization**  
   Symbol Table, undeclared variable checking, Three Address Code generation, Constant Folding optimization.

---

## Project Structure

```
EduCompile/
├── compiler_core/           # C compiler front-end
│   ├── lexer.h / lexer.c
│   ├── parser.h / parser.c
│   ├── semantic.h / semantic.c
│   ├── main.c               # Drives all phases and emits JSON
│   └── educheck.exe         # Compiled binary (Windows)
├── backend/                 # REST API
│   ├── app.py
│   └── requirements.txt
├── frontend/                # React dashboard
│   ├── src/
│   └── package.json
└── README.md
```

---

## How to Run

### Prerequisites
- GCC (MinGW recommended on Windows)
- Python 3.8+
- Node.js 18+

### 1. Build the C Compiler Core

**Windows (PowerShell):**
```powershell
cd compiler_core
gcc -Wall -std=c11 -O2 -o educheck.exe main.c lexer.c parser.c semantic.c
```

**Linux / macOS:**
```bash
cd compiler_core
gcc -Wall -std=c11 -O2 -o educheck main.c lexer.c parser.c semantic.c -lm
```

### 2. Start the Backend
```powershell
cd backend
pip install -r requirements.txt
python app.py
```
The API will be available at `http://localhost:5000`.

### 3. Start the Frontend
```powershell
cd frontend
npm install
npm run dev
```
Open the dashboard at `http://localhost:5173`.

---

## Sample Input

```
x = 3 + 5 * 2;
y = x - 4;
z = 100 / 5 + y;
```

### Expected Result (Constant Folding)

**Before Optimization:**
```
t1 = 5 * 2
t2 = 3 + t1
x  = t2
t3 = x - 4
y  = t3
...
```

**After Constant Folding:**
```
x  = 13
t1 = x - 4
y  = t1
t2 = 20 + y
z  = t2
```

The expression `3 + 5 * 2` is evaluated at compile time and replaced by the constant `13`, reducing the number of intermediate instructions.

---

## Learning Outcomes

After using EduCompile, a beginner can:

- Observe how source text is converted into a token stream
- Understand the structure of an Abstract Syntax Tree
- See the role of a symbol table and semantic checks
- Compare unoptimized and optimized Three Address Code
- Locate errors precisely using line and column information

---

## Authors

Compiler Design Lab 
Department of Computer Science and Engineering  
Group Project

---

## License

This project is developed for academic purposes as part of the Compiler Design Labcourse.