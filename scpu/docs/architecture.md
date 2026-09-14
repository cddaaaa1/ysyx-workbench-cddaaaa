1. PC 输出当前地址
2. ROM 根据 PC 输出 8 bit 指令
3. decoder 提取 opcode 和操作数
4. GPR 组合读出源寄存器
5. ALU 或立即数逻辑产生结果
6. 比较器判断是否跳转
7. 时钟上升沿：
   - 写入 GPR
   - 更新 PC



#### 指令

0	8B	li r0, 11
1	91	li r1, 1
2	A0	li r2, 0
3	B1	li r3, 1
4	29	add r2, r2, r1
5	17	add r1, r1, r3
6	E2	bner0 4, r1