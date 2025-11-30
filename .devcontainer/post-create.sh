#!/bin/bash
set -e

echo "🚀 Setting up RP2350 Multi-UART development environment..."

# Получаем путь к workspace (может быть разным в зависимости от монтирования)
WORKSPACE_DIR="${WORKSPACE_DIR:-${PWD}}"
if [ -n "${containerWorkspaceFolder}" ]; then
    WORKSPACE_DIR="${containerWorkspaceFolder}"
fi

echo "📂 Workspace directory: $WORKSPACE_DIR"

# Проверяем, установлен ли Pico SDK
if [ ! -d "$PICO_SDK_PATH" ] || [ -z "$PICO_SDK_PATH" ]; then
    echo "📦 Cloning Pico SDK..."
    cd "$WORKSPACE_DIR"
    if [ ! -d "pico-sdk" ]; then
        git clone --depth=1 --branch master https://github.com/raspberrypi/pico-sdk.git
    fi
    
    # Инициализируем подмодули SDK
    cd pico-sdk
    git submodule update --init --recursive --depth=1
    
    # Устанавливаем переменную окружения
    export PICO_SDK_PATH="$WORKSPACE_DIR/pico-sdk"
    echo "✅ Pico SDK installed at $PICO_SDK_PATH"
else
    echo "✅ Pico SDK already available at $PICO_SDK_PATH"
fi

# Создаем директорию для сборки
cd "$WORKSPACE_DIR"
if [ ! -d "build" ]; then
    mkdir -p build
    echo "📁 Created build directory"
fi

# Проверяем версии инструментов
echo ""
echo "🔧 Installed tools:"
echo "  CMake: $(cmake --version | head -n1)"
echo "  GCC ARM: $(arm-none-eabi-gcc --version | head -n1)"
echo "  Python: $(python3 --version)"
echo "  Git: $(git --version)"

echo ""
echo "✨ Development environment ready!"
echo "   To build the project, run:"
echo "   cd build && cmake .. && make"

