RENDER_LIB = libmediahal_videorender.so
RENDER_SERVER = render_server
AML_VERSION = amlVersion

SUPPORT_VIDEOTUNNEL = YES

#path set
SCANNER_TOOL ?= wayland-scanner
RENDERLIB_PATH = renderlib
PROTOCOL_PATH = $(RENDERLIB_PATH)/wayland-protocol
TOOLS_PATH = tools
SERVER_PATH = server

GENERATED_SOURCES = \
	$(PROTOCOL_PATH)/linux-dmabuf-unstable-v1-protocol.c \
	$(PROTOCOL_PATH)/linux-dmabuf-unstable-v1-client-protocol.h \
	$(PROTOCOL_PATH)/fullscreen-shell-unstable-v1-protocol.c \
	$(PROTOCOL_PATH)/fullscreen-shell-unstable-v1-client-protocol.h \
	$(PROTOCOL_PATH)/linux-explicit-synchronization-unstable-v1-protocol.c \
	$(PROTOCOL_PATH)/linux-explicit-synchronization-unstable-v1-client-protocol.h \
	$(PROTOCOL_PATH)/xdg-shell-protocol.c \
	$(PROTOCOL_PATH)/xdg-shell-client-protocol.h \
	$(PROTOCOL_PATH)/viewporter-protocol.c \
	$(PROTOCOL_PATH)/viewporter-client-protocol.h \
	$(PROTOCOL_PATH)/vpc-protocol.c \
	$(PROTOCOL_PATH)/vpc-client-protocol.h \
	$(PROTOCOL_PATH)/vpc-server-protocol.h \
	$(PROTOCOL_PATH)/simplebuffer-protocol.c \
	$(PROTOCOL_PATH)/simplebuffer-client-protocol.h \
	$(PROTOCOL_PATH)/simplebuffer-server-protocol.h \
	$(PROTOCOL_PATH)/simpleshell-protocol.c \
	$(PROTOCOL_PATH)/simpleshell-client-protocol.h \
	$(PROTOCOL_PATH)/simpleshell-server-protocol.h \
	$(PROTOCOL_PATH)/wayland-client-protocol.h

OBJ_WESTON_DISPLAY = \
	$(RENDERLIB_PATH)/plugins/weston/wayland_display.o \
	$(RENDERLIB_PATH)/plugins/weston/wayland_buffer.o \
	$(RENDERLIB_PATH)/plugins/weston/wayland_plugin.o \
	$(RENDERLIB_PATH)/plugins/weston/wayland_videoformat.o \
	$(RENDERLIB_PATH)/plugins/weston/wayland_window.o \
	$(RENDERLIB_PATH)/plugins/weston/wayland_shm.o \
	$(RENDERLIB_PATH)/plugins/weston/wayland_dma.o

OBJ_WESTEROS_DISPLAY = \
	$(RENDERLIB_PATH)/plugins/westeros/wstclient_wayland.o \
	$(RENDERLIB_PATH)/plugins/westeros/wstclient_socket.o \
	$(RENDERLIB_PATH)/plugins/westeros/wstclient_plugin.o

OBJ_VIDEOTUNNEL_DISPLAY = \
	$(RENDERLIB_PATH)/plugins/videotunnel/videotunnel_impl.o \
	$(RENDERLIB_PATH)/plugins/videotunnel/videotunnel_plugin.o

LOCAL_CFLAGS = -DSUPPORT_WAYLAND
LOCAL_CFLAGS += \
	-I$(RENDERLIB_PATH)/plugins/videotunnel \
	-I$(RENDERLIB_PATH)/plugins/weston \
	-I$(RENDERLIB_PATH)/plugins/westeros \
	-I$(PROTOCOL_PATH)

OBJ_RENDER_LIB += $(OBJ_WESTON_DISPLAY)
OBJ_RENDER_LIB += $(OBJ_WESTEROS_DISPLAY)
OBJ_RENDER_LIB += $(GENERATED_SOURCES:.c=.o)

ifeq ($(SUPPORT_VIDEOTUNNEL),YES)
OBJ_RENDER_LIB += $(OBJ_VIDEOTUNNEL_DISPLAY)
LOCAL_CFLAGS += -DSUPPORT_VIDEOTUNNEL
LD_SUPPORT += -lvideotunnel
endif

LD_SUPPORT += -lwayland-client

LOCAL_CFLAGS += \
	-I$(RENDERLIB_PATH) \
	-I$(TOOLS_PATH) \
	-I$(STAGING_DIR)/usr/include \
	-I$(STAGING_DIR)/usr/include/libdrm_meson \
	-I$(STAGING_DIR)/usr/include/libdrm \
	-I$(SERVER_PATH)/include-ext \
	-I../mediasync/include

