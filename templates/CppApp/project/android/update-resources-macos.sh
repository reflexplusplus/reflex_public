#!/bin/sh
if [ -e "$1" ]
then
	"$REFLEX_PATH/bin/tools/macos/ReflexResourceBuilder.app/Contents/MacOS/ReflexResourceBuilder" "$1"
else
	>&2 echo "You need to pass the path of a resources.xml file"
fi
