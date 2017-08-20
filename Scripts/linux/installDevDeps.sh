# this script install JUCE in sibling directory of LGML

NATIVE_CPU=`dpkg --print-architecture`
if [ -z ${TARGET_CPU+x} ]; then TARGET_CPU="$NATIVE_CPU"; fi
if [ "$TARGET_CPU" != "$NATIVE_CPU" ]; then echo "adding foreing arch $TARGET_CPU"; dpkg --add-architecture $TARGET_CPU;apt-get -qq update; fi
echo "arch is set to '$TARGET_CPU'"

# for dns utility
apt-get -y --force-yes install libavahi-compat-libdnssd-dev:$TARGET_CPU

## these are devloper libs needed for JUCE,   not sure wich are needed in released version...
# from Makefile alsa freetype2 libcurl x11 xext xinerama
apt-get -y --force-yes install libfreetype6-dev:$TARGET_CPU 
apt-get -y --force-yes install libx11-dev:$TARGET_CPU
apt-get -y --force-yes install libxinerama-dev:$TARGET_CPU
apt-get -y --force-yes install libxrandr-dev:$TARGET_CPU
apt-get -y --force-yes install libxcursor-dev:$TARGET_CPU
apt-get -y --force-yes install mesa-common-dev:$TARGET_CPU
apt-get -y --force-yes install libasound2-dev:$TARGET_CPU
apt-get -y --force-yes install freeglut3-dev:$TARGET_CPU
apt-get -y --force-yes install libxcomposite-dev:$TARGET_CPU
apt-get -y --force-yes install libjack-dev:$TARGET_CPU
apt-get -y -q --force-yes install libcurl4-openssl-dev:$TARGET_CPU

SCRIPTPATH=`pwd`/$(dirname "$0") 
cd $SCRIPTPATH
cd ../../..

pwd
ls


apt-get -y -q --force-yes install unzip
apt-get -y -q --force-yes install curl

if [ ! -d "JUCE" ]; then
  curl -L https://github.com/julianstorer/JUCE/archive/master.zip > JUCE.zip
  unzip -q JUCE.zip
  mv JUCE-master/ JUCE
fi


apt-get -y -q --force-yes install python


# cd ;
# if [ ! -d "Dev/Projucer/linux" ]; then
#   cd 
#   ls

#   cd Dev/JUCE/extras/Projucer/Builds/LinuxMakefile/
#   make -j2
#   cd 
#   pwd
#   mkdir -p Dev/Projucer
#   mkdir -p Dev/Projucer/linux
#   mv Dev/JUCE/extras/Projucer/Builds/LinuxMakefile/build/Projucer Dev/Projucer/linux/
# fi


