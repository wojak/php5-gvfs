--TEST--
gost
--SKIPIF--
<?php extension_loaded('gvfs') or die('skip'); ?>
--FILE--
<?php
echo gvfs_info(__FILE__);
?>
--EXPECT--
application/x-php

