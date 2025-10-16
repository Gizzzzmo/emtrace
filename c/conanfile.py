from conan import ConanFile
from conan.tools.build import check_min_cstd
from conan.tools.cmake import CMake, cmake_layout


class EmtraceConanfile(ConanFile):
    name: str = "emtrace"
    version: str = "0.1.0"
    description: str = "Minimal overhead tracing/logging library for embedded, and resource constrained projects."
    license: str = "Apache-2.0"
    homepage: str = "https://github.com/Gizzzzmo/emtrace"
    topics: tuple[str, ...] = ("tracing", "debugging", "header-only")

    settings: tuple[str, ...] = "os", "arch", "compiler", "build_type"
    exports_sources: tuple[str, ...] = (
        "CMakeLists.txt",
        "include/*",
        "test/*",
        "examples/*",
    )

    no_copy_source: bool = True
    generators: tuple[str, ...] = "CMakeDeps", "CMakeToolchain"
    package_type: str = "header-library"

    def layout(self):
        cmake_layout(self, src_folder=".")

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
        if not self.conf.get("tools.build:skip_test", default=False, check_type=bool):
            _ = self.run("ctest --output-on-failure", env="conanbuild")

    def package(self):
        cmake = CMake(self)
        cmake.install(self)

        pass

    def package_id(self):
        self.info.clear()

    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", "emtrace")
        self.cpp_info.set_property("cmake_target_name", "emtrace::emtrace")
        self.cpp_info.bindirs = []
        self.cpp_info.libdirs = []
