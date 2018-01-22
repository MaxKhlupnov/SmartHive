# Azure IoT Edge gateway modules for Zwave protocol
# Max R. Khlupnov (@MaxKhlupnov)
#
# Example usage:
#
# Build
# $] docker build --tag=smarthive .
#
# Example usage:
#
# Build
# $] docker build --tag=openzwave .
#
# Run
# $] docker run --rm --interactive --tty --device="/dev/ttyACM0:/dev/ttyUSB0" --entrypoint=/bin/bash --user=root smarthive
# $] docker run --rm --interactive --tty --device="/dev/ttyACM0:/dev/ttyUSB0" --entrypoint="/usr/SmartHive/SmartHive.ZWaveGateway /usr/SmartHive/zwave_device_cloud_upload_docker.json --user=root smarthive
#
#
# Thanks:
#
# Louis T. Getterman IV (@LTGIV)
# www.GotGetLLC.com | www.opensour.cc/ggcom/docker/openzwave
#
################################################################################
FROM        ubuntu:latest
LABEL       maintainer="m.khlupnov@gmail.com"
LABEL       version="0.1"
LABEL       description="Azure IoT Edge gateway modules for Zwave protocol.."

# Initial prerequisites
USER		root
RUN			apt-get -y update && apt-get -y install \
				libgnutls28-dev \
                libgnutlsxx28 \
				libudev-dev \
				libyaml-dev \
                curl \
                build-essential \
                libcurl4-openssl-dev \
                git \
                cmake \
                make \
                libssl-dev \
                uuid-dev \
                valgrind \
                libglib2.0-dev \
                libtool \
                autoconf \				
				nano \
				sudo

# Create a user and reciprocal environment variables
RUN			adduser --disabled-password --gecos "" smarthive_user
RUN			usermod -a -G dialout smarthive_user

# Set environment variables
USER		smarthive_user
ENV			HOME			/home/smarthive_user
ENV			PYENV_ROOT		$HOME/.pyenv
ENV			PATH			$PYENV_ROOT/shims:$PYENV_ROOT/bin:$HOME/config:$PATH
ENV			LD_LIBRARY_PATH	/usr/local/lib64:$PATH
WORKDIR		$HOME                

################################################################################
USER		smarthive_user
WORKDIR		$HOME

# Clone SmartHive OpenZWave and IoT Edge
RUN			mkdir -p $HOME/src/
RUN			git clone --recursive https://github.com/MaxKhlupnov/SmartHive  $HOME/src/

# Compile OpenZWave
WORKDIR		$HOME/src/open-zwave/
RUN			make

# Install OpenZWave
USER		root
WORKDIR		$HOME/src/open-zwave/
RUN			make install && ldconfig /usr/local/lib64

WORKDIR		$HOME/src/
# Copy SmartHive.ZWave modules into iot-edge project
RUN			mkdir -p $HOME/src/iot-edge/modules/SmartHive.ZWaveModule
RUN         cp -r $HOME/src/SmartHive.ZWaveModule $HOME/src/iot-edge/modules
RUN			mkdir -p $HOME/src/iot-edge/modules/SmartHive.ZWaveMappingModule
RUN         cp -r $HOME/src/SmartHive.ZWaveMappingModule $HOME/src/iot-edge/modules
RUN			mkdir -p $HOME/src/iot-edge/samples/SmartHive.ZWaveGateway
RUN         cp -r $HOME/src/SmartHive.ZWaveGateway $HOME/src/iot-edge/samples
# Prepare iot-edge CMakeLists.txt for compilation
RUN printf "\n add_subdirectory(SmartHive.ZWaveGateway) \n" >> $HOME/src/iot-edge/samples/CMakeLists.txt
RUN printf "\n add_subdirectory(SmartHive.ZWaveModule)\n add_subdirectory(SmartHive.ZWaveMappingModule) \n" >> $HOME/src/iot-edge/modules/CMakeLists.txt

# Compile IoT Edge
WORKDIR		$HOME/src/iot-edge/
RUN			tools/build.sh --disable-native-remote-modules

# Copy sample configuration file
RUN     cp $HOME/src/SmartHive.ZWaveGateway/zwave_device_cloud_upload_linARM.json $HOME/src/iot-edge/samples/SmartHive.ZWaveGateway/zwave_upload_config.json


# Install OpenZWave Device Database (https://github.com/OpenZWave/open-zwave/wiki/Adding-Devices)
USER		smarthive_user
WORKDIR		$HOME
RUN			ln -s /usr/local/etc/openzwave $HOME/config