#!/usr/bin/env python3
"""
Test and visualize semantic tokens from the Sif Language Server
"""

import subprocess
import json
import sys
from pathlib import Path

# Find the repository root
SCRIPT_DIR = Path(__file__).parent
REPO_ROOT = SCRIPT_DIR.parent.parent
LSP_SERVER = REPO_ROOT / "build" / "debug" / "sif_lsp"

# Simple test content
TEST_CONTENT = """function greet {name}
    print "Hello, {name}!"
end function

function the factorial of {n}
    if n <= 1 then
        return 1
    else
        return n * (the factorial of n - 1)
    end if
end function
"""

TOKEN_TYPES = [
    "namespace", "type", "class", "enum", "interface", "struct",
    "typeParameter", "parameter", "variable", "property", "enumMember",
    "event", "function", "method", "macro", "keyword", "modifier",
    "comment", "string", "number", "regexp", "operator"
]

def send_message(proc, method, params=None, msg_id=None):
    message = {"jsonrpc": "2.0", "method": method}
    if msg_id is not None:
        message["id"] = msg_id
    if params is not None:
        message["params"] = params

    content = json.dumps(message)
    header = f"Content-Length: {len(content)}\r\n\r\n"
    proc.stdin.write((header + content).encode())
    proc.stdin.flush()

def read_response(proc):
    headers = {}
    while True:
        line = proc.stdout.readline().decode('utf-8')
        if line == '\r\n' or line == '\n':
            break
        if ':' in line:
            key, value = line.split(':', 1)
            headers[key.strip()] = value.strip()

    content_length = int(headers.get('Content-Length', 0))
    if content_length > 0:
        content = proc.stdout.read(content_length).decode('utf-8')
        return json.loads(content)
    return None

def main():
    if not LSP_SERVER.exists():
        print(f"Error: Language server not found at {LSP_SERVER}")
        print("Please build it first: make debug")
        return 1

    proc = subprocess.Popen(
        [str(LSP_SERVER)],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE
    )

    # Initialize
    send_message(proc, "initialize", {
        "processId": None,
        "rootUri": None,
        "capabilities": {"textDocument": {"semanticTokens": {"requests": {"full": True}}}}
    }, msg_id=1)
    read_response(proc)

    send_message(proc, "initialized", {})

    # Open document
    send_message(proc, "textDocument/didOpen", {
        "textDocument": {
            "uri": "file:///test.sif",
            "languageId": "sif",
            "version": 1,
            "text": TEST_CONTENT
        }
    })

    # Request tokens
    send_message(proc, "textDocument/semanticTokens/full", {
        "textDocument": {"uri": "file:///test.sif"}
    }, msg_id=2)

    response = read_response(proc)
    if not response or "result" not in response:
        print("Error: No response from server")
        if response:
            print(f"Response: {response}")
        proc.kill()
        return 1

    data = response["result"]["data"]

    # Request diagnostics to see parse errors
    send_message(proc, "textDocument/publishDiagnostics", {
        "textDocument": {"uri": "file:///test.sif"}
    })

    print("Semantic Tokens Analysis")
    print("=" * 70)
    print(f"\nTest Content:")
    print("-" * 70)
    for i, line in enumerate(TEST_CONTENT.split('\n')):
        print(f"  {i:2d}: {line}")

    print(f"\n\nGenerated {len(data)} token values ({len(data)//5} tokens)")
    print("\nDecoded Tokens:")
    print("-" * 70)
    print(f"{'Line':>4} {'Col':>4} {'Len':>4} | {'Type':<12} | Text")
    print("-" * 70)

    lines = TEST_CONTENT.split('\n')
    line, char = 0, 0

    for i in range(0, len(data), 5):
        delta_line = data[i]
        delta_char = data[i+1]
        length = data[i+2]
        token_type = data[i+3]
        modifiers = data[i+4]

        line += delta_line
        if delta_line == 0:
            char += delta_char
        else:
            char = delta_char

        token_name = TOKEN_TYPES[token_type] if token_type < len(TOKEN_TYPES) else f"Unknown({token_type})"

        if line < len(lines):
            line_text = lines[line]
            if char < len(line_text) and char + length <= len(line_text):
                text = line_text[char:char+length]
            elif char < len(line_text):
                text = line_text[char:] + "[overflow]"
            else:
                text = "[out of bounds]"
        else:
            text = "[invalid line]"

        print(f"{line:4d} {char:4d} {length:4d} | {token_name:12s} | '{text}'")

    # Shutdown
    send_message(proc, "shutdown", {}, msg_id=3)
    read_response(proc)
    send_message(proc, "exit", {})
    proc.wait()

    stderr = proc.stderr.read().decode('utf-8')
    if stderr and "--debug" in sys.argv:
        print("\n\nDebug Output:")
        print("=" * 70)
        print(stderr)

    return 0

if __name__ == "__main__":
    sys.exit(main())