OBJ_RENDER_LIB += \
	$(RENDERLIB_PATH)/render_lib.o \
	$(RENDERLIB_PATH)/render_core.o \
	$(TOOLS_PATH)/Thread.o \
	$(TOOLS_PATH)/Times.o \
	$(TOOLS_PATH)/Poll.o \
	$(TOOLS_PATH)/Logger.o \
	$(TOOLS_PATH)/Utils.o

OBJ_RENDER_SERVER =  \
	$(SERVER_PATH)/vdo_sink.o \
	$(SERVER_PATH)/socket_sink.o \
	$(SERVER_PATH)/monitor_thread.o \
	$(SERVER_PATH)/renderlib_wrap.o \
	$(SERVER_PATH)/sink_manager.o \
	$(SERVER_PATH)/render_server.o

OBJ_AML_VERSION = \
	$(TOOLS_PATH)/amlVersion.o

LOCAL_CFLAGS += -fPIC -O -Wcpp -g

CFLAGS += $(LOCAL_CFLAGS)
CXXFLAGS += $(LOCAL_CFLAGS) -std=c++11

TARGET = $(RENDER_LIB) $(RENDER_SERVER) $(AML_VERSION)

all: $(GENERATED_SOURCES) $(TARGET)

LD_FLAG = -g -fPIC -O -Wcpp -lm -lpthread -lz -Wl,-Bsymbolic -laudio_client  -lcutils
LD_FLAG_RENDERLIB = $(LD_FLAG) -shared -lmediahal_mediasync $(LD_SUPPORT)
LD_FLAG_RENDERSERVER = $(LD_FLAG) -lmediahal_videorender


%.o:%.c $(DEPS)
	echo CC $@ $< $(FLAGS)
	$(CC) -c -o $@ $< $(CFLAGS) -fPIC

%.o:%.cpp $(DEPS)
	echo CXX $@ $< $(FLAGS)
	$(CXX) -c -o $@ $< $(CXXFLAGS) -fPIC

$(RENDER_LIB): $(OBJ_RENDER_LIB)
	$(CXX) -o $@ $^ $(LD_FLAG_RENDERLIB)
	cp -f $(RENDER_LIB) $(STAGING_DIR)/usr/lib
	rm -f $(OBJ_WESTON_DISPLAY)
	rm -f $(OBJ_WESTEROS_DISPLAY)
	rm -f $(PROTOCOL_PATH)/*.o
	rm -f $(TOOLS_PATH)/*.o
	rm -f $(RENDERLIB_PATH)/*.o
	rm -f $(RENDERLIB_PATH)/plugins/videotunnel/*.o

$(RENDER_SERVER):$(OBJ_RENDER_SERVER) $(RENDER_LIB)
	$(CXX) -o $@ $^ $(LD_FLAG_RENDERSERVER)
	chmod a+x $(RENDER_SERVER)
	cp -f $(RENDER_SERVER) $(STAGING_DIR)/usr/bin
	rm -f $(OBJ_RENDER_SERVER)

$(AML_VERSION):$(OBJ_AML_VERSION)
	$(CXX) -o $@ $^ -g -fPIC -O -Wcpp -ldl
	chmod a+x $(AML_VERSION)
	cp -f $(AML_VERSION) $(STAGING_DIR)/usr/bin
	rm -f $(OBJ_AML_VERSION)

$(PROTOCOL_PATH)/%-protocol.c : $(PROTOCOL_PATH)/%.xml
	echo $(@D)
	mkdir -p $(@D) && $(SCANNER_TOOL) public-code < $< > $@

$(PROTOCOL_PATH)/%-server-protocol.h : $(PROTOCOL_PATH)/%.xml
	mkdir -p $(@D) && $(SCANNER_TOOL) server-header < $< > $@

$(PROTOCOL_PATH)/%-client-protocol.h : $(PROTOCOL_PATH)/%.xml
	mkdir -p $(@D) && $(SCANNER_TOOL) client-header < $< > $@


.PHONY: install
install:
	echo $(TARGET_DIR)
	cp -f $(RENDER_LIB) $(TARGET_DIR)/usr/lib/
	cp -f $(RENDER_SERVER) $(TARGET_DIR)/usr/bin/

.PHONY: clean

clean:
	rm $(RENDER_LIB)
	rm $(RENDER_SERVER)
	rm -f $(OBJ_WESTON_DISPLAY)
	rm -f $(OBJ_WESTEROS_DISPLAY)
	rm -f $(PROTOCOL_PATH)/*.o
	rm -f $(TOOLS_PATH)/*.o
	rm -f $(RENDERLIB_PATH)/*.o
	rm -f $(RENDERLIB_PATH)/plugins/videotunnel/*.o
	rm -f $(OBJ_RENDER_SERVER)
