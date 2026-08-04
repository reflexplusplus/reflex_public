#!/bin/sh
if [ -e "$2" ]
then
	"$1" "$2"
else
	>&2 echo "You need to pass the path of a resources.xml file (does not exist: $2)"
fi
