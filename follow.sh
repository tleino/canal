#!/bin/sh

# Just an example of how to recursively list all locally defined
# functions that are called by a given function in C source files
# that follow OpenBSD KNF i.e. style(9).
#
# Does not use associative array but uses a temporary file for
# portability between different shells. Is very slow: could be
# replaced with awk, perl or by built-in implementation to canal.

TMPFILE=$(mktemp || exit 1)

follow() {
	while read FUNCTION ; do
		if [ ! -e $TMPFILE ] ; then
			touch $TMPFILE
		fi
		GREP=$(grep -e ^${FUNCTION}$ <$TMPFILE)
		HAVE=$(grep -e ^${FUNCTION}\( *.c)
		if [ "$GREP" == "" ] ; then
			if [ ! "$HAVE" == "" ] ; then
				if [ "$FUNCTION" != "$FIRST" ] ; then
					echo $FUNCTION
				fi
				echo $FUNCTION >>$TMPFILE
				cat *.c \
					| canal follow $FUNCTION \
					| grep -v "^id" \
					| awk '{ print $2 }' \
					| follow
			fi
		fi
	done
}

if [ ! $# == 1 ] ; then
	echo "usage: $0 FUNCTION"
	exit 1
fi

FUNCTION=$1
FIRST=$1

rm $TMPFILE
echo $FUNCTION | follow
rm $TMPFILE
