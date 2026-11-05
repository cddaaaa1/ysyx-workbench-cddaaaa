#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
export AM_HOME="$repo_root/abstract-machine"
export NPC_HOME="$repo_root/npc"
tests_dir="$repo_root/am-kernels/tests/cpu-tests"
log_dir="$repo_root/build/cpu-tests"
mkdir -p "$log_dir"
summary="$log_dir/summary.txt"
: > "$summary"

# 限制循环数和单项运行时间，避免错误程序卡住整个 CI。
make -C "$NPC_HOME" sim ARCH=minirv-npc MAXCYCLE=200000000 2>&1 | tee "$log_dir/build.log"
simulator="$NPC_HOME/build/sim-npc_top"

# 直接调用 AM 的单项构建规则，避免 cpu-tests/Makefile 吞掉失败退出码。
test_makefile=$(mktemp)
trap 'rm -f "$test_makefile"' EXIT
mapfile -t sources < <(find "$tests_dir/tests" -name '*.c' -type f | sort)
if (( ${#sources[@]} == 0 )); then
  echo 'No CPU tests found' | tee -a "$summary"
  exit 1
fi

failed=0
for source in "${sources[@]}"; do
  test_name=$(basename "$source" .c)
  build_log="$log_dir/$test_name.build.log"
  run_log="$log_dir/$test_name.run.log"
  echo "Building and running $test_name"
  printf 'NAME = %s\nSRCS = %s\ninclude $(AM_HOME)/Makefile\n' \
    "$test_name" "${source#"$tests_dir/"}" > "$test_makefile"
  if timeout --kill-after=10s 120s make -C "$tests_dir" -f "$test_makefile" \
      ARCH=minirv-npc insert-arg \
      > "$build_log" 2>&1; then
    status=0
    timeout --kill-after=10s 120s "$simulator" \
      "$tests_dir/build/$test_name-minirv-npc.bin" > "$run_log" 2>&1 || status=$?
    if [[ "$test_name" == wrong ]]; then
      # wrong.c 故意返回 1；超时、崩溃或编译失败均不是预期结果。
      if [[ "$status" == 1 ]] && grep -Fq 'EBREAK: BAD TRAP (a0 = 1)' "$run_log"; then
        echo '[wrong] PASS (expected BAD TRAP, exit 1)' | tee -a "$summary"
        continue
      fi
    elif [[ "$status" == 0 ]]; then
      echo "[$test_name] PASS" | tee -a "$summary"
      continue
    fi
    echo "[$test_name] FAIL (simulation exit $status)" | tee -a "$summary"
    tail -n 20 "$run_log"
  else
    status=$?
    echo "[$test_name] FAIL (build exit $status)" | tee -a "$summary"
    tail -n 20 "$build_log"
  fi
  failed=$((failed + 1))
done

echo "Total: ${#sources[@]}, passed: $((${#sources[@]} - failed)), failed: $failed" | tee -a "$summary"
(( failed == 0 ))
