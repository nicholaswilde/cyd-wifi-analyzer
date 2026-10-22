#!/usr/bin/env bash
# ==============================================================================
#
# flash.sh
# -------
# Downloads the latest release from GitHub to the /tmp directory,
# extracts the bin files, checks if esptool is installed, and flashes the device.
#
# Usage: bash -c "$(curl -fsSL https://raw.githubusercontent.com/nicholaswilde/cyd-wifi-analyzer/main/scripts/flash.sh)" _ [DEVICE] [SERIAL_PORT]
# Example: ./flash.sh cyd_28r /dev/ttyUSB0
#
# ==============================================================================

set -euo pipefail

# --- variables ---
GITHUB_REPO="nicholaswilde/cyd-wifi-analyzer"
DEVICE="${1:-cyd_28r}"
SERIAL_PORT="${2:-/dev/ttyUSB0}"

# --- Constants ---
BLUE=$(tput setaf 4 || echo "")
readonly BLUE
RED=$(tput setaf 1 || echo "")
readonly RED
YELLOW=$(tput setaf 3 || echo "")
readonly YELLOW
RESET=$(tput sgr0 || echo "")
readonly RESET

# --- functions ---

function log() {
  local type="$1"
  local message="$2"
  local color="$RESET"

  case "$type" in
    INFO) color="$BLUE";;
    WARN) color="$YELLOW";;
    ERRO) color="$RED";;
  esac

  echo -e "${color}${type}${RESET}[$(date +'%Y-%m-%d %H:%M:%S')] ${message}"
}

function commandExists() {
  command -v "$1" >/dev/null 2>&1
}

function check_dependencies() {
  if ! commandExists curl || ! commandExists grep || ! commandExists unzip || (! commandExists esptool && ! commandExists esptool.py); then
    log "ERRO" "Required dependencies (curl, grep, unzip, esptool/esptool.py) are not installed." >&2
    exit 1
  fi
}

function download_release(){
  local release
  release=$(curl -fsSL "https://api.github.com/repos/${GITHUB_REPO}/releases/latest" | grep -o '"tag_name": *"[^"]*"' | cut -d '"' -f 4 || true)
  if [ -z "$release" ]; then
    log "ERRO" "Could not determine the latest release version." >&2
    exit 1
  fi
  log "INFO" "Latest release: ${release}"
  log "INFO" "Target device: ${DEVICE}"

  # Get the download URL for the specific device zip file
  LATEST_RELEASE_URL=$(curl -s "https://api.github.com/repos/${GITHUB_REPO}/releases/latest" | grep "browser_download_url" | grep "${DEVICE}.zip" | grep -o 'https://[^"]*' || true)

  if [ -z "${LATEST_RELEASE_URL}" ]; then
    log "ERRO" "Could not find the release zip file for device: ${DEVICE}." >&2
    exit 1
  fi

  TMP_DIR=$(mktemp -d)
  log "INFO" "Downloading release from ${LATEST_RELEASE_URL}..."
  curl -sL "${LATEST_RELEASE_URL}" -o "${TMP_DIR}/latest_release.zip"
}

function extract_files() {
  log "INFO" "Extracting bin files to ${TMP_DIR}..."
  unzip -o "${TMP_DIR}/latest_release.zip" -d "${TMP_DIR}" "*.bin" &> /dev/null
}

function flash_device() {
  log "INFO" "Ready to flash the device on port ${SERIAL_PORT}."

  local esptool_cmd="esptool.py"
  if ! commandExists esptool.py && commandExists esptool; then
    esptool_cmd="esptool"
  fi

  "$esptool_cmd" \
    --chip esp32 \
    --port "${SERIAL_PORT}" \
    --baud 921600 \
    --before default-reset \
    --after hard-reset \
    write-flash \
      -z \
      --flash-mode dio \
      --flash-freq 40m \
      --flash-size 4MB \
      0x1000 "${TMP_DIR}/bootloader.bin" \
      0x8000 "${TMP_DIR}/partitions.bin" \
      0x10000 "${TMP_DIR}/firmware.bin"
}

function cleanup() {
  if [ -n "${TMP_DIR:-}" ] && [ -d "${TMP_DIR}" ]; then
    rm -rf "${TMP_DIR}"
  fi
}

trap cleanup EXIT

function main() {
  check_dependencies
  download_release
  extract_files
  flash_device
  log "INFO" "--- Flashing complete ---"
}

main "$@"
