<?php
class test{
public function exec(){
return 0;
}

public function exec1(){
return 0;
}

public function exec4(){
return 0;
}

}

class test2{
public function exec3(){
return 0;
}
}

$a = new test();
$a->exec();

$a->exec1();


$a->exec4();


$a->exec4();
$a->exec();

$a->exec1();
$a->exec();

$a->exec1();

$a = new test2();

$a->exec3();
?>
