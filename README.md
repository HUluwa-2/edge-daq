# EdgeDAQ

嵌入式 Linux 数据采集系统（开发中）。目标形态：多进程 + 多线程的数据采集守护进程——子进程模拟/接入传感器数据源，主进程汇总后由线程池并行处理、加锁落盘，并通过信号实现优雅启停与日志轮转。后续将移植到 i.MX6ULL 开发板，接入真实传感器，并扩展 LCD 显示、串口与网络传输。

## 当前进度

- **Week 1**：日志守护进程 v1（文件 I/O + 进程）——守护进程化（双 fork + setsid）、fork/exec 派生工作子进程、waitpid 回收并记录退出状态、心跳日志。
- **Week 2**：守护进程 v2（信号控制）+ 线程竞态实验台——sigaction + `volatile sig_atomic_t` 标志实现 SIGTERM/SIGINT 优雅退出、SIGHUP 日志重开（配合 logrotate 式归档）；竞态复现、互斥锁修复对照与锁粒度测量、AB-BA 死锁复现与 gdb 定位。
- **Week 3（计划）**：FIFO 进程间通信 + 线程池引擎。
- **Week 4（计划）**：多进程/多线程数据采集系统总装。

## 文件

| 文件 | 说明 |
|---|---|
| `daqmond.c` | 守护进程完全体：六步守护进程化骨架、3 个工作子进程的派生与状态回收、5 秒心跳、信号驱动的优雅退出与日志重开 |
| `race.c` | 竞态实验（无锁版）：双线程各对共享计数器自增 150 万次，实测结果仅约 153 万~156 万（预期 300 万） |
| `race_fixed.c` | 竞态实验（互斥锁版）：临界区为单次读-改-写，实测稳定输出 3000000 |
| `deadlock.c` | 死锁实验：两线程以相反顺序加锁（AB-BA），配合 gdb `info threads` / `thread apply all bt` 定位 |

## 构建与运行

```bash
gcc -Wall -o daqmond daqmond.c
gcc -Wall -pthread -o race race.c
gcc -Wall -pthread -o race_fixed race_fixed.c
gcc -g -Wall -pthread -o deadlock deadlock.c

./daqmond            # 日志写入 /tmp/daqmond.log
kill -HUP  <pid>     # 日志重开（先 mv 归档旧日志再发）
kill       <pid>     # 优雅退出，日志末尾记 daqmond stopped
```
