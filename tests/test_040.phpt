--TEST--
Ingres: Test MONEY and DATE conversion to PHP DOUBLE/STRING.
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--ENV--
II_SYSTEM=/opt/Ingres/II
--FILE--
<?php

require_once('connection.inc');

$options = array( "timezone" => "gmt", "date_format" => INGRES_DATE_FINNISH );

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
$rc=ingres_query($conn, "select money(100.22) as money1, date('01/20/2009 12:00:00') as date1, money(100.22-50.02) as money2, date('01/20/2009 12:00:00') - '1 day' as date2  from timezone_test") ;

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
		echo $object->money1 . " : ";
		echo $object->date1;
		echo " - " . $object->money2 . " : ";
		echo $object->date2;
	}
}

ingres_close($conn);
?>
--EXPECT--
Connection succeeded.100.22 : 2009-01-20 20:00:00 - 50.2 : 2009-01-19 20:00:00
