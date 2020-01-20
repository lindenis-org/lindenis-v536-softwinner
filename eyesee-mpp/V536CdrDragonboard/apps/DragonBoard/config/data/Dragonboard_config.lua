-- -------------------------------------------------------------------------------
-- Copyright (c), 2001-2016, Allwinner Tech  All rights reserved.
-- -------------------------------------------------------------------------------
-- file     ipc_config.lua
-- brief    ${DESCRIPTION}
-- author   id: 826
-- version  v0.3
-- date     2016-10-11

config = {
		--profile:(dragonboard:1,Qctest:2)
	profile = 2;
    dragonboard = {
		--TF TEST
        testTf = 1,
		testTfPath = "/usr/bin/tftester",
		--MIC TEST
        testMic = 1,
		testMicPath = "/usr/bin/mictester",
		--SPC TEST
		testSpk = 1,
		testSpkPath = "/usr/bin/spktester",
		--RTC TEST
		testRtc = 1,
		testRtcPath = "/usr/bin/rtctester",
		--ACC TEST
		testAcc = 1,
		testAccPath = "/usr/bin/acctester",
		--NOR TEST
		testNor = 1,
		testNorPath = "/usr/bin/nortester",
		--DDR TEST
		testDdr = 1,
		testDdrPath = "/usr/bin/ddrtester",
		--WIFI TEST
		testWifi = 1,
		testWifiPath = "/usr/bin/wifitester",
		--TP TEST
		testTp = 0,
		testTpPath = "/usr/bin/tptester",
		--VIDEO TEST
		testVideo = 1,
		--KEY TEST
		testKey = 1,
		testKeyPath = "/usr/bin/keytester",
		--ETHERNET TEST
		testEthernet = 0,
		testEthernetPath = "/usr/bin/ethernettest",
		--G2D TEST
		testG2d = 0,
		testG2dPath = "/usr/bin/G2Dtest",
		--CVE TEST
		testCVE = 0,
		testCVEPath = "/usr/bin/CVEtest",
		--EVE TEST
		testEVE = 0,
		testEVEPath = "/usr/bin/EVEtest",
		--ISE TEST
		testISE = 0,
		testISEPath = "/usr/bin/ISEtest",
		--CE TEST
		testCE = 0,
		testCEPath = "/usr/bin/CEtest",
		--CPU TEST
		testCPU = 1,
		testCPUPatth = "/usr/bin/CPUtest",
		--CPUS TEST
		testCPUs = 1,
		testCPUsPath = "/usr/bin/CPUstest",
		--USB TEST
		testUSB = 0,
		testUSBPath = "/usr/bin/USBtest",
		-- fish test
		testFish = 0,
		testFishPath = "/usr/bin/FishTest",
		--hdmi test
		testHdmi = 1,
		testHdmiPath = "/usr/bin/vi2voHdmi",
    },
	Qctest = {
		--TF TEST
		testTf = 1,
		testTfPath = "/usr/bin/tftester",
		--MIC TEST
        testMic = 1,
		testMicPath = "/usr/bin/mictester",
		--SPC TEST
		testSpk = 1,
		testSpkPath = "/usr/bin/spktester",
		--RTC TEST
		testRtc = 1,
		testRtcPath = "/usr/bin/rtctester",
		--ACC TEST
		testAcc = 0,
		testAccPath = "/usr/bin/acctester",
		--NOR TEST
		testNor = 1,
		testNorPath = "/usr/bin/nortester",
		--DDR TEST
		testDdr = 1,
		testDdrPath = "/usr/bin/ddrtester",
		--wifi TEST
		testWifi = 1,
		testWifiPath = "/usr/bin/wifitester",
		--TP TEST
		testTp = 0,
		testTpPath = "/usr/bin/tptester",
		--VIDEO TEST
		testVideo = 1,
		--KEY TEST
		testKey = 1,
		testKeyPath = "/usr/bin/keytester",
		--ETHERNET TEST
		testEthernet = 0,
		testEthernetPath = "/usr/bin/ethernettest",
		--G2D TEST
		testG2d = 0,
		testG2dPath = "/usr/bin/G2Dtest",
		--CVE TEST
		testCVE = 0,
		testCVEPath = "/usr/bin/CVEtest",
		--EVE TEST
		testEVE = 0,
		testEVEPath = "/usr/bin/EVEtest",
		--ISE TEST
		testISE = 0,
		testISEPath = "/usr/bin/ISEtest",
		--CE TEST
		testCE = 0,
		testCEPath = "/usr/bin/CEtest",
		--CPU TEST
		testCPU = 1,
		testCPUPatth = "/usr/bin/CPUtest",
		--CPUS TEST
		testCPUs = 1,
		testCPUsPath = "/usr/bin/CPUstest",
		--USB TEST
		testUSB = 0,
		testUSBPath = "/usr/bin/USBtest",
		-- fish test
		testFish = 0,
		testFishPath = "/usr/bin/FishTest",
		--hdmi test
		testHdmi = 1,
		testHdmiPath = "/usr/bin/vi2voHdmi",
	}
}
