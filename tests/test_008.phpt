--TEST--
Ingres: execute an insert using a parameter.
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
require_once('connection.inc');

$conn = ingres_connect($database,$user,$password);

if ($conn) {
    echo "Connection succeeded.";
    	$rc=ingres_query($conn, "delete from param_tests",$conn);
        $param = array(1,1.1,"Row 1");
	$rc=ingres_query($conn, "insert into param_tests values (?,?,?)",$param);
	if ($rc) {
		echo "Insert succeeded.";
		$param = array(1);
		$rc=ingres_query($conn, "select * from param_tests where col1 = ?",$param);
		if ($rc) {
			while ( $object = ingres_fetch_object($rc)) {
				echo $object->col1 . " " . $object->col2 . " " . $object->col3;
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
Connection succeeded.Insert succeeded.1 1.1 Row 1
