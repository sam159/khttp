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
CND_CONF=Debug
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/lib/http_parser.o \
	${OBJECTDIR}/src/http/http.o \
	${OBJECTDIR}/src/http/parse.o \
	${OBJECTDIR}/src/main.o \
	${OBJECTDIR}/src/socket.o


# C Compiler Flags
CFLAGS=-O0

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/khttp

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/khttp: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.c} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/khttp ${OBJECTFILES} ${LDLIBSOPTIONS}

${OBJECTDIR}/lib/http_parser.o: lib/http_parser.c 
	${MKDIR} -p ${OBJECTDIR}/lib
	${RM} "$@.d"
	$(COMPILE.c) -g -Werror -Ilib -include lib/http_parser.h -std=c99 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/lib/http_parser.o lib/http_parser.c

${OBJECTDIR}/src/http/http.o: src/http/http.c 
	${MKDIR} -p ${OBJECTDIR}/src/http
	${RM} "$@.d"
	$(COMPILE.c) -g -Werror -Ilib -include lib/http_parser.h -std=c99 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/http/http.o src/http/http.c

${OBJECTDIR}/src/http/parse.o: src/http/parse.c 
	${MKDIR} -p ${OBJECTDIR}/src/http
	${RM} "$@.d"
	$(COMPILE.c) -g -Werror -Ilib -include lib/http_parser.h -std=c99 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/http/parse.o src/http/parse.c

${OBJECTDIR}/src/main.o: src/main.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.c) -g -Werror -Ilib -include lib/http_parser.h -std=c99 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/main.o src/main.c

${OBJECTDIR}/src/socket.o: src/socket.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.c) -g -Werror -Ilib -include lib/http_parser.h -std=c99 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/socket.o src/socket.c

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
