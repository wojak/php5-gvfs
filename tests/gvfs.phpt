--TEST--
GVFS Test
--SKIPIF--
<?php extension_loaded('gvfs') or die('skip'); ?>
--FILE--
<?php
$result = gvfs_info(__FILE__,"standard::content-type");
echo $result['standard::content-type'];
?>
--EXPECT--
application/x-php

