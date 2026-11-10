# EdgeDAQ

嵌入式 Linux 数据采集系统。形态：多进程 + 多线程的数据采集守护进程——多个传感器进程经命名管道（FIFO）汇入主进程，主进程切行分发、线程池并行处理、加锁落盘；信号实现优雅停机与日志轮转；最终以六步骨架守护进程化为 `daqmond`。后续（12 月）将移植到 i.MX6ULL 开发板，接入真实 I2C 传感器，并扩展 LCD 显示、串口与网络传输。

## 当前进度

- **Week 1**：日志守护进程 v1（文件 I/O + 进程）——守护进程化（双 fork + setsid）、fork/exec 派生工作子进程、waitpid 回收并记录退出状态、心跳日志。
- **Week 2**：守护进程 v2（信号控制）+ 线程竞态实验台——sigaction + `volatile sig_atomic_t` 标志实现 SIGTERM/SIGINT 优雅退出、SIGHUP 日志重开（配合 logrotate 式归档）；竞态复现、互斥锁修复对照与锁粒度测量、AB-BA 死锁复现与 gdb 定位。
- **Week 3**：FIFO 进程间通信 + 线程池引擎——FIFO 单向通道与双向聊天室（镜像打开顺序、EOF 语义）；线程池：环形任务队列、一把 mutex + 一个条件变量、signal/broadcast 分工、排空式退出。
- **Week 4（总装）**：EdgeDAQ 完全体——3 个自生成传感器进程（温度/湿度/光照）共用一条 FIFO 送数；主进程阻塞读、按行切分、每行堆拷贝后投递线程池（所有权：主线程申请、工人释放）；4 工人加锁写数据文件；SIGTERM 全链优雅停机（排空不丢数据，写端经 SIGPIPE 自行退出）、SIGHUP 数据日志重开（轮转后按路径重开）；守护进程化后 PPID=1、无控制终端，轮转 + 停机全程零丢失（实测 27+9=36 与任务计数一致）。

## 文件

| 文件 | 说明 |
|---|---|
| `daq.c` | EdgeDAQ 完全体：FIFO 读端 + 切行分发 + 内嵌线程池 + 信号全链（SIGTERM 优雅停机 / SIGHUP 日志重开）+ 六步守护进程化，编译为 `daqmond` |
| `sensor.c` | 传感器进程：按名字（temp/humi/light）自生成读数、每秒一条写入同一 FIFO，种子 `srand(time^pid)` 保证多进程不撞数据 |
| `daqmond.c` | Week 1–2 的守护进程：六步骨架、3 个工作子进程的派生与状态回收、5 秒心跳、信号驱动的优雅退出与日志重开 |
| `race.c` | 竞态实验（无锁版）：双线程各对共享计数器自增 150 万次，实测结果仅约 153 万~156 万（预期 300 万） |
| `race_fixed.c` | 竞态实验（互斥锁版）：临界区为单次读-改-写，实测稳定输出 3000000 |
| `deadlock.c` | 死锁实验：两线程以相反顺序加锁（AB-BA），配合 gdb `info threads` / `thread apply all bt` 定位 |
| `binary_search.c` | 二分查找：`mid = left + (right - left) / 2` 防整型溢出 |
| `reverse_str.c` | 字符串原地反转：双指针 + 逐字交换 |
| `rotate_right.c` | 字符串右旋：三次反转法 |

（FIFO 读写端、双向聊天室 `chat.c`、线程池单文件与 `my_cp.c` 待补入。）

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

EdgeDAQ 完全体：

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
