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

#include <osmium/osmfile.hpp>

std::string example_file_content("Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua. At vero eos et accusam et justo duo dolores et ea rebum. Stet clita kasd gubergren, no sea takimata sanctus est Lorem ipsum dolor sit amet. Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua. At vero eos et accusam et justo duo dolores et ea rebum. Stet clita kasd gubergren, no sea takimata sanctus est Lorem ipsum dolor sit amet.\n");

BOOST_AUTO_TEST_SUITE(OSMFile_Output)
/* Test scenarios for OSMFile objects
 */


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
    Osmium::OSMFile file("test.osm");
    BOOST_REQUIRE_EQUAL(file.fd(), -1);

    file.open_for_output();
    BOOST_REQUIRE_GE(file.fd(), 0);

    write(file.fd(), example_file_content.c_str(), example_file_content.size());
    file.close();
    BOOST_REQUIRE_EQUAL(file.fd(), -1);

    compare_file_content(new std::ifstream("test.osm", std::ios::binary), example_file_content);
}


/* Test gz encoding of output file:
 * Open output file with "osm.gz" extension 
 * and check if file written is gzip encoded
 */
BOOST_AUTO_TEST_CASE( write_to_xml_gz_output_file ) {
    Osmium::OSMFile file("test.osm.gz");
    file.open_for_output();
    write(file.fd(), example_file_content.c_str(), example_file_content.size());
    file.close();

    std::ifstream inputfile("test.osm.gz", std::ios::binary);
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
    Osmium::OSMFile file("test.osm.bz2");
    file.open_for_output();
    write(file.fd(), example_file_content.c_str(), example_file_content.size());
    file.close();

    std::ifstream inputfile("test.osm.bz2", std::ios::binary);
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

void read_by_method_and_compare( Osmium::OSMFile &target, std::string& expected_content) {
    const int buf_length = 1000;
    char buffer[buf_length];

    int read_length = target.read_input(buffer, buf_length-1);

    BOOST_CHECK_EQUAL(std::string(buffer, read_length), expected_content);
}

/* Test basic file input operations:
 * Open an input file and check if correct content ist returned
 */
BOOST_AUTO_TEST_CASE( read_from_xml_file ) {
    // write content
    std::ofstream outputfile("test.osm", std::ios::binary);
    outputfile << example_file_content;
    outputfile.close();

    Osmium::OSMFile file("test.osm");
    file.open_for_input();

    read_by_method_and_compare(file, example_file_content);
    file.close();
}

/* Test gzip decoding of input file:
 * Write gzip compressed data
 * and read it back through OSMFile
 */
BOOST_AUTO_TEST_CASE( read_from_xml_gz_file ) {
    // write content
    std::ofstream outputfile("test.osm.gz", std::ios::binary);
    boost::iostreams::filtering_ostream out;
    out.push(boost::iostreams::gzip_compressor());
    out.push(outputfile);
    out << example_file_content;
    boost::iostreams::close(out);

    Osmium::OSMFile file("test.osm.gz");
    file.open_for_input();
    read_by_method_and_compare(file, example_file_content);
    file.close();
}

/* Test bzip2 decoding of input file:
 * Write bzip2 compressed data
 * and read it back through OSMFile
 */
BOOST_AUTO_TEST_CASE( read_from_xml_bz2_file ) {
    // write content
    std::ofstream outputfile("test.osm.bz2", std::ios::binary);
    boost::iostreams::filtering_ostream out;
    out.push(boost::iostreams::bzip2_compressor());
    out.push(outputfile);
    out << example_file_content;
    boost::iostreams::close(out);

    Osmium::OSMFile file("test.osm.bz2");
    file.open_for_input();
    read_by_method_and_compare(file, example_file_content);
    file.close();
}


BOOST_AUTO_TEST_SUITE_END()
