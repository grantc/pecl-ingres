<?php

require_once('connection.inc');

/* Insert base GMT value for reference */
$options = array("timezone" => "GMT");

$conn=ingres_connect($database,$user,$password,$options);

if ( ingres_errno($conn) ) {
    trigger_error(ingres_errno()."-".ingres_error(),E_USER_ERROR);
} 

$rc=ingres_query($conn, "create table timezone_test(idx integer not null, date date not null)");
if ( ingres_errno($conn) ) {
    trigger_error(ingres_errno()."-".ingres_error(),E_USER_ERROR);
}

$rc=ingres_query($conn, "insert into timezone_test values (1,'06-sep-2005 12:00:00')");
if ( ingres_errno($conn) ) {
    trigger_error(ingres_errno()."-".ingres_error(),E_USER_ERROR);
}

ingres_commit($conn);
if ( ingres_errno($conn) ) {
    trigger_error(ingres_errno()."-".ingres_error(),E_USER_ERROR);
}

ingres_close($conn);
if ( ingres_errno($conn) ) {
    trigger_error(ingres_errno()."-".ingres_error(),E_USER_ERROR);
} 

$options = array( "timezone" => getenv("INGRES_TIMEZONE"));
$conn=ingres_connect($database,$user,$password,$options);

if ( ingres_errno($conn) ) {
    trigger_error(ingres_errno()."-".ingres_error(),E_USER_ERROR);
} 
else
{
	echo "Connection succeeded.";
}

$rc=ingres_query($conn, "select * from timezone_test");

if ( ingres_errno() ) {
    trigger_error(ingres_errno()."-".ingres_error(),E_USER_ERROR);
} 
else
{
	while ( $object = ingres_fetch_object ($rc) ) {
		echo $object->date;
	}
}

ingres_free_result($rc);

$rc=ingres_query($conn, "drop table timezone_test");
if ( ingres_errno($conn) ) {
    trigger_error(ingres_errno()."-".ingres_error(),E_USER_ERROR);
}

ingres_commit($conn);
if ( ingres_errno($conn) ) {
    trigger_error(ingres_errno()."-".ingres_error(),E_USER_ERROR);
}

ingres_close($conn);

?>
