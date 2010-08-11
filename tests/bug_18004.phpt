--TEST--
Ingres: Bug #18004 - test non-associated calls to ingres_query()
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--ENV--
II_SYSTEM=/opt/Ingres/II
--FILE--
<?php

require_once('connection.inc');

$conn = ingres_connect($database,$user,$password);

if ($conn) {
	ingres_query($conn, "set lockmode session where readlock=nolock");
    if (ingres_errno())
    {
        echo ingres_errno() . " - " . ingres_error()."\n";
    }
	ingres_query($conn, "set session read write, isolation level read uncommitted");
    if (ingres_errno())
    {
        echo ingres_errno() . " - " . ingres_error()."\n";
    }
	$rc=ingres_query($conn, "select dbmsinfo('cursor_limit') as cursor_limit");
    if (ingres_errno())
    {
        echo ingres_errno() . " - " . ingres_error()."\n";
    }
    $result = ingres_fetch_object($rc);
    echo "Cursor limit is - " . $result->cursor_limit;
    ingres_close($conn);
} else {
    echo "Connection failed.";
}

?>
--EXPECTF--
Cursor limit is - %d
