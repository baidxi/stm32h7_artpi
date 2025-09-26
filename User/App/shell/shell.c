#include <device/tty/tty.h>

#include <shell.h>

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>

#define SHELL_HISTORY_SIZE  8
#define SHELL_BUF_SIZE      256

struct shell_command_info_t {
    uint8_t len;
    char name[SHELL_BUF_SIZE];
};

struct history_t {
    uint16_t current_idx;
    uint16_t saved;
    uint16_t mask;
    uint16_t browsing;  /* 标记是否正在浏览历史记录 */
    struct shell_command_info_t info[SHELL_HISTORY_SIZE];
    char temp[SHELL_BUF_SIZE];  /* 保存当前编辑的命令 */
};
struct shell_ctx {
    struct tty_device *tty;
    char prompt[16];
    char *buf;
    int buf_offset;
    bool echo_enabled;
    SemaphoreHandle_t lock;
    struct history_t *history;
    char edit_buf[SHELL_BUF_SIZE];  /* 编辑缓冲区，避免直接修改历史记录 */
};

static struct shell_ctx *ctx;

static void print_prompt(void)
{
    shell_puts(ctx->prompt);
}

int shell_init(const char *tty_name, const char *prompt)
{
    struct tty_device *tty = tty_device_lookup_by_name(tty_name);

    if (!tty)
        return -ENODEV;

    ctx = pvPortMalloc(sizeof(*ctx));
    if (!ctx)
        return -ENOMEM;

    memset(ctx, 0, sizeof(*ctx));

    if (!prompt) {
        strlcpy(ctx->prompt, "shell> ", 7);
    } else {
        strlcpy(ctx->prompt, prompt, sizeof(ctx->prompt) - 1);
    }

    ctx->lock = xSemaphoreCreateMutex();
    if (!ctx->lock) {
        return -1;
    }

    ctx->tty = tty;

    if (tty_open(tty)) {
        ctx->tty = NULL;
        return -1;
    }

    ctx->echo_enabled = true;
    ctx->buf_offset = 0;
    ctx->history = pvPortMalloc(sizeof(*ctx->history));
    if(!ctx->history) {
        vPortFree(ctx);
        return -1;
    }

    memset(ctx->history, 0, sizeof(*ctx->history));

    ctx->history->mask = SHELL_HISTORY_SIZE - 1;
    ctx->history->browsing = 0;
    ctx->buf = ctx->edit_buf;
    ctx->edit_buf[0] = '\0';

    shell_puts("\r\n");
    shell_puts("STM32 Shell v1.1\r\n");
    shell_puts("Type 'help' for available commands\r\n");
    shell_puts("\r\n");

    print_prompt();

    return 0;
}

int shell_puts(const char *str)
{
    if (ctx && ctx->tty && ctx->echo_enabled)
        return tty_write(ctx->tty, str, strlen(str));
    return -ENODEV;
}

static void shell_putchar(char c)
{
    if (ctx->tty && ctx->echo_enabled)
        tty_write(ctx->tty, &c, 1);
}

int shell_printf(const char *fmt, ...)
{
    va_list args;
    char buf[SHELL_BUF_SIZE];
    size_t len;

    va_start(args, fmt);
    len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    buf[SHELL_BUF_SIZE -1] = '\0';

    if (ctx && ctx->tty && ctx->echo_enabled)
        return tty_write(ctx->tty, buf, len);

    return -ENODEV;
}

static int shell_getchar(void)
{
    char c = 0;
    ssize_t ret;

    if (!ctx->tty) {
        return -1;
    }

    ret = tty_read(ctx->tty, &c, 1);

    if (ret < 0) {
        return -1;
    }

    return c;
}

static void shell_clean_all()
{
    shell_puts("\r");
    shell_puts("\033[K");
}

int parse_command(char *cmd_str, char *argv[], int max_args)
{
    int argc = 0;
    char *p = cmd_str;
    bool in_quote = false;

    while(*p && argc < max_args) {
        while(*p && isspace((unsigned char)*p) && !in_quote) {
            *p++ = '\0';
        }

        if (*p == '\0')
            break;

        if (*p == '"') {
            in_quote = !in_quote;
            p++;
            continue;
        }

        argv[argc++] = p;

        while(*p && (!isspace((unsigned char)*p) || in_quote)) {
            if (*p == '"') {
                in_quote = !in_quote;
            }
            p++;
        }
    }
    return argc;
}

struct shell_command *find_command(const char *name)
{
    const struct shell_command *cmd;
    size_t i;

    if (!name)
        return NULL;

