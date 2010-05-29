--TEST--
Ingres: connect to a database changing the effective user.
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--ENV--
II_SYSTEM=/opt/Ingres/II
--FILE--
<?php

require_once('connection.inc');

$conn=ingres_connect("iidbdb",$user,$password);

if (ingres_errno())
{
    trigger_error(ingres_errno() . " - " . ingres_error());
	die("i died");
}

$rc = ingres_query($conn, "create user phptestuser");

if (ingres_errno())
{
    trigger_error(ingres_errno() . " - " . ingres_error());
	die("i died");
}

ingres_commit($conn);
if (ingres_errno())
{
    trigger_error(ingres_errno() . " - " . ingres_error());
	die("i died");
}

ingres_close($conn);
if (ingres_errno())
{
    trigger_error(ingres_errno() . " - " . ingres_error());
	die("i died");
}

$options = array( "effective_user" => "phptestuser");

$conn=ingres_connect($database,$user,$password, $options);

if ( ingres_errno($conn) ) {
	$error_code=ingres_errno();
	$error_text=ingres_error();
	$error_sqlstate=ingres_errsqlstate();
	printf ( "Error during connect : %s\n\r",$error_code);
	printf ( "Error during connect : %s\n\r", $error_text);
	printf ( "Error during connect : %s\n\r", $error_sqlstate);
	die("i died");
} 
else
{
	echo "Connection succeeded.";
}
$rc=ingres_query($conn, "select dbmsinfo('system_user') as system_user, dbmsinfo('username') as username");

if ( ingres_errno() ) {
	$error_code=ingres_errno($conn);
	$error_text=ingres_error($conn);
	$error_sqlstate=ingres_errsqlstate($conn);
	printf ( "Error during query : %s\n\r",$error_code);
	printf ( "Error during query : %s\n\r", $error_text);
	printf ( "Error during query : %s\n\r", $error_sqlstate);
	die("i died");
} 
else
{
	while ( $object = ingres_fetch_object ($rc) ) {
		echo "The user " . $object->system_user . " is impersonating ". $object->username .".";
	}
}

ingres_close($conn);

$conn=ingres_connect("iidbdb",$user,$password);

if (ingres_errno())
{
    trigger_error(ingres_errno() . " - " . ingres_error());
	die("i died");
}

$rc = ingres_query($conn, "drop user phptestuser");

if (ingres_errno())
{
    trigger_error(ingres_errno() . " - " . ingres_error());
	die("i died");
}

ingres_commit($conn);
if (ingres_errno())
{
    trigger_error(ingres_errno() . " - " . ingres_error());
	die("i died");
}

ingres_close($conn);
if (ingres_errno())
{
    trigger_error(ingres_errno() . " - " . ingres_error());
	die("i died");
}
?>
--EXPECT--
Connection succeeded.The user ingres is impersonating phptestuser.
