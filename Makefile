SRCDIR = src
STRUCTURE = $(shell cd $(SRCDIR) && find . -type d)

DEPSDIR = thirdparty

CXX ?= g++
CXXFLAGS ?= -g

BINARYDIR = bin
OBJECTDIR = $(BINARYDIR)/obj

TARGET = $(BINARYDIR)/plug

LIB_DIR += 

ifeq ($(ENV), prod)
	LIBS += -lsioclient_tls
else
	LIBS += -lsioclient
endif

LIBS += -lpthread
LIBS += -latomic
LIBS += -lssl
LIBS += -lcrypto
LIBS += -lhomecontroller
LIBS += -lpigpio

# driver
_OBJECTS += driver/driver.o
_HEADERS += driver/driver.h

_OBJECTS += driver/rpi_z_driver.o
_HEADERS += driver/rpi_z_driver.h

_OBJECTS += driver/test_driver.o
_HEADERS += driver/test_driver.h

# root
_OBJECTS += config.o
_HEADERS += config.h

_OBJECTS += main.o

_OBJECTS += plug.o
_HEADERS += plug.h

OBJECTS = $(patsubst %,$(OBJECTDIR)/%,$(_OBJECTS))
HEADERS = $(patsubst %,$(SRCDIR)/%,$(_HEADERS))

$(OBJECTDIR)/%.o: $(SRCDIR)/%.cpp $(HEADERS) | $(OBJECTDIR)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

$(TARGET): $(OBJECTS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)

$(OBJECTDIR):
	mkdir -p $(OBJECTDIR)
	mkdir -p $(addprefix $(OBJECTDIR)/,$(STRUCTURE))

relink: $(OBJECTS)
	$(CXX) -o $(TARGET) $^ $(CXXFLAGS) $(LIBS)

clean:
	rm -rf bin

.PHONY: clean