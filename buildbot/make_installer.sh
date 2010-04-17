set -e
cd $(dirname $0)/..

CONFIG=${1}
BRANCH=${2}
MINGWLIBS_PATH=${3}
MINGW_HOST=/opt/mingw32/bin/i586-pc-mingw32-
BUILDDIR=${PWD}/build-${CONFIG}
TMP_BASE=/tmp/sl
TMP_PATH=${TMP_BASE}/${CONFIG}/${BRANCH}
REV=$(git describe --tags)

echo "!define MINGWLIBS_DIR \"${MINGWLIBS_PATH}\"" > installer/custom_defines.nsi
echo "!define BUILD_DIR \"${BUILDDIR}\"" >> installer/custom_defines.nsi


#strip symbols and archive them
for tostripfile in spring.exe unitsync.dll ; do
	debugfile=${tostripfile}.dbg
	${MINGW_HOST}objcopy --only-keep-debug ${BUILDDIR}/${tostripfile} ${BUILDDIR}/${debugfile}
	${MINGW_HOST}strip --strip-debug --strip-unneeded ${BUILDDIR}/${tostripfile}
	${MINGW_HOST}objcopy --add-gnu-debuglink=${BUILDDIR}/${debugfile} ${BUILDDIR}/${tostripfile}
done

./installer/make_installer.pl

mv ./installer/spring*.exe ${TMP_PATH}

