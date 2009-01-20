--TEST--
Ingres: execute a simple select against default link - fetch assoc array.
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--ENV--
II_SYSTEM=/opt/Ingres/II
--FILE--
<?php

require_once('connection.inc');

$conn = ingres_connect($database,$user,$password);

if ($conn) {
    echo "Connection succeeded.";
	$rc=ingres_query($conn, "select * from iirelation where relid='iirelation'");
	if (is_resource($rc)) {
		echo "Query succeeded.";
		while ($row=ingres_fetch_array($rc)) {
			echo $row["relowner"];
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
