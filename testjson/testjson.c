/*
 * Copyright (c) 2009-2014 Petri Lehtinen <petri@digip.org>
 *
 * Jansson is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 * 
 * 这个代码我测试了jansson和cjson，在解析方便程度上，二者没啥区别，速度差不多
 * jansson是antoconf的，需要make，cjson是直接源文件的，放进去目录一起编译即可
 * 唯一的区别，cjson会将最原始的json value呈现给你，也就是value带有 “”
 * jansson是经过了处理，把value中 “” 去掉了
 */

#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#include "cJSON.h"
#include "jansson.h"

struct write_result
{
    char * data;
    int pos;
};

#define MAX_BUF_SIZE 2048
#define MIN_BUF_SIZE 1024

/***************************************************************
* Function: curl写缓存的钩子函数，这个函数是curl官方提供
* 从代码的数据结构 struct_result，他的成员pos来看，这个结构
* 和写缓存方式，是支持缓存追加方式的，也就是说，是多个缓存
* 根据pos变量，续写data成员，data成员在外面用malloc给了地址
* Parameters:
* Return:
* Description:
****************************************************************/
size_t write_response(void * ptr, size_t size, size_t nmemb, void * stream)
{
    struct write_result * result = (struct write_result *)stream;

    if(result->pos + size * nmemb >= MAX_BUF_SIZE - 1)
    {
        fprintf(stderr, "error: too small buffer\n");
        return 0;
    }

    memcpy(result->data + result->pos, ptr, size * nmemb);
    result->pos += size * nmemb;

    return size * nmemb;
}

/***************************************************************
* Function: curl发http请求的函数
* Parameters:
* Return:
* Description:
****************************************************************/
char * request(char * url)
{
    //定义CURL类型的指针
    CURL * curl = NULL; 
    //定义CURLcode类型的变量，保存返回状态码
    CURLcode res;
    struct curl_slist * headers = NULL;
    curl_global_init(CURL_GLOBAL_ALL);
    
    char * data = (char *)malloc(MAX_BUF_SIZE);
    bzero(data, MAX_BUF_SIZE);
    struct write_result write_result = {
        .data = data,
        .pos = 0
    };

    //初始化一个CURL类型的指针
    curl = curl_easy_init();
    if(curl!=NULL)
    {
        //设置curl选项. 其中CURLOPT_URL是让用户指定url
        curl_easy_setopt(curl, CURLOPT_URL, url);
        headers = curl_slist_append(headers, "content-type:application/json;charset=utf8");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_response);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &write_result);
        
        //调用curl_easy_perform 执行我们的设置.并进行相关的操作，这个函数类似 ioctl，自动触发IO
        res = curl_easy_perform(curl);
        
         /* Check for errors */  
        if(res != CURLE_OK)
        {
            fprintf(stderr, "curl_easy_perform() failed: %s\n",  curl_easy_strerror(res));
            curl_global_cleanup();
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            return NULL;
        }
        
        //清除curl操作.
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
        curl_global_cleanup();

        /* zero-terminate the result */
        data[write_result.pos] = '\0';
    }
    return data;
}

/***************************************************************
* Function: cjson构造的json解析，依赖库为cjson.c和cjson.h
* 这俩文件无需安装，跟随代码一起编译即可
* Parameters:
* Return:
* Description:
****************************************************************/
void cjson_parse(char * text)
{
    cJSON * json_root = NULL;
    cJSON * json_arrayItem = NULL;
    cJSON * json_item = NULL;
    cJSON * json_a = NULL;
    cJSON * json_b = NULL;
    cJSON * json_c = NULL;
    cJSON * json_d = NULL;
    cJSON * json_e = NULL;
    int i = 0;
    int size = 0;
    char * pr = NULL;
    char * a = NULL;
    char * b = NULL;
    char * c = NULL;
    char * d = NULL;
    char * e = NULL;

    //将字符串解析成json结构体
    json_root = cJSON_Parse(text);

    //根据结构体获取数组大小
    size = cJSON_GetArraySize(json_root);

    puts("CJSON Parse.....");
    printf("== array size: %d\n", size);
    
    for(i=0; i<size; i++)
    {
        //获得json结构的数组一行
        json_arrayItem = cJSON_GetArrayItem(json_root, i);

        if(json_arrayItem)
        {
            //通过pr将一行转换为字符串
            pr = cJSON_Print(json_arrayItem);
            
            //解析这个pr（字符串）为json的item结构
            json_item = cJSON_Parse(pr);
            
            //分别获得json item结构下的json成员
            json_a = cJSON_GetObjectItem(json_item, "username");
            json_b = cJSON_GetObjectItem(json_item, "storedKey");
            json_c = cJSON_GetObjectItem(json_item, "serverKey");
            json_d = cJSON_GetObjectItem(json_item, "name");
            json_e = cJSON_GetObjectItem(json_item, "creationDate");
            
            //将json成员通过pr转为字符串
            a = cJSON_Print(json_a);
            b = cJSON_Print(json_b);
            c = cJSON_Print(json_c);
            d = cJSON_Print(json_d);
            e = cJSON_Print(json_e);
            
            printf("== a=%s\n", a);
            printf("== b=%s\n", b);
            printf("== c=%s\n", c);
            printf("== d=%s\n", d);
            printf("== e=%s\n", e);
            
            printf("============================================\n");
        }
    }
}

