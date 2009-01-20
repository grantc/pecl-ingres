--TEST--
Ingres: execute a simple select against default link - fetch row.
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--ENV--
II_SYSTEM=/opt/Ingres/II
--FILE--
<?php

require_once('connection.inc');

ini_set('ingres.array_index_start',1);

$conn = ingres_connect($database,$user,$password);

if ($conn) {
    echo "Connection succeeded.";
	$rc=ingres_query($conn, "select relowner from iirelation where relid='iirelation'");
	if (is_resource($rc)) {
		echo "Query succeeded.";
		while ($row=ingres_fetch_row($rc)) {
			echo $row[1];
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
