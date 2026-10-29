## D3 RISC-V指令集

### YEMU 如何执行程序

**解释机器码**
```c
typedef union { //成员共享内存， rtype, mtype, inst 不同时独立存在
  struct { uint8_t rs : 2, rt : 2, op : 4; } rtype;
  struct { uint8_t addr : 4,       op : 4; } mtype;
  uint8_t inst;
} inst_t;
```

**提取操作数**
```c 
#define DECODE_R(inst) uint8_t rt = (inst).rtype.rt, rs = (inst).rtype.rs //文本替换
#define DECODE_M(inst) uint8_t addr = (inst).mtype.addr 
```

**`exec_once()` 一条指令的流程**

```c
void exec_once() {
  inst_t this;
  this.inst = M[pc]; // 取指
  switch (this.rtype.op) {
  //  操作码译码       操作数译码           执行
    case 0b0000: { DECODE_R(this); R[rt]   = R[rs];   break; }
    case 0b0001: { DECODE_R(this); R[rt]  += R[rs];   break; }
    case 0b1110: { DECODE_M(this); R[0]    = M[addr]; break; }
    case 0b1111: { DECODE_M(this); M[addr] = R[0];    break; }
    default:
      printf("Invalid instruction with opcode = %x, halting...\n", this.rtype.op);
      halt = 1;
      break;
  }
  pc ++; // 更新PC
}
```

#### YEMU上执行的加法程序的状态机

```
(PC, r0, r1, r2, r3)
//状态中的 PC 表示下一条要执行的指令地址：

(0,  0,  0, 0, 0)  // 初始状态
(1, 33,  0, 0, 0)  // 执行地址 0：load 6
(2, 33, 33, 0, 0)  // 执行地址 1：mov r1, r0
(3, 16, 33, 0, 0)  // 执行地址 2：load 5
(4, 49, 33, 0, 0)  // 执行地址 3：add r0, r1
(5, 49, 33, 0, 0)  // 执行地址 4：store 7 
```
- 状态机：从机器状态的角度描述程序执行，一条指令对应一次状态转移
- exec_once()：通过取指、译码、执行和更新 PC，用 C 代码实现这次转移
- main() 中的循环：反复调用 exec_once()，使机器沿着状态机不断更新


### NEMU指令处理流程 

包括取值，译码， 执行， 更新PC， 详见ysyx-2.md NEMU 代码学习部分


### 在NEMU中运行C程序

```sh 
make -C am-kernels/tests/cpu-tests/ ARCH=riscv32-nemu run
```

由于没有实现`li	s0,0`, 输出
```sh
There are two cases which will trigger this unexpected exception:
1. The instruction at PC = 0x80000000 is not implemented.
2. Something is implemented incorrectly.
Find this PC(0x80000000) in the disassembling result to distinguish which case it is.
```

在反汇编结果(am-kernels/tests/cpu-tests/build/dummy-$ISA-nemu.txt)中查看还没有实现的指令

#### 实现更多的指令

**加一条新指令**
1. 查手册确定行为和编码
2. 按需补充操作数提取
3. 在 decode_exec() 中添加匹配规则
4. 编译并验证


#### RV32I 指令速查（按手册顺序）

