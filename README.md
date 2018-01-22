# SmartHive.ZWave is Azure IoT Edge gateway modules for Zwave protocol.
<div>SmartHive.ZWave  based on:</div>
<ul>
<li><a href="https://github.com/Azure/iot-edge#azure-iot-edge">Azure IoT Edge framework</a> - Microsoft framework to build IoT Gateway.</li>
<li><a href="https://github.com/OpenZWave/open-zwave">Open Z-Wave</a> - free software library that interfaces with Z-Wave protocol stack.</li>
</ul>

<div>How to build sources</div>
<ul>
  <li><div>Run following script:</div>
  <code>apt-get -y update && apt-get -y install libgnutls28-dev libgnutlsxx28 libudev-dev libyaml-dev curl build-essential libcurl4-openssl-dev git cmake make libssl-dev \       uuid-dev valgrind libglib2.0-dev libtool autoconf nano sudo
  </code>
  </li>
</ul>

<div>How to build Docker image</div>
<ul>
  <li><a href='https://docs.docker.com/engine/installation/'>Install Docker</a></li>
</ul>

<div>How to deploy</div>
<ul>
  <li><a href="SmartHive.ZWaveGateway">SmartHive.ZWaveGateway</a> is IoT Edge gateway that forwards telemetry from a ZWave devices to IoT Hub.</li>
</ul>

<div>There are three parts in the project. </div>
<ul>
  <li><a href="SmartHive.ZWaveGateway">SmartHive.ZWaveGateway</a> is IoT Edge gateway that forwards telemetry from a ZWave devices to IoT Hub.</li>
  <li><a href="SmartHive.ZWaveModule">SmartHive.ZWaveModule</a> is a Azure IoT Gateway SDK module, capable of reading data from ZWave devices and publish the data to the Azure IoT Hub via <a href="SmartHive.ZWaveMappingModule">SmartHive.ZWaveMappingModule</a></li>
  <li><a href="SmartHive.ZWaveMappingModule">SmartHive.ZWaveMappingModule</a> is a Azure IoT Gateway SDK module, capable maps ZWave network device addresses to device id and keys in Azure IoT Hub. 
  This module is not multi-threaded, all work will be completed in the Receive callback. Based on <a href="https://github.com/Azure/iot-edge/tree/master/modules/identitymap">Azure IoT Gateway SDK identitymap module</a></li>
</ul>
<img src="https://raw.githubusercontent.com/MaxKhlupnov/SmartHive/master/Docs/Images/Architecture.PNG"/>
