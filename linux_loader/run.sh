#!/bin/sh
#
# http://ubuntuforums.org/showthread.php?t=451004&page=2
#

# Function to find the real directory a program resides in.
# Feb. 17, 2000 - Sam Lantinga, Loki Entertainment Software
# This code is licensed under Public Domain (I asked the author)
FindPath()
{
    fullpath="`echo $1 | grep /`"
    if [ "$fullpath" = "" ]; then
        oIFS="$IFS"
        IFS=:
        for path in $PATH
        do if [ -x "$path/$1" ]; then
               if [ "$path" = "" ]; then
                   path="."
               fi
               fullpath="$path/$1"
               break
           fi
        done
        IFS="$oIFS"
    fi
    if [ "$fullpath" = "" ]; then
        fullpath="$1"
    fi

    # Is the sed/ls magic portable?
    if [ -L "$fullpath" ]; then
        #fullpath="`ls -l "$fullpath" | awk '{print $11}'`"
        fullpath=`ls -l "$fullpath" |sed -e 's/.* -> //' |sed -e 's/\*//'`
    fi
    dirname $fullpath
}

# Set the home if not already set.
if [ "${SKYCHECKERS_DATA_PATH}" = "" ]; then
    SKYCHECKERS_DATA_PATH="`FindPath $0`"
fi

LD_LIBRARY_PATH=.:${SKYCHECKERS_DATA_PATH}:${LD_LIBRARY_PATH}
export LD_LIBRARY_PATH

# Let's boogie!
if [ -x "${SKYCHECKERS_DATA_PATH}/sc-bin" ]
then
	cd "${SKYCHECKERS_DATA_PATH}/"
	exec "./sc-bin" $*
fi
echo "Couldn't run SkyCheckers (sc-bin). Is SKYCHECKERS_DATA_PATH set?"
exit 1

