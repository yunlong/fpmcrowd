<?php

echo strtotime('-15 day')."\n";
echo strtotime('now')."\n";
$str = "yunlong.lee@163.com; balancesli@gmail.com; longtails@163.com";
$arr = split('[,|;]', $str);
echo strtotime("now");

print_r($arr);
?>
