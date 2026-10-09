"""
EduCompile Backend - Flask API
Calls the C compiler core and returns JSON results to React frontend.
Works on both Windows and Linux.
"""

from flask import Flask, request, jsonify
from flask_cors import CORS
import subprocess
import os
import json
import platform

app = Flask(__name__)
CORS(app)

# ---------- Find the C compiler binary ----------
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
CORE_DIR = os.path.join(BASE_DIR, "..", "compiler_core")

if platform.system() == "Windows":
    COMPILER_PATH = os.path.join(CORE_DIR, "educheck.exe")
else:
    COMPILER_PATH = os.path.join(CORE_DIR, "educheck")

# Normalize path (important on Windows)
COMPILER_PATH = os.path.normpath(COMPILER_PATH)


@app.route("/api/health", methods=["GET"])
def health():
    return jsonify({
        "status": "ok",
        "compiler_path": COMPILER_PATH,
        "compiler_found": os.path.exists(COMPILER_PATH)
    })


@app.route("/api/compile", methods=["POST"])
def compile_code():
    data = request.get_json()
    if not data or "source" not in data:
        return jsonify({"success": False, "error": "No source code provided"}), 400

    source = data["source"]
    if not source.strip():
        return jsonify({"success": False, "error": "Empty source code"}), 400

    if not os.path.exists(COMPILER_PATH):
        return jsonify({
            "success": False,
            "error": f"Compiler binary not found at:\n{COMPILER_PATH}\n\nPlease build it first:\ngcc -Wall -std=c11 -O2 -o educheck.exe main.c lexer.c parser.c semantic.c"
        }), 500

    try:
        result = subprocess.run(
            [COMPILER_PATH],
            input=source.encode("utf-8"),
            capture_output=True,
            timeout=5,
            cwd=os.path.dirname(COMPILER_PATH)
        )

        stdout = result.stdout.decode("utf-8", errors="replace")
        stderr = result.stderr.decode("utf-8", errors="replace")

        if not stdout.strip():
            return jsonify({
                "success": False,
                "error": "Compiler produced no output",
                "stderr": stderr,
                "returncode": result.returncode
            }), 500

        try:
            compiler_output = json.loads(stdout)
            return jsonify(compiler_output)
        except json.JSONDecodeError as e:
            return jsonify({
                "success": False,
                "error": f"Invalid JSON from compiler: {str(e)}",
                "raw_output": stdout[:1500]
            }), 500

    except subprocess.TimeoutExpired:
        return jsonify({"success": False, "error": "Compilation timed out"}), 500
    except Exception as e:
        return jsonify({"success": False, "error": str(e)}), 500


@app.route("/api/samples", methods=["GET"])
def get_samples():
    samples = [
        {
            "name": "Basic Arithmetic + Folding",
            "code": "x = 3 + 5 * 2;\ny = x - 4;\nz = 100 / 5 + y;"
        },
        {
            "name": "Parentheses & Unary",
            "code": "a = -(10 - 4) * 2;\nb = a + 5;"
        },
        {
            "name": "Undeclared Variable (Semantic Error)",
            "code": "x = 10;\ny = x + undeclared_var;"
        },
        {
            "name": "Syntax Error Example",
            "code": "x = 3 + * 5;"
        },
        {
            "name": "Lexical Error Example",
            "code": "x = 3 $ 5;"
        }
    ]
    return jsonify(samples)


if __name__ == "__main__":
    print("=" * 50)
    print("EduCompile Backend")
    print("=" * 50)
    print(f"Looking for compiler at:\n  {COMPILER_PATH}")

    if os.path.exists(COMPILER_PATH):
        print("[OK] Compiler binary found!")
    else:
        print("[WARNING] Compiler binary NOT found!")
        print("Build it with:")
        print("  cd compiler_core")
        print("  gcc -Wall -std=c11 -O2 -o educheck.exe main.c lexer.c parser.c semantic.c")

    print("\nBackend running on http://localhost:5000")
    print("=" * 50)
    app.run(host="0.0.0.0", port=5000, debug=True)
