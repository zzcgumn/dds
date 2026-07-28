FROM ubuntu:24.04

# Lean, contributor-facing dev image for dds: hermetic Bazel plus the LSP/format
# tooling. No agent tooling here — that lives in a private overlay image so this
# stays a fast, generic C++ dev environment for anyone building dds.

ARG USERNAME=dev
ARG USER_UID=1000
ARG USER_GID=1000
ARG BAZELISK_VERSION=v1.25.0
ARG LLVM_MAJOR=21

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
        ca-certificates curl wget git openssh-client gnupg lsb-release \
        python3 python3-venv unzip zip xz-utils \
        libtinfo6 libxml2 zlib1g libncurses6 \
        less procps ripgrep sudo \
    && rm -rf /var/lib/apt/lists/*

# clangd/clang-format/clang-tidy only (no full toolchain — toolchains_llvm
# fetches LLVM 21.1.8 hermetically). Pinned to the same major so IDE diagnostics
# match what Bazel builds.
RUN curl -fsSL https://apt.llvm.org/llvm-snapshot.gpg.key \
        | gpg --dearmor -o /usr/share/keyrings/llvm.gpg \
    && echo "deb [signed-by=/usr/share/keyrings/llvm.gpg] http://apt.llvm.org/noble/ llvm-toolchain-noble-${LLVM_MAJOR} main" \
        > /etc/apt/sources.list.d/llvm.list \
    && apt-get update \
    && apt-get install -y --no-install-recommends clangd-${LLVM_MAJOR} clang-format-${LLVM_MAJOR} clang-tidy-${LLVM_MAJOR} \
    && update-alternatives --install /usr/bin/clangd clangd /usr/bin/clangd-${LLVM_MAJOR} 100 \
    && update-alternatives --install /usr/bin/clang-format clang-format /usr/bin/clang-format-${LLVM_MAJOR} 100 \
    && update-alternatives --install /usr/bin/clang-tidy clang-tidy /usr/bin/clang-tidy-${LLVM_MAJOR} 100 \
    && rm -rf /var/lib/apt/lists/*

RUN curl -fsSL -o /usr/local/bin/bazelisk \
        "https://github.com/bazelbuild/bazelisk/releases/download/${BAZELISK_VERSION}/bazelisk-linux-arm64" \
    && chmod 0755 /usr/local/bin/bazelisk \
    && ln -s /usr/local/bin/bazelisk /usr/local/bin/bazel

RUN userdel -r ubuntu 2>/dev/null || true \
    && groupadd --gid ${USER_GID} ${USERNAME} \
    && useradd --uid ${USER_UID} --gid ${USER_GID} -m -s /bin/bash ${USERNAME} \
    && echo "${USERNAME} ALL=(root) NOPASSWD:ALL" > /etc/sudoers.d/${USERNAME} \
    && chmod 0440 /etc/sudoers.d/${USERNAME}

USER ${USERNAME}

# symlink_prefix keeps container builds from clobbering the host's bazel-*
# symlinks in the bind-mounted worktree; the shared caches make fresh containers
# fast (repo cache avoids re-downloading external repos, ~150MB+).
RUN printf 'build --symlink_prefix=bazel-container-\nbuild --disk_cache=/bazel-disk-cache\ncommon --repository_cache=/bazel-repo-cache\n' \
        > /home/${USERNAME}/.bazelrc \
    && mkdir -p /home/${USERNAME}/.cache/bazel

SHELL ["/bin/bash", "-c"]
