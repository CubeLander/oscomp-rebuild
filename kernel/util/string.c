// See LICENSE for license details.

#include <ctype.h>
#include <kernel/types.h>
#include <kernel/util/string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>


void* memcpy(void* dest, const void* src, size_t len) {
	const char* s = src;
	char* d = dest;

	if ((((uintptr_t)dest | (uintptr_t)src) & (sizeof(uintptr_t) - 1)) == 0) {
		while ((void*)d < (dest + len - (sizeof(uintptr_t) - 1))) {
			*(uintptr_t*)d = *(const uintptr_t*)s;
			d += sizeof(uintptr_t);
			s += sizeof(uintptr_t);
		}
	}

	while (d < (char*)(dest + len)) *d++ = *s++;

	return dest;
}

void* memset(void* dest, int byte, size_t len) {
	if ((((uintptr_t)dest | len) & (sizeof(uintptr_t) - 1)) == 0) {
		uintptr_t word = byte & 0xFF;
		word |= word << 8;
		word |= word << 16;
		word |= word << 16 << 16;

		int32* d = dest;
		while ((uintptr_t)d < (uintptr_t)(dest + len)) *d++ = word;
	} else {
		char* d = dest;
		while (d < (char*)(dest + len)) *d++ = byte;
	}
	return dest;
}

// void* memset(void* dest, int byte, size_t len) {
//     char* d = (char*)dest;
//     for (size_t i = 0; i < len; i++) {
//         d[i] = (char)byte;
//     }
//     return dest;
// }

/**
 * 比较两个字符串的前n个字符
 * @param s1 第一个字符串
 * @param s2 第二个字符串
 * @param n 要比较的字符数量
 * @return 如果s1和s2的前n个字符相同，则返回0；
 *         如果s1小于s2，则返回小于0的值；
 *         如果s1大于s2，则返回大于0的值
 */
int strncmp(const char *s1, const char *s2, size_t n) {
    // 如果n为0，则不需要比较，直接返回0
    if (n == 0) {
        return 0;
    }
    
    // 比较字符直到遇到不同的字符或者到达n个字符或者某个字符串结束
    while (n-- > 0) {
        // 如果当前字符不同或者到达字符串结尾，返回差值
        if (*s1 != *s2 || *s1 == '\0' || *s2 == '\0') {
            // 返回当前字符的ASCII差值
            return (*(unsigned char *)s1) - (*(unsigned char *)s2);
        }
        s1++;
        s2++;
    }
    
    // 如果前n个字符都相同，返回0
    return 0;
}



size_t strlen(const char* s) {
	const char* p = s;
	while (*p) p++;
	return p - s;
}

int strcmp(const char* s1, const char* s2) {
	unsigned char c1, c2;

	do {
		c1 = *s1++;
		c2 = *s2++;
	} while (c1 != 0 && c1 == c2);

	return c1 - c2;
}

char* strcpy(char* dest, const char* src) {
	char* d = dest;
	while ((*d++ = *src++));
	return dest;
}

char* strchr(const char* p, int ch) {
	char c;
	c = ch;
	for (;; ++p) {
		if (*p == c) return ((char*)p);
		if (*p == '\0') return (NULL);
	}
}

char* strtok(char* str, const char* delim) {
	static char* current;
	if (str != NULL) current = str;
	if (current == NULL) return NULL;

	char* start = current;
	while (*start != '\0' && strchr(delim, *start) != NULL) start++;

	if (*start == '\0') {
		current = NULL;
		return current;
	}

	char* end = start;
	while (*end != '\0' && strchr(delim, *end) == NULL) end++;

	if (*end != '\0') {
		*end = '\0';
		current = end + 1;
	} else
		current = NULL;
	return start;
}

char* strcat(char* dst, const char* src) {
	strcpy(dst + strlen(dst), src);
	return dst;
}

long atol(const char* str) {
	long res = 0;
	int sign = 0;

	while (*str == ' ') str++;

	if (*str == '-' || *str == '+') {
		sign = *str == '-';
		str++;
	}

	while (*str) {
		res *= 10;
		res += *str++ - '0';
	}

	return sign ? -res : res;
}

