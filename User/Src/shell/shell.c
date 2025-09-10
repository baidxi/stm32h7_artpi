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

#define SHELL_HISTORY_SIZE  10
#define SHELL_BUF_SIZE      256

struct shell_ctx {
    struct tty_device *tty;
    char prompt[16];
    char buf[SHELL_BUF_SIZE];
    int buf_offset;
    bool echo_enabled;
    char history[SHELL_HISTORY_SIZE][SHELL_BUF_SIZE];
    int history_cnt;
    int history_idx;
    int history_saved_idx;
    char temp_buf[SHELL_BUF_SIZE];
    SemaphoreHandle_t lock;
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
    ctx->history_cnt = 0;
    ctx->history_idx = 0;
    ctx->history_saved_idx = 0;

    memset(ctx->history, 0, sizeof(ctx->history));
    memset(ctx->temp_buf, 0, sizeof(ctx->temp_buf));

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
    if (!ctx->tty && ctx->echo_enabled)
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
    char c;
    ssize_t ret;

    if (!ctx->tty) {
        return -1;
    }

    ret = tty_read(ctx->tty, &c, 1);

    if (ret <= 0) {
        return -1;
    }

    return 1;
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
}

int execute_command(const char *cmd_str)
{
    char cmd_copy[SHELL_BUF_SIZE];
    char *argv[16];
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

    shell_putchar('\r');
    shell_putchar('\n');

    if (ctx->buf_offset > 0) {
        ret = execute_command(ctx->buf);
        if (ret != -2) {

        }
    }

    ctx->buf_offset = 0;
    ctx->buf[0] = '\0';

    ctx->history_idx = ctx->history_cnt;


    print_prompt();
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
            break;
        case 27:    /* ESC序列开始 */
            break;
        default:
            if (isprint(c)) {

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

void shell_run(void)
{
    main_loop();
}