/***************************************************************
* Function: jansson构造的json解析，依赖库为jansson，这个库是
* autoconf的，需要进行configure & make & make install；
* Parameters:
* Return:
* Description:
****************************************************************/
void jansson_parse(char * text)
{
    json_t * json_root = NULL;
    json_t * json_arrayItem = NULL;
    json_t * json_a = NULL;
    json_t * json_b = NULL;
    json_t * json_c = NULL;
    json_t * json_d = NULL;
    json_t * json_e = NULL;
    json_error_t error;
    const char * message_text = NULL;

    json_root = json_loads(text, 0, &error);
    free(text);

    if(!json_root)
    {
        fprintf(stderr, "error: on line %d: %s\n", error.line, error.text);
        return;
    }

    //这里可以走一个分支，如果是数组，按照数组解析，如果json返回非数组，直接解析obiect即可
    if(!json_is_array(json_root))
    {
        fprintf(stderr, "error: root is not an array\n");
        json_decref(json_root);
        return;
    }
    
    //下面是按照数组解析的
    int i = 0;
    int size = json_array_size(json_root);
    puts("Jansson Parse.....");
    printf("** array size: %d\n", size);
    for(i=0; i<size; i++)
    {
        json_arrayItem = json_array_get(json_root, i);
        
        json_a = json_object_get(json_arrayItem, "username");
        json_b = json_object_get(json_arrayItem, "storedKey");
        json_c = json_object_get(json_arrayItem, "serverKey");
        json_d = json_object_get(json_arrayItem, "name");
        json_e = json_object_get(json_arrayItem, "creationDate");
        
        message_text = json_string_value(json_a);
        printf("** username: %s\n", message_text);
        message_text = json_string_value(json_b);
        printf("** storedKey: %s\n", message_text);
        message_text = json_string_value(json_c);
        printf("** serverKey: %s\n", message_text);
        message_text = json_string_value(json_d);
        printf("** name: %s\n", message_text);
        message_text = json_string_value(json_e);
        printf("** creationDate: %s\n", message_text);
        
        printf("******************************\n");
    }

    json_decref(json_root);
    return;
}


/***************************************************************
* Function: jansson构造的json解析，解析为单行json数据
* Parameters:
* Return:
* Description:
****************************************************************/
void non_jansson_parse(char * text)
{
    json_t * json_root = NULL;
    json_t * json_a = NULL;
    json_t * json_b = NULL;
    json_t * json_c = NULL;
    json_t * json_d = NULL;
    json_t * json_e = NULL;
    json_error_t error;
    const char * message_text = NULL;

    json_root = json_loads(text, 0, &error);
    free(text);

    if(!json_root)
    {
        fprintf(stderr, "error: on line %d: %s\n", error.line, error.text);
        return;
    }

    //这里可以走一个分支，如果是数组，按照数组解析，如果json返回非数组，直接解析object即可
    if(!json_is_array(json_root))
    {
        fprintf(stderr, "info: root is not an array, non array!\n");
    }
         
    json_a = json_object_get(json_root, "a");
    json_b = json_object_get(json_root, "b");
    json_c = json_object_get(json_root, "c");
    json_d = json_object_get(json_root, "d");
    json_e = json_object_get(json_root, "e");
        
    message_text = json_string_value(json_a);
    printf("** a: %s\n", message_text);
    message_text = json_string_value(json_b);
    printf("** b: %s\n", message_text);
    message_text = json_string_value(json_c);
    printf("** c: %s\n", message_text);
    message_text = json_string_value(json_d);
    printf("** d: %s\n", message_text);
    message_text = json_string_value(json_e);
    printf("** e: %s\n", message_text);
        
    json_decref(json_root);
    
    return;
}


