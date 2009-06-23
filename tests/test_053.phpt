--TEST--
Ingres: Execute a procedure using parameters
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--ENV--
II_SYSTEM=/opt/Ingres/II
--FILE--
<?php

  include_once("connection.inc");
  $link = ingres_connect("demodb", $user, $password);
  if (ingres_errno())
  {
      trigger_error("Connect : " . ingres_errno() . " - " - ingres_error(), E_USER_ERROR);
  }

  $result = ingres_query($link, "execute procedure get_my_airports(?,?)", array("ccode" => "ES", "area"=>"V%"));

  if ( ingres_errno() != 0 )
  {
      $error_text = ingres_error();
      echo($error_text);
  }
  else
  {
      $airport = ingres_fetch_object($result);
      echo "Aiport $airport->col1 is in $airport->col2\n";
  }
--EXPECT--
Aiport VLL is in Valladolid