    if (xSemaphoreTake(ctx->lock, portMAX_DELAY)) {
        for (i = 0; i < SHELL_CMD_COUNT; i++) {
            cmd = &SHELL_CMD_LIST_START[i];
            if (strcmp(cmd->name, name) == 0) {
                xSemaphoreGive(ctx->lock);
                return (struct shell_command *)cmd;
            }
        }
    }
    xSemaphoreGive(ctx->lock);
    return NULL;
}

int execute_command(const char *cmd_str)
{
    char cmd_copy[SHELL_BUF_SIZE] = {0};
    char *argv[16] = {0};
    int argc;
    struct shell_command *cmd;

    if (!cmd_str || !*cmd_str) {
        return 0;
    }

    strncpy(cmd_copy, cmd_str, sizeof(cmd_copy) - 1);
    cmd_copy[sizeof(cmd_copy) - 1] = '\0';

    argc = parse_command(cmd_copy, argv, sizeof(argv));
    if (argc == 0) {
        return 0;
    }

    cmd = find_command(argv[0]);
    if (!cmd) {
        shell_puts("Command not found: ");
        shell_puts(argv[0]);
        shell_puts("\r\n");
        return -2;
    }

    return cmd->func(argc, argv);
}

static void handle_enter(void)
{
    int ret;
    struct shell_command_info_t *info;

    shell_puts("\r\n");

    if (ctx->buf_offset > 0) {
        ret = execute_command(ctx->buf);
        if (ret == 0)
        {
            /* 保存命令到历史记录 */
            info = &ctx->history->info[ctx->history->saved & ctx->history->mask];
            strncpy(info->name, ctx->buf, SHELL_BUF_SIZE - 1);
            info->name[SHELL_BUF_SIZE - 1] = '\0';
            info->len = ctx->buf_offset;
            ctx->history->saved++;
            
            /* 确保历史记录不超出缓冲区大小 */
            if (ctx->history->saved > SHELL_HISTORY_SIZE) {
                ctx->history->saved = SHELL_HISTORY_SIZE;
            }
        }
    }

    /* 重置缓冲区和历史记录浏览状态 */
    ctx->history->browsing = 0;
    ctx->history->current_idx = 0;
    ctx->history->temp[0] = '\0';
    ctx->buf = ctx->edit_buf;
    ctx->buf_offset = 0;
    ctx->edit_buf[0] = '\0';

    print_prompt();
}

static void handle_char(char c)
{
    /* 如果正在浏览历史记录，复制当前历史命令到编辑缓冲区并退出浏览模式 */
    if (ctx->history->browsing) {
        /* 将当前显示的历史命令复制到编辑缓冲区，以便用户可以编辑它 */
        strncpy(ctx->edit_buf, ctx->buf, SHELL_BUF_SIZE - 1);
        ctx->edit_buf[SHELL_BUF_SIZE - 1] = '\0';
        ctx->buf_offset = strlen(ctx->edit_buf);
        ctx->buf = ctx->edit_buf;
        ctx->history->browsing = 0;
        ctx->history->temp[0] = '\0';
    }
    
    if (ctx->buf_offset < SHELL_BUF_SIZE -1) {
        ctx->buf[ctx->buf_offset++] = c;
        ctx->buf[ctx->buf_offset] = '\0';
        shell_putchar(c);
    }
}

static void handle_backspace()
{
    /* 如果正在浏览历史记录，复制当前历史命令到编辑缓冲区并退出浏览模式 */
    if (ctx->history->browsing) {
        /* 将当前显示的历史命令复制到编辑缓冲区，以便用户可以编辑它 */
        strncpy(ctx->edit_buf, ctx->buf, SHELL_BUF_SIZE - 1);
        ctx->edit_buf[SHELL_BUF_SIZE - 1] = '\0';
        ctx->buf_offset = strlen(ctx->edit_buf);
        ctx->buf = ctx->edit_buf;
        ctx->history->browsing = 0;
        ctx->history->temp[0] = '\0';
        
        /* 然后执行正常的退格操作 */
        if (ctx->buf_offset > 0) {
            ctx->buf_offset--;
            ctx->buf[ctx->buf_offset] = '\0';
            shell_clean_all();
            print_prompt();
            shell_puts(ctx->buf);
        }
        return;
    }
    
    if (ctx->buf_offset > 0) {
        ctx->buf_offset--;
        ctx->buf[ctx->buf_offset] = '\0';
        shell_puts("\b \b");
    }
}

