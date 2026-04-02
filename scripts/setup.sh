#!/usr/bin/env bash
set -e  # exit on any error
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# ── Colours ───────────────────────────────────────────────────────────────────
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; BLUE='\033[0;34m'; NC='\033[0m'
info()    { echo -e "${BLUE}[nova2d]${NC} $1"; }
success() { echo -e "${GREEN}[nova2d]${NC} $1"; }
warn()    { echo -e "${YELLOW}[nova2d]${NC} $1"; }
error()   { echo -e "${RED}[nova2d]${NC} $1"; exit 1; }

# ── Detect platform ───────────────────────────────────────────────────────────
OS="$(uname -s)"
info "Detected OS: $OS"

# ── Required versions ─────────────────────────────────────────────────────────
CMAKE_MIN="3.20"
SDL2_MIN="2.0.20"
SPDLOG_MIN="1.9.0"
CATCH2_MIN="3.0.0"

# ── Version comparison helper ─────────────────────────────────────────────────
version_gte() {
    [ "$(printf '%s\n' "$2" "$1" | sort -V | head -n1)" = "$2" ]
}

# ── Install on macOS via Homebrew ─────────────────────────────────────────────
install_macos() {
    if ! command -v brew &>/dev/null; then
        info "Installing Homebrew..."
        /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    fi

    info "Updating Homebrew..."
    brew update --quiet

    install_or_upgrade_brew() {
        local pkg=$1
        if brew list "$pkg" &>/dev/null; then
            warn "$pkg already installed — checking for upgrades..."
            brew upgrade "$pkg" 2>/dev/null || true
        else
            info "Installing $pkg..."
            brew install "$pkg"
        fi
    }

    install_or_upgrade_brew cmake
    install_or_upgrade_brew sdl2
    install_or_upgrade_brew spdlog
    install_or_upgrade_brew catch2
}

# ── Install on Linux ──────────────────────────────────────────────────────────
install_linux() {
    if ! command -v apt-get &>/dev/null; then
        error "Only apt-based Linux distros are supported by this script (Ubuntu/Debian/Pop!_OS)."
    fi

    info "Updating apt..."
    sudo apt-get update -qq

    # ── cmake: apt version is often too old, use Kitware's repo ──────────────
    CMAKE_INSTALLED=$(cmake --version 2>/dev/null | head -n1 | grep -oP '\d+\.\d+\.\d+' || echo "0.0.0")
    if version_gte "$CMAKE_INSTALLED" "$CMAKE_MIN"; then
        success "cmake $CMAKE_INSTALLED already up to date."
    else
        info "Installing cmake $CMAKE_MIN+ from Kitware apt repo..."
        sudo apt-get install -y ca-certificates gpg wget
        wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null \
            | gpg --dearmor - \
            | sudo tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null
        echo "deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] \
https://apt.kitware.com/ubuntu/ $(lsb_release -cs) main" \
            | sudo tee /etc/apt/sources.list.d/kitware.list >/dev/null
        sudo apt-get update -qq
        sudo apt-get install -y cmake
    fi

    # ── SDL2: apt version is fine ─────────────────────────────────────────────
    info "Installing SDL2..."
    sudo apt-get install -y libsdl2-dev

    # ── spdlog: apt version lacks CMake config — build from source ────────────
    SPDLOG_INSTALLED=$(pkg-config --modversion spdlog 2>/dev/null || echo "0.0.0")
    if version_gte "$SPDLOG_INSTALLED" "$SPDLOG_MIN" && \
       [ -f "/usr/local/lib/cmake/spdlog/spdlogConfig.cmake" ]; then
        success "spdlog $SPDLOG_INSTALLED already installed with CMake config."
    else
        info "Building spdlog from source (apt version lacks CMake config)..."
        sudo apt-get install -y git
        TMP=$(mktemp -d)
        git clone --depth 1 --branch v1.12.0 https://github.com/gabime/spdlog.git "$TMP/spdlog"
        cmake -S "$TMP/spdlog" -B "$TMP/spdlog/build" \
              -DCMAKE_BUILD_TYPE=Release \
              -DSPDLOG_BUILD_SHARED=OFF \
              -DCMAKE_INSTALL_PREFIX=/usr/local
        cmake --build "$TMP/spdlog/build" -j$(nproc)
        sudo cmake --install "$TMP/spdlog/build"
        rm -rf "$TMP"
        success "spdlog installed to /usr/local."
    fi

    # ── Catch2 v3: apt only has v2 — build from source ───────────────────────
    CATCH2_CMAKE="/usr/local/lib/cmake/Catch2/Catch2Config.cmake"
    if [ -f "$CATCH2_CMAKE" ]; then
        success "Catch2 v3 already installed."
    else
        info "Building Catch2 v3 from source (apt only has v2)..."
        TMP=$(mktemp -d)
        git clone --depth 1 --branch v3.5.2 https://github.com/catchorg/Catch2.git "$TMP/catch2"
        cmake -S "$TMP/catch2" -B "$TMP/catch2/build" \
              -DCMAKE_BUILD_TYPE=Release \
              -DBUILD_TESTING=OFF \
              -DCMAKE_INSTALL_PREFIX=/usr/local
        cmake --build "$TMP/catch2/build" -j$(nproc)
        sudo cmake --install "$TMP/catch2/build"
        rm -rf "$TMP"
        success "Catch2 v3 installed to /usr/local."
    fi
}

# ── Run the right installer ───────────────────────────────────────────────────
case "$OS" in
    Darwin) install_macos ;;
    Linux)  install_linux  ;;
    *)      error "Unsupported OS: $OS. Use setup.ps1 on Windows." ;;
esac

# Write a cmake hints file so CMake can find Homebrew packages regardless of prefix
if [ "$OS" = "Darwin" ]; then
    BREW_PREFIX=$(brew --prefix)
    cat > "${SCRIPT_DIR}/../cmake/BrewHints.cmake" << EOF
# Auto-generated by setup.sh — do not edit manually
list(APPEND CMAKE_PREFIX_PATH "${BREW_PREFIX}")
EOF
    info "Wrote Homebrew prefix hint: $BREW_PREFIX"
fi

# ── Verify everything CMake can find ─────────────────────────────────────────
info "Verifying installs..."
cmake --version | head -n1

success "All dependencies installed. You're ready to build:"
echo ""
echo "    cmake -B build"
echo "    cmake --build build -j\$(nproc)"
echo "    ./build/bin/nova2d_game"
echo ""