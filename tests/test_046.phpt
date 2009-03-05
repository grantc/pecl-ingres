--TEST--
Ingres: Test ingres_unbuffered_query() with a single statement
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--ENV--
II_SYSTEM=/opt/Ingres/II
--FILE--
<?php

require_once('connection.inc');

$conn=ingres_connect($database,$user,$password);

if (!is_resource($conn)) {
    echo ingres_errno() . " - " . ingres_error . "\n";
    die("i died");
} 

$result = ingres_unbuffered_query($conn, "select date('01-Jan-1970') as date");

if (!is_resource($result)) {
    echo ingres_errno() . " - " . ingres_error() . "\n";
    die("i died");
}
else
{
    $obj=ingres_fetch_object($result);
    if (ingres_errno())
    {
        echo ingres_errno() . " - " . ingres_error(). "\n";
    }
    else
    {
        echo $obj->date;
    }
    ingres_free_result($result);
}

ingres_close($conn);
?>
--EXPECT--
01-jan-1970
