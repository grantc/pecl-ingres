--TEST--
Ingres: Adjust the timezone to NA-EASTERN and retrieve a date value.
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--ENV--
II_SYSTEM=/opt/Ingres/II
INGRES_TIMEZONE=na-eastern
--FILE_EXTERNAL--
files/timezone_test.php
--EXPECTREGEX--
Connection succeeded.06-sep-2005 0[78]:00:00
