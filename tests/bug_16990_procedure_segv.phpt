--TEST--
Ingres: Bug 16990 - SEGV when executing a procedure
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--ENV--
II_SYSTEM=/opt/Ingres/II
--FILE--
<?php
require_once('connection.inc');

$conn = ingres_connect("imadb", $user, $password);

if (ingres_errno())
{
    trigger_error(ingres_errno() . " - " . ingres_error());
	die("i died");
}

echo "Connected. ";
$query = "EXECUTE PROCEDURE ima_set_vnode_domain";

$res = ingres_query($conn,$query);

if (ingres_errno())
{
    trigger_error(ingres_errno() . " - " . ingres_error());
	die("i died");
}
echo "Procedure executed. ";

ingres_close($conn);

?>
--EXPECT--
Connected. Procedure executed.
