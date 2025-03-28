#include "x86.h"
#include "device.h"

// TODO5:
// 实现 irqHandle() 函数，处理各种中断
// 完善 KeyboardHandle() 函数处理键盘输入
// 实现 timerHandler() 函数处理时钟中断
// 完成 sysPrint() 函数实现屏幕输出

extern int displayRow;
extern int displayCol;

extern uint32_t keyBuffer[MAX_KEYBUFFER_SIZE];
extern int bufferHead;
extern int bufferTail;

int tail = 0;
int lineBufferReady = 0;

int timeFlag = 0;

void GProtectFaultHandle(struct TrapFrame *tf);

void KeyboardHandle(struct TrapFrame *tf);

void timerHandler(struct TrapFrame *tf);
void syscallHandle(struct TrapFrame *tf);
void sysWrite(struct TrapFrame *tf);
void sysPrint(struct TrapFrame *tf);
void sysRead(struct TrapFrame *tf);
void sysGetChar(struct TrapFrame *tf);
void sysGetStr(struct TrapFrame *tf);
void sysSetTimeFlag(struct TrapFrame *tf);
void sysGetTimeFlag(struct TrapFrame *tf);

void sysGetTime(struct TrapFrame *tf);

void irqHandle(struct TrapFrame *tf)
{ // pointer tf = esp
    /*
     * 中断处理程序
     */
    /* Reassign segment register */

    asm volatile("movw %%ax, %%ds" ::"a"(KSEL(SEG_KDATA)));

    switch (tf->irq)
    {
        // TODO: 填好中断处理程序的调用
    case -1:
        break;

    case 0x8:
    case 0xa:
    case 0xb:
    case 0xc:
    case 0xd:
    case 0xe:
    case 0x11:
    case 0x1e:
        GProtectFaultHandle(tf);
        break;

    case 0x20:
        timerHandler(tf);
        break;

    case 0x21:
        KeyboardHandle(tf);
        break;

    case 0x80:
        syscallHandle(tf);
        break;

    default:
        assert(0);
    }
}

void GProtectFaultHandle(struct TrapFrame *tf)
{
    assert(0);
    return;
}

void KeyboardHandle(struct TrapFrame *tf)
{
    // 首次使用前初始化
    static int initialized = 0;
    if (!initialized)
    {
        lineBufferReady = 0;
        initialized = 1;
    }

    uint32_t code = getKeyCode();

    // 忽略断码和扩展码
    if (code == 0 || code == 0x0 || code > 0x80)
    {
        return;
    }
    // 忽略 Shift 和 Capslock 等修饰键
    if (code == 0x2a || code == 0x36 || code == (0x2a + 0x80) || code == (0x36 + 0x80))
    {
        return;
    }

    if (code == 0xe)
    { // 退格符
        // 要求只能退格用户键盘输入的字符串，且最多退到当行行首
        if (displayCol > 0 && displayCol > tail)
        {
            displayCol--;
            uint16_t data = 0 | (0x0c << 8);
            int pos = (80 * displayRow + displayCol) * 2;
            asm volatile("movw %0, (%1)" ::"r"(data), "r"(pos + 0xb8000));

            if (bufferTail > 0)
            {
                bufferTail = (bufferTail - 1) % MAX_KEYBUFFER_SIZE;
            }
        }
    }
    else if (code == 0x1c)
    { // 回车符
        // 处理回车情况
        keyBuffer[bufferTail] = '\n';
        bufferTail = (bufferTail + 1) % MAX_KEYBUFFER_SIZE;

        // 行缓冲区就绪
        lineBufferReady = 1;

        displayRow++;
        displayCol = 0;
        tail = 0;
        if (displayRow == 25)
        {
            scrollScreen();
            displayRow = 24;
        }
    }
    else if (code < 0x81)
    {
        // 处理正常的字符
        char ch = getChar(code);
        // 过滤掉控制字符和非打印字符
        if (ch != 0)
        {
            keyBuffer[bufferTail] = ch;
            bufferTail = (bufferTail + 1) % MAX_KEYBUFFER_SIZE;

            uint16_t data = ch | (0x0c << 8);
            int pos = (80 * displayRow + displayCol) * 2;
            asm volatile("movw %0, (%1)" ::"r"(data), "r"(pos + 0xb8000));

            displayCol++;
            if (displayCol == 80)
            {
                displayCol = 0;
                tail = 0;
                displayRow++;

                if (displayRow == 25)
                {
                    scrollScreen();
                    displayRow = 24;
                }
            }
        }
    }
    updateCursor(displayRow, displayCol);
}

