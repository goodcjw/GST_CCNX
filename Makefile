CC      = g++
LIBS    = `pkg-config gstreamer-0.10 --libs` -lccn -lcrypto
DEFS    = -DGST_CCNX_DEBUG
CFLAGS	= -c `pkg-config gstreamer-0.10 --cflags` -Wall -DDEBUG -g

SOURCES	= \
	test.cc \
	gstCCNxSrc.cc \
	gstCCNxDepacketizer.cc \
	gstCCNxSegmenter.cc \
	gstCCNxFetchBuffer.cc

OBJECTS	= $(SOURCES:.cc=.o)
TARGET	= test

all: $(SOURCES) $(TARGET)

.cc.o:
	$(CC) $(CFLAGS) $(DEFS) $< -o $@

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(LIBS) -o $@

clean:
	rm $(OBJECTS)
	rm $(TARGET)
