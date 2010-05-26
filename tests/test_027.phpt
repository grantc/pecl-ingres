--TEST--
Ingres: Adjust the date format to MULTINATIONAL4 and retrieve a date value.
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--ENV--
II_SYSTEM=/opt/Ingres/II
INGRES_DATE_FORMAT=INGRES_DATE_MULTINATIONAL4
--FILE_EXTERNAL--
files/date_format_test.php
--EXPECT--
Connection succeeded.06/09/2005 12:00:00
