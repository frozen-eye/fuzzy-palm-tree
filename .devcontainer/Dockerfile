FROM ubuntu:22.04

# Избегаем интерактивных запросов во время установки
ENV DEBIAN_FRONTEND=noninteractive

# Установка базовых инструментов
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    gcc-arm-none-eabi \
    libnewlib-arm-none-eabi \
    git \
    wget \
    python3 \
    python3-pip \
    ninja-build \
    && rm -rf /var/lib/apt/lists/*

# Создание пользователя vscode (если не существует)
RUN groupadd -r vscode && useradd -r -g vscode -m -s /bin/bash vscode || true

# Установка дополнительных Python пакетов для Pico SDK
RUN pip3 install --no-cache-dir pyserial

# Установка pico-sdk-tools (опционально, для некоторых утилит)
RUN apt-get update && apt-get install -y \
    picotool \
    || echo "picotool not available in this repository"

# Переключение на пользователя vscode
USER vscode

# Установка переменных окружения
ENV PATH=$PATH:/usr/bin

