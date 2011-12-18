PLUGIN_NAME=gunzip
SOURCEMOD=${HOME}/srcds/css/cstrike/addons/sourcemod

all: addons/sourcemod/scripting/${PLUGIN_NAME}/${PLUGIN_NAME}.sp
	${SOURCEMOD}/scripting/spcomp -o${SOURCEMOD}/plugins/${PLUGIN_NAME}.smx ${SOURCEMOD}/scripting/${PLUGIN_NAME}/${PLUGIN_NAME}.sp

asm:
	${SOURCEMOD}/scripting/spcomp -l -o./${PLUGIN_NAME} ${SOURCEMOD}/scripting/${PLUGIN_NAME}/${PLUGIN_NAME}.sp
