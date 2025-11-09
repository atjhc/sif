#!/usr/bin/env python3
"""
Test script for Sif Language Server
Simulates LSP client communication to verify server functionality
"""

import subprocess
import json
import sys
import os
from pathlib import Path

# Find the repository root
SCRIPT_DIR = Path(__file__).parent
REPO_ROOT = SCRIPT_DIR.parent.parent
LSP_SERVER = REPO_ROOT / "build" / "debug" / "sif_lsp"
TEST_FILE = REPO_ROOT / "support" / "vscode-extension" / "test.sif"

def send_message(proc, method, params=None, msg_id=None):
    """Send a JSON-RPC message to the language server"""
    message = {
        "jsonrpc": "2.0",
        "method": method
    }

    if msg_id is not None:
        message["id"] = msg_id

    if params is not None:
        message["params"] = params

    content = json.dumps(message)
    header = f"Content-Length: {len(content)}\r\n\r\n"
    full_message = header + content

    print(f"\n>>> Sending {method}")
    if len(content) <= 200:
        print(f"    {content}")
    else:
        print(f"    {content[:200]}...")

    proc.stdin.write(full_message.encode())
    proc.stdin.flush()

def read_response(proc):
    """Read a JSON-RPC response from the language server"""
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
        response = json.loads(content)
        print(f"\n<<< Received response")
        response_str = json.dumps(response, indent=2)
        if len(response_str) <= 500:
            print(response_str)
        else:
            print(f"{response_str[:500]}...")
        return response
    return None

def main():
    if not LSP_SERVER.exists():
        print(f"Error: Language server not found at {LSP_SERVER}")
        print(f"Please build it first: make debug")
        return 1

    if not TEST_FILE.exists():
        print(f"Error: Test file not found at {TEST_FILE}")
        return 1

    print("Sif Language Server Test")
    print("=" * 70)

    with open(TEST_FILE, 'r') as f:
        test_content = f.read()

    test_uri = f"file://{TEST_FILE.resolve()}"

    print(f"\nStarting language server: {LSP_SERVER}")
    proc = subprocess.Popen(
        [str(LSP_SERVER)],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE
    )

    try:
        # Step 1: Initialize
        print("\n" + "=" * 70)
        print("STEP 1: Initialize")
        print("=" * 70)
        send_message(proc, "initialize", {
            "processId": None,
            "rootUri": f"file://{REPO_ROOT.resolve()}",
            "capabilities": {
                "textDocument": {
                    "semanticTokens": {
                        "dynamicRegistration": False,
                        "requests": {"full": True}
                    }
                }
            }
        }, msg_id=1)
        response = read_response(proc)

        if response and "result" in response:
            print("\n✓ Server initialized successfully")
            if "capabilities" in response["result"]:
                caps = response["result"]["capabilities"]
                print(f"  Capabilities: {list(caps.keys())}")
                if "semanticTokensProvider" in caps:
                    legend = caps["semanticTokensProvider"].get("legend", {})
                    print(f"  Token types: {len(legend.get('tokenTypes', []))}")
                    print(f"  Token modifiers: {len(legend.get('tokenModifiers', []))}")

        # Step 2: Initialized notification
        print("\n" + "=" * 70)
        print("STEP 2: Initialized")
        print("=" * 70)
        send_message(proc, "initialized", {})

        # Step 3: Open document
        print("\n" + "=" * 70)
        print("STEP 3: Open Document")
        print("=" * 70)
        print(f"  URI: {test_uri}")
        print(f"  Content length: {len(test_content)} chars")
        print(f"  Content:\n{test_content[:200]}...")
        send_message(proc, "textDocument/didOpen", {
            "textDocument": {
                "uri": test_uri,
                "languageId": "sif",
                "version": 1,
                "text": test_content
            }
        })

        # Step 4: Request semantic tokens
        print("\n" + "=" * 70)
        print("STEP 4: Request Semantic Tokens")
        print("=" * 70)
        send_message(proc, "textDocument/semanticTokens/full", {
            "textDocument": {"uri": test_uri}
        }, msg_id=2)
        response = read_response(proc)

        if response and "result" in response:
            data = response["result"].get("data", [])
            print(f"\n✓ Semantic tokens received: {len(data)} values ({len(data)//5} tokens)")

            if len(data) >= 5:
                print("\n  Decoded tokens (first 10):")
                print("  " + "-" * 60)
                line, char = 0, 0
                for i in range(0, min(len(data), 50), 5):
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

                    print(f"  Line {line:2d}, Col {char:3d}, Len {length:2d}, Type {token_type:2d}, Mods {modifiers}")
        else:
            print("\n✗ No semantic tokens received!")
            if response and "error" in response:
                print(f"  Error: {response['error']}")

        # Step 5: Shutdown
        print("\n" + "=" * 70)
        print("STEP 5: Shutdown")
        print("=" * 70)
        send_message(proc, "shutdown", {}, msg_id=3)
        response = read_response(proc)

        send_message(proc, "exit", {})

        # Check stderr for debug messages
        print("\n" + "=" * 70)
        print("Server Debug Output (stderr):")
        print("=" * 70)
        proc.wait(timeout=2)
        stderr = proc.stderr.read().decode('utf-8')
        if stderr:
            # Only show first 2000 chars to avoid overwhelming output
            if len(stderr) > 2000:
                print(stderr[:2000])
                print(f"\n... (truncated, {len(stderr)} total chars)")
            else:
                print(stderr)
        else:
            print("(no debug output)")

        print("\n" + "=" * 70)
        print("✓ Test Complete!")
        print("=" * 70)
        return 0

    except Exception as e:
        print(f"\n✗ Error during test: {e}")
        import traceback
        traceback.print_exc()
        proc.kill()
        return 1

if __name__ == "__main__":
    sys.exit(main())