| 实际指令 | 写法 | 格式 | 作用（C 语句） |
|---|---|---|---|
| `LUI` | `lui t0,0x12345` | U | `R(rd) = imm;` Load Upper Immediate；高 20 位立即数，低 12 位补零 |
| `AUIPC` | `auipc t0,0x1` | U | `R(rd) = s->pc + imm;` Add Upper Immediate to PC；构造 PC 相对地址 |
| `JAL` | `jal ra,target` | J | `R(rd) = s->snpc; s->dnpc = s->pc + imm;` Jump and Link；跳转并保存返回地址，`j target` 为 `jal zero,target` |
| `JALR` | `jalr ra,0(t0)` | I | `R(rd) = s->snpc; s->dnpc = (src1 + imm) & ~1u;` Jump and Link Register；寄存器间接跳转，`ret` 为 `jalr zero,0(ra)` |
| `BEQ` | `beq a0,a1,target` | B | `if (src1 == src2) s->dnpc = s->pc + imm;` Branch if Equal；相等则跳转，`beqz a0,target` 与 `zero` 比较 |
| `BNE` | `bne a0,a1,target` | B | `if (src1 != src2) s->dnpc = s->pc + imm;` Branch if Not Equal；不等则跳转，`bnez a0,target` 与 `zero` 比较 |
| `BLT` | `blt a0,a1,target` | B | `if ((sword_t)src1 < (sword_t)src2) s->dnpc = s->pc + imm;` Branch if Less Than；有符号小于则跳转 |
| `BGE` | `bge a0,a1,target` | B | `if ((sword_t)src1 >= (sword_t)src2) s->dnpc = s->pc + imm;` Branch if Greater Than or Equal；有符号大于等于则跳转 |
| `BLTU` | `bltu a0,a1,target` | B | `if (src1 < src2) s->dnpc = s->pc + imm;` Branch if Less Than Unsigned；无符号小于则跳转 |
| `BGEU` | `bgeu a0,a1,target` | B | `if (src1 >= src2) s->dnpc = s->pc + imm;` Branch if Greater Than or Equal Unsigned；无符号大于等于则跳转 |
| `LB` | `lb a0,1(t0)` | I | `R(rd) = SEXT(Mr(src1 + imm, 1), 8);` Load Byte；读取 1 字节，符号扩展到 32 位 |
| `LH` | `lh a0,2(t0)` | I | `R(rd) = SEXT(Mr(src1 + imm, 2), 16);` Load Halfword；读取 2 字节，符号扩展到 32 位 |
| `LW` | `lw ra,12(sp)` | I | `R(rd) = Mr(src1 + imm, 4);` Load Word；读取 4 字节 |
| `LBU` | `lbu a0,1(t0)` | I | `R(rd) = Mr(src1 + imm, 1);` Load Byte Unsigned；读取 1 字节，零扩展到 32 位 |
| `LHU` | `lhu a0,2(t0)` | I | `R(rd) = Mr(src1 + imm, 2);` Load Halfword Unsigned；读取 2 字节，零扩展到 32 位 |
| `SB` | `sb a0,1(t0)` | S | `Mw(src1 + imm, 1, src2);` Store Byte；写入源寄存器的低 8 位 |
| `SH` | `sh a0,2(t0)` | S | `Mw(src1 + imm, 2, src2);` Store Halfword；写入源寄存器的低 16 位 |
| `SW` | `sw ra,12(sp)` | S | `Mw(src1 + imm, 4, src2);` Store Word；写入源寄存器的 32 位 |
| `ADDI` | `addi sp,sp,-16` | I | `R(rd) = src1 + imm;` Add Immediate；`mv rd,rs` 为 `addi rd,rs,0`，小立即数 `li rd,7` 为 `addi rd,zero,7` |
| `SLTI` | `slti a0,a1,-1` | I | `R(rd) = (sword_t)src1 < (sword_t)imm;` Set Less Than Immediate；有符号小于置 1，否则置 0 |
| `SLTIU` | `sltiu a0,a1,1` | I | `R(rd) = src1 < imm;` Set Less Than Immediate Unsigned；立即数先符号扩展，再无符号比较；`seqz a0,a1` 为本例 |
| `XORI` | `xori a0,a1,-1` | I | `R(rd) = src1 ^ imm;` Exclusive OR Immediate；按位异或，本例也写作 `not a0,a1` |
| `ORI` | `ori a0,a1,0x10` | I | `R(rd) = src1 \| imm;` OR Immediate；按位或 |
| `ANDI` | `andi a0,a1,0xff` | I | `R(rd) = src1 & imm;` AND Immediate；按位与 |
| `SLLI` | `slli a5,s2,0x2` | I（移位） | `R(rd) = src1 << (imm & 0x1f);` Shift Left Logical Immediate；逻辑左移，低位补零 |
| `SRLI` | `srli a0,a1,3` | I（移位） | `R(rd) = src1 >> (imm & 0x1f);` Shift Right Logical Immediate；逻辑右移，高位补零 |
| `SRAI` | `srai a0,a1,3` | I（移位） | `R(rd) = (sword_t)src1 >> (imm & 0x1f);` Shift Right Arithmetic Immediate；算术右移，高位补符号位 |
| `ADD` | `add a3,a3,a5` | R | `R(rd) = src1 + src2;` Add；两寄存器相加 |
| `SUB` | `sub a0,a0,s0` | R | `R(rd) = src1 - src2;` Subtract；第一个源操作数减去第二个 |
| `SLL` | `sll a0,a1,a2` | R | `R(rd) = src1 << (src2 & 0x1f);` Shift Left Logical；移位量取第二个源寄存器低 5 位 |
| `SLT` | `slt a0,a1,a2` | R | `R(rd) = (sword_t)src1 < (sword_t)src2;` Set Less Than；有符号小于置 1，否则置 0 |
| `SLTU` | `sltu a0,a1,a2` | R | `R(rd) = src1 < src2;` Set Less Than Unsigned；无符号比较；`snez a0,a1` 为 `sltu a0,zero,a1` |
| `XOR` | `xor a0,a1,a2` | R | `R(rd) = src1 ^ src2;` Exclusive OR；按位异或 |
| `SRL` | `srl a0,a1,a2` | R | `R(rd) = src1 >> (src2 & 0x1f);` Shift Right Logical；逻辑右移，高位补零 |
| `SRA` | `sra a0,a1,a2` | R | `R(rd) = (sword_t)src1 >> (src2 & 0x1f);` Shift Right Arithmetic；算术右移，高位补符号位 |
| `OR` | `or a0,a1,a2` | R | `R(rd) = src1 \| src2;` OR；按位或 |
| `AND` | `and a0,a1,a2` | R | `R(rd) = src1 & src2;` AND；按位与 |
<!-- | `FENCE` | `fence rw,rw` | I（FENCE 专用字段） | `fence(pred, succ);` Fence；约束前驱集合与后继集合中的内存、I/O 操作顺序 |
| `FENCE.TSO` | `fence.tso` | I（FENCE 特殊编码） | `fence_tso();` Fence, Total Store Ordering；普通访存中约束读→读/写、写→写，不强制写→读有序 |
| `PAUSE` | `pause` | I（FENCE 提示编码） | `pause_hint();` 暂停提示；可短暂降低执行速率，也允许无额外效果，属于 Zihintpause 扩展 |
| `ECALL` | `ecall` | I（SYSTEM 固定编码） | `raise_ecall(s->pc);` Environment Call；触发环境调用异常，请求执行环境服务 |
| `EBREAK` | `ebreak` | I（SYSTEM 固定编码） | `raise_breakpoint(s->pc);` Environment Breakpoint；触发断点异常；当前 NEMU 用 `NEMUTRAP(s->pc, R(10))` 结束客户程序 | -->

