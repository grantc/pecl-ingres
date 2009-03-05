--TEST--
Ingres: Test ingres_unbuffered_query() with two statements
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
// The first result will be deleted by the second call to ingres_unbuffered_query()
$result2 = ingres_unbuffered_query($conn, "select relowner from iirelation where relid = 'iirelation'");

if (!is_resource($result2)) {
    if (ingres_errno())
    {
        echo ingres_errno() . " - " . ingres_error() . "\n";
        die("i died");
    }
}
else
{
    $obj=ingres_fetch_object($result2);
    if (ingres_errno())
    {
        echo ingres_errno() . " - " . ingres_error(). "\n";
    }
    else
    {
        echo $obj->relowner;
    }
    ingres_free_result($result2);
}

ingres_close($conn);
?>
--EXPECT--
$ingres
