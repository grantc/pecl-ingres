--TEST--
Ingres: execute an update using a parameter.
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
require_once('connection.inc');

$conn = ingres_connect($database,$user,$password);

if ($conn) {
    echo "Connection succeeded.";
        $param = array(1,1.1,"Row 1");
	$rc=ingres_query($conn, "insert into param_tests values (?,?,?)",$param);
	if ($rc) {
		echo "Insert succeeded.";
		$param = array ("col2"=>2.2,"col3"=>"Row 2","col1"=> 1);
		$rc=ingres_query($conn, "update param_tests set col2=?, col3=? where col1=?",$param);

			if (ingres_errno() != 0)
			{
				echo "Update failed.";
			}
			else
			{
				echo "Update succeeded.";
			}
		$param = array(1);
		$rc=ingres_query($conn, "select * from param_tests where col1 = ?",$param);
		if ($rc) {
			while ( $object = ingres_fetch_object($rc)) {
				echo $object->col1 . " " . $object->col2 . " " . $object->col3 .".";
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
Connection succeeded.Insert succeeded.Update succeeded.1 2.2 Row 2.
