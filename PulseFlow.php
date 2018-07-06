<?php
die;
print libiaofunc2();
if(!extension_loaded('PulseFlow')) {
	dl('PulseFlow.' . PHP_SHLIB_SUFFIX);
}
$module = 'PulseFlow';
$functions = get_extension_funcs($module);
echo "Functions available in the test extension:$br\n";
foreach($functions as $func) {
    echo $func."$br\n";
}
//usleep(10000);
echo "$br\n";
$function = 'confirm_' . $module . '_compiled';
if (extension_loaded($module)) {
	$str = $function($module);
} else {
	$str = "Module $module is not compiled into PHP";
}?>