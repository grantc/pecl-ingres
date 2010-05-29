--TEST--
Ingres: Test insert using DESCRIBE INPUT
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--INI--
ingres.array_index_start=1
ingres.scrollable=FALSE
ingres.utf8=TRUE
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

$sql_table = "select * from php01_tb1";

$result = ingres_query($link,$sql_table);

if ( ingres_errno() )
{
   die ( ingres_errno() . " - " . ingres_error() . "\n" );
}

$no_columns = ingres_num_fields($result);

for ($field = 1; $field < $no_columns + 1; $field++)
{
   $metadata[$field] = array("name" => ingres_field_name($result, $field), "type" => ingres_field_type($result, $field));
}

for ($field = 1; $field < $no_columns + 1; $field++)
{
   /* These types fail or cannot be selected */
   if (($metadata[$field]["type"] != "IIAPI_LBYTE_TYPE") &&
       ($metadata[$field]["type"] != "IIAPI_LVCH_TYPE") &&
       ($metadata[$field]["type"] != "IIAPI_LNVCH_TYPE"))
   {
       echo $metadata[$field]["name"] . " - ";
       echo $params[$field] . " - ";
       echo $metadata[$field]["type"];
       $sql = "select ".$metadata[$field]["name"]." from php01_tb1 where " . $metadata[$field]["name"] . " = ?";
       $result = ingres_query($link, $sql, array($params[$field]));
       if (!is_resource($result))
       {
           if (ingres_errno()) {
               echo " - " . ingres_errno() . " - " . ingres_error() . "\n";
               while (ingres_next_error($result))
               {
                   echo  ingres_errno() . " - " . ingres_error() . "\n";
               }

           }
           die( "Failed " . $metadata[$field]["name"] . " - " . $metadata[$field]["type"] . " - " . $params[$field]);
       }
       else
       {
           $row = ingres_fetch_row($result);
           echo " - " . trim($row[1]);
           if ($row)
           {
               echo " Passed\n";
           }
           else
           {
               echo "..\n";
           }
           ingres_free_result($result);
       }
   }
}
ingres_rollback($link);
ingres_close($link);
?>
--EXPECT--
Connected.Insert Succeeded
a1 - S - IIAPI_CHR_TYPE - S Passed
a2 - TUIR - IIAPI_CHA_TYPE - TUIR Passed
a3 - VDWE - IIAPI_TXT_TYPE - VDWE Passed
a4 - BGAW - IIAPI_VCH_TYPE - BGAW Passed
a6 - BGRT - IIAPI_NCHA_TYPE - BGRT Passed
a7 - ADFGH - IIAPI_NVCH_TYPE - ADFGH Passed
a9 - 12 - IIAPI_INT_TYPE - 12 Passed
a10 - 22 - IIAPI_INT_TYPE - 22 Passed
a11 - 123 - IIAPI_INT_TYPE - 123 Passed
a12 - 1 - IIAPI_INT_TYPE - 1 Passed
a13 - 123 - IIAPI_DEC_TYPE - 123 Passed
a14 - 234 - IIAPI_FLT_TYPE - 234 Passed
a15 - 231 - IIAPI_FLT_TYPE - 231 Passed
a16 - 12-12-2006 - IIAPI_DTE_TYPE - 12-dec-2006 Passed
a17 - 12:12:12 - IIAPI_TMWO_TYPE - 12:12:12 Passed
a18 - 2006-12-12 12:12:12 - IIAPI_TSWO_TYPE - 2006-12-12 12:12:12.000000 Passed
a19 - 123 - IIAPI_MNY_TYPE - 123 Passed
a20 - v - IIAPI_TXT_TYPE - v Passed
a21 - c - IIAPI_TXT_TYPE - c Passed
a22 - 154 - IIAPI_BYTE_TYPE - 154 Passed
a23 - 333 - IIAPI_VBYTE_TYPE - 333 Passed
a25 - 2007-01-01 - IIAPI_ADATE_TYPE - 2007-01-01 Passed
a1nn - C - IIAPI_CHR_TYPE - C Passed
a2nn - GTRE - IIAPI_CHA_TYPE - GTRE Passed
a3nn - LIYE - IIAPI_TXT_TYPE - LIYE Passed
a4nn - WEBG - IIAPI_VCH_TYPE - WEBG Passed
a6nn - HJYT - IIAPI_NCHA_TYPE - HJYT Passed
a7nn - LOPU - IIAPI_NVCH_TYPE - LOPU Passed
a9nn - 1223 - IIAPI_INT_TYPE - 1223 Passed
a10nn - 31 - IIAPI_INT_TYPE - 31 Passed
a11nn - 1111 - IIAPI_INT_TYPE - 1111 Passed
a12nn - 5 - IIAPI_INT_TYPE - 5 Passed
a13nn - 987 - IIAPI_DEC_TYPE - 987 Passed
a14nn - 765 - IIAPI_FLT_TYPE - 765 Passed
a15nn - 987 - IIAPI_FLT_TYPE - 987 Passed
a16nn - 12-12-2006 - IIAPI_DTE_TYPE - 12-dec-2006 Passed
a17nn - 12:12:12 - IIAPI_TMWO_TYPE - 12:12:12 Passed
a18nn - 2007-01-01 12:12:12 - IIAPI_TSWO_TYPE - 2007-01-01 12:12:12.000000 Passed
a19nn - 456 - IIAPI_MNY_TYPE - 456 Passed
a20nn - v - IIAPI_TXT_TYPE - v Passed
a21nn - v - IIAPI_TXT_TYPE - v Passed
a22nn - 567 - IIAPI_BYTE_TYPE - 567 Passed
a23nn - 307 - IIAPI_VBYTE_TYPE - 307 Passed
a25nn - 2006-12-12 - IIAPI_ADATE_TYPE - 2006-12-12 Passed

