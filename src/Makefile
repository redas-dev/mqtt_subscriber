TARGET = mqtt_sub
CFILES = $(wildcard *.c)

.PHONY: all clean

all: $(TARGET)

LDFLAGS += -lmosquitto -luci -lcjson -lcurl

$(TARGET):
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $(TARGET) $(CFILES) $(LDFLAGS)

clean:
	rm -f $(TARGET)