static void handle_arrow_key(char key)
{
    struct shell_command_info_t *info;
    
    switch(key) {
        case 'A': /* 向上箭头 - 显示更早的命令 */
            if (ctx->history->saved > 0) {
                /* 如果是第一次按向上箭头，保存当前正在编辑的命令 */
                if (!ctx->history->browsing) {
                    strncpy(ctx->history->temp, ctx->buf, SHELL_BUF_SIZE - 1);
                    ctx->history->temp[SHELL_BUF_SIZE - 1] = '\0';
                    ctx->history->browsing = 1;
                    /* 初始化 current_idx 为最新命令的索引 */
                    ctx->history->current_idx = ctx->history->saved - 1;
                } else {
                    /* 已经在浏览历史记录，移动到更早的命令 */
                    if (ctx->history->current_idx > 0) {
                        ctx->history->current_idx--;
                    }
                }
                
                /* 显示选中的历史命令 */
                info = &ctx->history->info[ctx->history->current_idx & ctx->history->mask];
                /* 确保只显示有效的历史记录 */
                if (info->len > 0) {
                    /* 将历史命令复制到编辑缓冲区，而不是直接指向历史记录 */
                    strncpy(ctx->edit_buf, info->name, SHELL_BUF_SIZE - 1);
                    ctx->edit_buf[SHELL_BUF_SIZE - 1] = '\0';
                    ctx->buf = ctx->edit_buf;
                    ctx->buf_offset = info->len;
                    shell_clean_all();
                    print_prompt();
                    shell_puts(ctx->buf);
                }
            }
            break;
            
        case 'B': /* 向下箭头 - 显示更晚的命令 */
            if (ctx->history->browsing) {
                if (ctx->history->current_idx < ctx->history->saved - 1) {
                    /* 还有更晚的历史命令，显示它 */
                    ctx->history->current_idx++;
                    info = &ctx->history->info[ctx->history->current_idx & ctx->history->mask];
                    /* 确保只显示有效的历史记录 */
                    if (info->len > 0) {
                        /* 将历史命令复制到编辑缓冲区，而不是直接指向历史记录 */
                        strncpy(ctx->edit_buf, info->name, SHELL_BUF_SIZE - 1);
                        ctx->edit_buf[SHELL_BUF_SIZE - 1] = '\0';
                        ctx->buf = ctx->edit_buf;
                        ctx->buf_offset = info->len;
                    }
                } else {
                    /* 已经到达最新命令，恢复到用户之前编辑的命令 */
                    ctx->history->browsing = 0;
                    /* 将临时缓冲区的内容复制到编辑缓冲区 */
                    strncpy(ctx->edit_buf, ctx->history->temp, SHELL_BUF_SIZE - 1);
                    ctx->edit_buf[SHELL_BUF_SIZE - 1] = '\0';
                    ctx->buf = ctx->edit_buf;
                    ctx->buf_offset = strlen(ctx->edit_buf);
                }
                
                shell_clean_all();
                print_prompt();
                shell_puts(ctx->buf);
            }
            break;
            
        case 'C': /* 右箭头 - 暂不实现 */
            break;
            
        case 'D': /* 左箭头 - 暂不实现 */
            break;
    }
}

static void handle_esc_seq()
{
    char c = -1;

    switch(shell_getchar()) {
        case '[':
            switch((c = shell_getchar())) {
                case 'A':
                case 'B':
                case 'C':
                case 'D':
                    handle_arrow_key(c);
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
}

static void handle_special(char c)
{
    switch(c) {
        case '\r':
        case '\n':
            handle_enter();
            break;
        case '\b':
        case 127:   /* DEL */
            handle_backspace();
            break;
        case 0x1b:    /* ESC序列开始 */
            handle_esc_seq();
            break;
        default:
            if (isprint(c)) {
                handle_char(c);
            }
            break;

    }
}

static void main_loop(void)
{
    int c;

    while(1) {
        c = shell_getchar();
        if (c >= 0) {
            handle_special(c);
        }

        taskYIELD();
    }
}

int shell_show_available_cmd()
{
    const struct shell_command *cmd;
    int i;

    shell_puts("available commands\r\n");

    if (xSemaphoreTake(ctx->lock, portMAX_DELAY)) {
        for (i = 0; i < SHELL_CMD_COUNT; i++) {
            cmd = &SHELL_CMD_LIST_START[i];
            shell_printf("  %s - %s\r\n", cmd->name, cmd->help_str);
        }
        xSemaphoreGive(ctx->lock);
    }
    return 0;
}

int shell_show_cmd_help(const char *name, int argc, char *argv[])
{
    struct shell_command *cmd = find_command(name);
    if (cmd) {
        if (cmd->help_fn)
            cmd->help_fn(argc, argv);
    }
    return 0;
}

void shell_run(void)
{
    main_loop();
}

int _read(int file, char *ptr, int len)
{
    (void)file;
    if (ctx && ctx->tty)
        return tty_read(ctx->tty, ptr, len);
    return -ENODEV;
}

int _write(int file, char *ptr, int len)
{
    (void)file;
    if (ctx && ctx->tty)
        return tty_write(ctx->tty, ptr, len);
    return -ENODEV;
}