/***************************************************************
* Function: curl发http请求的函数，这里与request函数不同在于
* 这里设置了json post
* Parameters:
* Return:
* Description:
****************************************************************/
char * request_json(char * url)
{
    char szJsonData[MIN_BUF_SIZE];  
    memset(szJsonData, 0, MIN_BUF_SIZE);  
    
    sprintf(szJsonData, "[{\"subject\":\"%s\"},{\"nick\":\"%s\"}]", "reg", "喔喔我");
   
    //定义CURL类型的指针
    CURL * curl = NULL; 
    //定义CURLcode类型的变量，保存返回状态码
    CURLcode res;
    struct curl_slist * headers = NULL;
    curl_global_init(CURL_GLOBAL_ALL);
    
    char * data = (char *)malloc(MAX_BUF_SIZE);
    bzero(data, MAX_BUF_SIZE);
    struct write_result write_result = {
        .data = data,
        .pos = 0
    };

    //初始化一个CURL类型的指针
    curl = curl_easy_init();
    if(curl!=NULL)
    {
        //设置curl选项. 其中CURLOPT_URL是让用户指定url
        curl_easy_setopt(curl, CURLOPT_URL, url);
        headers = curl_slist_append(headers, "content-type:application/json;charset=utf8");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, szJsonData);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_response);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &write_result);
        
        //调用curl_easy_perform 执行我们的设置.并进行相关的操作，这个函数类似 ioctl，自动触发IO
        res = curl_easy_perform(curl);
        
        // Check for errors
        if(res != CURLE_OK)
        {
            fprintf(stderr, "curl_easy_perform() failed: %s\n",  curl_easy_strerror(res));
            curl_global_cleanup();
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            return NULL;
        }
        
        //清除curl操作.
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
        curl_global_cleanup();

        // zero-terminate the result 
        data[write_result.pos] = '\0';
    }
    return data;
}


/***************************************************************
* Function: 解析http post json后返回的特定数据json
* Parameters:
* Return:
* Description:
****************************************************************/
void post_jansson_parse(char * text)
{
    json_t * json_root = NULL;
    json_t * json_arrayItem = NULL;
    json_t * json_a = NULL;
    json_t * json_b = NULL;
    json_t * json_c = NULL;
    json_t * json_d = NULL;
    json_t * json_e = NULL;
    json_error_t error;
    const char * message_text = NULL;

    json_root = json_loads(text, 0, &error);
    free(text);

    if(!json_root)
    {
        fprintf(stderr, "error: on line %d: %s\n", error.line, error.text);
        return;
    }

    //这里可以走一个分支，如果是数组，按照数组解析，如果json返回非数组，直接解析obiect即可
    if(!json_is_array(json_root))
    {
        fprintf(stderr, "error: root is not an array\n");
        json_decref(json_root);
        return;
    }
    
    //下面是按照数组解析的
    int i = 0;
    int size = json_array_size(json_root);
    puts("Jansson Parse.....");
    printf("$$ array size: %d\n", size);
    for(i=0; i<size; i++)
    {
        json_arrayItem = json_array_get(json_root, i);
        
        json_a = json_object_get(json_arrayItem, "subject");
        if( json_is_string( json_a ) ) 
        {
            message_text = json_string_value(json_a);
            printf("$$ subject: %s\n", message_text);
        }
        
        json_b = json_object_get(json_arrayItem, "nicktype");
        if( json_is_string( json_b ) ) 
        {
            message_text = json_string_value(json_b);
            printf("$$ nicktype: %s\n", message_text);
        }
        
        json_c = json_object_get(json_arrayItem, "orginick");
        if( json_is_string( json_c ) ) 
        {
            message_text = json_string_value(json_c);
            printf("$$ orginick: %s\n", message_text);
        }
        printf("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n");
    }

    json_decref(json_root);
    return;
}


/***************************************************************
* Function: main函数，这里分别使用了cjson和jansson进行了解析测试
* Parameters:
* Return:
* Description:
****************************************************************/
int main(int argc, char *argv[])
{
    char url[256] = {0};
    sprintf(url, "%s", "https://192.168.199.103/testjson/testjson.php");
    char * text = NULL;
    
    /*
    ////////////测试http带有json post的方式，post了json数据，收服务端json响应，测试行为--多行数组jansson库处理/////////////////////
    {
        text = request_json(url);
        if( text != NULL )
        {
            printf("! post json data: \n");
            printf("%s\n", text);
         }
	    else
    	{
	    	return -1;
	    }
	
	    //测试json post数据解析
	    post_jansson_parse(text);
    }
    */
    
    /*
    ////////////测试了普通的http，不带post，只收服务器回来的json数据，测试行为--多行数组的cjson库处理/////////////////////////////
    {        
        text = request(url);
        if( text != NULL )
        {
            printf("! orgi json data: \n");
            printf("%s\n", text);
        }
    	else
	    {
	    	return -1;
    	}
    	//cjson多行数组解析测试
	    cjson_parse(text);
    }
	*/
	
	/*
	////////////测试了普通的http，不带post，只收服务器回来的json数据，测试行为--多行数组的jansson库处理/////////////////////////////
	{        
        text = request(url);
        if( text != NULL )
        {
            printf("! orgi json data: \n");
            printf("%s\n", text);
        }
    	else
	    {
	    	return -1;
    	}
    	//jansson多行数组解析测试
	    jansson_parse(text);
    }
    */
    
    ////////////测试了普通的http，不带post，只收服务器回来的json数据，测试行为--单行json的jansson库处理/////////////////////////////
    {        
        text = request(url);
        if( text != NULL )
        {
            printf("! orgi json data: \n");
            printf("%s\n", text);
        }
    	else
	    {
	    	return -1;
    	}
    	//jansson单行数据解析测试
	    non_jansson_parse(text);
    }
         
    return 0;
}



