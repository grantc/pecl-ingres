--TEST--
Ingres: Bug #17302 - Test for closure of statements
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--ENV--
II_SYSTEM=/opt/Ingres/II
--FILE--
<?php

$database="demodb";

$conn = ingres_connect($database);

if ($conn) 
{
    echo "Connection succeeded.";
    $rc=ingres_query($conn, "create table statement_test (idx integer not null, bool integer not null)");
    $param = array(1,1);
	$rc=ingres_query($conn, "insert into statement_test values (?,?)",$param);
    if ($rc) 
    {
		echo "Insert succeeded.";
		$param = array(1);
		$rc=ingres_query($conn, "select * from statement_test where idx = ?",$param);
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
