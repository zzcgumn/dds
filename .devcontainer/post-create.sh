#!/usr/bin/env bash
set -euo pipefail

echo "==> Claiming shared bazel caches"
sudo mkdir -p /bazel-disk-cache /bazel-repo-cache
sudo chown -R dev:dev /bazel-disk-cache /bazel-repo-cache

cat <<'EOF'

==> dds dev container ready.

    Note: on Apple Silicon this container is linux/aarch64. dds pins the LLVM
    toolchain for that platform in MODULE.bazel.

    Build:  bazelisk build //library/src:dds
    Test:   bazelisk test //...
EOF
