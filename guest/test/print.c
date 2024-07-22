/*
 * libc printf and friends
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Library General Public License version 2.
 */

#define BUFSZ 2000

#include <stdarg.h>
#include "aj_types.h"
#include "aj_string.h"
#include "io.h"
#include "spinlock.h"

#define BINSTR_SZ (sizeof(uint32_t) * 8 + sizeof(uint32_t) * 2)

#define BUFSZ 2000

spinlock_t lock;

void io_init() 
{
	spinlock_init(&lock);
}

void uart_putstr(const char *str);
#define puts uart_putstr

void uart_putchar(char c)
{
    volatile unsigned int *const UART0DR = (unsigned int *)0x9000000;
	spin_lock(&lock);
    *UART0DR = (unsigned int)c;
	spin_unlock(&lock);
}

void uart_putstr(const char *str)
{
    while (*str)
    {
        uart_putchar(*str++);
    }
}

void print_info(const char *info)
{
    // ANSI 转义序列: "\033[32m" 设置绿色前景色, "\033[0m" 重置颜色
    printf("\033[32m%s\033[0m", info);
}

void print_warn(const char *info)
{
    // ANSI 转义序列: "\033[32m" 设置绿色前景色, "\033[0m" 重置颜色
    printf("\033[33m%s\033[0m", info);
}

static char digits[16] = "0123456789abcdef";

typedef struct pstream
{
	char *buffer;
	int remain;
	int added;
} pstream_t;

typedef struct strprops
{
	char pad;
	int npad;
	bool alternate;
} strprops_t;

static void addchar(pstream_t *p, char c)
{
	if (p->remain)
	{
		*p->buffer++ = c;
		--p->remain;
	}
	++p->added;
}

static void print_str(pstream_t *p, const char *s, strprops_t props)
{
	const char *s_orig = s;
	int npad = props.npad;

	if (npad > 0)
	{
		npad -= strlen(s_orig);
		while (npad > 0)
		{
			addchar(p, props.pad);
			--npad;
		}
	}

	while (*s)
		addchar(p, *s++);

	if (npad < 0)
	{
		props.pad = ' '; /* ignore '0' flag with '-' flag */
		npad += strlen(s_orig);
		while (npad < 0)
		{
			addchar(p, props.pad);
			++npad;
		}
	}
}

static void print_int(pstream_t *ps, long n, int base, strprops_t props)
{

	char buf[sizeof(long) * 3 + 2], *p = buf;
	int s = 0, i;

	if (n < 0)
	{
		n = -n;
		s = 1;
	}

	while (n)
	{
		*p++ = digits[n % base];
		n /= base;
	}

	if (s)
		*p++ = '-';

	if (p == buf)
		*p++ = '0';

	for (i = 0; i < (p - buf) / 2; ++i)
	{
		char tmp;

		tmp = buf[i];
		buf[i] = p[-1 - i];
		p[-1 - i] = tmp;
	}

	*p = 0;

	print_str(ps, buf, props);
}

static void print_unsigned(pstream_t *ps, uint32_t n, int base,
						   strprops_t props)
{
	char buf[sizeof(long) * 3 + 3], *p = buf;
	int i;

	while (n)
	{
		*p++ = digits[n % base];
		n /= base;
	}

	if (p == buf)
		*p++ = '0';
	else if (props.alternate && base == 16)
	{
		if (props.pad == '0')
		{
			addchar(ps, '0');
			addchar(ps, 'x');

			if (props.npad > 0)
				props.npad = MAX(props.npad - 2, 0);
		}
		else
		{
			*p++ = 'x';
			*p++ = '0';
		}
	}

	for (i = 0; i < (p - buf) / 2; ++i)
	{
		char tmp;

		tmp = buf[i];
		buf[i] = p[-1 - i];
		p[-1 - i] = tmp;
	}

	*p = 0;

	print_str(ps, buf, props);
}

static int fmtnum(const char **fmt)
{
	const char *f = *fmt;
	int len = 0, num;

	if (*f == '-')
		++f, ++len;

	while (*f >= '0' && *f <= '9')
		++f, ++len;

	num = atol(*fmt);
	*fmt += len;
	return num;
}

