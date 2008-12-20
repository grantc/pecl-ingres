--TEST--
Ingres: Setting money format to € in the trailing position.
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php

require_once('connection.inc');

$options = array( "money_sign" => "€", "money_lort" => INGRES_MONEY_TRAILING );

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
$rc=ingres_query($conn, "select varchar(money(1.99)) as money");

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
		echo trim($object->money);
	}
}

ingres_close($conn);
?>
--EXPECT--
Connection succeeded.€1.99
