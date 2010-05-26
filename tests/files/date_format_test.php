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

switch (getenv("INGRES_DATE_FORMAT"))
{
    case "INGRES_DATE_US":
        $date_format = INGRES_DATE_US;
        break;
    case "INGRES_DATE_MULTINATIONAL":
        $date_format = INGRES_DATE_MULTINATIONAL;
        break;
    case "INGRES_DATE_MULTINATIONAL4":
        $date_format = INGRES_DATE_MULTINATIONAL4;
        break;
    case "INGRES_DATE_FINNISH":
        $date_format = INGRES_DATE_FINNISH;
        break;
    case "INGRES_DATE_ISO":
        $date_format = INGRES_DATE_ISO;
        break;
    case "INGRES_DATE_ISO4":
        $date_format = INGRES_DATE_ISO4;
        break;
    case "INGRES_DATE_GERMAN":
        $date_format = INGRES_DATE_GERMAN;
        break;
    case "INGRES_DATE_MDY":
        $date_format = INGRES_DATE_MDY;
        break;
    case "INGRES_DATE_DMY":
        $date_format = INGRES_DATE_DMY;
        break;
    case "INGRES_DATE_YMD":
        $date_format = INGRES_DATE_YMD;
        break;
}

$options = array( "timezone" => "gmt",  "date_format" => $date_format );
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
