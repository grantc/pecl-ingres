--TEST--
Ingres: Test reflection against ingres_connect()
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--ENV--
II_SYSTEM=/opt/Ingres/II
--FILE--
<?php
$function = new ReflectionFunction('ingres_connect');

// Print out basic information
printf("-- The %s function %s\n",
    $function->isInternal() ? 'internal' : 'user-defined',
    $function->getName()
);

// Dump each parameter
foreach ($function->getParameters() as $i => $param) {
    printf(
        "--  Parameter #%d: %s {\n".
        "    Allows NULL: %s\n".
        "    Passed to by reference: %s\n".
        "    Is optional?: %s\n".
        "}\n",
        $i, // $param->getPosition() can be used from PHP 5.2.3
        $param->getName(),
        var_export($param->allowsNull(), 1),
        var_export($param->isPassedByReference(), 1),
        $param->isOptional() ? 'yes' : 'no'
    );
}
?>
--EXPECT--
-- The internal function ingres_connect
--  Parameter #0: database {
    Allows NULL: false
    Passed to by reference: false
    Is optional?: no
}
--  Parameter #1: username {
    Allows NULL: false
    Passed to by reference: false
    Is optional?: yes
}
--  Parameter #2: password {
    Allows NULL: false
    Passed to by reference: false
    Is optional?: yes
}
--  Parameter #3: options {
    Allows NULL: false
    Passed to by reference: false
    Is optional?: yes
}
