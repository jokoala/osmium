#ifndef TEMP_FILE_FIXTURE
#define TEMP_FILE_FIXTURE

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>

boost::filesystem::path tempdir_path;

/* TempDirFixture:  Prepare a temp directory and clean up afterwards
 */
struct TempDirFixture {
    TempDirFixture() {
        tempdir_path = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path();
        std::cout << "create " <<tempdir_path <<std::endl;
        boost::filesystem::create_directory(tempdir_path);
    }

    ~TempDirFixture() {
        std::cout << "remove " <<tempdir_path <<std::endl;
        boost::filesystem::remove_all(tempdir_path);
    }
};

struct TempFileFixture {
    TempFileFixture(std::string name) {
        path = tempdir_path / name;
    }

    ~TempFileFixture() {
        std::cout <<"remove " <<path <<std::endl;
        boost::filesystem::remove(path);
    }

    operator const char*() const {
        return path.c_str();
    }

    operator const std::string&() const {
        return path.native();
    }

    boost::filesystem::path path;
};


BOOST_GLOBAL_FIXTURE(TempDirFixture)

#endif
