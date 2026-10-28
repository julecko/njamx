#!/usr/bin/env bash

set -e

trap 'die "Command failed at line $LINENO"' ERR

die() {
    echo
    echo "[ERROR] $1" >&2
    exit 1
}

usage() {
    echo
    echo "Usage: $0 [-d] [--release] [-r [args]]"
    echo
    echo "  -d           Build in Debug mode (default)"
    echo "  --release    Build in Release mode"
    echo "  -r [args]    Run the executable after build with optional arguments"
    echo "  -c           Run Cppcheck only"
    echo
}

run_cppcheck() {
    echo
    echo "Running Cppcheck..."

    CPPCHECK_CMD=$(command -v cppcheck || true)
    if [[ -z "$CPPCHECK_CMD" ]]; then
        die "Cppcheck not found"
    fi

    "$CPPCHECK_CMD" \
        --enable=all \
        --inconclusive \
        --force \
        --inline-suppr \
        --std=c11 \
        --std=c++23 \
        --quiet \
        --suppress=missingIncludeSystem \
        -I"$(pwd)/include" \
        "$(pwd)/src"

    local CPPCHECK_EXIT=$?
    echo "Cppcheck finished with exit code $CPPCHECK_EXIT"
    exit $CPPCHECK_EXIT
}

# Defaults
BUILD_TYPE="Debug"
RUN_EXEC=0
EXEC_NAME="njamx"
RUN_ARGS=()

# Argument parsing
while [[ $# -gt 0 ]]; do
    case "$1" in
        -d)
            BUILD_TYPE="Debug"
            shift
            ;;
        --release)
            BUILD_TYPE="Release"
            shift
            ;;
        -r)
            RUN_EXEC=1
            shift
            # Collect remaining args for runtime
            while [[ $# -gt 0 ]]; do
                RUN_ARGS+=("$1")
                shift
            done
            ;;
        -c)
            run_cppcheck
            ;;
        *)
            echo "Unknown argument: $1"
            usage
            exit 1
            ;;
    esac
done

# Build
BUILD_DIR="build"

# If this script itself was launched via sudo (e.g. so the final run has
# the privileges the raw-socket binary needs), don't let the actual
# configure/build steps run as root too - that just leaves root-owned
# files in $BUILD_DIR that the invoking user can no longer rebuild over.
# Step back down to the original user for everything except running the
# built binary.
if [[ $EUID -eq 0 && -n "${SUDO_USER:-}" ]]; then
    AS_BUILD_USER=(sudo -u "$SUDO_USER")
else
    AS_BUILD_USER=()
fi

"${AS_BUILD_USER[@]}" mkdir -p "$BUILD_DIR"


echo
echo "Configuring ($BUILD_TYPE)..."
"${AS_BUILD_USER[@]}" cmake -S . -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DEXECUTABLE_OUTPUT_NAME="$EXEC_NAME" \
    || die "CMake configuration failed"

echo
echo "Building..."
"${AS_BUILD_USER[@]}" cmake --build "$BUILD_DIR" --config "$BUILD_TYPE" \
    || die "Build failed"

# Run
if [[ $RUN_EXEC -eq 1 ]]; then
    EXEC="$BUILD_DIR/$EXEC_NAME"

    if [[ ! -x "$EXEC" ]]; then
        echo "Error: executable not found or not executable: $EXEC"
        exit 1
    fi

    echo
    echo "Running $EXEC ${RUN_ARGS[*]}"
    set +e
    trap - ERR
    "$EXEC" "${RUN_ARGS[@]}"
    EXEC_EXIT=$?
    trap 'die "Command failed at line $LINENO"' ERR
    set -e

    echo
    echo "Executable exited with code $EXEC_EXIT"
fi
