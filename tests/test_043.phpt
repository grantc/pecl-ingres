--TEST--
Ingres: Test SELECT using DESCRIBE INPUT
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--INI--
ingres.scrollable = FALSE
ingres.utf8 = TRUE
--ENV--
II_SYSTEM=/opt/Ingres/II
--FILE--
<?php

require_once('connection.inc');

$link = ingres_connect("demodb");

if (!is_resource($link))
{
  die( "Error connecting - " . ingres_error() . "<br/>\r");
}
else
{
    echo "Connected.";
}

$create_sql = "create table php01_tb1 (a1 c(300), a2 char(301), a3 text(302), a4 varchar(303),  a5 long varchar,  a6 nchar(304), a7 nvarchar(305), a8 long nvarchar, a9 integer, a10 smallint, a11 bigint, a12 tinyint, a13 decimal(7,2), a14 float, a15 float4, a16 ingresdate, a17 time, a18 timestamp, a19 money,a20 text, a21 text, a22 byte(306),a23 byte varying(307),  a24 long byte,  a25 ansidate, a1nn c(300) not null, a2nn char(301) not null, a3nn text(302) not null, a4nn varchar(303) not null, a5nn long varchar not null,  a6nn nchar(304) not null, a7nn nvarchar(305) not null, a8nn long nvarchar not null,  a9nn integer not null, a10nn smallint not null, a11nn bigint not null, a12nn tinyint not null, a13nn decimal(7,2) not null,a14nn float not null, a15nn float4 not null, a16nn ingresdate not null, a17nn time not null, a18nn timestamp not null, a19nn money not null,a20nn text not null, a21nn text not null, a22nn byte(306) not null, a23nn byte varying(307) not null, a24nn long byte not null,  a25nn ansidate not null)";

$rc = ingres_query($link,$create_sql);

if (ingres_errno())
{
    die(ingres_errno() . " - " . ingres_error());
}

// Parameter values
$params = array(1 => 'S','TUIR','VDWE','BGAW','QAWE','BGRT','ADFGH','IUYT',12,22,123,1,123.00,234,231,'12-12-2006','12:12:12','2006-12-12 12:12:12',123,'v','c','154','333','123','2007-01-01','C','GTRE','LIYE','WEBG','KJUY','HJYT','LOPU','NGET',1223,31,1111,5,987,765,987,'12-12-2006','12:12:12','2007-01-01 12:12:12',456,'v','v','567','307','308','2006-12-12');

$insert_sql = "insert into php01_tb1 values (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)";

$result = ingres_query($link, $insert_sql, $params);
if (ingres_errno()) {
   echo " - " . ingres_errno() . " - " . ingres_error() . "\n";
   while (ingres_next_error($result))
   {
       echo  ingres_errno() . " - " . ingres_error() . "\n";
   }
   echo "Insert Failed";
}
else
{
   echo "Insert Succeeded\n";
}
ingres_close($link);
?>
--EXPECT--
Connected.Insert Succeeded
