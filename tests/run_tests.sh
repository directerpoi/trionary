#!/usr/bin/env bash
# Regression test runner for Trionary
# Usage: bash tests/run_tests.sh   (or: make test)
#
# For each tests/test_*.tri file that has a matching tests/test_*.expected
# file, the interpreter is run and its combined stdout+stderr is compared
# against the expected file.  An optional tests/test_*.args file may supply
# extra command-line arguments to pass after the script path.

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TRI="${SCRIPT_DIR}/../tri"

PASS=0
FAIL=0

if [[ ! -x "${TRI}" ]]; then
    echo "ERROR: interpreter '${TRI}' not found or not executable. Run 'make' first." >&2
    exit 1
fi

for tri_file in "${SCRIPT_DIR}"/test_*.tri; do
    base="${tri_file%.tri}"
    name="$(basename "${tri_file}")"
    expected_file="${base}.expected"

    if [[ ! -f "${expected_file}" ]]; then
        echo "SKIP: ${name} (no .expected file)"
        continue
    fi

    # Read optional extra arguments (one line, space-separated)
    extra_args=()
    if [[ -f "${base}.args" ]]; then
        read -ra extra_args < "${base}.args"
    fi

    # Read optional stdin input
    stdin_file="/dev/null"
    if [[ -f "${base}.stdin" ]]; then
        stdin_file="${base}.stdin"
    fi

    # Run and capture combined stdout + stderr
    tmp=$(mktemp)
    "${TRI}" run "${tri_file}" "${extra_args[@]}" < "${stdin_file}" > "${tmp}" 2>&1 || true

    if diff -q "${tmp}" "${expected_file}" > /dev/null 2>&1; then
        echo "PASS: ${name}"
        PASS=$((PASS + 1))
    else
        echo "FAIL: ${name}"
        diff "${tmp}" "${expected_file}" || true
        FAIL=$((FAIL + 1))
    fi

    rm -f "${tmp}"
done

echo ""
echo "Results: ${PASS} passed, ${FAIL} failed"
[[ ${FAIL} -eq 0 ]]
