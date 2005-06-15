--TEST--
Ingres: execute a simple select against default link - fetch object.
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php

require_once('connection.inc');

$conn = ingres_connect($database,$user,$password);

if ($conn) {
    echo "Connection succeeded.";
	$rc=ingres_query("select * from iirelation where relid='iirelation'");
	if ($rc) {
		echo "Query succeeded.";
		while ($object=ingres_fetch_object($rc)) {
			echo $object->relowner;
		}
	} else {
		echo "Query failed.";
	}
    ingres_close($conn);
} else {
    echo "Connection failed.";
}

?>
--EXPECT--
Connection succeeded.Query succeeded.$ingres
