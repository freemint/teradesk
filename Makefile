# Define this to the version of the package.
# used only for archive names, so no dot here
VERSION=408

srcdir=.

include support/config.am
include support/silent.am

# Additional defines.
DEFS =
INCLUDES = -I$(srcdir)/library/xdialog  -I$(srcdir)/include 

# Define this to the warning level you want.
WARN = \
	-Wall \
	-Wmissing-prototypes \
	-W \
	-Wshadow \
	-Wpointer-arith \
	-Wcast-qual \
	-Waggregate-return \
	-Wundef \
	-Wwrite-strings \
	-Werror

CFLAGS = $(DEFS) $(OPTS) $(INCLUDES) $(WARN)

ifeq ($(CPU),v4e)
CFLAGS += -mcpu=5475
else
CFLAGS += -m68000
endif

PROGRAMS = desktop.prg


SRCS = \
	main.c \
	config.c config.h \
	resource.c resource.h \
	icon.c icon.h \
	window.c window.h \
	copy.c copy.h \
	printer.c printer.h \
	lists.c lists.h \
	applik.c applik.h \
	prgtype.c prgtype.h \
	icontype.c icontype.h \
	filetype.c filetype.h \
	screen.c screen.h \
	showinfo.c showinfo.h \
	font.c font.h \
	va.c va.h \
	dragdrop.c dragdrop.h \
	\
	environm.c environm.h \
	startprg.c startprg.h \
	floppy.c floppy.h \
	video.c video.h \
	file.c file.h \
	\
	dir.c dir.h \
	open.c open.h \
	viewer.c viewer.h \
	\
	events.c events.h \
	slider.c slider.h \
	stringf.c stringf.h \
	xfilesys.c xfilesys.h \
	error.c error.h \
	\
	library/xdialog/xscncode.h \
	library/xdialog/internal.h \
	library/xdialog/xdunused.h \
	library/xdialog/xdialog.c library/xdialog/xdialog.h \
	library/xdialog/xddraw.c \
	library/xdialog/xdemodes.c \
	library/xdialog/xdevent.c \
	library/xdialog/xdnmdial.c \
	library/xdialog/xdutil.c \
	library/xdialog/xwindow.c library/xdialog/xwindow.h \
	\
	library/utility/cookie.c \
	library/utility/gettos.c \
	library/utility/minmax.c \
	library/utility/other.c \
	library/utility/pathutil.c \
	library/utility/strsncpy.c \
	library/utility/xerror.c \
	library/utility/itoa.c \
	library/utility/nf_ops.c \
	library/utility/nf_shutd.c \
	$(empty)

OBJS = $(patsubst %.c, %.o, $(filter %.c, $(SRCS)))

LIBS = -lgem

all: $(PROGRAMS)

desktop.prg: $(OBJS)
	$(AM_V_CCLD) $(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)



clean::
	$(RM) *.o library/utility/*.o library/xdialog/*.o $(PROGRAMS)

include Makefile.dist
