--TEST--
Ingres: Bug 16752 - Passing zero length strings using parameter typing
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--ENV--
II_SYSTEM=/opt/Ingres/II
--INI--
ingres.describe=0
--FILE--
<?php

  include_once("connection.inc");

  $link = ingres_connect($database, $user, $password);

  if (!is_resource($link))
  {
    trigger_error(ingres_error(), E_USER_ERROR);
  }

  $sql = "create table insert_test ( idx integer not null, chr1 char(3) not null, chr2 char(1) not null)";

  $rs = ingres_query($link,$sql);

  if (!is_resource($rs))
  {
    trigger_error(ingres_error(), E_USER_ERROR);
  }

  $sql = "insert into insert_test values (?,?,?)";
  $params = array(1,"2","");
  $types = "icc";

  $rs = ingres_query($link,$sql,$params,$types);

  if (!is_resource($rs))
  {
    trigger_error(ingres_error(), E_USER_ERROR);
  }


  $sql = "select * from insert_test";
  $rs = ingres_query($link,$sql,$params);

  if (!is_resource($rs))
  {
    trigger_error(ingres_error(), E_USER_ERROR);
  }
  $row = ingres_fetch_row($rs);
  var_dump($row);


--EXPECT--
array(3) {
  [1]=>
  int(1)
  [2]=>
  string(3) "2  "
  [3]=>
  string(1) " "
}
