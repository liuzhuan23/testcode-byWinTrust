<?php
//if ((($_FILES["file"]["type"] == "image/gif") || ($_FILES["file"]["type"] == "image/jpeg") || ($_FILES["file"]["type"] == "image/pjpeg")) && ($_FILES["file"]["size"] < 90000))
if (($_FILES["sendfile"]["size"] < 10000000))
{
	if ($_FILES["sendfile"]["error"] > 0)
	{
		echo "Return Code: " . $_FILES["sendfile"]["error"] . "<br />";
	}
	else
	{
		echo "Upload: " . $_FILES["sendfile"]["name"] . "<br />";
		echo "Type: " . $_FILES["sendfile"]["type"] . "<br />";
		echo "Size: " . ($_FILES["sendfile"]["size"] / 1024) . " Kb<br />";
		echo "Temp file: " . $_FILES["sendfile"]["tmp_name"] . "<br />";

		if (file_exists("fileshare/" . $_FILES["sendfile"]["name"]))
		{
			echo $_FILES["sendfile"]["name"] . " already exists. ";
		}
		else
		{
			move_uploaded_file($_FILES["sendfile"]["tmp_name"], "fileshare/" . $_FILES["sendfile"]["name"]);
			echo "Stored in: " . "fileshare/" . $_FILES["sendfile"]["name"];
		}
	}
}
else
{
	echo "Invalid file";
}
?>


