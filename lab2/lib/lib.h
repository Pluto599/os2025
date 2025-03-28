#ifndef __lib_h__
#define __lib_h__

#define SYS_WRITE 0 // 可修改 可自定义
#define STD_OUT 0   // 可修改 可自定义

#define SYS_READ 1 // 可修改 可自定义
#define STD_IN 0   // 可修改 可自定义
#define STD_STR 1  // 可修改 可自定义

#define SYS_SET_TIME_FLAG 2
#define SYS_GET_TIME_FLAG 3
#define SYS_GET_TIME 4

#define MAX_BUFFER_SIZE 256

/* 读 I/O 端口 */
static inline char inByte(short port)
{
    char data;
    asm volatile("in %1,%0" : "=a"(data) : "d"(port));
    return data;
}

/* 写 I/O 端口 */
static inline void outByte(short port, char data)
{
    asm volatile("out %0,%1" : : "a"(data), "d"(port));
}

void printf(const char *format, ...);
void sleep(unsigned int seconds);
char getChar();
void getStr(char *str, int size);

struct TimeInfo
{
    int second;
    int minute;
    int hour;
    int m_day;
    int month;
    int year;
};

void now(struct TimeInfo *tm_info);

#endif