void timerHandler(struct TrapFrame *tf)
{
    // TODO
    timeFlag = 1;
}

void syscallHandle(struct TrapFrame *tf)
{

    switch (tf->eax)
    { // syscall number
    case 0:
        sysWrite(tf);
        break; // for SYS_WRITE
    case 1:
        sysRead(tf);
        break; // for SYS_READ
    case 2:
        sysSetTimeFlag(tf);
        break; // for SYS_SET_TIME_FLAG
    case 3:
        sysGetTimeFlag(tf);
        break; // for SYS_GET_TIME_FLAG
    case 4:
        sysGetTime(tf);
        break; // for SYS_GET_TIME
    default:
        break;
    }
}

void sysWrite(struct TrapFrame *tf)
{

    switch (tf->ecx)
    { // file descriptor
    case 0:
        sysPrint(tf);
        break; // for STD_OUT
    default:
        break;
    }
}

void sysPrint(struct TrapFrame *tf)
{
    int sel = USEL(SEG_UDATA);
    char *str = (char *)tf->edx;
    int size = tf->ebx;
    int i = 0;
    int pos = 0;
    char character = 0;
    uint16_t data = 0;
    asm volatile("movw %0, %%es" ::"m"(sel));
    for (i = 0; i < size; i++)
    {
        asm volatile("movb %%es:(%1), %0" : "=r"(character) : "r"(str + i));
        // TODO: 完成光标的维护和打印到显存
        if (character == '\n')
        {
            displayCol = 0;
            displayRow++;

            if (displayRow == 25)
            {
                scrollScreen();
                displayRow = 24;
            }
        }
        else
        {
            data = character | (0x0c << 8);
            pos = (80 * displayRow + displayCol) * 2;
            asm volatile("movw %0, (%1)" ::"r"(data), "r"(pos + 0xb8000));

            displayCol++;
            if (displayCol == 80)
            {
                displayCol = 0;
                displayRow++;

                if (displayRow == 25)
                {
                    scrollScreen();
                    displayRow = 24;
                }
            }
        }
    }
    tail = displayCol;
    updateCursor(displayRow, displayCol);
}

void sysRead(struct TrapFrame *tf)
{
    switch (tf->ecx)
    { // file descriptor
    case 0:
        sysGetChar(tf);
        break; // for STD_IN
    case 1:
        sysGetStr(tf);
        break; // for STD_STR
    default:
        break;
    }
}

// TODO8: finished
// 实现 sysGetChar() 和 sysGetStr() 函数，支持系统调用接口
// 实现 sysGetTimeFlag() 和 sysSetTimeFlag() 函数，支持时间相关系统调用
void sysGetChar(struct TrapFrame *tf)
{
    // TODO: 自由实现

    char ch = 0;
    enableInterrupt();

    while (1)
    {
        if (bufferHead != bufferTail)
        {
            if (lineBufferReady == 1)
            {
                break;
            }
        }

        asm volatile("hlt");
    }

    disableInterrupt();

    ch = keyBuffer[bufferHead];
    bufferHead = (bufferHead + 1) % MAX_KEYBUFFER_SIZE;

    if (ch == '\n' || bufferHead == bufferTail)
    {
        lineBufferReady = 0;
    }

    tf->eax = ch;
}

