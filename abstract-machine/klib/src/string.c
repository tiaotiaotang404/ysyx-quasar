#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s)
{
  // panic("Not implemented");
  size_t len = 0;
  while (*s != '\0')
  {
    len++;
    s++;
  }
  return len;
}

char *strcpy(char *dst, const char *src)
{
  // panic("Not implemented");
  char *dst_ptr = dst;
  while (*src != '\0')
  {
    *dst_ptr = *src;
    dst_ptr++;
    src++;
  }
  *dst_ptr = '\0';
  return dst;
}

char *strncpy(char *dst, const char *src, size_t n)
{
  // panic("Not implemented");
  char *dst_ptr = dst;
  size_t i;

  for (i = 0; i < n && *src != '\0'; i++)
  {
    *dst_ptr++ = *src++; // 复制字符并移动指针
  }

  while (i < n)
  {
    *dst_ptr++ = '\0'; // 用 '\0' 填充剩余位置
    i++;
  }

  return dst;
}

char *strcat(char *dst, const char *src) // 把src接在dst后面
{
  // panic("Not implemented");
  char *dst_ptr = dst;

  // 定位目标字符串的末尾
  while (*dst_ptr != '\0')
  {
    dst_ptr++;
  }

  // 追加源字符串到目标字符串的末尾
  while (*src != '\0')
  {
    *dst_ptr++ = *src++;
  }
  *dst_ptr = '\0'; // 添加结束符

  return dst;
}

int strcmp(const char *s1, const char *s2)
{
  // panic("Not implemented");
  while (*s1 != '\0' && *s1 == *s2)
  {
    s1++;
    s2++;
  }

  return (int)(*s1) - (int)(*s2); // s1长返回正数，否则返回负数，两者相同返回0
}

int strncmp(const char *s1, const char *s2, size_t n)
{
  // panic("Not implemented");
  while (n > 0 && *s1 != '\0' && *s1 == *s2)
  {
    s1++;
    s2++;
    n--;
  }

  if (n == 0)
  {
    return 0; // 比较长度达到 n，表示两个字符串相等
  }

  return (int)(*s1) - (int)(*s2);
}

void *memset(void *s, int c, size_t n) // 将c存入大小为n的内存s中
{
  // panic("Not implemented");
  unsigned char *p = (unsigned char *)s; // 类型转换，进行字节操作
  unsigned char value = (unsigned char)c;

  for (size_t i = 0; i < n; i++)
  {
    p[i] = value;
  }

  return s;
}

void *memmove(void *dst, const void *src, size_t n) // 将大小为n个字节的src指向的内存中的内容移动到dstd所指向的内存中
{
  // panic("Not implemented");
  unsigned char *d = (unsigned char *)dst;
  const unsigned char *s = (const unsigned char *)src;

  if (s < d && s + n > d)
  {
    // 源地址和目标地址重叠，从后向前复制数据
    for (size_t i = n; i > 0; i--)
    {
      d[i - 1] = s[i - 1];
    }
  }
  else
  {
    // 源地址和目标地址不重叠，从前向后复制数据
    for (size_t i = 0; i < n; i++)
    {
      d[i] = s[i];
    }
  }

  return dst;
}

void *memcpy(void *out, const void *in, size_t n)
{
  // panic("Not implemented");
  unsigned char *dest = (unsigned char *)out;
  const unsigned char *src = (const unsigned char *)in;

  for (size_t i = 0; i < n; i++)
  {
    dest[i] = src[i];
  }

  return out;
}

int memcmp(const void *s1, const void *s2, size_t n)
{
  // panic("Not implemented");
  const unsigned char *p1 = (const unsigned char *)s1;
  const unsigned char *p2 = (const unsigned char *)s2;

  for (size_t i = 0; i < n; i++)
  {
    if (p1[i] < p2[i])
    {
      return -1;
    }
    else if (p1[i] > p2[i])
    {
      return 1;
    }
  }

  return 0;
}

#endif
