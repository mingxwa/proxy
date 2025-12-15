#!/usr/bin/env bash

if [ $# -ne 2 ]; then
  echo "Usage: $0 <compiler> <output path>"
  exit 1
fi

COMPILER="$1"
OUTPUT_PATH="$2"

COMPILER_INFO="$("$COMPILER" --version 2>/dev/null | awk 'NF {print; exit}')"
if [ -z "$COMPILER_INFO" ]; then
  echo "Unable to retrieve compiler info for '$COMPILER'."
  exit 1
fi

trim() {
  printf '%s' "$1" | sed -e 's/^[[:space:]]*//' -e 's/[[:space:]]*$//'
}

read_lscpu_field() {
  local key="$1"
  command -v lscpu >/dev/null 2>&1 || return
  while IFS=: read -r field value; do
    field="$(trim "$field")"
    if [ "$field" = "$key" ]; then
      echo "$(trim "$value")"
      return
    fi
  done < <(lscpu 2>/dev/null)
}

format_mem_kib() {
  local kib="$1"
  if [[ -z "$kib" ]]; then
    echo "Unknown"
    return
  fi
  if [[ ! "$kib" =~ ^[0-9]+$ ]]; then
    echo "Unknown"
    return
  fi
  awk -v val="$kib" 'BEGIN { printf "%.2f GB", val / 1048576 }'
}

human_readable_bytes() {
  local bytes="$1"
  if [[ -z "$bytes" ]]; then
    echo "Unknown"
    return
  fi
  if [[ ! "$bytes" =~ ^[0-9]+$ ]]; then
    echo "Unknown"
    return
  fi
  awk -v val="$bytes" 'BEGIN {
    split("B KB MB GB TB PB", units)
    idx = 1
    while (val >= 1024 && idx < length(units)) {
      val /= 1024
      idx++
    }
    printf "%.2f %s", val, units[idx]
  }'
}

read_sysfs_cache() {
  local level="$1"
  local type_filter="$2"
  local path
  for path in /sys/devices/system/cpu/cpu0/cache/index*; do
    [ -d "$path" ] || continue
    local lvl
    lvl="$(cat "$path/level" 2>/dev/null)"
    [ "$lvl" = "$level" ] || continue
    local typ
    typ="$(cat "$path/type" 2>/dev/null)"
    if [ -n "$type_filter" ] && [ "$typ" != "$type_filter" ]; then
      continue
    fi
    if [ -f "$path/size" ]; then
      cat "$path/size"
      return
    fi
  done
}

get_memory_speed_linux() {
  local speed=""
  if command -v dmidecode >/dev/null 2>&1; then
    speed="$(dmidecode -t memory 2>/dev/null | awk -F: '/Configured Memory Speed/ {gsub(/^[ \t]+|[ \t]+$/, "", $2); if ($2 !~ /Unknown/ && $2 !~ /0 MHz/) {print $2; exit}}')"
    if [ -z "$speed" ]; then
      speed="$(dmidecode -t memory 2>/dev/null | awk -F: '/Speed/ {gsub(/^[ \t]+|[ \t]+$/, "", $2); if ($2 !~ /Unknown/ && $2 !~ /0 MHz/) {print $2; exit}}')"
    fi
  fi
  if [ -z "$speed" ] && command -v lshw >/dev/null 2>&1; then
    speed="$(lshw -class memory 2>/dev/null | awk -F: '/clock/ {gsub(/^[ \t]+|[ \t]+$/, "", $2); sub(/\(.*/, "", $2); if ($2 ~ /Hz/) {print $2; exit}}')"
  fi
  echo "${speed:-Unknown}"
}

get_memory_speed_macos() {
  local speed
  speed="$(system_profiler SPMemoryDataType 2>/dev/null | awk -F: '/Speed/ {gsub(/^[ \t]+|[ \t]+$/, "", $2); if ($2 !~ /Unknown/) {print $2; exit}}')"
  echo "${speed:-Unknown}"
}

KERNEL_NAME="$(uname -s)"
OS_NAME="$KERNEL_NAME"
if [ "$KERNEL_NAME" = "Linux" ]; then
  if [ -f /etc/os-release ]; then
    OS_NAME="$(grep ^PRETTY_NAME= /etc/os-release | cut -d= -f2 | tr -d '"')"
  fi
elif [ "$KERNEL_NAME" = "Darwin" ]; then
  OS_NAME="$(sw_vers -productName) $(sw_vers -productVersion)"
fi

RAM_TOTAL="Unknown"
RAM_SPEED="Unknown"
CPU_MODEL="Unknown"
CPU_BASE_SPEED="Unknown"
PHYSICAL_CORES="Unknown"
LOGICAL_PROCESSORS="Unknown"
L1D_CACHE="Unknown"
L1I_CACHE="Unknown"
L2_CACHE="Unknown"
L3_CACHE="Unknown"

