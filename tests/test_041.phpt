--TEST--
Ingres: Test ingres_result_seek()/ingres_data_seek()
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--ENV--
II_SYSTEM=/opt/Ingres/II
--FILE--
<?php

require_once('connection.inc');

$database = "demodb";
/* Convert NVARCHAR to UTF-8 */
ini_set("ingres.utf8", 1);

$conn=ingres_connect($database,$user,$password);

if (!is_resource($conn)) {
    echo ingres_errno() . " - " . ingres_error . "\n";
    die("i died");
} 

$result=ingres_query($conn, "select * from airport where ap_ccode = 'ES' order by ap_place asc") ;

if ( !is_resource($result) ) {
    echo ingres_errno() . " - " . ingres_error . "\n";
    die("i died");
} 
else
{
    $row_count = ingres_num_rows($result);
    echo "$row_count rows returned\n";

    /* goto row 3 */
    if (!ingres_result_seek($result, 3))
    {
        echo ingres_errno() . " - " . ingres_error . "\n";
        die("i died");
    }
    else
    {
        $airport = ingres_fetch_object ($result);
        {
            echo $airport->ap_iatacode . " - " .  $airport->ap_name . "\n";
        }
    }
    /* goto row 1 */
    if (!ingres_result_seek($result, 1))
    {
        echo ingres_errno() . " - " . ingres_error . "\n";
        die("i died");
    }
    else
    {
        $airport = ingres_fetch_object ($result);
        {
            echo $airport->ap_iatacode . " - " .  $airport->ap_name . "\n";
        }
    }
    /* goto row 2 */
    if (!ingres_data_seek($result, 2))
    {
        echo ingres_errno() . " - " . ingres_error . "\n";
        die("i died");
    }
    else
    {
        $airport = ingres_fetch_object ($result);
        {
            echo $airport->ap_iatacode . " - " .  $airport->ap_name . "\n";
        }
    }
}

ingres_close($conn);
?>
--EXPECT--
3 rows returned
VLL - Valladolid
BCN - El Prat De Llobregat
MAD - Barajas
