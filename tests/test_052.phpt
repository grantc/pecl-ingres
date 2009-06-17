--TEST--
Ingres: Test the insertion/selection of a LONG NVARCHAR
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--ENV--
II_SYSTEM=/opt/Ingres/II
--FILE--
<?php

  ini_set("ingres.describe", TRUE);
  ini_set("ingres.utf8", TRUE);
  ini_set("ingres.scrollable", FALSE);

  include_once("connection.inc");
  $file = "tests/utf8.txt";

  if (is_file($file))
  {
    $fh = fopen($file,"r");
    $text = stream_get_contents($fh);
    fclose($fh);

    $link = ingres_connect("phpdb_unicode", $user, $password);
    if (ingres_errno())
    {
        trigger_error("Connect : " . ingres_errno() . " - " - ingres_error(), E_USER_ERROR);
    }
    $rc = ingres_query($link, "create table nclob_test (idx integer not null, ntext long nvarchar not null)");
    if (ingres_errno())
    {
        trigger_error("Create table : " . ingres_errno() . " - " . ingres_error(), E_USER_ERROR);
    }
    $rc = ingres_query($link, "insert into nclob_test values (1, ?)", array($text));
    if (ingres_errno())
    {
        trigger_error("Insert : " . ingres_errno() . " - " . ingres_error(), E_USER_ERROR);
    }
    $rc = ingres_query($link, "select * from  nclob_test where idx = 1");
    if (ingres_errno())
    {
        trigger_error("Insert : " . ingres_errno() . " - " . ingres_error(), E_USER_ERROR);
    }
    $data = ingres_fetch_object($rc);
    if ($data->ntext == $text)
    {
        echo "Data In/Data Out\n";
    }
    else
    {
        echo "Garbage In/Garbage Out\n";
        echo $data->ntext;
    }
  }
  else
  {
    echo "Missing $file\n";
  }
--EXPECT--
Data In/Data Out
