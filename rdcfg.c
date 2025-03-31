
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
    printf("Content-Type: application/json; charset=UTF-8\r\n\r\n");

    char *len_str = getenv("CONTENT_LENGTH");
    int len = 0;
    if (len_str)
    {
        len = atoi(len_str);
    }
    if (len <= 0)
    {
        printf("{\"error\":\"No POST data received\"}");
        return 1;
    }

    char *post_data = malloc(len + 1);
    if (!post_data)
    {
        printf("{\"error\":\"Memory allocation failed\"}");
        return 1;
    }
    if (fread(post_data, 1, len, stdin) != (size_t)len)
    {
        printf("{\"error\":\"Failed to read POST data\"}");
        free(post_data);
        return 1;
    }
    post_data[len] = '\0';

    char *p = strstr(post_data, "new_json=");
    if (!p)
    {
        printf("{\"error\":\"new_json parameter not found\"}");
        free(post_data);
        return 1;
    }
    char *encoded = p + strlen("new_json=");
    char *decoded = url_decode(encoded);

    cJSON *root = cJSON_Parse(decoded);
    if (root == NULL)
    {
        printf("{\"error\":\"Invalid JSON data\"}");
        free(decoded);
        free(post_data);
        return 1;
    }
    char *formatted_json = cJSON_Print(root);
    if (!formatted_json)
    {
        printf("{\"error\":\"Failed to format JSON data\"}");
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
        printf("{\"message\":\"Configuration updated successfully\"}");
    }
    else
    {
        printf("{\"error\":\"Failed to write configuration file\"}");
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

