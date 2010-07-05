--TEST--
Ingres: Test SQL comments with embedded parameter markers
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--ENV--
II_SYSTEM=/opt/Ingres/II
--FILE--
<?php

  include_once("connection.inc");

  $link = ingres_connect($database, $user, $password);

  $drop_query=<<<EOSQL
drop  table sometable
EOSQL;
   $create_query=<<<EOSQL
create table sometable (
col1 varchar(60),
col2 varchar(60),
col3 varchar(60),
col4 varchar(60),
col5 varchar(60),
col6 varchar(60)
)
EOSQL;

  $sql_query=<<<EOSQL
select 
    /* do we need more columns? qmark here! */ col1, col2 as "/* comment in delim?*/"
from 
    sometable
where
/*
Multi line comment here?
*/
    col3 = ?
AND col4 = '/* start of comment marker only in string literal?'
AND col5 = ?
AND col6 = 'close comment marker only in string?*/'
EOSQL;

  $rc = ingres_query($link, $drop_query);
  $rc = ingres_query($link, $create_query);
  if (ingres_errno())
  {
    trigger_error("CREATE :" . ingres_errno() . "-" . ingres_error(),E_USER_WARNING);
  }
  echo "Execute SELECT with parameters\n";
  $rc = ingres_query($link, $sql_query, array("param1","param2"));
  if (ingres_errno())
  {
     trigger_error("SELECT :" . ingres_errno() . "-" . ingres_error(),E_USER_WARNING);
  }
  else
  {
    echo "Success";
  }
  ingres_close($link);

?>
--EXPECT--
Execute SELECT with parameters
Success
