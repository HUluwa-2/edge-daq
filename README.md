# EdgeDAQ

嵌入式 Linux 数据采集系统。形态：多进程 + 多线程的数据采集守护进程——多个传感器进程经命名管道（FIFO）汇入主进程，主进程切行分发、线程池并行处理、加锁落盘；信号实现优雅停机与日志轮转；最终以六步骨架守护进程化为 `daqmond`。2026 年 11 月已移植到正点原子 i.MX6ULL 开发板，接入真实传感器（ICM-20608 六轴陀螺仪内置温度 + AP3216C 环境光/接近），主进程代码零改动。后续扩展 LCD 显示与网络传输。

## 当前进度

- **Week 1**：日志守护进程 v1（文件 I/O + 进程）——守护进程化（双 fork + setsid）、fork/exec 派生工作子进程、waitpid 回收并记录退出状态、心跳日志。
- **Week 2**：守护进程 v2（信号控制）+ 线程竞态实验台——sigaction + `volatile sig_atomic_t` 标志实现 SIGTERM/SIGINT 优雅退出、SIGHUP 日志重开（配合 logrotate 式归档）；竞态复现、互斥锁修复对照与锁粒度测量、AB-BA 死锁复现与 gdb 定位。
- **Week 3**：FIFO 进程间通信 + 线程池引擎——FIFO 单向通道与双向聊天室（镜像打开顺序、EOF 语义）；线程池：环形任务队列、一把 mutex + 一个条件变量、signal/broadcast 分工、排空式退出。
- **Week 4（总装）**：EdgeDAQ 完全体——3 个自生成传感器进程（温度/湿度/光照）共用一条 FIFO 送数；主进程阻塞读、按行切分、每行堆拷贝后投递线程池（所有权：主线程申请、工人释放）；4 工人加锁写数据文件；SIGTERM 全链优雅停机（排空不丢数据，写端经 SIGPIPE 自行退出）、SIGHUP 数据日志重开（轮转后按路径重开）；守护进程化后 PPID=1、无控制终端，轮转 + 停机全程零丢失（实测 27+9=36 与任务计数一致）。
- **板级阶段（11 月 10–11 日）**：daq 零改动交叉编译上板，`sensor_board.c` 把三路数据源换成真实芯片读数（FIFO 文本行接口不变）：temp ← ICM-20608 的 read（28 字节一帧，温度为芯片结温），light/prox ← AP3216C 的 ioctl。板上验收：三路并发各 20 条，数据文件恰好 60 行、每路 seq 1~20 完整、无撕裂无丢失，EOF 后 daq 排空退出。

### 板级移植踩坑实录：出厂版 AP3216C 驱动没有 read

按教程 APP 的写法读 `/dev/ap3216c` 恒得 `-EINVAL`。排查结论（定位到正点原子官方 GitHub 的出厂内核源码 `drivers/char/ap3216c.c`，LiteOn 版驱动）：**该驱动的 file_operations 根本没有实现 `.read`**，只有空壳 open/release 和 `unlocked_ioctl`——内核在驱动代码运行前就直接返回 EINVAL，与应用代码无关。出厂系统里读这颗芯片的正门是 ioctl（`AP3216C_GET_ALS = 0x80024401`、`AP3216C_GET_PS = 0x80024402`）或 sysfs（`/sys/class/misc/ap3216c/{als,ps,ir}`）；芯片在 probe 时已复位使能，用户态无需初始化。数值口径：ALS 是 16 位原始计数而非 lux，PS 量程 0~1023，无效时 -1。ICM-20608 的出厂驱动即教程第 62 章代码以模块形式加载：一次 read 返回 `signed int[7]`，**返回 0 为成功**，温度换算 `temp = (raw − 25) / 326.8 + 25`。

## 文件