if [ "$KERNEL_NAME" = "Linux" ]; then
  if [ -r /proc/meminfo ]; then
    total_kib="$(awk '/MemTotal/ {print $2; exit}' /proc/meminfo)"
    RAM_TOTAL="$(format_mem_kib "$total_kib")"
  fi
  RAM_SPEED="$(get_memory_speed_linux)"

  if command -v lscpu >/dev/null 2>&1; then
    CPU_MODEL="$(read_lscpu_field "Model name")"
    CPU_BASE_SPEED="$(read_lscpu_field "CPU max MHz")"
    if [ -z "$CPU_BASE_SPEED" ]; then
      CPU_BASE_SPEED="$(read_lscpu_field "CPU MHz")"
    fi
    cores_per_socket="$(read_lscpu_field "Core(s) per socket")"
    sockets="$(read_lscpu_field "Socket(s)")"
    if [[ -n "$cores_per_socket" && -n "$sockets" && "$cores_per_socket" =~ ^[0-9]+$ && "$sockets" =~ ^[0-9]+$ ]]; then
      PHYSICAL_CORES=$((cores_per_socket * sockets))
    fi
    logical="$(read_lscpu_field "CPU(s)")"
    if [[ -n "$logical" && "$logical" =~ ^[0-9]+$ ]]; then
      LOGICAL_PROCESSORS="$logical"
    fi
    L1D_CACHE="$(read_lscpu_field "L1d cache")"
    L1I_CACHE="$(read_lscpu_field "L1i cache")"
    L2_CACHE="$(read_lscpu_field "L2 cache")"
    L3_CACHE="$(read_lscpu_field "L3 cache")"
  fi

  if [ -z "$LOGICAL_PROCESSORS" ] && command -v nproc >/dev/null 2>&1; then
    LOGICAL_PROCESSORS="$(nproc --all 2>/dev/null)"
  fi
  if [ -z "$LOGICAL_PROCESSORS" ]; then
    LOGICAL_PROCESSORS="$(getconf _NPROCESSORS_ONLN 2>/dev/null)"
  fi

  if [ "$L1D_CACHE" = "" ] || [ "$L1D_CACHE" = "Unknown" ]; then
    cache_val="$(read_sysfs_cache 1 Data)"
    L1D_CACHE="${cache_val:-Unknown}"
  fi
  if [ "$L1I_CACHE" = "" ] || [ "$L1I_CACHE" = "Unknown" ]; then
    cache_val="$(read_sysfs_cache 1 Instruction)"
    L1I_CACHE="${cache_val:-Unknown}"
  fi
  if [ "$L2_CACHE" = "" ] || [ "$L2_CACHE" = "Unknown" ]; then
    cache_val="$(read_sysfs_cache 2)"
    L2_CACHE="${cache_val:-Unknown}"
  fi
  if [ "$L3_CACHE" = "" ] || [ "$L3_CACHE" = "Unknown" ]; then
    cache_val="$(read_sysfs_cache 3)"
    L3_CACHE="${cache_val:-Unknown}"
  fi
elif [ "$KERNEL_NAME" = "Darwin" ]; then
  RAM_TOTAL="$(human_readable_bytes "$(sysctl -n hw.memsize 2>/dev/null)")"
  RAM_SPEED="$(get_memory_speed_macos)"
  CPU_MODEL="$(sysctl -n machdep.cpu.brand_string 2>/dev/null)"
  freq_hz="$(sysctl -n hw.cpufrequency_max 2>/dev/null)"
  if [ -z "$freq_hz" ]; then
    freq_hz="$(sysctl -n hw.cpufrequency 2>/dev/null)"
  fi
  if [[ -n "$freq_hz" && "$freq_hz" =~ ^[0-9]+$ ]]; then
    CPU_BASE_SPEED="$(awk -v val="$freq_hz" 'BEGIN { printf "%.2f MHz", val / 1000000 }')"
  fi
  PHYSICAL_CORES="$(sysctl -n hw.physicalcpu 2>/dev/null)"
  LOGICAL_PROCESSORS="$(sysctl -n hw.logicalcpu 2>/dev/null)"
  L1D_CACHE="$(human_readable_bytes "$(sysctl -n hw.l1dcachesize 2>/dev/null)")"
  L1I_CACHE="$(human_readable_bytes "$(sysctl -n hw.l1icachesize 2>/dev/null)")"
  L2_CACHE="$(human_readable_bytes "$(sysctl -n hw.l2cachesize 2>/dev/null)")"
  L3_CACHE="$(human_readable_bytes "$(sysctl -n hw.l3cachesize 2>/dev/null)")"
fi

RAM_TOTAL="${RAM_TOTAL:-Unknown}"
RAM_SPEED="${RAM_SPEED:-Unknown}"
CPU_MODEL="${CPU_MODEL:-Unknown}"
CPU_BASE_SPEED="${CPU_BASE_SPEED:-Unknown}"
PHYSICAL_CORES="${PHYSICAL_CORES:-Unknown}"
LOGICAL_PROCESSORS="${LOGICAL_PROCESSORS:-Unknown}"
L1D_CACHE="${L1D_CACHE:-Unknown}"
L1I_CACHE="${L1I_CACHE:-Unknown}"
L2_CACHE="${L2_CACHE:-Unknown}"
L3_CACHE="${L3_CACHE:-Unknown}"

cat <<EOF > "$OUTPUT_PATH"
{
  "OS": "$OS_NAME",
  "KernelVersion": "$(uname -r)",
  "Architecture": "$(uname -m)",
  "Compiler": "$COMPILER_INFO",
  "Hardware": {
    "Memory": {
      "Total": "$RAM_TOTAL",
      "Speed": "$RAM_SPEED"
    },
    "CPU": {
      "Model": "$CPU_MODEL",
      "BaseSpeed": "$CPU_BASE_SPEED",
      "PhysicalCores": "$PHYSICAL_CORES",
      "LogicalProcessors": "$LOGICAL_PROCESSORS",
      "Caches": {
        "L1d": "$L1D_CACHE",
        "L1i": "$L1I_CACHE",
        "L2": "$L2_CACHE",
        "L3": "$L3_CACHE"
      }
    }
  }
}
EOF
