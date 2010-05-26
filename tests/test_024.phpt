--TEST--
Ingres: Adjust the timezone to GMT3-AND-HALF and retrieve a date value.
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--ENV--
II_SYSTEM=/opt/Ingres/II
INGRES_TIMEZONE=GMT3-AND-HALF
--FILE_EXTERNAL--
files/timezone_test.php
--EXPECT--
Connection succeeded.06-sep-2005 15:30:00
