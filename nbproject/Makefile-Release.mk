#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-Linux-x86
CND_DLIB_EXT=so
CND_CONF=Release
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/lib/http_parser.o \
	${OBJECTDIR}/lib/ini.o \
	${OBJECTDIR}/src/config.o \
	${OBJECTDIR}/src/http-reader.o \
	${OBJECTDIR}/src/http-server.o \
	${OBJECTDIR}/src/http.o \
	${OBJECTDIR}/src/log.o \
	${OBJECTDIR}/src/main.o \
	${OBJECTDIR}/src/mime.o \
	${OBJECTDIR}/src/queue.o \
	${OBJECTDIR}/src/socket.o \
	${OBJECTDIR}/src/thread-pool.o \
	${OBJECTDIR}/src/util.o


# C Compiler Flags
CFLAGS=-m64 -march=native

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=-lmagic -lpthread

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/khttp

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/khttp: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.c} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/khttp ${OBJECTFILES} ${LDLIBSOPTIONS}

${OBJECTDIR}/lib/http_parser.o: nbproject/Makefile-${CND_CONF}.mk lib/http_parser.c 
	${MKDIR} -p ${OBJECTDIR}/lib
	${RM} "$@.d"
	$(COMPILE.c) -O2 -Werror -DINI_ALLOW_BOM=0 -DINI_ALLOW_MULTILINE=0 -D_GNU_SOURCE -Ilib -std=c99 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/lib/http_parser.o lib/http_parser.c

${OBJECTDIR}/lib/ini.o: nbproject/Makefile-${CND_CONF}.mk lib/ini.c 
	${MKDIR} -p ${OBJECTDIR}/lib
	${RM} "$@.d"
	$(COMPILE.c) -O2 -Werror -DINI_ALLOW_BOM=0 -DINI_ALLOW_MULTILINE=0 -D_GNU_SOURCE -Ilib -std=c99 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/lib/ini.o lib/ini.c

${OBJECTDIR}/src/config.o: nbproject/Makefile-${CND_CONF}.mk src/config.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.c) -O2 -Werror -DINI_ALLOW_BOM=0 -DINI_ALLOW_MULTILINE=0 -D_GNU_SOURCE -Ilib -std=c99 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/config.o src/config.c

${OBJECTDIR}/src/http-reader.o: nbproject/Makefile-${CND_CONF}.mk src/http-reader.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.c) -O2 -Werror -DINI_ALLOW_BOM=0 -DINI_ALLOW_MULTILINE=0 -D_GNU_SOURCE -Ilib -std=c99 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/http-reader.o src/http-reader.c

${OBJECTDIR}/src/http-server.o: nbproject/Makefile-${CND_CONF}.mk src/http-server.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.c) -O2 -Werror -DINI_ALLOW_BOM=0 -DINI_ALLOW_MULTILINE=0 -D_GNU_SOURCE -Ilib -std=c99 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/http-server.o src/http-server.c

${OBJECTDIR}/src/http.o: nbproject/Makefile-${CND_CONF}.mk src/http.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.c) -O2 -Werror -DINI_ALLOW_BOM=0 -DINI_ALLOW_MULTILINE=0 -D_GNU_SOURCE -Ilib -std=c99 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/http.o src/http.c

${OBJECTDIR}/src/log.o: nbproject/Makefile-${CND_CONF}.mk src/log.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.c) -O2 -Werror -DINI_ALLOW_BOM=0 -DINI_ALLOW_MULTILINE=0 -D_GNU_SOURCE -Ilib -std=c99 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/log.o src/log.c

${OBJECTDIR}/src/main.o: nbproject/Makefile-${CND_CONF}.mk src/main.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.c) -O2 -Werror -DINI_ALLOW_BOM=0 -DINI_ALLOW_MULTILINE=0 -D_GNU_SOURCE -Ilib -std=c99 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/main.o src/main.c

${OBJECTDIR}/src/mime.o: nbproject/Makefile-${CND_CONF}.mk src/mime.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.c) -O2 -Werror -DINI_ALLOW_BOM=0 -DINI_ALLOW_MULTILINE=0 -D_GNU_SOURCE -Ilib -std=c99 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/mime.o src/mime.c

${OBJECTDIR}/src/queue.o: nbproject/Makefile-${CND_CONF}.mk src/queue.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.c) -O2 -Werror -DINI_ALLOW_BOM=0 -DINI_ALLOW_MULTILINE=0 -D_GNU_SOURCE -Ilib -std=c99 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/queue.o src/queue.c

${OBJECTDIR}/src/socket.o: nbproject/Makefile-${CND_CONF}.mk src/socket.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.c) -O2 -Werror -DINI_ALLOW_BOM=0 -DINI_ALLOW_MULTILINE=0 -D_GNU_SOURCE -Ilib -std=c99 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/socket.o src/socket.c

${OBJECTDIR}/src/thread-pool.o: nbproject/Makefile-${CND_CONF}.mk src/thread-pool.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.c) -O2 -Werror -DINI_ALLOW_BOM=0 -DINI_ALLOW_MULTILINE=0 -D_GNU_SOURCE -Ilib -std=c99 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/thread-pool.o src/thread-pool.c

${OBJECTDIR}/src/util.o: nbproject/Makefile-${CND_CONF}.mk src/util.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.c) -O2 -Werror -DINI_ALLOW_BOM=0 -DINI_ALLOW_MULTILINE=0 -D_GNU_SOURCE -Ilib -std=c99 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/util.o src/util.c

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/khttp

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
