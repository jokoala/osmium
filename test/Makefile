CXX = g++

CXXFLAGS += -g

CXXFLAGS += -Wall -Wextra -Wdisabled-optimization -pedantic -Wctor-dtor-privacy -Wnon-virtual-dtor -Woverloaded-virtual -Wsign-promo -Wno-long-long

CXXFLAGS_GEOS    = $(shell geos-config --cflags)
CXXFLAGS_LIBXML2 = $(shell xml2-config --cflags)
CXXFLAGS_OGR     = $(shell gdal-config --cflags)

CXXFLAGS += -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -DBOOST_TEST_DYN_LINK
CXXFLAGS += -I../include -I.

# remove this if you do not want debugging to be compiled in
CXXFLAGS += -DOSMIUM_WITH_DEBUG

LIB_EXPAT  = -lexpat
LIB_GD     = -lgd -lz -lm
LIB_GEOS   = $(shell geos-config --libs)
LIB_OGR    = $(shell gdal-config --libs)
LIB_PBF    = -lz -lpthread -lprotobuf-lite -losmpbf
LIB_SHAPE  = -lshp $(LIB_GEOS)
LIB_SQLITE = -lsqlite3
LIB_XML2   = $(shell xml2-config --libs)

LDFLAGS += $(LIB_EXPAT) $(LIB_PBF) -lboost_unit_test_framework -lboost_regex -lboost_iostreams -lboost_filesystem -lboost_system

SCAN_DIRS = \
	t/geometry \
	t/osm \
	t/geometry_geos \
	t/geometry_ogr \
	t/osmfile \
	t/utils \
	t/tags \

PROBLEMS = t/geometry t/tags
ALL_TESTS = $(shell find $(SCAN_DIRS) -name "*.cpp" | sed -e "s/.cpp$$/.o/")

.PHONY: test_main clean coverage

ALL:	test_main
	./test_main

clean:
	find . -name "*.o" -exec rm {} ";"
	find . -name "*.gcda" -exec rm {} ";"
	find . -name "*.gcno" -exec rm {} ";"

test_main: $(ALL_TESTS) test_main.o test_utils.o
	$(CXX) $(LDFLAGS) $(LIB_SHAPE) $(LIB_OGR) $(LIB_GD) $(LIB_GEOS) $(LIB_XML2) -o $@ $^

%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $(CXXFLAGS_LIBXML2) $(CXXFLAGS_GEOS) $(CXXFLAGS_OGR) -o $@ $< 

%.test: %.o test_main.o test_utils.o
	$(CXX) $(LDFLAGS) $(LIB_SHAPE) $(LIB_OGR) $(LIB_GD) $(LIB_GEOS) $(LIB_XML2) -o $@ $< test_main.o test_utils.o

coverage:
	lcov --zerocounters --directory .
	export CXXFLAGS="$(CXXFLAGS) --coverage" LDFLAGS="$(LDFLAGS) --coverage" && make ALL || echo "test errors"
	lcov --capture --directory . --base-directory . -o tests.info
	#lcov --extract -o tests-programcoverage 
	genhtml -o coverage tests.info

coverage-gcov:
	lcov --zerocounters --directory .
	export CXXFLAGS="$(CXXFLAGS) --coverage" LDFLAGS="$(LDFLAGS) --coverage" && make ALL || echo "test errors"
	find . -name "*.cpp" | xargs gcov -r
	mkdir gcov || true
	mv *.gcov gcov/
