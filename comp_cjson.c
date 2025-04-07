
#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    <stdbool.h>
#include    <unistd.h>
#include	<stdint.h>
#include    <ctype.h>

#include    "cJSON.h"

#define __________________________________________________________
#define __________________________________________________________

/* Error Codes */
typedef enum 
{
    CJH_RET_OK,               
    CJH_RET_KO,               
    CJH_RET_NULL_PTR,         
    CJH_RET_INVALID_PATH,     
    CJH_RET_KEY_NOT_FOUND,    
    CJH_RET_TYPE_MISMATCH,    
    CJH_RET_BUFFER_TOO_SMALL, 
    CJH_RET_ALLOC_FAIL        
} cjh_ret_code_t;

/* Data Types */
typedef enum 
{
    CJH_DT_INT,
    CJH_DT_FLOAT,
    CJH_DT_STRING,
    CJH_DT_BOOL,
    CJH_DT_BYTEARRAY,
    CJH_DT_OBJECT,
    CJH_DT_ARRAY
} cjh_data_type_t;

/* Debug Macro */
#define cjh_dbg(format, ...) \
    printf("cjhdbg-->[%s@%-4u]:" format, __FUNCTION__, __LINE__, ##__VA_ARGS__)

/* Handler Structure */
typedef struct cjsonData_handler cjsonData_handler_t;

struct cjsonData_handler
{
    cJSON* json_parsed;
    cJSON* json_builder;
    char* encoded_str;
    
    cjh_ret_code_t (*cjh_get_value_by_path)(cjsonData_handler_t* self, const char* path, 
                                          cjh_data_type_t data_type, void* value, uint16_t buf_len);
    
    cjh_ret_code_t (*cjh_set_value_by_path)(cjsonData_handler_t* self, const char* path,
                                          cjh_data_type_t data_type, void* value, uint16_t buf_len);
    
    cjh_ret_code_t (*cjh_mk_jsonstr)(cjsonData_handler_t* self, char* json_str, uint16_t buf_len);
    
    cjh_ret_code_t (*cjh_init)(cjsonData_handler_t* self, char* raw_json_in);
    
    cjh_ret_code_t (*cjh_free)(cjsonData_handler_t* self);
};

/* Constructor */
int cjh_create(cjsonData_handler_t* p, void* args);

#define __________________________________________________________
#define __________________________________________________________

static cJSON* _cjh_traverse_path(cjsonData_handler_t* self, const char* path, bool create_missing)
{
    if (!self || !path || !*path)
    {
        cjh_dbg("Null pointer or empty path\n");
        return NULL;
    }
    
    cJSON* current = self->json_parsed;
    char* path_copy = strdup(path);
    if (!path_copy)
    {
        cjh_dbg("Memory allocation failed\n");
        return NULL;
    }
    char* token = strtok(path_copy, ".");
    
    while (token != NULL)
    {
        /* Array index handling */
        char* bracket = strchr(token, '[');
        if (bracket != NULL)
        {
            *bracket = '\0';
            int index = atoi(bracket + 1);
            
            cJSON* array = cJSON_GetObjectItemCaseSensitive(current, token);
            if (!array || !cJSON_IsArray(array))
            {
                if (create_missing)
                {
                    array = cJSON_AddArrayToObject(current, token);
                }
                else
                {
                    free(path_copy);
                    return NULL;
                }
            }
            
            current = cJSON_GetArrayItem(array, index);
            if (!current && create_missing)
            {
                while (cJSON_GetArraySize(array) <= index)
                {
                    cJSON_AddItemToArray(array, cJSON_CreateNull());
                }
                current = cJSON_GetArrayItem(array, index);
            }
        }
        else
        {
            /* Object handling */
            cJSON* next = cJSON_GetObjectItemCaseSensitive(current, token);
            if (!next)
            {
                if (create_missing)
                {
                    next = cJSON_AddObjectToObject(current, token);
                }
                else
                {
                    free(path_copy);
                    return NULL;
                }
            }
            current = next;
        }
        token = strtok(NULL, ".");
    }
    
    free(path_copy);
    return current;
}

/* Getter Implementation */
static cjh_ret_code_t _cjh_get_value_by_path(cjsonData_handler_t* self, const char* path, 
                                           cjh_data_type_t data_type, void* value, uint16_t buf_len)
{
    if (!self || !path || !value)
    {
        cjh_dbg("Null pointer detected\n");
        return CJH_RET_NULL_PTR;
    }
    
    cJSON* item = _cjh_traverse_path(self, path, false);
    if (!item)
    {
        cjh_dbg("Path not found: %s\n", path);
        return CJH_RET_KEY_NOT_FOUND;
    }
    
    memset(value, 0, buf_len);
    
    switch (data_type)
    {
        case CJH_DT_INT:
            if (!cJSON_IsNumber(item))
            {
                cjh_dbg("Type mismatch: Expected int\n");
                return CJH_RET_TYPE_MISMATCH;
            }
            *(int*)value = item->valueint;
            cjh_dbg("%-24s, CJH_DT_INT(%d), val=%d\n",path,data_type,*(int*)value);
            break;
            
        case CJH_DT_FLOAT:
            if (!cJSON_IsNumber(item))
            {
                cjh_dbg("Type mismatch: Expected float\n");
                return CJH_RET_TYPE_MISMATCH;
            }
            *(float*)value = item->valuedouble;
            
            cjh_dbg("%-24s, CJH_DT_FLOAT(%d), val=%f\n",path,data_type,*(float*)value);
            break;
            
        case CJH_DT_BOOL:
            if (!cJSON_IsBool(item))
            {
                cjh_dbg("Type mismatch: Expected bool\n");
                return CJH_RET_TYPE_MISMATCH;
            }
            *(bool*)value = cJSON_IsTrue(item);
            
            cjh_dbg("%-24s, CJH_DT_BOOL(%d), val=%d(%s)\n",path,data_type,*(bool*)value, *(bool*)value ? "true" : "false");
            break;
            
        case CJH_DT_STRING:
            if (!cJSON_IsString(item))
            {
                cjh_dbg("Type mismatch: Expected string\n");
                return CJH_RET_TYPE_MISMATCH;
            }
            if (strlen(item->valuestring) >= buf_len)
            {
                cjh_dbg("Buffer too small for string\n");
                return CJH_RET_BUFFER_TOO_SMALL;
            }
            strcpy((char*)value, item->valuestring);
            cjh_dbg("%-24s, CJH_DT_STRING(%d), val=%s\n",path,data_type,(char*)value);
            break;
            
        case CJH_DT_ARRAY:
        {
            if (!cJSON_IsArray(item)) 
            {
                cjh_dbg("Expected Array, got other type\n");
                return CJH_RET_TYPE_MISMATCH;
            }
            char* json_str = cJSON_PrintUnformatted(item);
            //char* json_str = cJSON_Print(item);
            if (!json_str)
            {
                return CJH_RET_ALLOC_FAIL;
            }
            if (strlen(json_str) >= buf_len)
            {
                free(json_str);
                cjh_dbg("Buffer too small for JSON\n");
                return CJH_RET_BUFFER_TOO_SMALL;
            }
            strcpy((char*)value, json_str);
            free(json_str);
            
            cjh_dbg("%-24s, CJH_DT_ARRAY(%d), val=%s\n",path,data_type,(char*)value);
            
            break;
        }            
        case CJH_DT_OBJECT:
        {
            if (!cJSON_IsObject(item)) 
            {
                cjh_dbg("Expected Object, got other type\n");
                return CJH_RET_TYPE_MISMATCH;
            }
            char* json_str = cJSON_PrintUnformatted(item);
            //char* json_str = cJSON_Print(item);
            if (!json_str)
            {
                return CJH_RET_ALLOC_FAIL;
            }
            if (strlen(json_str) >= buf_len)
            {
                free(json_str);
                cjh_dbg("Buffer too small for JSON\n");
                return CJH_RET_BUFFER_TOO_SMALL;
            }
            strcpy((char*)value, json_str);
            free(json_str);
            
            cjh_dbg("%-24s, CJH_DT_OBJECT(%d), val=%s\n",path,data_type,(char*)value);
            
            break;
        }


        case CJH_DT_BYTEARRAY: 
        {
            if (!cJSON_IsString(item)) 
            {
                cjh_dbg("Type mismatch: Expected string\n");
                return CJH_RET_TYPE_MISMATCH;
            }
            
            const char* hex_str = item->valuestring;
            size_t hex_len = strlen(hex_str);
            
            // 校验十六进制字符串长度是否为偶数
            if (hex_len % 2 != 0) 
            {
                cjh_dbg("Hex string length must be even (got %zu)\n", hex_len);
                return CJH_RET_TYPE_MISMATCH;
            }
            
            // 校验缓冲区大小
            size_t required_len = hex_len / 2;
            if (buf_len < required_len) 
            {
                cjh_dbg("Buffer too small: need %zu, got %u\n", required_len, buf_len);
                return CJH_RET_BUFFER_TOO_SMALL;
            }

            // 校验字符合法性
            for (size_t i = 0; i < hex_len; i++) 
            {
                if (!isxdigit(hex_str[i])) 
                {
                    cjh_dbg("Invalid hex char: %c\n", hex_str[i]);
                    return CJH_RET_TYPE_MISMATCH;
                }
            }
            
            // 安全转换
            for (size_t i = 0; i < required_len; i++) 
            {
                if (sscanf(hex_str + 2*i, "%2hhx", &((uint8_t*)value)[i]) != 1) 
                {
                    cjh_dbg("Hex conversion failed at position %zu\n", i);
                    return CJH_RET_KO;
                }
            }
            
            cjh_dbg("%-24s, CJH_DT_BYTEARRAY(%d), val= ",path,data_type);
            for(size_t i=0; i<required_len; i++) printf("%X ", ((uint8_t*)(value))[i]); printf("\n");
            
            break;
        }

        
        default:
            cjh_dbg("Unknown data type: %d\n", data_type);
            return CJH_RET_KO;
    }
    
    return CJH_RET_OK;
}

/* Setter Implementation */
static cjh_ret_code_t _cjh_set_value_by_path(cjsonData_handler_t* self, const char* path,
                                           cjh_data_type_t data_type, void* value, uint16_t buf_len)
{
    if (!self || !path)
    {
        cjh_dbg("Null pointer detected\n");
        return CJH_RET_NULL_PTR;
    }
    
    if (!self->json_builder)
    {
        self->json_builder = cJSON_CreateObject();
        if (!self->json_builder)
        {
            cjh_dbg("JSON object creation failed\n");
            return CJH_RET_ALLOC_FAIL;
        }
    }
    
    /* Path parsing logic */
    cJSON* current = _cjh_traverse_path(self, path, true);
    if (!current)
    {
        return CJH_RET_INVALID_PATH;
    }
    
    /* Set the value based on the data type */
    switch (data_type)
    {
        case CJH_DT_INT:
            cJSON_AddNumberToObject(current, path, *(int*)value);
            break;
        /* ... other cases ... */
        default:
            cjh_dbg("Unknown data type: %d\n", data_type);
            return CJH_RET_KO;
    }
    
    return CJH_RET_OK;
}

cjh_ret_code_t _cjh_mk_jsonstr(cjsonData_handler_t* self, char* json_str, uint16_t buf_len)
{
    if (!self || !self->json_builder) 
    {
        return CJH_RET_KO;
    }

    // Free previous encoded string
    if (self->encoded_str) 
    {
        free(self->encoded_str);
    }

    self->encoded_str = cJSON_PrintUnformatted(self->json_builder);
    if (!self->encoded_str)
    {
        return CJH_RET_ALLOC_FAIL;
    }
    
    strncpy(json_str, self->encoded_str, buf_len - 1);
    json_str[buf_len - 1] = '\0';
    
    return CJH_RET_OK;
}

static cjh_ret_code_t _cjh_free(cjsonData_handler_t* self)
{
    if (!self)
    {
        cjh_dbg("Null handler\n");
        return CJH_RET_NULL_PTR;
    }
    if (self->json_parsed)
    {
        cJSON_Delete(self->json_parsed);
        self->json_parsed = NULL;
    }
    
    if (self->json_builder)
    {
        cJSON_Delete(self->json_builder);
        self->json_builder = NULL;
    }
    
    if (self->encoded_str)
    {
        free(self->encoded_str);
        self->encoded_str = NULL;
    }
    
    return CJH_RET_OK;
}

/* Initialize and Free */
static cjh_ret_code_t _cjh_init(cjsonData_handler_t* self, char* raw_json_in)
{
    if (!self)
    {
        cjh_dbg("Null handler\n");
        return CJH_RET_NULL_PTR;
    }

    
    if(raw_json_in)
    {
        self->json_parsed = cJSON_Parse(raw_json_in);

        if(self->json_parsed == NULL)
            return CJH_RET_KO;
    }

    
    /*
    if (!self->json_builder)
    {
        self->json_builder = cJSON_CreateObject();
        if (!self->json_builder)
        {
            return CJH_RET_ALLOC_FAIL;
        }
    }
    */
    

    self->cjh_get_value_by_path = _cjh_get_value_by_path;
    self->cjh_set_value_by_path = _cjh_set_value_by_path;
    self->cjh_mk_jsonstr = _cjh_mk_jsonstr;
    //self->cjh_init = _cjh_init;
    self->cjh_free = _cjh_free;
    

    return CJH_RET_OK ;
    
}

#define __________________________________________________________
#define __________________________________________________________

/* Construction Function */
int cjh_create(cjsonData_handler_t* p, void* args)
{
    if (!p)
    {
        cjh_dbg("Null pointer\n");
        return -1;
    }
    memset(p, 0, sizeof(cjsonData_handler_t));

    p->cjh_init = _cjh_init;
    return (p->cjh_init(p, (char*)args) == CJH_RET_OK) ? 0 : -1;
}

int main()
{

const char* jsondata = 
"{"
"  \"deviceId\": \"BLE-Gateway-01\","
"  \"timestamp\": 1717032468,"
"  \"status\": {"
"    \"online\": true,"
"    \"battery\": 78,"
"    \"rssi\": -45,"
"    \"firmware\": \"v2.3.1\""
"  },"
"  \"sensors\": ["
"    {"
"      \"type\": \"temperature\","
"      \"value\": 25.6,"
"      \"unit\": \"°C\","
"      \"alarm\": false,"
"      \"location\": {"
"        \"lat\": 31.2304,"
"        \"lon\": 121.4737,"
"        \"alt\": 12.3"
"      }"
"    },"
"    {"
"      \"type\": \"humidity\","
"      \"value\": 65.2,"
"      \"unit\": \"%\","
"      \"alarm\": false"
"    }"
"  ],"
"  \"commands\": ["
"    {"
"      \"cmdId\": \"20240401-001\","
"      \"op\": \"set\","
"      \"params\": {"
"        \"led\": \"on\","
"        \"brightness\": 80"
"      },"
"      \"ttl\": 3600"
"    }"
"  ],"
"  \"security\": {"
"    \"signature\": \"a1b2c3d4e5f6\","
"    \"encrypted\": false,"
"    \"keys\": ["
"      {"
"        \"keyId\": \"netKey-01\","
"        \"value\": \"3FA985DA6D4CA22DA05C7E7A1F11C783\""
"      }"
"    ]"
"  }"
"}";


    cjsonData_handler_t handler, *p;
    p = &handler;
    if (cjh_create(&handler, (void*)jsondata) != 0)
    {
        fprintf(stderr, "Handler creation failed\n");
        return -1;
    }
    int varint;
    int ret = handler.cjh_get_value_by_path(&handler, "status.battery", CJH_DT_INT, &varint,  sizeof(varint) );


    bool varbool;
    ret = handler.cjh_get_value_by_path(&handler, "sensors[0].alarm", CJH_DT_BOOL, &varbool,  sizeof(varbool) );

    
    float varfloat;
    ret = handler.cjh_get_value_by_path(&handler, "sensors[0].value", CJH_DT_FLOAT, &varfloat,  sizeof(varfloat) );


    char varstr[512]={0};
    ret = handler.cjh_get_value_by_path(&handler, "sensors[0].type", CJH_DT_STRING, varstr,  sizeof(varstr) );

    ret = handler.cjh_get_value_by_path(&handler, "commands[0]", CJH_DT_OBJECT, varstr,  sizeof(varstr) );

    ret = handler.cjh_get_value_by_path(&handler, "security.keys", CJH_DT_ARRAY, varstr,  sizeof(varstr) );
    ret = handler.cjh_get_value_by_path(&handler, "security.keys[0].value", CJH_DT_BYTEARRAY, varstr,  sizeof(varstr) );


/*
    int brightness = 80;
    cjh_ret_code_t ret = handler.cjh_set_value_by_path(
        &handler, 
        "device.display.brightness", 
        CJH_DT_INT, 
        &brightness, 
        0
    );
    
    if (ret != CJH_RET_OK)
    {
        fprintf(stderr, "Set failed: %d\n", ret);
    }


    int read_val;
    ret = handler.cjh_get_value_by_path(
        &handler, 
        "device.display.brightness", 
        CJH_DT_INT, 
        &read_val, 
        0
    );
    if (ret == CJH_RET_OK)
    {
        printf("Brightness: %d\n", read_val);
    }


    char json_str[512];
    ret = handler.cjh_mk_jsonstr(&handler, json_str, sizeof(json_str));
    if (ret == CJH_RET_OK)
    {
        printf("Generated JSON:\n%s\n", json_str);
    }

*/
    handler.cjh_free(&handler);
    
    return 0;
}

#define __________________________________________________________
#define __________________________________________________________



