#ifndef DATA_FILE_FIXTURE
#define DATA_FILE_FIXTURE

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>

/* DataFileFixture:  Encapsulates acces to the files in the data directory
 */
struct DataFileFixture {
    DataFileFixture(const std::string& name) {
        path = boost::filesystem::path("data")/name;
        pattern_path = boost::filesystem::path("data")/(name + ".pattern");
    }

    operator const char*() const {
        return path.c_str();
    }

    operator const std::string&() const {
        return path.native();
    }

    const std::string& pattern() const {
        return pattern_path.native();
    }

    boost::filesystem::path path;
    boost::filesystem::path pattern_path;
};

#endif
