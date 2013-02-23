#define BOOST_TEST_DYN_LINK
#ifdef STAND_ALONE
# define BOOST_TEST_MODULE Main
#endif
#include <boost/test/unit_test.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <streambuf>

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filter/bzip2.hpp>
#include <boost/filesystem.hpp>

#include <osmium/osmfile.hpp>

std::string example_file_content("Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua. At vero eos et accusam et justo duo dolores et ea rebum. Stet clita kasd gubergren, no sea takimata sanctus est Lorem ipsum dolor sit amet. Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua. At vero eos et accusam et justo duo dolores et ea rebum. Stet clita kasd gubergren, no sea takimata sanctus est Lorem ipsum dolor sit amet.\n");

/* Test scenarios for OSMFile objects
 */


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

    boost::filesystem::path path;
};


BOOST_GLOBAL_FIXTURE(TempDirFixture)

BOOST_AUTO_TEST_SUITE(OSMFile_Output)
/* Helper function
 * Verify if the istream <inputfile> contains exactly the text <expected_content>
 */
void compare_file_content(std::istream* inputfile, std::string& expected_content)
{
    std::string file_content((std::istreambuf_iterator<char>(*inputfile)),(std::istreambuf_iterator<char>()));

    BOOST_CHECK_EQUAL(file_content, expected_content);
}

/* Test basic file operations:
 * Open an output file and check if correct fd ist returned
 */
BOOST_AUTO_TEST_CASE( write_to_xml_output_file ) {
    TempFileFixture test_osm("test.osm");

    Osmium::OSMFile file(test_osm.path.native());
    BOOST_REQUIRE_EQUAL(file.fd(), -1);

    file.open_for_output();
    BOOST_REQUIRE_GE(file.fd(), 0);

    write(file.fd(), example_file_content.c_str(), example_file_content.size());
    file.close();
    BOOST_REQUIRE_EQUAL(file.fd(), -1);

    compare_file_content(new std::ifstream(test_osm.path.c_str(), std::ios::binary), example_file_content);
}


/* Test gz encoding of output file:
 * Open output file with "osm.gz" extension 
 * and check if file written is gzip encoded
 */
BOOST_AUTO_TEST_CASE( write_to_xml_gz_output_file ) {
    TempFileFixture test_osm_gz("test.osm.gz");

    Osmium::OSMFile file(test_osm_gz.path.native());
    file.open_for_output();
    write(file.fd(), example_file_content.c_str(), example_file_content.size());
    file.close();

    std::ifstream inputfile(test_osm_gz.path.c_str(), std::ios::binary);
    boost::iostreams::filtering_istream in;
    in.push(boost::iostreams::gzip_decompressor());
    in.push(inputfile);
    compare_file_content(&in, example_file_content);
}


/* Test bzip encoding of output file:
 * Open output file with "osm.bz2" extension
 * and check if file written is bzip2 encoded
 */
BOOST_AUTO_TEST_CASE( write_to_xml_bz2_output_file ) {
    TempFileFixture test_osm_bz2("test.osm.bz2");

    Osmium::OSMFile file(test_osm_bz2.path.native());
    file.open_for_output();
    write(file.fd(), example_file_content.c_str(), example_file_content.size());
    file.close();

    std::ifstream inputfile(test_osm_bz2.path.c_str(), std::ios::binary);
    boost::iostreams::filtering_istream in;
    in.push(boost::iostreams::bzip2_decompressor());
    in.push(inputfile);

    compare_file_content(&in, example_file_content);
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE(OSMFile_Input)

void read_from_fd_and_compare(int fd, std::string& expected_content) {
    const int buf_length = 1000;
    char buffer[buf_length];

    int read_length = read(fd, buffer, buf_length-1);

    BOOST_CHECK_EQUAL(std::string(buffer, read_length), expected_content);
}

/* Test basic file input operations:
 * Open an input file and check if correct content ist returned
 */
BOOST_AUTO_TEST_CASE( read_from_xml_file ) {
    TempFileFixture test_osm("test.osm");

    // write content
    std::ofstream outputfile(test_osm.path.c_str(), std::ios::binary);
    outputfile << example_file_content;
    outputfile.close();

    Osmium::OSMFile file(test_osm.path.native());
    file.open_for_input();

    read_from_fd_and_compare(file.fd(), example_file_content);
    file.close();
}

/* Test gzip decoding of input file:
 * Write gzip compressed data
 * and read it back through OSMFile
 */
BOOST_AUTO_TEST_CASE( read_from_xml_gz_file ) {
    TempFileFixture test_osm_gz("test.osm.gz");

    // write content
    std::ofstream outputfile(test_osm_gz.path.c_str(), std::ios::binary);
    boost::iostreams::filtering_ostream out;
    out.push(boost::iostreams::gzip_compressor());
    out.push(outputfile);
    out << example_file_content;
    boost::iostreams::close(out);

    Osmium::OSMFile file(test_osm_gz.path.native());
    file.open_for_input();
    read_from_fd_and_compare(file.fd(), example_file_content);
    file.close();
}

/* Test bzip2 decoding of input file:
 * Write bzip2 compressed data
 * and read it back through OSMFile
 */
BOOST_AUTO_TEST_CASE( read_from_xml_bz2_file ) {
    TempFileFixture test_osm_bz2("test.osm.bz2");
    // write content
    std::ofstream outputfile(test_osm_bz2.path.c_str(), std::ios::binary);
    boost::iostreams::filtering_ostream out;
    out.push(boost::iostreams::bzip2_compressor());
    out.push(outputfile);
    out << example_file_content;
    boost::iostreams::close(out);

    Osmium::OSMFile file(test_osm_bz2.path.c_str());
    file.open_for_input();
    read_from_fd_and_compare(file.fd(), example_file_content);
    file.close();
}


BOOST_AUTO_TEST_SUITE_END()
