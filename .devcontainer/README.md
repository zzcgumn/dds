# dds dev container

A generic, self-contained dev environment for building and testing dds:
hermetic Bazel (via bazelisk), clangd/clang-format/clang-tidy 21, and Python.
No external repos or credentials required.

## Use it
    devcontainer up --workspace-folder .            # CLI
    # or VS Code: Dev Containers: Reopen in Container

Then:
    bazelisk build //library/src:dds
    bazelisk test //...

## Notes
- On Apple Silicon the container runs `linux/aarch64`; dds pins the LLVM
  toolchain for that platform in `MODULE.bazel`.
- Bazel output base is per-container; the disk and repository caches are shared
  Docker volumes so fresh containers build without re-downloading dependencies.
