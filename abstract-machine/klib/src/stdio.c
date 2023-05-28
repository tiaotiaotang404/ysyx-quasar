#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

int printf(const char *fmt, ...)
{ // 直接输出内容到终端中
  panic("Not implemented");
}

int vsprintf(char *out, const char *fmt, va_list ap)
{
  panic("Not implemented");
}
int my_itoa(char *s, int d)
{
  char t[20];
  int n = 0;
  if (d < 0)
  {
    *s++ = '-';
    d = -d;
  }
  do
  {
    t[n++] = '0' + (d % 10);
  } while ((d /= 10) != 0);
  for (int i = n - 1; i >= 0; --i)
  {
    *s++ = t[i];
  }
  return n;
}
int sprintf(char *out, const char *fmt, ...)
{                    // 将各种类型转换成字符串存入out
                     // panic("Not implemented");
                     /*
                     va_list args;
                     va_start(args, fmt);
                   
                     int count = 0;
                   
                     while (*fmt != '\0')
                     {
                       if (*fmt == '%')
                       {
                         fmt++; // 跳过 '%'
                   
                         if (*fmt == 's')
                         {
                           // 处理字符串参数
                           char *str = va_arg(args, char *);
                   
                           while (*str != '\0')
                           {
                             *out = *str;
                             out++;
                             str++;
                             count++;
                           }
                         }
                         else if (*fmt == 'd')
                         {
                           // 处理整数参数
                           int num = va_arg(args, int);
                           int digits = 0;
                   
                           if (num == 0)
                           {
                             *out = '0';
                             out++;
                             count++;
                           }
                           else
                           {
                             if (num < 0)
                             {
                               *out = '-';
                               out++;
                               num = -num;
                               count++;
                             }
                   
                             int temp = num;
                             while (temp > 0)
                             {
                               temp /= 10;
                               digits++;
                             }
                   
                             temp = num;
                             while (digits > 0)
                             {
                               int digit = temp % 10;
                               *out = '0' + digit;
                               out++;
                               temp /= 10;
                               digits--;
                               count++;
                             }
                           }
                         }
                         else
                         {
                           // 未知格式，直接输出字符
                           *out = *fmt;
                           out++;
                           count++;
                         }
                       }
                       else
                       {
                         // 非格式化字符，直接输出
                         *out = *fmt;
                         out++;
                         count++;
                       }
                   
                       fmt++;
                     }
                   
                     va_end(args);
                   
                     *out = '\0'; // 添加字符串结束符
                   
                     return count;
                     */
  va_list ap;        // 定义 va_list 类型变量 ap
  va_start(ap, fmt); // 使用 va_start 宏初始化 ap

  int n = 0; // n 表示 out 数组中存储的字符数
  for (const char *p = fmt; *p != '\0'; ++p)
  {
    if (*p == '%')
    {
      switch (*(++p))
      { // 进入格式化字符的 switch 语句
      case 's':
      { // 处理字符串类型
        const char *s = va_arg(ap, const char *);
        while (*s != '\0')
        {
          out[n++] = *s++; // 逐个字符复制到 out 数组中
        }
        break;
      }
      case 'd':
      { // 处理整数类型
        int d = va_arg(ap, int);
        int q = d / 10, r = d % 10;
        if (d < 0)
        { // 处理负数情况
          out[n++] = '-';
          q = -q, r = -r;
        }
        if (q != 0)
        { // 如果 q 不为 0，继续处理下一位数字
          n += my_itoa(out + n, q);
        }
        out[n++] = '0' + r; // 输出最后一位数字
        break;
      }
      default: // 其它字符直接输出
        out[n++] = *p;
        break;
      }
    }
    else
    {
      out[n++] = *p; // 非格式化字符直接输出
    }
  }
  out[n] = '\0'; // 在 out 末尾添加 null 字符
  va_end(ap);    // 使用 va_end 宏结束可变参数列表
  return n;      // 返回输出字符个数
}

int snprintf(char *out, size_t n, const char *fmt, ...)
{ // 将n个字节转换成字符串存入out
  panic("Not implemented");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap)
{
  panic("Not implemented");
}

#endif
