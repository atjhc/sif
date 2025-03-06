DSTROOT := build

release: ${DSTROOT}/release
	cmake -S . -B ${DSTROOT}/release
	cmake --build ${DSTROOT}/release

debug: build/debug
	cmake -DCMAKE_BUILD_TYPE=Debug -S . -B ${DSTROOT}/debug
	cmake --build ${DSTROOT}/debug

test: debug
	cd ${DSTROOT}/debug && ctest --progress --output-on-failure

format:
	find src -type f -name '*.cc' -not -path 'src/tests/*' -not -path 'src/extern/*' -exec clang-format -i --style=file {} \;
	find src -type f -name '*.h' -not -path 'src/tests/*' -not -path 'src/extern/*' -exec clang-format -i --style=file {} \;

${DSTROOT}/release:
	mkdir -p ${DSTROOT}/release

${DSTROOT}/debug:
	mkdir -p ${DSTROOT}/debug

clean:
	rm -rf ${DSTROOT}
