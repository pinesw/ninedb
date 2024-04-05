#!/bin/bash

set -e

PURPLE='\033[0;35m'
NC='\033[0m'

for f in ./profiles/*.conf; do
	[ ! -f "$f" ] && continue

	config_name="$f"
	config_name=${config_name##*/}
	config_name=${config_name#"profile_"}
	config_name=${config_name%".conf"}

	[ "$config_name" == "build" ] && continue

	echo -e "${PURPLE}Building ${config_name} configuration...${NC}"

	conan install . -pr:b profiles/profile_build.conf -pr:h profiles/profile_${config_name}.conf -of ${config_name} --build=missing

	cmake \
	    -B ${config_name} \
	    -S js -DCMAKE_BUILD_TYPE=Release \
	    -DCONAN_PATH=/project/${config_name} \
	    -DNODE_API_HEADERS=/project/node_modules/node-api-headers \
	    -DCMAKE_TOOLCHAIN_FILE=/project/${config_name}/conan_toolchain.cmake

	cmake --build ${config_name}

	echo -e "${PURPLE}Configuration ${config_name} built successfully.${NC}"
done
