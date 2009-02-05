--TEST--
Ingres: Test ingres_escape_string()
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--ENV--
II_SYSTEM=/opt/Ingres/II
--FILE--
<?php

require_once('connection.inc');

$link = ingres_connect($database, $user, $password);

if (!is_resource($link))
{
  die( "Error connecting - " . ingres_error() . "<br/>\r");
}

$text = "this is xyz some xyz text";

$testdata = array ( 1 => "\x00", "'");

for ( $pos = 1; $pos <= count($testdata); $pos++)
{
  $text_value = str_replace("xyz", $testdata[$pos], $text);
  echo addslashes($text_value). " becomes ";
  $new_text = ingres_escape_string ($link, $text_value);
  echo $new_text. "\n";
}
ingres_close($link);
?>
--EXPECT--
this is \0 some \0 text becomes this is '+x'00'+' some '+x'00'+' text
this is \' some \' text becomes this is '' some '' text
