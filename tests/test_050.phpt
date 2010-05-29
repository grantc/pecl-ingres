--TEST--
Ingres: Bug 16541 - Binding UTF-8 data causes an additional NULL to be sent to the server
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--ENV--
II_SYSTEM=/opt/Ingres/II
--FILE--
<?php
ini_set("ingres.utf8", TRUE);
ini_set("ingres.describe", TRUE);

require_once("connection.inc");

function add_airport($link, $airport_details)
{
    /* Insert a new airport */
    $insert = "insert into airport values (?,?,?,?,?)";
    $result = ingres_query($link, $insert, $airport_details);
    if (ingres_errno())
    {
        die(ingres_errno() . " - " . ingres_error() . "\n");
    }
}

function get_airport($link, $airport_code)
{
    /* Select data from the table */
    $select = "select * from airport where ap_iatacode = ?";
    $select_params = array("ap_iatacode" => $airport_code);

    $result = ingres_query($link, $select, $select_params);

    if (is_resource($result))
    {
        return ingres_fetch_object($result);
    }
    else
    {
        echo ingres_errno() . " - " . ingres_error() . "\n";
    }
}

$link = ingres_connect($database, $user, $password);
if (!is_resource($link))
{
    die(ingres_errno() . " - " . ingres_error() . "\n");
}

/* array keys are not needed but are useful */
$insert_params["ap_id"] = 406;             # integer
$insert_params["ap_iatacode"] = "OVD";     # nchar           3
$insert_params["ap_place"] = "Asturias";   # nvarchar       30
$insert_params["ap_name"] = "Olviedo";     # nvarchar       50
$insert_params["ap_ccode"] = "ES";         # nchar           2

add_airport($link, $insert_params);
$airport = get_airport($link, $insert_params["ap_iatacode"]);
echo bin2hex($airport->ap_place) . ":" . $airport->ap_place . " - " . bin2hex($airport->ap_name) . ":" . $airport->ap_name . "\n";

ingres_close($link);

?>
--EXPECT--
4173747572696173:Asturias - 4f6c766965646f:Olviedo
