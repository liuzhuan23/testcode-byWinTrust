/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2012, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at http://curl.haxx.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/ 

//cc -o testcu testcurl.c -lcurl

// 上传文件主要利用的是 http 的 post 请求，函数为 curl_formadd 
// curl_formadd 主要是模拟表单，使用了http的Multipart/form-data 这个ConType，通过下面这个抓包数据
// 可以看到在tcp三次握手后，第一个http POST包中就构造了这种类型的http头，这种类型的http头专门用于post上传文件
// 我这里只是模拟了一个html的表单操作，实际上该表单并不存在文件，sendfile的html标签（带入php $_FILE数组），submit提交
// 表单动作，也只是一个模拟动作，curl_formadd函数它可以添加普通的name-value html section，也可以添加file upload html section
//0040  24 c4 50 4f 53 54 20 2f  75 70 6c 6f 61 64 2f 75   $.POST / upload/u
//0050  70 6c 6f 61 64 5f 66 69  6c 65 2e 70 68 70 20 48   pload_fi le.php H
//0060  54 54 50 2f 31 2e 31 0d  0a 48 6f 73 74 3a 20 31   TTP/1.1. .Host: 1
//0070  39 32 2e 31 36 38 2e 31  39 39 2e 32 32 36 0d 0a   92.168.1 99.226..
//0080  41 63 63 65 70 74 3a 20  2a 2f 2a 0d 0a 43 6f 6e   Accept:  */*..Con
//0090  74 65 6e 74 2d 4c 65 6e  67 74 68 3a 20 37 31 30   tent-Len gth: 710
//00a0  30 31 35 0d 0a 45 78 70  65 63 74 3a 20 31 30 30   015..Exp ect: 100
//00b0  2d 63 6f 6e 74 69 6e 75  65 0d 0a 43 6f 6e 74 65   -continu e..Conte
//00c0  6e 74 2d 54 79 70 65 3a  20 6d 75 6c 74 69 70 61   nt-Type:  multipa
//00d0  72 74 2f 66 6f 72 6d 2d  64 61 74 61 3b 20 62 6f   rt/form- data; bo
//00e0  75 6e 64 61 72 79 3d 2d  2d 2d 2d 2d 2d 2d 2d 2d   undary=- --------
//00f0  2d 2d 2d 2d 2d 2d 2d 2d  2d 2d 2d 2d 2d 2d 2d 2d   -------- --------
//0100  2d 2d 2d 37 33 35 66 66  36 61 39 35 30 64 61 0d   ---735ff 6a950da.

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <curl/curl.h>

int upload_file()
{
    CURL * curl;
    CURLcode res;
    
    struct curl_httppost * formpost=NULL;
    struct curl_httppost * lastptr=NULL;  
    struct curl_slist * headerlist=NULL;  
    
    //用于构造Expect:-100问询，该问询作用是续传大于1024字节的文件
    static const char buf[] = "Expect:";
  
    curl_global_init(CURL_GLOBAL_ALL);
    
    //sendfile为html表单中的变量（type=file, name=sendfile），获得文件名, 文件名必须为全路径
    //sendfile会传递至php中的 _$FILE[] 数组，sendfile作为标签数组的一个标签出现
    curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "sendfile", CURLFORM_FILE, "/home/leon/桌面/case/testcu/help.pdf", CURLFORM_CONTENTTYPE, "application/octet-stream", CURLFORM_END);
  
    /* Fill in the submit field too, even if this is rarely needed */
    //submit为html表单中变量名字（type=submit, name=OK）
    curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "submit", CURLFORM_COPYCONTENTS, "OK", CURLFORM_END);
    
    curl = curl_easy_init();
    /* initalize custom header list (stating that Expect: 100-continue is not wanted */  
    headerlist = curl_slist_append(headerlist, buf);
    if(curl) 
    {  
        //设置跳过验证证书和验证证书内的主机
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);

        /* what URL that receives this POST */ 
        curl_easy_setopt(curl, CURLOPT_URL, "https://192.168.199.103/upload/upload_file.php");
        curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost); //这里把 formpost n内的数据，关联进了curl变量
        
        /* Perform the request, res will get the return code */  
        res = curl_easy_perform(curl); //curl所有的io，都是这个接口，类似IOCTL自动触发IO操作
        /* Check for errors */  
        if(res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n",  
        
        curl_easy_strerror(res));
        
        /* always cleanup */
        curl_easy_cleanup(curl);
        
        /* then cleanup the formpost chain */
        curl_formfree(formpost);
        
        /* free slist */
        curl_slist_free_all (headerlist);
    }
    
    return 0;
}


/***************************************************************
* Function: 下载文件的钩子函数
* Parameters:
* Return:
* Description:
****************************************************************/
static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
  size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
  return written;
}


/***************************************************************
* Function: curl下载操作
* Parameters:
* Return:
* Description:
****************************************************************/
static int download_file()
{
    CURL * curl_handle;
    static const char * pagefilename = "curl.out";
    FILE * pagefile;
    
    char * url = "https://192.168.199.103/upload/fileshare/help.pdf";
 
    curl_global_init(CURL_GLOBAL_ALL);
    curl_handle = curl_easy_init();
 
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_data);
 
    pagefile = fopen(pagefilename, "wb");
    if (pagefile) 
    {
         //设置跳过验证证书和验证证书内的主机
        curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0);
        curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, pagefile);
        curl_easy_perform(curl_handle);
        fclose(pagefile);
    }

    curl_easy_cleanup(curl_handle);
}


/***************************************************************
 * Function: curl下载操作
 * Parameters:
 * Return:
 * Description:
 ****************************************************************/
static int TEST_REST_API()
{
	CURL * curl;
	CURLcode res;
 
	curl_global_init(CURL_GLOBAL_ALL);

	curl = curl_easy_init();
  	if(curl) 
	{
		//curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.199.225:9090/plugins/restapi/userservice?type=add&secret=JsEBwzx48Yo83j5R&username=kafka&password=drowssap&name=franz&email=franz@kafka.com");

		curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.199.225:9090/plugins/userService/userservice?type=add&secret=9M55JGwN&username=kafka&password=drowssap&name=franz&email=franz@kafka.com");

		res = curl_easy_perform(curl);

	    if(res != CURLE_OK)
		{
      		fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
		}
 
    	curl_easy_cleanup(curl);
  	}

	curl_global_cleanup();
}

 
/***************************************************************
* Function: main函数，这里分别使用了上传，下载的curl进行测试
* Parameters:
* Return:
* Description:
****************************************************************/
int main()
{
    //upload_file();
    
    //download_file();

	TEST_REST_API();
    return 0;
}




