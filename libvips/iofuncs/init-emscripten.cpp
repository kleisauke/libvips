#include <emscripten/bind.h>

extern "C" char **_emscripten_get_dynamic_libraries_js() {
	auto dynamic_libraries = emscripten::val::module_property("dynamicLibraries");
	int length = dynamic_libraries["length"].as<int>();
	char **result = new char *[length + 1];
	for (int i = 0; i < length; i++) {
		result[i] = strdup(dynamic_libraries[i].as<std::string>().c_str());
	}
	result[length] = NULL;
	return result;
}
