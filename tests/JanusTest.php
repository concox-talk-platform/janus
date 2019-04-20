#!/usr/bin/php -q
<?php
require "JanusObject.php";

if (!isset($argv[1]) || empty($argv[1]) || !isset($argv[2]) || empty($argv[2]))
	die("USAGE: $argv[0] server room\n");

$server = $argv[1];//eg. 127.0.0.1:1234
$janus = new Janus("http://$server/janus",'',1,true);

// Connect
$janus->connect();
$handle = $janus->attach('janus.plugin.pocroom');

// Create a room
if ($handle)
{
	// $room = rand (2000,9000);
	// /*$params = array('description' => "Test Room $room",
	// 				'bitrate' => 0,
	// 				'publishers' => 6,
	// 				//'rec_dir' => "/tmp",
	// 				//'record' => true,
	// 				//'secret' => $janus->gRS(20),
	// 				//'pin' => $janus->gRS(5),
	// 				);
	// if ($ret = $janus->createRoom($room,$params)) */
	// if ($ret = $janus->easyRoom("Test Room $room","/tmp",$room))
	// 	echo "Room $ret created!\n\n";
	// else
	// 	echo "Could not create room!\n\n";

	//join demo room
	$testRoom = $argv[2];
	$janus->join($testRoom);

	//keep-alive
	while (true)
	{
		$janus->keepAlive();
		sleep(2);
	}
}

$janus->detach();
$janus->disconnect();

// Clean up
unset($janus);