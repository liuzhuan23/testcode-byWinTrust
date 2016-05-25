<?php

require_once ('lib/OpenFireUserService.php');

/***************************************************************
 * Function: 
 * Parameters:
 * Return:
 * Description:
 ****************************************************************/
function test_reg()
{
	//Create the OpenFireUserService object.
	$pofus = new OpenFireUserService();

	//Set the required config parameters
	$pofus->secret = "N9LLX0ks";
	$pofus->host = "192.168.199.103";
	//default 9090 http端口，https的是9091
	$pofus->port = "9091";

	//Optional parameters (showing default values)
	$pofus->useCurl = true;
	$pofus->useSSL = true;
	//plugin folder location，类的根名就是userService，不要和目录混淆, 最好可以看看每一个插件的类根名称
	$pofus->plugin = "/plugins/userService/userservice";

	//Add a new user to OpenFire
	$result = $pofus->addUser('liuzhuantest', '123456', '刘钻', 'email@email.tld.cn');

	//Check result if command is succesful
	if($result) 
	{
		//Display result, and check if it's an error or correct response
		echo ($result['result']) ? 'Success: ' : 'Error: ';
		echo $result['message'];
	}
}


/***************************************************************
 * Function: main
 * Parameters:
 * Return:
 * Description:
 ****************************************************************/
{
	test_reg();
}

?>


