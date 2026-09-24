# 性能记录

各阶段的性能数据存档。**标记区内的表格由脚本生成，不要手工编辑**。

更新方式：

```sh
cd archbench/scripts
bash run.sh ARCH=minirv-npc mainargs=train      # 跑评测, 结果落在 archbench/result/*.log
python3 record.py -w --note "备注"              # 刷新本文件的表格
```


```sh
cd npc
./build/sim-sim_top program/prog_sb.bin
NPC_TRACE=prog_sb.vcd NPC_TRACE_DEPTH=99 ./build/sim-sim_top program/prog_sb.bin   # 顺带 dump 波形
```


<!-- archbench:start -->
### 汇总

| 日期 | commit | ARCH | mainargs | 成功/总数 | GEOMEAN | MEAN | 备注 |
|---|---|---|---|---:|---:|---:|---|
| 2026-09-22 13:01 | 6e6b6fe | minirv-npc | train | 18/20 | 336 | 813 | 支持 SimpleBus 的 IFU |
| 2026-09-22 13:14 | 6595118 | minirv-npc | train | 19/20 | 563 | 1542 | 基线: SimpleBus 之前的单周期 IFU (IPC=1) |
| 2026-09-22 14:02 | 6e6b6fe | minirv-npc | train | 19/20 | 279 | 770 | 支持 SimpleBus 的 IFU |
| 2026-09-22 14:23 | 6e6b6fe* | minirv-npc | train | 19/20 | 279 | 770 | 计数器改 64 位后复测 (RTL 同上一行, 分数一致) |
| 2026-09-22 17:47 | 82e13c8* | minirv-npc | train | 19/20 | 239 | 662 | 支持simplebus的lsu |
| 2026-09-23 09:42 | 37215c3 | minirv-npc | train | 19/20 | 239 | 662 | 支持有效信号的SimpleBus协议 |
| 2026-09-24 10:43 | 9a44e22* | minirv-npc | train | 19/20 | 239 | 662 | 备注 |

### 明细（最近一次: 2026-09-24 10:43）

| bench | Marks | Scored | Total | WALL | 指令数 | 周期数 | IPC | 备注 |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| 100.blockchain | 502 | 85606 us | 85959 us | 5.86 s | 15203390 | 35653307 | 0.43 | OK |
| 101.igemm | 12 | 1810635 us | 2172668 us | 112.26 s | 368916531 | 870746776 | 0.42 | OK |
| 102.queen | 1042 | 528787 us | 529014 us | 36.38 s | 89017683 | 213140829 | 0.42 | OK |
| 103.dinic | 1405 | 78255 us | 681403 us | 48.45 s | 116575231 | 273923083 | 0.43 | OK |
| 104.malloc | 1203 | 537388 us | 540219 us | 37.88 s | 93665903 | 217627036 | 0.43 | OK |
| 105.bf | 775 | 207476 us | 210331 us | 15.24 s | 37000221 | 85542926 | 0.43 | OK |
| 106.qsort | 1990 | 121588 us | 1271947 us | 76.97 s | 216475430 | 510237222 | 0.42 | OK |
| 107.stream | 613 | 440279 us | 499119 us | 38.94 s | 88292406 | 201130062 | 0.44 | OK |
| 108.ntt | 12 | 561007 us | 564347 us | 42.44 s | 95155949 | 227230231 | 0.42 | OK |
| 200.genann | 14 | 4601491 us | 4601650 us | 187.62 s | 778613333 | 1842449374 | 0.42 | OK |
| 201.jpeg | 290 | 285333 us | 323563 us | 22.49 s | 55857022 | 130865140 | 0.43 | OK |
| 202.llama2 | 8 | 23566922 us | 23611012 us | 776.59 s | 3990810707 | 9446418121 | 0.42 | OK |
| 203.rsa | 155 | 1393048 us | 1393269 us | 83.69 s | 235407315 | 558960719 | 0.42 | OK |
| 204.hypergraph | 1695 | 1027506 us | 1353084 us | 80.70 s | 234239290 | 542815837 | 0.43 | OK |
| 300.bzip3 | 881 | 635042 us | 635766 us | 45.61 s | 108703311 | 255845277 | 0.42 | OK |
| 301.gsim | 562 | 170521 us | 213817 us | 14.48 s | 36684217 | 86911164 | 0.42 | OK |
| 302.lua | 981 | 70270 us | 70278 us | 6.50 s | 12554361 | 29367729 | 0.43 | OK |
| 303.cproc | — | — | — | 9.58 s | — | — | — | 无结果 |
| 304.mp3 | 7 | 2112650 us | 2127743 us | 102.96 s | 360505147 | 852731654 | 0.42 | OK |
| 305.h264 | 437 | 431987 us | 535440 us | 37.99 s | 92150323 | 215666758 | 0.43 | OK |

> 在 `archbench/scripts` 下执行 `bash run.sh ARCH=... mainargs=train`，再 `python3 record.py -w` 更新上表。
> `commit` 带 `*` 表示评测时 `npc/` 有未提交改动, 不代表 HEAD 那个版本。
> GEOMEAN/MEAN 只对产出 `[RESULT]` 的项计算（`run.sh` 自己的 excel.txt 在任一项为 0 时会把 GEOMEAN 算成 0）。
<!-- archbench:end -->
