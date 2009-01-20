--TEST--
Ingres: connect to a database as current user
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--ENV--
II_SYSTEM=/opt/Ingres/II
--FILE--
<?php

require_once('connection.inc');

$conn = ingres_connect($database,"","");

if ($conn) {
    echo "Connection succeeded.";
    ingres_close($conn);
}
else {
    echo "Connection failed - " . ingres_errno() . " - " . ingres_error();
}

?>
--EXPECT--
Connection succeeded.
