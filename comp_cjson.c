
#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    <stdbool.h>
#include    <unistd.h>
#include	<stdint.h>
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
    
    cjh_ret_code_t (*cjh_init)(cjsonData_handler_t* self,  char *raw_json_in);
    
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
    
    switch (data_type)
    {
        case CJH_DT_INT:
            if (!cJSON_IsNumber(item))
            {
                cjh_dbg("Type mismatch: Expected int\n");
                return CJH_RET_TYPE_MISMATCH;
            }
            *(int*)value = item->valueint;
            break;
            
        case CJH_DT_FLOAT:
            if (!cJSON_IsNumber(item))
            {
                cjh_dbg("Type mismatch: Expected float\n");
                return CJH_RET_TYPE_MISMATCH;
            }
            *(float*)value = item->valuedouble;
            break;
            
        /* ... similar checks for other types ... */
            
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


    cjsonData_handler_t handler;
    if (cjh_create(&handler, (void*)jsondata) != 0)
    {
        fprintf(stderr, "Handler creation failed\n");
        return -1;
    }
    int varint;
    int ret = handler.cjh_get_value_by_path(&handler, "status.battery", CJH_DT_INT, &varint,  0 );
    if (ret == CJH_RET_OK)
    {
        printf("varint: %d\n", varint);
    }  

    bool varbool;
    ret = handler.cjh_get_value_by_path(&handler, "sensors[0].alarm", CJH_DT_BOOL, &varbool,  0 );
    if (ret == CJH_RET_OK)
    {
        printf("varbool: %s\n", (varbool) ? "true" : "false");
    } 
    
    float varfloat;
    ret = handler.cjh_get_value_by_path(&handler, "sensors[0].value", CJH_DT_FLOAT, &varfloat,  0 );
    if (ret == CJH_RET_OK)
    {
        printf("varfloat: %f\n", varfloat);
    }  

    char varstr[512];
    ret = handler.cjh_get_value_by_path(&handler, "sensors[0].type", CJH_DT_STRING, &varstr,  0 );
    if (ret == CJH_RET_OK)
    {
        printf("varstr: %s\n", varstr);
    }  

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