void* memmove(void* dst, const void* src, size_t n) {
	const char* s;
	char* d;

	s = src;
	d = dst;
	if (s < d && s + n > d) {
		s += n;
		d += n;
		while (n-- > 0) *--d = *--s;
	} else
		while (n-- > 0) *d++ = *s++;

	return dst;
}

/**
 * strncpy - 将源字符串的前n个字符复制到目标字符串
 * @dest: 目标字符串
 * @src: 源字符串
 * @n: 最多复制的字符数
 *
 * 如果src的长度小于n，则用'\0'填充剩余空间
 * 如果src的长度大于等于n，则不会自动添加'\0'
 *
 * 返回值: 目标字符串指针
 */
char* strncpy(char* dest, const char* src, size_t n) {
	size_t i;

	// 复制最多n个字符
	for (i = 0; i < n && src[i] != '\0'; i++) { dest[i] = src[i]; }

	// 如果源字符串长度小于n，用'\0'填充剩余空间
	for (; i < n; i++) { dest[i] = '\0'; }

	return dest;
}



/**
 * memcmp - Compare two memory regions
 * @s1: First memory region
 * @s2: Second memory region
 * @n: Number of bytes to compare
 *
 * Compares two memory regions byte by byte.
 *
 * Returns:
 *   0 if the regions are identical
 *   < 0 if the first differing byte in s1 is less than in s2
 *   > 0 if the first differing byte in s1 is greater than in s2
 */
int memcmp(const void* s1, const void* s2, size_t n) {
	const unsigned char* p1 = s1;
	const unsigned char* p2 = s2;

	if (s1 == s2 || n == 0) return 0;

	/* Compare byte by byte */
	while (n--) {
		if (*p1 != *p2) return *p1 - *p2;
		p1++;
		p2++;
	}

	return 0;
}

int32_t vsnprintf(char* out, size_t n, const char* s, va_list vl) {
	bool format = false;
	bool longarg = false;
	size_t pos = 0;

	for (; *s; s++) {
		if (format) {
			switch (*s) {
			case 'l':
				longarg = true;
				break;
			case 'p':
				longarg = true;
				if (++pos < n) out[pos - 1] = '0';
				if (++pos < n) out[pos - 1] = 'x';
			case 'x': {
				int64 num = longarg ? va_arg(vl, int64) : va_arg(vl, int);
				for (int i = 2 * (longarg ? sizeof(int64) : sizeof(int)) - 1; i >= 0; i--) {
					int d = (num >> (4 * i)) & 0xF;
					if (++pos < n) out[pos - 1] = (d < 10 ? '0' + d : 'a' + d - 10);
				}
				longarg = false;
				format = false;
				break;
			}
			case 'd': {
				int64 num = longarg ? va_arg(vl, int64) : va_arg(vl, int);
				if (num < 0) {
					num = -num;
					if (++pos < n) out[pos - 1] = '-';
				}
				int64 digits = 1;
				for (int64 nn = num; nn /= 10; digits++);
				for (int i = digits - 1; i >= 0; i--) {
					if (pos + i + 1 < n) out[pos + i] = '0' + (num % 10);
					num /= 10;
				}
				pos += digits;
				longarg = false;
				format = false;
				break;
			}
			case 's': {
				const char* s2 = va_arg(vl, const char*);
				while (*s2) {
					if (++pos < n) out[pos - 1] = *s2;
					s2++;
				}
				longarg = false;
				format = false;
				break;
			}
			case 'c': {
				if (++pos < n) out[pos - 1] = (char)va_arg(vl, int);
				longarg = false;
				format = false;
				break;
			}
			default:
				break;
			}
		} else if (*s == '%')
			format = true;
		else if (++pos < n)
			out[pos - 1] = *s;
	}
	if (pos < n)
		out[pos] = 0;
	else if (n)
		out[n - 1] = 0;
	return pos;
}
