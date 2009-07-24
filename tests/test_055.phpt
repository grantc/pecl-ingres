--TEST--
Ingres: Test ingres_fetch_assoc()
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--ENV--
II_SYSTEM=/opt/Ingres/II
--FILE--
<?php

  include_once("connection.inc");

  $link = ingres_connect("demodb", $user, $password);

  if (!is_resource($link))
  {
	  trigger_error( "Error connecting - " . ingres_error() . "<br/>\r");
  }

  $query = "select * from airport where ap_iatacode = ?";
  $params[] = "VLL";

  $result = ingres_query($link, $query, $params);

  if (!is_resource($result))
  {
	  trigger_error( "Error in query - " . ingres_error() . "<br/>\r");
  }

  while ($data=ingres_fetch_assoc($result))
  {
      echo $data["ap_iatacode"]." is located in ".$data["ap_name"].", ".$data["ap_ccode"]."\n";
  }

--EXPECT--
VLL is located in Valladolid, ES
