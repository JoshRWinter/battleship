apk=battleship-debug.apk

all:../assets/assets.pack ../assets/uiassets.pack ../assets/aassets.apack
	ndk-build -C ..
	ant debug -f ../build.xml
	adb install -r $(CURDIR)/../bin/$(apk)
	
../assets/assets.pack:../assets_local/build.txt ../assets_local/*.tga
	cd ../assets_local/ && pack --build

../assets/uiassets.pack:../assets_local/uibuild.txt ../assets_local/*.tga
	cd ../assets_local/ && pack --build uibuild.txt

../assets/aassets.apack:../assets_local/apack.txt ../assets_local/*.ogg
	cd ../assets_local/ && apack --build
