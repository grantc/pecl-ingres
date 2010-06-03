--TEST--
Ingres: Bug #17556 - test for error handling in non-result returning statements
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--ENV--
II_SYSTEM=/opt/Ingres/II
--FILE--
<?php

require_once('connection.inc');

$conn = ingres_connect($database,$user,$password);

if ($conn) {
	$rc=ingres_query($conn, "DELETE from missing_table");
    if (ingres_errno())
    {
        echo ingres_errno() . " - " . ingres_error()."\n";
    }
    ingres_close($conn);
} else {
    echo "Connection failed.";
}

?>
--EXPECT--
2117 - Table 'missing_table' does not exist or is not owned by you.
