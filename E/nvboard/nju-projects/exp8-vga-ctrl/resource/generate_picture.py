from pathlib import Path

WIDTH = 640
HEIGHT = 512
VISIBLE_HEIGHT = 480
OUTPUT = Path(__file__).with_name("picture.hex")

with OUTPUT.open("w", encoding="ascii") as picture:
    for h_addr in range(WIDTH):
        for v_addr in range(HEIGHT):
            if v_addr >= VISIBLE_HEIGHT:
                pixel = 0x000
            elif h_addr < WIDTH // 3:
                pixel = 0xF00
            elif h_addr < 2 * WIDTH // 3:
                pixel = 0x0F0
            else:
                pixel = 0x00F

            if 16 <= h_addr < WIDTH - 16 and 16 <= v_addr < VISIBLE_HEIGHT - 16:
                if h_addr % 64 < 32:
                    pixel = ((pixel >> 1) & 0x777)

            picture.write(f"{pixel:03X}\n")

print(f"generated {OUTPUT} with {WIDTH * HEIGHT} RGB444 words")
