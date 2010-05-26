--TEST--
Ingres: Adjust the timezone to UNITED-KINGDOM and retrieve a date value.
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--ENV--
II_SYSTEM=/opt/Ingres/II
INGRES_TIMEZONE=united-kingdom
--FILE_EXTERNAL--
files/timezone_test.php
--EXPECTREGEX--
Connection succeeded.06-sep-2005 1[23]:00:00
