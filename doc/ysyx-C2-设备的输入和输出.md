# C2-设备的输入和输出

## GUI输入输出

### 时钟

`make -C am-kernels/tests/am-tests ARCH=native mainargs=t run`

核心为两个函数：`io_read(AM_TIMER_UPTIME)` 调用AM中IO接口函数`__am_timer_uptime`提供运行经过时间 ， `io_read(AM_TIMER_RTC)` 读取日历时间

以下用 ` io_read(AM_TIMER_UPTIME)` 为例，梳理从宏展开、IOE 接口，到底层时钟函数的调用链。
```c
// am-kernels/tests/am-tests/src/tests/rtc.c: 
while(io_read(AM_TIMER_UPTIME).us / 1000000 < sec) ;

// abstract-machine/klib/include/klib-macros.h
#define io_read(reg) ({ reg##_T __io_param; ioe_read(reg, &__io_param); __io_param; })
// 展开后 ({ AM_TIMER_UPTIME_T __io_param; ioe_read(AM_TIMER_UPTIME, &__io_param); __io_param; }).us

// abstract-machine/am/src/native/ioe.c:77
void ioe_read(int reg, void *buf) {do_io(reg, buf);}

static void do_io(int reg, void *buf) {  
  if (!ioe_init_done) {
    __am_ioe_init();   
  }
  ((handler_t)lut[reg])(buf);  //展开为 __am_timer_uptime(buf); 
}

// abstract-machine/am/src/native/ioe/timer.c:22
void __am_timer_uptime(AM_TIMER_UPTIME_T *uptime) {
  struct timeval now;
  gettimeofday(&now, NULL);
  long seconds = now.tv_sec - boot_time.tv_sec;  //  boot_time 是文件内保存的变量，在时钟__am_timer_init时赋值
  long useconds = now.tv_usec - boot_time.tv_usec;
  uptime->us = seconds * 1000000 + (useconds + 500);
}
```

### 按键

`make -C am-kernels/tests/am-tests ARCH=native mainargs=k run`

```c 
// am-kernels/tests/am-tests/src/tests/keyboard.c:28

typedef struct {
  bool keydown;  // true 表示按下，false 表示释放
  int keycode;   // 哪个键，例如 AM_KEY_A。
} AM_INPUT_KEYBRD_T;

if (has_kbd) {
  while (1) {
    AM_INPUT_KEYBRD_T ev = io_read(AM_INPUT_KEYBRD); // 返回一个AM_INPUT_KEYBRD_T 结构体
    if (ev.keycode == AM_KEY_NONE) break;
    printf("Got  (kbd): %s (%d) %s\n",
           names[ev.keycode], ev.keycode,
           ev.keydown ? "DOWN" : "UP");
  }
}
```

### 显示

`make -C am-kernels/tests/am-tests ARCH=native mainargs=v run`

```c 
void video_test() {
  //...
  
  while (1) {
    unsigned long upt = io_read(AM_TIMER_UPTIME).us / 1000; 
    if (upt - last > 1000 / FPS) { // 距离上次打印多久
      update();         // 重新计算画布每个像素的值 canvas[N][N] 
      redraw();         // 
      last = upt;
      fps ++;
    }

  //...
  }
}

// 向屏幕(x * w, y * h)坐标处绘制w*h的矩形图
io_write(AM_GPU_FBDRAW, x * w, y * h, color_buf, w, h, false); 
```s



TODO: 
1. video_test(), drain_keys()的调用链
2. 
