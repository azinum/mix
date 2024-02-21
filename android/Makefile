APP_PATH=app/build/outputs/apk/debug/app-debug.apk

all: build

prepare:
	./setup-sdk
	cp -r ../src app/src/main/cpp
	cp -r ../include app/src/main/cpp
	cp -r ../data app/src/main/assets

build: prepare
	./gradlew assembleDebug --parallel

release: prepare
	./gradlew assembleRelease --parallel

install:
	adb -d install -r ${APP_PATH}

run:
	./adb-run ${APP_PATH}

log:
	adb logcat -c && adb logcat -v tag | grep -e 'I/' --color=auto -i

clean:
	rm -dr app/build