int vsnprintf(char *buf, int size, const char *fmt, va_list va)
{
	pstream_t s;

	s.buffer = buf;
	s.remain = size - 1;
	s.added = 0;
	while (*fmt)
	{
		char f = *fmt++;
		int nlong = 0;
		strprops_t props;
		memset(&props, 0, sizeof(props));
		props.pad = ' ';

		if (f != '%')
		{
			addchar(&s, f);
			continue;
		}
	morefmt:
		f = *fmt++;
		switch (f)
		{
		case '%':
			addchar(&s, '%');
			break;
		case 'c':
			addchar(&s, va_arg(va, int));
			break;
		case '\0':
			--fmt;
			break;
		case '#':
			props.alternate = true;
			goto morefmt;
		case '0':
			props.pad = '0';
			++fmt;
			/* fall through */
		case '1' ... '9':
		case '-':
			--fmt;
			props.npad = fmtnum(&fmt);
			goto morefmt;
		case 'l':
			++nlong;
			goto morefmt;
		case 't':
		case 'z':
			/* Here we only care that sizeof(size_t) == sizeof(long).
			 * On a 32-bit platform it doesn't matter that size_t is
			 * typedef'ed to int or long; va_arg will work either way.
			 * Same for ptrdiff_t (%td).
			 */
			nlong = 1;
			goto morefmt;
		case 'd':
			switch (nlong)
			{
			case 0:
				print_int(&s, va_arg(va, int), 10, props);
				break;
			case 1:
				print_int(&s, va_arg(va, long), 10, props);
				break;
			default:
				print_int(&s, va_arg(va, long long), 10, props);
				break;
			}
			break;
		case 'u':
			switch (nlong)
			{
			case 0:
				print_unsigned(&s, va_arg(va, unsigned), 10, props);
				break;
			case 1:
				print_unsigned(&s, va_arg(va, uint32_t), 10, props);
				break;
			default:
				print_unsigned(&s, va_arg(va, uint32_t), 10, props);
				break;
			}
			break;
		case 'x':
			switch (nlong)
			{
			case 0:
				print_unsigned(&s, va_arg(va, unsigned), 16, props);
				break;
			case 1:
				print_unsigned(&s, va_arg(va, uint32_t), 16, props);
				break;
			default:
				print_unsigned(&s, va_arg(va, uint32_t), 16, props);
				break;
			}
			break;
		case 'p':
			props.alternate = true;
			print_unsigned(&s, *(uint32_t *)va_arg(va, void *), 16, props);
			break;
		case 's':
			print_str(&s, va_arg(va, const char *), props);
			break;
		default:
			addchar(&s, f);
			break;
		}
	}
	*s.buffer = 0;
	return s.added;
}

int snprintf(char *buf, int size, const char *fmt, ...)
{
	va_list va;
	int r;

	va_start(va, fmt);
	r = vsnprintf(buf, size, fmt, va);
	va_end(va);
	return r;
}

int vprintf(const char *fmt, va_list va)
{
	char buf[BUFSZ];
	int r;

	r = vsnprintf(buf, sizeof(buf), fmt, va);
	puts(buf);
	return r;
}

int printf(const char *fmt, ...)
{
	va_list va;
	char buf[BUFSZ];
	int r;

	va_start(va, fmt);
	r = vsnprintf(buf, sizeof buf, fmt, va);
	va_end(va);

	puts(buf);

	return r;
}

int warning(const char *fmt, ...)
{
	va_list va;
	char buf[BUFSZ];
	int r;

	va_start(va, fmt);
	r = vsnprintf(buf, sizeof buf, fmt, va);
	va_end(va);

	puts(buf);

	return r;
}

int error(const char *fmt, ...)
{
	va_list va;
	char buf[BUFSZ];
	int r;

	va_start(va, fmt);
	r = vsnprintf(buf, sizeof buf, fmt, va);
	va_end(va);

	puts(buf);

	return r;
}

void binstr(uint32_t x, char out[BINSTR_SZ])
{
	int i;
	char *c;
	int n;

	n = sizeof(uint32_t) * 8;
	i = 0;
	c = &out[0];
	for (;;)
	{
		*c++ = (x & (1ul << (n - i - 1))) ? '1' : '0';
		i++;

		if (i == n)
		{
			*c = '\0';
			break;
		}
		if (i % 4 == 0)
			*c++ = '\'';
	}
	// assert(c + 1 - &out[0] == BINSTR_SZ);
}

void print_binstr(uint32_t x)
{
	char out[BINSTR_SZ];
	binstr(x, out);
	printf("%s", out);
}
