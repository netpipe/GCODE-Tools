<?php

//if ($file!=''){}
require('rttl.class.php');
  //  $fname =$argv
if ($argv!=''){
    $fname = $argv[1];  //t.mid";
    echo $argv[0];
    echo $argv[1];
    $fhandle = fopen("./t.mid", "r") or die("Unable to open file!");
   # $tmidi = fopen("test.txt", "r") or die("Unable to open file!");
    $content = fread($fhandle,filesize($fname));

//$rttlStr = $rttl->getRttl();

//$fn = glob(__DIR__ . "/../../midifiles/*.mid");
//shuffle($files);
//$file = $files[0];

  // echo fread($file,filesize("t.mid"));
   // $fn= fread($file,filesize("t.mid"));
	//$fn = $_FILES['mid_upload']['name'];
	$bn = strtok($content, '.');
   // fclose($myfile);



	$midi = new Rttl();
	$midi->importMid($fname);

    $test =$midi->getRttl($bn);
echo $test;
/* RTTL converted from: <?=$fn?> */
}
?>

