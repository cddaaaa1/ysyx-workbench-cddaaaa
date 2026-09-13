#include <cstdint>
#include <cstdio>
#include "../obj_dir/Vtop.h"

#ifdef NVBOARD_SIM

#include "verilated.h"
#include "verilated_vcd_c.h"

static VerilatedContext *contextp;
static VerilatedVcdC *tfp;
static Vtop *top;

static void step_and_dump_wave()
{
    top->eval();
    contextp->timeInc(1);
    tfp->dump(contextp->time());
}

static void sim_init()
{
    contextp = new VerilatedContext;
    tfp = new VerilatedVcdC;
    top = new Vtop;
    contextp->traceEverOn(true);
    top->trace(tfp, 0);
    tfp->open("dump.vcd");
}

static void sim_exit()
{
    step_and_dump_wave();
    tfp->close();
    delete top;
    delete tfp;
    delete contextp;
}

static uint8_t expected_segments(uint8_t value)
{
    static const uint8_t hex_segments[16] = {
        0x40, 0x79, 0x24, 0x30,
        0x19, 0x12, 0x02, 0x78,
        0x00, 0x10, 0x08, 0x03,
        0x46, 0x21, 0x06, 0x0e};
    uint8_t segments = hex_segments[value & 0xf];
    return static_cast<uint8_t>(
        ((segments & 0x01) << 7) |
        ((segments & 0x02) << 5) |
        ((segments & 0x04) << 3) |
        ((segments & 0x08) << 1) |
        ((segments & 0x10) >> 1) |
        ((segments & 0x20) >> 3) |
        ((segments & 0x40) >> 5) |
        0x01);
}

static int check_display(uint8_t expected)
{
    uint8_t expected_low = expected_segments(expected);
    uint8_t expected_high = expected_segments(expected >> 4);
    uint8_t got_low = static_cast<uint8_t>(top->seg0);
    uint8_t got_high = static_cast<uint8_t>(top->seg1);

    if (got_low == expected_low && got_high == expected_high)
        return 0;

    std::printf(
        "Error: expected %02x, got seg1=%02x seg0=%02x\n",
        expected, got_high, got_low);
    return 1;
}

int main()
{
    sim_init();

    uint8_t expected = 0x01;
    top->btn = 0;
    step_and_dump_wave();

    int error_count = check_display(expected);
    for (int step = 0; step < 255; step++)
    {
        top->btn = 1;
        step_and_dump_wave();
        top->btn = 0;
        step_and_dump_wave();

        expected = static_cast<uint8_t>(
                       ((expected & 0x10) >> 4) ^
                       ((expected & 0x08) >> 3) ^
                       ((expected & 0x04) >> 2) ^
                       (expected & 0x01))
                       << 7 |
                   (expected >> 1);
        if (expected == 0)
            expected = 0x01;
        error_count += check_display(expected);
    }

    sim_exit();

    if (error_count == 0)
    {
        std::printf("ALL 255 LFSR STEPS PASSED\n");
        return 0;
    }
    std::printf("TEST FAILED: %d errors\n", error_count);
    return 1;
}

#else

#include <nvboard.h>

static Vtop dut;

void nvboard_bind_all_pins(TOP_NAME *top);

int main()
{
    nvboard_bind_all_pins(&dut);
    nvboard_init();

    while (true)
    {
        nvboard_update();
        dut.eval();
    }
}

#endif