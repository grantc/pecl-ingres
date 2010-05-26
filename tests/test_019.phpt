--TEST--
Ingres: Adjust the timezone to NA-PACIFIC and retrieve a date value.
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--ENV--
II_SYSTEM=/opt/Ingres/II
INGRES_TIMEZONE=na-pacific
--FILE_EXTERNAL--
files/timezone_test.php
--EXPECTREGEX--
Connection succeeded.06-sep-2005 0[45]:00:00
