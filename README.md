# SmartHive.ZWave is Azure IoT Edge gateway modules for Zwave protocol.
<div>SmartHive.ZWave  based on:</div>
<ul>
<li><a href="https://github.com/Azure/iot-edge#azure-iot-edge">Azure IoT Edge framework</a> - Microsoft framework to build IoT Gateway.</li>
<li><a href="https://github.com/OpenZWave/open-zwave">Open Z-Wave</a> - free software library that interfaces with Z-Wave protocol stack.</li>
</ul>


<h2>How to build modules</h2>
<p>You can now build the IoT Edge runtime and samples on your local machine:</p>
<ol>
<li>
  <h3>Install the prerequisites</h3>
<p>The steps in this tutorial assume you are running Ubuntu Linux.</p>
<p>To install the prerequisite packages, open a shell and run the following commands:</p>
  <pre>sudo apt-get -y update && sudo apt-get -y install libgnutls28-dev libgnutlsxx28 libudev-dev libyaml-dev curl build-essential libcurl4-openssl-dev git cmake make libssl-dev uuid-dev valgrind libglib2.0-dev libtool autoconf nano
  </pre>
<p>In the shell, run the following command to clone the Azure IoT Edge GitHub repository to your local machine:</p>
  <pre>git clone --recursive https://github.com/MaxKhlupnov/SmartHive SmartHive</pre>
</li>
<li>Open a shell.
  Navigate to the root folder in your local copy of the SmartHive repository.
</li>
<li>
    <h3>Compile and install OpenZWave</h3>
    <p>Run the build script as follows:</p>
      <pre>cd open-zwave && make && make install && ldconfig /usr/local/lib64</pre>      
     <p>See https://github.com/OpenZWave/open-zwave for more info</p>
</li>
<li>
    <h3>Merge SmartHive project and iot-edge project for compilation</h3>
    <div>Copy <i>SmartHive.ZWaveModule</i> and <i>SmartHive.ZWaveMappingModule</i> folders into <i>iot-edge/modules</i> directory</div>
    <div>Copy <i>SmartHive.ZWaveGateway</i> folders into <i>iot-edge/samples</i> directory</div>
    <p>Prepare iot-edge CMakeLists.txt for compilation. Run scripts as follows:</p>
    <pre> printf "\n add_subdirectory(SmartHive.ZWaveGateway) \n" >> $HOME/src/iot-edge/samples/CMakeLists.txt</pre>
    <pre> printf "\n add_subdirectory(SmartHive.ZWaveModule)\n add_subdirectory(SmartHive.ZWaveMappingModule) \n" >> $HOME/src/iot-edge/modules/CMakeLists.txt</pre>
</li>
<li>
    <h3>Compile iot-edge with SmartHive modules</h3>
    <p>Run the build script from iot-edge directory as follows:</p>
      <pre>tools/build.sh --disable-native-remote-modules</pre>
      <p>See https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-linux-iot-edge-get-started for more info</p>           
</li>
<li>
    Compiled modules and gateway sample will be placed in the iot-edge\build directory
</li>

</ol>

<h2>How to build Docker image</h2>
<ol>
  <li><a href='https://docs.docker.com/engine/installation/'>Install Docker</a></li>
  <p>In the shell, run the following command to clone the Azure IoT Edge GitHub repository to your local machine:</p>
  <pre>git clone https://github.com/MaxKhlupnov/SmartHive SmartHive</pre>
  <p>Run script as follow from Docker folder</p>
  <pre>docker build -t zwavemodule ./</pre>
</ol>

<h2>How to run</h2>
<ul>
  <li>
  <p>Run <a href="SmartHive.ZWaveGateway">SmartHive.ZWaveGateway</a> same ways as https://github.com/Azure/iot-edge/tree/master/v1/samples/simulated_device_cloud_upload.</p>
  <pre>samples/SmartHive.ZWaveGateway/SmartHive.ZWaveGateway zwave_device_cloud_upload_&lt;your platform&gt;.json</pre>
  </li>
  <li>
  <p>Run Docker container as follow:</p>
  <pre>sudo docker run -p 8008:8008 -i -t --privileged  --device /dev/ttyACM0 openzwave/openzwave-control-panel /bin/bash</pre>
  </li>
</ul>

<h2>There are three parts in the project. </h2>
<ul>
  <li><a href="SmartHive.ZWaveGateway">SmartHive.ZWaveGateway</a> is IoT Edge gateway that forwards telemetry from a ZWave devices to IoT Hub.</li>
  <li><a href="SmartHive.ZWaveModule">SmartHive.ZWaveModule</a> is a Azure IoT Gateway SDK module, capable of reading data from ZWave devices and publish the data to the Azure IoT Hub via <a href="SmartHive.ZWaveMappingModule">SmartHive.ZWaveMappingModule</a></li>
  <li><a href="SmartHive.ZWaveMappingModule">SmartHive.ZWaveMappingModule</a> is a Azure IoT Gateway SDK module, capable maps ZWave network device addresses to device id and keys in Azure IoT Hub. 
  This module is not multi-threaded, all work will be completed in the Receive callback. Based on <a href="https://github.com/Azure/iot-edge/tree/master/modules/identitymap">Azure IoT Gateway SDK identitymap module</a></li>
</ul>
<img src="https://raw.githubusercontent.com/MaxKhlupnov/SmartHive/master/Docs/Images/Architecture.PNG"/>
