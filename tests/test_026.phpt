--TEST--
Ingres: Adjust the date format to MULTINATIONAL and retrieve a date value.
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--ENV--
II_SYSTEM=/opt/Ingres/II
INGRES_DATE_FORMAT=INGRES_DATE_MULTINATIONAL
--FILE_EXTERNAL--
files/date_format_test.php
--EXPECT--
Connection succeeded.06/09/05 12:00:00
