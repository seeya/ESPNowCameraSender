monitor:
	platformio run --target monitor --monitor-port "/dev/tty.usbserial-120"

build:
	platformio run --target upload --upload-port "/dev/tty.usbserial-120"

both: build	monitor
