#include "gloox/gloox.h"
#include "gloox/client.h"
#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <cstdio>
#include <iostream>
#include <fstream>

//[liuzhuan] 没用其他的数据结构，这里只用了两个命名空间，std和gloox
using namespace std;
using namespace gloox;

int main( )
{
	//[liuzhuan] 输出gloox版本号，GLOOX_VERSION为gloox中一个全局变量
	printf("Gloox Ver: %s\n", GLOOX_VERSION.c_str());

	return 0;
}

