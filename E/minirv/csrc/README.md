ADDI rd, rs1, imm 

imm[11:0] rs1[4:0] 000 rd[4:0] 0010011

g++ -O2 -Wall -Wextra -o E/minirv/build/minirvemu E/minirv/csrc/main.cpp E/minirv/csrc/minirvemu.cpp
./E/minirv/build/minirvemu     