#!/usr/bin/env bash
# build_flash.sh — 编译并烧录 epd1in54_V2-demo 到 ESP32 (COM6)
#
# 用法:
#   bash build_flash.sh          # 编译 + 烧录
#   bash build_flash.sh compile  # 仅编译
#   bash build_flash.sh flash    # 仅烧录（需先编译过一次）
#
# 前置条件: 已安装 arduino-cli，见 README 或脚本末尾注释

set -e

# ── 配置区 ──────────────────────────────────────────────────────────────────
SKETCH_DIR="$(cd "$(dirname "$0")" && pwd)"
PORT="COM6"
FQBN="esp32:esp32:esp32dev"           # 常见 ESP32 开发板；如板型不对见下方说明
BAUD="921600"                         # 烧录波特率
BUILD_DIR="$SKETCH_DIR/.build"        # 编译产物目录（已加入 .gitignore 建议）

# 复用 Arduino IDE 的核心数据，不重复下载
ARDUINO_DATA="C:/Users/lenovo/AppData/Local/Arduino15"
# ────────────────────────────────────────────────────────────────────────────

check_arduino_cli() {
    if ! command -v arduino-cli &>/dev/null; then
        echo "❌  找不到 arduino-cli，请先安装："
        echo "    winget install ArduinoSA.CLI"
        echo "    或访问 https://arduino.github.io/arduino-cli/latest/installation/"
        exit 1
    fi
}

do_compile() {
    echo "🔨  编译中 (FQBN: $FQBN) ..."
    arduino-cli compile \
        --fqbn "$FQBN" \
        --build-path "$BUILD_DIR" \
        --config-file /dev/null \
        --additional-urls "https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json" \
        --build-property "runtime.platform.path=$ARDUINO_DATA/packages/esp32/hardware/esp32/2.0.9" \
        "$SKETCH_DIR"
    echo "✅  编译成功，产物在 $BUILD_DIR"
}

do_flash() {
    if [ ! -f "$BUILD_DIR/epd1in54_V2-demo.ino.bin" ]; then
        echo "❌  找不到编译产物，请先编译："
        echo "    bash build_flash.sh compile"
        exit 1
    fi
    echo "🚀  烧录到 $PORT ..."
    arduino-cli upload \
        --fqbn "$FQBN" \
        --port "$PORT" \
        --input-dir "$BUILD_DIR" \
        "$SKETCH_DIR"
    echo "✅  烧录完成！"
}

# ── 主流程 ───────────────────────────────────────────────────────────────────
check_arduino_cli

ACTION="${1:-all}"

case "$ACTION" in
    compile)
        do_compile
        ;;
    flash)
        do_flash
        ;;
    all|"")
        do_compile
        do_flash
        ;;
    *)
        echo "用法: bash build_flash.sh [compile|flash|all]"
        exit 1
        ;;
esac

# ── FQBN 参考（如果烧录失败提示 board not found，换这里的值）──────────────
# ESP32 Dev Module:      esp32:esp32:esp32dev   ← 默认
# Generic ESP32:         esp32:esp32:esp32
# ESP32-S3 Dev Module:   esp32:esp32:esp32s3dev
# 查看所有已安装板型:  arduino-cli board listall esp32
