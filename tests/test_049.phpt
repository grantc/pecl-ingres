--TEST--
Ingres: Test insert/select with ingres_prepare()/ingres_execute()
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--ENV--
II_SYSTEM=/opt/Ingres/II
--FILE--
<?php

ini_set("ingres.cursor_mode", INGRES_CURSOR_READONLY);
require_once('connection.inc');

$conn=ingres_connect($database,$user,$password);
if (!is_resource($conn)) 
{
    die("connect ". ingres_errno(). " - " . ingres_error($conn));
}
//Setup table for inserts
$ct = <<<CT
create table prepared ( idx integer not null, txt_value varchar (100))
CT;

$rc = ingres_query($conn, $ct);
if (!$rc)
{
    die (ingres_error()."\n");
}
$stmt = ingres_prepare($conn, 'insert into prepared values (?,?)');
if (!is_resource($stmt))
{
    die("prepare ". ingres_error($conn));
}

$params[]=array(1,"10");
$params[]=array(2,"20");
$params[]=array(3,"30");
$params[]=array(4,"40");
$params[]=array(5,"50");
$params[]=array(6,"60");
$params[]=array(7,"70");
$params[]=array(8,"80");
$params[]=array(9,"90");
$params[]=array(10,"100");
$params[]=array(11,"110");
$params[]=array(12,"120");
$params[]=array(13,"130");
$params[]=array(14,"140");

foreach ($params as $param)
{
    if (!ingres_execute($stmt,$param))
    {
        die("Execute - " . ingres_error($link));
    }
}
$stmt = ingres_prepare($conn, 'select * from prepared where idx = ?');
if (!is_resource($stmt))
{
    die("prepare ". ingres_error($conn));
}
$params = array(5);
$rc = ingres_execute($stmt,$params);
if (ingres_errno())
{
    die("Execute - " . ingres_error($conn));
}
while ($row = ingres_fetch_array($stmt,INGRES_ASSOC))
{
    echo $row["txt_value"];
}
ingres_close($conn);
?>
--EXPECT--
50
