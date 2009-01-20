--TEST--
Ingres: test freeing multiple resultsets in random order
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--ENV--
II_SYSTEM=/opt/Ingres/II
--FILE--
<?php

require_once('connection.inc');

$no_results = 10;

$conn=ingres_connect($database,$user,$password);

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
	echo "Connection succeeded.\n";
}

for ($i = 0; $i < $no_results; $i++) 
{
  $result[$i] = ingres_query($conn, "SELECT * FROM iitables");
  if (empty($result[$i]))
  {
    echo "An error occured :" . ingres_errno() . " " . ingres_error()."\n";
    die();
  }
}

echo "Generated $i results.\n";

shuffle($result);

for ( $i = 0; $i < $no_results; $i++) 
{
  ingres_free_result($result[$i]);

  if (ingres_errno($conn))
  {
    echo "An error occured :" . ingres_errno() . " " . ingres_error()."\n";
    die();
  }
}

echo "Freed $i results.\n";

ingres_commit($conn);
ingres_close($conn);
?>
--EXPECT--
Connection succeeded.
Generated 10 results.
Freed 10 results.