#### RV32M 指令速查（按手册顺序）

| 实际指令 | 写法 | 格式 | 作用（C 语句） |
|---|---|---|---|
| `MUL` | `mul a0,a1,a2` | R | `R(rd) = src1 * src2` Multiply；取乘积低 32 位，低位结果不区分有符号和无符号 |
| `MULH` | `mulh a0,a1,a2` | R | ` R(rd) = (SEXT(src1,32) * SEXT(src2,32)) >> 32` Multiply High；有符号 × 有符号，取乘积高 32 位 |
| `MULHSU` | `mulhsu a0,a1,a2` | R | `R(rd) = (SEXT(src1,32) * (uint64_t)src2) >> 32)` Multiply High Signed-Unsigned；`rs1` 有符号 × `rs2` 无符号，取高 32 位 |
| `MULHU` | `mulhu a0,a1,a2` | R | `R(rd) = ((uint64_t)src1 * (uint64_t)src2) >> 32);` Multiply High Unsigned；无符号 × 无符号，取乘积高 32 位 |
| `DIV` | `div a0,a1,a2` | R | `R(rd) = b == 0 ? UINT32_MAX : (word_t)(a / b);` Divide；有符号除法，商向零截断 |
| `DIVU` | `divu a0,a1,a2` | R | `R(rd) = ub == 0 ? UINT32_MAX : ua / ub;` Divide Unsigned；无符号除法 |
| `REM` | `rem a4,s0,a5` | R | `R(rd) = b == 0 ? src1 : (word_t)(a % b);` Remainder；有符号求余，非零余数与被除数同号 |
| `REMU` | `remu a0,a1,a2` | R | `R(rd) = ub == 0 ? src1 : ua % ub;` Remainder Unsigned；无符号求余 |



**符号，位数**

```c
word_t src     // uint32_t：32 位无符号数
(sword_t)src   // 转为 int32_t：按有符号数解释，位宽仍为 32 位
(uint64_t)src  // 零扩展到 64 位，结果类型为 uint64_t
SEXT(src, 32)  // 将低 32 位符号扩展到 64 位，结果类型仍为 uint64_t
```

**除零**

- 手册13.2. Division Operations： 
  The quotient of division by zero has all bits set(全1及0xffffffff), and the remainder of division by zero equals the dividend.

- 为什么除零不触发trap?:
  - 整数除零会成为标准 ISA 中唯一触发 trap 的算术异常。要处理除零需要执行环境的 trap 处理机制配合，实现更复杂。
  - 如果语言要求除零时改变控制流， 只需要在除法运算后添加一条错误处理branch。正常程序很少除零，所以这条branch通常不会跳转，容易被分支预测器预测，额外开销较小。


**有符号除法溢出**
- Signed division overflow occurs only when the most-negative integer(0x80000000) is divided by -1. The quotient of a signed division with overflow is equal to the dividend, and the remainder is zero. Unsigned division overflow cannot occur.

#### 验收-实现支持RV32IM的NEMU
- 尚未实现的指令：FENCE，ECALL
- jal 跳转地址未对齐异常尚未处理
- cpu-test: string, hello-str 本阶段允许暂时FAIL, 其余均符合预期
- 在`string.c`，`stdio.c` 实现字符串，sprinf()等函数后， cpu-test:string , hello-str 均PASS
- risc-tests: `make -C riscv-tests ARCH=riscv32-nemu run`  ALL PASS



