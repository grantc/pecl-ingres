--TEST--
Ingres: Test select with ingres_prepare()/ingres_execute()
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
$stmt = ingres_prepare($conn, 'select rtrim(varchar(table_name)) as table_name from iitables where table_name = ?');
if (!is_resource($stmt))
{
    die("prepare ". ingres_error($conn));
}
$params = array("iitables");
$rc = ingres_execute($stmt,$params);
if (ingres_errno())
{
    die("Execute - " . ingres_error($conn));
}
while ($row = ingres_fetch_array($stmt,INGRES_ASSOC))
{
    echo trim($row["table_name"])."\n";
}
//ingres_free_result($stmt);
//ingres_close($conn);
?>
--EXPECT--
iitables
