--TEST--
Ingres: Test ingres_charset()
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

echo ingres_charset($conn);

ingres_close($conn);
?>
--EXPECT--
ISO88591
