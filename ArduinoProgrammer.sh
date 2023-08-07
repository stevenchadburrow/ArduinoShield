echo "ArduinoProgrammer.sh <sketch_file>"

x=${1%.*}
y=${x##*/}

/home/chad/Arduino/arduino-cli sketch new $x

cp $1 $x/$y.ino

/home/chad/Arduino/arduino-cli compile --fqbn arduino:avr:uno $x/$y.ino

/home/chad/Arduino/arduino-cli upload -p /dev/ttyACM0 --fqbn arduino:avr:uno $x/$y.ino

/home/chad/Arduino/arduino-cli monitor -p /dev/ttyACM0

