
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"
#include <stdint.h>
#include <unistd.h>
#include <ctype.h>
#include <stdbool.h>

char from_hex(char ch)
{
    if (ch >= '0' && ch <= '9')
    {
        return ch - '0';
    }
    if (ch >= 'A' && ch <= 'F')
    {
        return ch - 'A' + 10;
    }
    if (ch >= 'a' && ch <= 'f')
    {
        return ch - 'a' + 10;
    }
    return 0;
}

char *url_decode(const char *src)
{
    char *dest = malloc(strlen(src) + 1);
    char *p = dest;
    while (*src)
    {
        if (*src == '%')
        {
            if (*(src + 1) && *(src + 2))
            {
                *p++ = (from_hex(*(src + 1)) << 4) | from_hex(*(src + 2));
                src += 3;
            }
            else
            {
                src++;
            }
        }
        else if (*src == '+')
        {
            *p++ = ' ';
            src++;
        }
        else
        {
            *p++ = *src++;
        }
    }
    *p = '\0';
    return dest;
}

int main(void)
{
    printf("Content-Type: text/html; charset=UTF-8\r\n\r\n");

    char *len_str = getenv("CONTENT_LENGTH");
    int len = 0;
    if (len_str)
    {
        len = atoi(len_str);
    }
    if (len <= 0)
    {
        printf("<html><body><h1>错误：无 POST 数据</h1></body></html>");
        return 1;
    }

    char *post_data = malloc(len + 1);
    if (!post_data)
    {
        printf("<html><body><h1>错误：内存分配失败</h1></body></html>");
        return 1;
    }
    if (fread(post_data, 1, len, stdin) != (size_t)len)
    {
        printf("<html><body><h1>错误：读取 POST 数据失败</h1></body></html>");
        free(post_data);
        return 1;
    }
    post_data[len] = '\0';

    char *p = strstr(post_data, "new_json=");
    if (!p)
    {
        printf("<html><body><h1>错误：未找到 new_json 参数</h1></body></html>");
        free(post_data);
        return 1;
    }
    char *encoded = p + strlen("new_json=");
    char *decoded = url_decode(encoded);

    cJSON *root = cJSON_Parse(decoded);
    if (root == NULL)
    {
        printf("<html><body><h1>错误：无效的 JSON 数据</h1></body></html>");
        free(decoded);
        free(post_data);
        return 1;
    }
    char *formatted_json = cJSON_Print(root);
    if (!formatted_json)
    {
        printf("<html><body><h1>错误：无法格式化 JSON 数据</h1></body></html>");
        cJSON_Delete(root);
        free(decoded);
        free(post_data);
        return 1;
    }

    FILE *fp = fopen("../www/prj_cfg.json", "w");
    if (fp)
    {
        fputs(formatted_json, fp);
        fclose(fp);
        printf("<html><body><h1>配置更新成功。</h1>");
        printf("<a href=\"/index.html\">返回配置编辑器</a>");
        printf("</body></html>");
    }
    else
    {
        printf("<html><body><h1>错误：无法写入配置文件</h1></body></html>");
        free(formatted_json);
        cJSON_Delete(root);
        free(decoded);
        free(post_data);
        return 1;
    }

    free(formatted_json);
    cJSON_Delete(root);
    free(decoded);
    free(post_data);
    return 0;
}


