--TEST--
Ingres: Specifying the default structure for a new index.
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php

require_once('connection.inc');

$options = array( "table_structure" => "hash",  "index_structure" => "btree");

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
$rc=ingres_query("create table table_structure as select * from iirelation");

if ( ingres_errno() ) {
	$error_code=ingres_errno($conn);
	$error_text=ingres_error($conn);
	$error_sqlstate=ingres_errsqlstate($conn);
	printf ( "Error during query : %s\n\r",$error_code);
	printf ( "Error during query : %s\n\r", $error_text);
	printf ( "Error during query : %s\n\r", $error_sqlstate);
	die("i died");
} 

$rc=ingres_query("create index index_structure on table_structure(relid)");

	if ( ingres_errno() ) {
		$error_code=ingres_errno($conn);
		$error_text=ingres_error($conn);
		$error_sqlstate=ingres_errsqlstate($conn);
		printf ( "Error during query : %s\n\r",$error_code);
		printf ( "Error during query : %s\n\r", $error_text);
		printf ( "Error during query : %s\n\r", $error_sqlstate);
		die("i died");
} 

$rc=ingres_query("select * from iitables where table_name = 'index_structure'");

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
		echo "The index, index_structure has the storage structure, " . trim($object->storage_structure) . ".";
	}
}

ingres_close($conn);
?>
--EXPECT--
Connection succeeded.The index, index_structure has the storage structure, BTREE.