void sysGetStr(struct TrapFrame *tf)
{
    // TODO: 自由实现

    int sel = USEL(SEG_UDATA);
    char *str = (char *)tf->edx;
    int size = tf->ebx;
    int count = 0;
    int skip = 1;

    asm volatile("movw %0, %%es" ::"m"(sel));

    enableInterrupt();

    while (count < size - 1)
    {
        while (bufferHead == bufferTail || !lineBufferReady)
        {
            asm volatile("hlt");
        }

        char ch = keyBuffer[bufferHead];

        if (skip) // 跳过缓冲区的空白符
        {
            if (ch == ' ' || ch == '\t' || ch == '\n')
            {
                bufferHead = (bufferHead + 1) % MAX_KEYBUFFER_SIZE;

                if (ch == '\n')
                {
                    lineBufferReady = 0;
                }

                continue;
            }
            else
            {
                skip = 0;
            }
        }

        bufferHead = (bufferHead + 1) % MAX_KEYBUFFER_SIZE;

        if (ch == ' ' || ch == '\t' || ch == '\n') // 结束读取
        {
            if (ch == '\n')
            {
                lineBufferReady = 0;
            }
            break;
        }

        asm volatile("movb %0, %%es:(%1)" ::"b"(ch), "r"(str + count));
        count++;
    }

    disableInterrupt();

    asm volatile("movb %0, %%es:(%1)" ::"b"('\0'), "r"(str + count));

    if (bufferHead == bufferTail)
    {
        lineBufferReady = 0;
    }

    tf->eax = count;
}

void sysGetTimeFlag(struct TrapFrame *tf)
{
    // TODO: 自由实现
    tf->eax = timeFlag;
}

void sysSetTimeFlag(struct TrapFrame *tf)
{
    // TODO: 自由实现
    timeFlag = 0;
}

struct TimeInfo
{
    int second;
    int minute;
    int hour;
    int m_day;
    int month;
    int year;
};

void sysGetTime(struct TrapFrame *tf)
{
    int sel = USEL(SEG_UDATA);

    struct TimeInfo *tm_info = (struct TimeInfo *)tf->ecx;
    asm volatile("movw %0, %%es" ::"m"(sel));

    disableInterrupt();

    outByte(0x70, 0x00);
    uint8_t second = inByte(0x71);

    outByte(0x70, 0x02);
    uint8_t minute = inByte(0x71);

    outByte(0x70, 0x04);
    uint8_t hour = inByte(0x71);

    outByte(0x70, 0x07);
    uint8_t m_day = inByte(0x71);

    outByte(0x70, 0x08);
    uint8_t month = inByte(0x71);

    outByte(0x70, 0x09);
    uint8_t year = inByte(0x71);

    outByte(0x70, 0x0B);
    uint8_t statusB = inByte(0x71);

    // 如果是 BCD 格式，转换为二进制格式
    if (!(statusB & 0x04))
    {
        second = (second & 0x0F) + ((second / 16) * 10);
        minute = (minute & 0x0F) + ((minute / 16) * 10);
        hour = (hour & 0x0F) + ((hour / 16) * 10);
        m_day = (m_day & 0x0F) + ((m_day / 16) * 10);
        month = (month & 0x0F) + ((month / 16) * 10);
        year = (year & 0x0F) + ((year / 16) * 10);
    }

    int sec = (int)second;
    int min = (int)minute;
    int hr = (int)hour;
    int day = (int)m_day;
    int mon = (int)month;
    int yr = (int)year + 2000;

    asm volatile("movl %0, %%es:(%1)" : : "r"(sec), "r"(&tm_info->second));
    asm volatile("movl %0, %%es:(%1)" : : "r"(min), "r"(&tm_info->minute));
    asm volatile("movl %0, %%es:(%1)" : : "r"(hr), "r"(&tm_info->hour));
    asm volatile("movl %0, %%es:(%1)" : : "r"(day), "r"(&tm_info->m_day));
    asm volatile("movl %0, %%es:(%1)" : : "r"(mon), "r"(&tm_info->month));
    asm volatile("movl %0, %%es:(%1)" : : "r"(yr), "r"(&tm_info->year));

    enableInterrupt();
}