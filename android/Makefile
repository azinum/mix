APP_PATH=app/build/outputs/apk/debug/app-debug.apk

all: build

prepare:
	./setup-sdk
	cp -r ../src app/src/main/cpp
	cp -r ../include app/src/main/cpp

build: prepare
	./gradlew assembleDebug --parallel

install:
	adb -d install -r ${APP_PATH}

run:
	./adb-run ${APP_PATH}

log:
	adb logcat -c && adb logcat -v tag | grep -e 'I/Debug' --color=auto -i