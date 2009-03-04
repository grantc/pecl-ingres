--TEST--
Ingres: Bug #15586 - test for a new line at the start of a query
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--ENV--
II_SYSTEM=/opt/Ingres/II
--FILE--
<?php

require_once('connection.inc');

$sql = <<<EOSQL

 select * from iirelation where relid='iirelation'
EOSQL;
$conn = ingres_connect($database,$user,$password);

if ($conn) {
    echo "Connection succeeded.";
	$rc=ingres_query($conn, $sql);
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
