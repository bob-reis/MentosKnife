#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
auth_test_binary=$(mktemp /tmp/mentos-auth-test.XXXXXX)
trap 'rm -f "$auth_test_binary"' EXIT
c++ -std=c++17 -Wall -Wextra -Werror -I test/host_auth/stubs -I include -I src \
  src/security/Auth.cpp test/host_auth/session_test.cpp -o "$auth_test_binary"
"$auth_test_binary"
