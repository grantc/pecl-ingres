--TEST--
Ingres: Insert a boolean value using DESCRIBE INPUT.
--SKIPIF--
<?php 
    require_once('skipif.inc');
    /* We require Ingres 10.0 or higher to perform this test */
    if (INGRES_API_VERSION < 7)
    {
        die('skip');
    }
?>
--ENV--
II_SYSTEM=/opt/Ingres/II
--FILE--
<?php
require_once('connection.inc');

$database="demodb";

$conn = ingres_connect($database,$user,$password);

if ($conn) 
{
    echo "Connection succeeded.";
    $rc=ingres_query($conn, "create table boolean_test (idx integer not null, bool boolean not null)");
    $param = array(1,true);
	$rc=ingres_query($conn, "insert into boolean_test values (?,?)",$param);
    if ($rc) 
    {
		echo "Insert succeeded.";
		$param = array(1);
		$rc=ingres_query($conn, "select * from boolean_test where idx = ?",$param);
        if ($rc) 
        {

            while ( $object = ingres_fetch_object($rc)) 
            {
				echo $object->idx . " " . $object->bool;
			}
		}

	} else {
		echo "Insert failed.";
	}
    ingres_close($conn);
} else {
    echo "Connection failed.";
}

?>
--EXPECT--
Connection succeeded.Insert succeeded.1 1