| 文件 | 说明 |
|---|---|
| `daq.c` | EdgeDAQ 完全体：FIFO 读端 + 切行分发 + 内嵌线程池 + 信号全链（SIGTERM 优雅停机 / SIGHUP 日志重开）+ 六步守护进程化，编译为 `daqmond` |
| `sensor.c` | 传感器进程（模拟版）：按名字（temp/humi/light）自生成读数、每秒一条写入同一 FIFO，种子 `srand(time^pid)` 保证多进程不撞数据 |
| `sensor_board.c` | 传感器进程（板级真实版）：骨架同 `sensor.c`，temp 走 ICM-20608 read、light/prox 走 AP3216C ioctl；读失败写 `value=-1` 占位保 seq 对齐；交叉编译后在板上运行 |
| `ap3216_ioctl.c` | AP3216C 探针：ioctl 三命令字（IR/ALS/PS）读取验证，先打印命令字自检宏展开 |
| `icm20608_read.c` | ICM-20608 探针：read 一帧 7 个原始值并换算温度，以物理自洽验收（静止时一轴 ≈1g、陀螺近零、结温略高于室温，翻面时 1g 换轴变号） |
| `daqmond.c` | Week 1–2 的守护进程：六步骨架、3 个工作子进程的派生与状态回收、5 秒心跳、信号驱动的优雅退出与日志重开 |
| `runner.c` | 工作子进程的 exec 目标（由 daqmond 派生执行） |
| `fork.c` / `exec.c` | 进程基础实验：fork 语义与 exec 族替换 |
| `race.c` | 竞态实验（无锁版）：双线程各对共享计数器自增 150 万次，实测结果仅约 153 万~156 万（预期 300 万） |
| `race_fixed.c` | 竞态实验（互斥锁版）：临界区为单次读-改-写，实测稳定输出 3000000 |
| `deadlock.c` | 死锁实验：两线程以相反顺序加锁（AB-BA），配合 gdb `info threads` / `thread apply all bt` 定位 |
| `reader.c` / `write.c` | FIFO 单向通道读写端 |
| `chat.c` | FIFO 双向聊天室：两条 FIFO 镜像打开顺序、双线程收发、`!=0` 校验 pthread_create 返回值 |
| `feeder.c` | 固定节奏向 FIFO 送数的喂料进程（总装调试期使用） |
| `thread_pool.c` | 线程池单文件：环形任务队列、一把 mutex + 一个条件变量、拷贝-解锁-执行、排空式退出（20 任务 × 8 工人恰好各执行一次） |
| `binary_search.c` | 二分查找：`mid = left + (right - left) / 2` 防整型溢出 |
| `reverse_str.c` | 字符串原地反转：双指针 + 逐字交换 |
| `rotate_right.c` | 字符串右旋：三次反转法 |

（`my_cp.c` 原稿遗失，待按已验证规格重建后补入。）

## 构建与运行

Week 1–2 守护进程与实验台：

```bash
gcc -Wall -o daqmond_v1 daqmond.c
gcc -Wall -pthread -o race race.c
gcc -Wall -pthread -o race_fixed race_fixed.c
gcc -g -Wall -pthread -o deadlock deadlock.c

./daqmond_v1         # 日志写入 /tmp/daqmond.log
kill -HUP  <pid>     # 日志重开（先 mv 归档旧日志再发）
kill       <pid>     # 优雅退出，日志末尾记 daqmond stopped
```

EdgeDAQ 完全体（Ubuntu 模拟版）：

```bash
gcc -Wall -pthread -o daqmond daq.c
gcc -Wall -o sensor sensor.c

./daqmond                     # 守护进程，运行日志在 /tmp/daqmond.log
./sensor temp 10              # 温度传感器，发 10 条自退（humi / light 同理，可多路同开）
tail -f /tmp/edgedaq_data.log # 数据文件：带时间戳的采集记录

mv /tmp/edgedaq_data.log /tmp/edgedaq_data.log.1
pkill -HUP  -x daqmond        # 数据日志按原路径重开
pkill -TERM -x daqmond        # 全链优雅停机：排空落盘后退出，日志记总数
```

板级版（i.MX6ULL，出厂工具链较老，需显式 `-std=gnu99`）：

```bash
arm-linux-gnueabihf-gcc -std=gnu99 -Wall -pthread -o daq daq.c
arm-linux-gnueabihf-gcc -std=gnu99 -Wall -o sensor sensor_board.c

./daq &                       # 板上先以前台形态观测（daq.c 代码不变）
./sensor temp 20 &            # ICM-20608 结温
./sensor light 20 &           # AP3216C 环境光（原始计数）
./sensor prox 20 &            # AP3216C 接近（0~1023，随手距变化）
wc -l /tmp/edgedaq_data.log   # 期望 60，三路各 20
```
