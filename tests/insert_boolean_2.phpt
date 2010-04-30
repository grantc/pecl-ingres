--TEST--
Ingres: Insert a boolean value using a static query.
--SKIPIF--
<?php 
    require_once('tests/skipif.inc');
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
require_once('tests/connection.inc');

$database="demodb";

$conn = ingres_connect($database,$user,$password);

if ($conn) 
{
    echo "Connection succeeded.";
    $rc=ingres_query($conn, "create table boolean_test (idx integer not null, bool boolean not null)");
	$rc=ingres_query($conn, "insert into boolean_test values (1,true)");
    if ($rc) 
    {
		echo "Insert succeeded.";
		$param = array(1);
		$rc=ingres_query($conn, "select * from boolean_test where idx = 1");
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
