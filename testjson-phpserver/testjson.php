<?php

//多维数组的json和单个维度json，其格式是不同的，在经过json_encode处理后
//数组会变成下面两个格式字符串
//单个维度格式为：{"key":"value","key":"value"}，这一点注意和多行json数组的格式区别
//多个维度json数组格式为：[{"key":"value","key":"value"},{....}]

/***************************************************************
* Function: 返回非数组情况的json数据
* Parameters:
* Return:
* Description:
****************************************************************/
function non_array_json()
{
	//这里是拼接一个json字符串出去
	$arr = array('a'=>'1','b'=>'2','c'=>'3','d'=>'4','e'=>'5');
	$retval = json_encode($arr);
	$retval = str_replace("\n", "", $retval);
	return $retval;
}

/***************************************************************
 * Function: 返回多个数组情况的json数据
 * Parameters:
 * Return:
 * Description:
 ****************************************************************/
function mult_arrays_json()
{
	//需要执行的SQL语句
	$sql="select username, storedKey, serverKey, name, creationDate from ofUser";

	//调用conn.php文件进行数据库操作
	require('conn.php');

	$line = mysql_num_rows($result);
	$i = 0;
	
	//提示操作成功信息，注意：$result存在于conn.php文件中，被调用出来
	if( $line>0 )
	{
		while($row=mysql_fetch_assoc($result))
		{
			//这里是拼接一个json字符串出去，因为是数据库多行，所以拼接的是一个json数组
			//格式为：[{"key":"value","key":"value"},{....}]
			//json_encode函数直接输出的就是个字串，直接改就可以了
			if( $i<$line-1 ) //line-1，表示最后一行，json的最后一行末尾不要加逗号
			{
				$json = json_encode($row);
				$view = $view . $json;
				$view = str_replace("\n", "", $view);
				$view = $view . ",";
			}
			else
			{
				$json = json_encode($row);
				$view = $view . $json;
				$view = str_replace("\n", "", $view);
				$view = $view . "]";
			}
			$i++;
		}
	}
	
	mysql_free_result($result);
	mysql_close();

	//因为输出的是一个json数组，所以，必须前后都要加[]符号
	$view = str_replace("\n", "", $view);
 	$retval = "[" . $view;
   	return  $retval;
}


/***************************************************************
 * Function: 处置json post上来的数据，逻辑也很简单，看一下post数据
 * 然后设置一个二维数组为json响应，发送回去
 * Parameters:
 * Return:
 * Description:
 ****************************************************************/
function dispath_jsonpost($jsonpost)
{
	$arr = array();
	$arr[0]['subject'] = 'success';
	$arr[1]['nicktype'] = 'public';
	$arr[1]['orginick'] = urlencode($jsonpost[1]->nick);
	$retval = json_encode( $arr );
	$retval = str_replace("\n", "", $retval);
	return urldecode($retval);
}


/***************************************************************
 * Function: main 入口
 * Parameters:
 * Return:
 * Description:
 ****************************************************************/
{
	//设置http头，表示带有json数据
	header('content-type:application/json;charset=utf8');

	//测试处理post来的json数据
	//$jsonpost = json_decode( file_get_contents("php://input") );
	//$data = dispath_jsonpost( $jsonpost );
	//echo $data;

	//测试单行返回json数据
	$data = non_array_json();
	echo $data;
	
	//测试多行数组返回json数据
	//$data = mult_arrays_json();
	//echo $data;
}

?>




