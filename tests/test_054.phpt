--TEST--
Ingres: Verify ingres.array_index_start with the field functions
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--ENV--
II_SYSTEM=/opt/Ingres/II
--FILE--
<?php

  include_once("connection.inc");
  $query = "select table_name, table_owner, create_date,alter_date,is_compressed,key_is_compressed from iitables";

  $link = ingres_connect($database, $user, $password);

  if (!is_resource($link))
  {
	  trigger_error( "Error connecting - " . ingres_error() . "<br/>\r");
  }
  $result_set = ingres_query($link, $query);
  ini_set('ingres.array_index_start',1);
  $num_fields = ingres_num_fields($result_set);
  echo ingres_field_name($result_set,ini_get('ingres.array_index_start')). ", ";
  echo ingres_field_type($result_set,ini_get('ingres.array_index_start')). ", ";
  echo ingres_field_length($result_set,ini_get('ingres.array_index_start')). "\n";
  echo ingres_field_name($result_set,$num_fields). ", ";
  echo ingres_field_type($result_set,$num_fields). ", ";
  echo ingres_field_length($result_set,$num_fields). "\n";
  ini_set('ingres.array_index_start',0);
  echo ingres_field_name($result_set,ini_get('ingres.array_index_start')). ", ";
  echo ingres_field_type($result_set,ini_get('ingres.array_index_start')). ", ";
  echo ingres_field_length($result_set,ini_get('ingres.array_index_start')). "\n";
  echo ingres_field_name($result_set,$num_fields-1). ", ";
  echo ingres_field_type($result_set,$num_fields-1). ", ";
  echo ingres_field_length($result_set,$num_fields-1). "\n";

--EXPECT--
table_name, IIAPI_CHA_TYPE, 32
key_is_compressed, IIAPI_CHA_TYPE, 1
table_name, IIAPI_CHA_TYPE, 32
key_is_compressed, IIAPI_CHA_TYPE, 1
